#!/usr/bin/env python3
"""End-to-end wire/ROOT comparison for current DAQ banks. Requires PyROOT and lz4 CLI.

Run from an activated ROOT environment:
  python tests/verify_current_banks.py --app . --output .artifacts/bank_format_verification

Runs the actual unpacker in isolated output directories, then independently parses
all MIDAS banks and compares every stored hit, packet, waveform and timing field.
"""
import argparse
from collections import Counter, defaultdict
import json
import math
from pathlib import Path
import struct
import subprocess


def events(path):
    proc = None
    if path.suffix == '.lz4':
        proc = subprocess.Popen(['lz4', '-dc', str(path)], stdout=subprocess.PIPE)
        stream = proc.stdout
    else:
        stream = path.open('rb')
    try:
        while True:
            header = stream.read(16)
            if not header:
                break
            assert len(header) == 16, 'Truncated MIDAS header'
            eid, mask, serial, timestamp, size = struct.unpack('<HHIII', header)
            data = stream.read(size)
            assert len(data) == size, 'Truncated MIDAS payload'
            if eid >= 0x8000:
                continue
            bank_size, flags = struct.unpack_from('<II', data)
            assert bank_size + 8 == len(data) and flags == 17
            banks = {}
            pos = 8
            while pos < len(data):
                name, tid, length = struct.unpack_from('<4sII', data, pos)
                pos += 12
                assert pos + length <= len(data)
                name = name.decode('ascii')
                assert name not in banks
                banks[name] = data[pos:pos+length]
                pos += (length + 7) & ~7
            assert pos == len(data)
            yield serial, banks
    finally:
        stream.close()
        if proc:
            assert proc.wait() == 0, 'LZ4 decompression failed'


def number(value):
    return ord(value) if isinstance(value, str) else value


def fields(obj, names, values):
    for name, expected in zip(names.split(), values):
        actual = number(getattr(obj, name))
        assert actual == expected or (isinstance(expected, float) and math.isnan(expected) and math.isnan(actual)), (name, actual, expected)


def sampic(tree, banks, summary):
    for prefix, flag in [('SD','event'),('SH','hit_metadata'),('ST','event_timing'),('SC','collector_timing'),('SR','readout_tick'),('SB','time_bin'),('SG','trigger_group'),('SV','trigger_records')]:
        assert bool(getattr(tree, 'has_sampic_'+flag)) == any(k.startswith(prefix) for k in banks)
    n = None
    for bank, data in banks.items():
        prefix = bank[:2]
        if prefix == 'SD':
            magic, version, size, n = struct.unpack_from('<4sHHI', data)
            assert (magic, version, size) == (b'SDC1', 1, 154)
            assert len(data) == 12+154*n
            hits = tree.sampic_event.hits
            assert len(hits) == n
            summary['hits'] += n
            summary['empty_events'] += n == 0
            for i, hit in enumerate(hits):
                values = struct.unpack_from('<BBiBBBBBB64hHfd', data, 12+154*i)
                fields(hit, 'fe_board_index channel hit_number sampic_index channel_index', values[:5])
                count, flags, cell, first_cell = values[5:9]
                fields(hit, 'flags cell_info first_cell_physical_index', (flags,cell,first_cell))
                for bit, field in enumerate(['inl_corrected','adc_corrected','residual_pedestal_corrected','waveform_clipped','waveform_nonfinite','raw_tot_clipped']):
                    assert bool(getattr(hit,field)) == bool(flags & (1 << bit))
                expected = values[9:9+count]
                assert list(hit.encoded_waveform) == list(expected)
                assert len(hit.corrected_waveform) == count
                for raw, decoded in zip(expected, hit.corrected_waveform):
                    assert math.isclose(decoded, raw/10000 if flags & 2 else raw, rel_tol=1e-7, abs_tol=1e-8)
                fields(hit, 'raw_tot_value tot_value first_cell_timestamp', values[-3:])
                summary['samples'] += count
        elif prefix == 'SH':
            version,size,count = struct.unpack_from('<HHI',data)
            assert (version,size)==(2,32) and len(data)==8+32*count
            hits = tree.sampic_hit_metadata.hits
            assert len(hits)==count
            if n is not None: assert count==n
            for i,hit in enumerate(hits):
                fields(hit,'physical_cell0_timestamp_ns sampic_timestamp_a sampic_timestamp_b fpga_timestamp trigger_position_mask',struct.unpack_from('<diiQQ',data,8+32*i))
        elif prefix == 'ST':
            fields(tree.sampic_event_timing,'fe_timestamp_ns nhits nparents sp_prepare_us_sum sp_read_us_sum sp_decode_us_sum sp_total_us_sum sp_prepare_us_max sp_read_us_max sp_decode_us_max sp_total_us_max sp_acq_retry_max sp_acq_retry_sum',struct.unpack('<Q12I',data))
            if n is not None: assert tree.sampic_event_timing.nhits==n
        elif prefix == 'SC':
            fields(tree.sampic_collector_timing,'collector_timestamp_ns n_events total_hits wait_us group_build_us finalize_us total_us',struct.unpack('<Q6I',data))
        elif prefix == 'SR':
            vals=struct.unpack('<HHI5QII',data)
            assert vals[:2]==(1,56)
            fields(tree.sampic_readout_tick,'format_version record_size flags first_tick ticks_elapsed start_ns end_ns actual_readout_ns hit_count parent_count',vals)
            if n is not None: assert vals[-2]==n
        elif prefix == 'SB':
            vals=struct.unpack('<HHIqdd',data)
            assert vals[:2]==(1,32)
            fields(tree.sampic_time_bin,'format_version record_size hit_count bin_index start_ns end_ns',vals)
            if n is not None: assert vals[2]==n
        else:
            raise AssertionError('Unexpected verification bank: '+bank)


def hdsoc(tree,banks,summary):
    assert set(banks)=={'HD00','HT00'}
    data=banks['HD00']
    h=struct.unpack_from('<HBIIIIIHQBH',data)
    fields(tree.nalu_event.header,'event_header event_info event_index event_reference_time time_threshold event_completion_time_us clock_frequency packet_size channel_mask num_windows num_packets',h)
    assert h[7]==80 and len(data)==38+80*h[-1]
    packets=tree.nalu_event.packets.packets
    assert len(packets)==h[-1]
    groups=defaultdict(list)
    for i,packet in enumerate(packets):
        offset=36+80*i
        vals=struct.unpack_from('<HBBIHH32HHH',data,offset)
        fields(packet,'packet_header packet_info channel trigger_time logical_position window_position',vals[:6])
        assert list(packet.trace)==list(vals[6:38])
        fields(packet,'parser_index packet_footer',vals[-2:])
        # Packet raw sample bytes retain board big-endian order.
        samples=struct.unpack_from('>32H',data,offset+12)
        groups[(vals[2],vals[3])].append((vals[5],samples))
    summary['packets']+=len(packets)
    summary['waveforms']+=len(groups)
    summary['samples']+=len(packets)*32
    summary['max_waveforms_per_event']=max(summary['max_waveforms_per_event'],len(groups))
    waveforms=tree.nalu_event.waveforms.waveforms
    assert len(waveforms)==len(groups)
    seen=set()
    for waveform in waveforms:
        key=(number(waveform.channel_num),int(waveform.trigger_time))
        assert key not in seen
        seen.add(key)
        group=groups[key]
        assert len(group)==h[-2], (key,len(group),h[-2])
        # Complete channel groups in these acquisitions arrive in waveform order;
        # verify against preserved packet order, independently of C++ sorting.
        expected=[sample for _,samples in group for sample in samples]
        assert list(waveform.trace)==expected, ('waveform sample/order mismatch',key,[p for p,_ in group])
    fields(tree.nalu_event.footer,'event_footer',struct.unpack_from('<H',data,len(data)-2))
    fields(tree.nalu_time.time,'collection_cycle_index collection_cycle_timestamp_ns udp_time parse_time event_time total_time data_processed data_rate',struct.unpack('<QQddddQd',banks['HT00']))


def verify(app,out,source,profile):
    import ROOT
    dest=out/source.name.split('.')[0]
    dest.mkdir(parents=True,exist_ok=True)
    if not (dest/'build').exists(): (dest/'build').symlink_to(app/'build', target_is_directory=True)
    with (dest/'unpack.log').open('w') as log:
        subprocess.run([str(app/'build/bin/unpacker'), str(source), '--profile', profile],cwd=dest,stdout=log,stderr=subprocess.STDOUT,check=True)
    log=(dest/'unpack.log').read_text()
    assert '[error]' not in log and '[warning]' not in log, f'Unpacker diagnostics: {dest}/unpack.log'
    f=ROOT.TFile.Open(str(dest/'output.root'))
    assert f and not f.IsZombie()
    tree=f.Get('events')
    summary=Counter(events=0,hits=0,packets=0,waveforms=0,samples=0,empty_events=0,max_waveforms_per_event=0)
    counts=Counter()
    for serial,banks in events(source):
        idx=summary['events']
        assert idx<tree.GetEntries(),('missing ROOT event',serial)
        tree.GetEntry(idx)
        (sampic if profile=='SAMPIC' else hdsoc)(tree,banks,summary)
        summary['events']+=1
        counts.update(banks.keys())
    assert tree.GetEntries()==summary['events'],('extra ROOT entries',tree.GetEntries(),summary)
    f.Close()
    result=dict(file=source.name,profile=profile,counts=dict(summary),banks=dict(counts),status='PASS')
    print(json.dumps(result),flush=True)
    return result


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--app',type=Path,default=Path(__file__).resolve().parents[1])
    p.add_argument('--output',type=Path,default=Path('.artifacts/bank_format_verification'))
    p.add_argument('--file',type=Path)
    p.add_argument('--profile',choices=['SAMPIC','HDSoC'],default='SAMPIC')
    args=p.parse_args();app=args.app.resolve();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    import ROOT
    ROOT.gROOT.SetBatch(True)
    for lib in ['unpacker_data_products_core','unpacker_data_products_sampic','unpacker_data_products_nalu']:
        assert ROOT.gSystem.Load(str(app/'build/lib'/('lib'+lib+'.so')))>=0
    cases=[(args.file.resolve(),args.profile)] if args.file else [(app/'data'/f'run{r}.mid.lz4',profile) for r,profile in [('121479','HDSoC'),('121485','HDSoC'),('00266','SAMPIC'),('00267','SAMPIC')]]
    results=[verify(app,out,path,profile) for path,profile in cases]
    (out/'verification.json').write_text(json.dumps(results,indent=2)+'\n')

if __name__=='__main__': main()
