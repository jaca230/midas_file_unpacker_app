#!/usr/bin/env python3
"""Compare HDSoC builder ROOT products with every wire field and ADC sample."""
import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path
import struct
import subprocess
import numpy as np
from verify_current_banks import events, fields, number

HEADER=struct.Struct('<HBIIIIIHQBH')
HEADER_FIELDS='event_header event_info event_index event_reference_time time_threshold event_completion_time_us clock_frequency packet_size channel_mask num_windows num_packets'
TIMING_FIELDS='collection_cycle_index collection_cycle_timestamp_ns udp_time parse_time event_time total_time data_processed data_rate'


def source_event(obj, raw, counts, simulated):
    h=HEADER.unpack_from(raw)
    fields(obj.header,HEADER_FIELDS,h)
    assert len(raw)==36+h[-1]*80+2 and len(obj.packets.packets)==h[-1]
    groups=defaultdict(list)
    for i,p in enumerate(obj.packets.packets):
        values=struct.unpack_from('<HBBIHH',raw,36+i*80)
        fields(p,'packet_header packet_info channel trigger_time logical_position window_position',values)
        assert list(p.trace)==list(struct.unpack_from('<32H',raw,48+i*80))
        fields(p,'parser_index packet_footer',struct.unpack_from('<HH',raw,112+i*80))
        samples=np.frombuffer(raw,dtype='>u2',count=32,offset=48+i*80).astype(np.int32)
        groups[(values[3],values[2])].append((values[4],samples))
    assert len(obj.waveforms.waveforms)==len(groups)
    for wf in obj.waveforms.waveforms:
        key=(int(wf.trigger_time),int(wf.channel_num))
        packets=groups.pop(key)
        # Simulation emits increasing physical/logical positions. Compare actual
        # wire acquisition order rather than reproducing the C++ reorder code.
        expected=np.concatenate([samples for _,samples in packets])
        trace=np.array(wf.trace,dtype=np.int32)
        assert np.array_equal(trace,expected),('reassembled waveform differs',key)
        assert len(trace)==h[-2]*32
        if simulated:
            channel=key[1];rise=min(6.0,h[-2]);fall=6*rise
            onset=h[-2]*16-min(24,h[-2]*4)+channel%9-4
            t=np.maximum(0,np.arange(len(trace))-onset)
            peak_t=np.log(6)/(1/rise-1/fall)
            norm=np.exp(-peak_t/fall)-np.exp(-peak_t/rise)
            model=1800+channel+np.floor(480*(np.exp(-t/fall)-np.exp(-t/rise))/norm+0.5).astype(int)
            assert np.array_equal(trace,model),('simulated waveform mismatch',key)
        counts['waveforms']+=1;counts['samples']+=len(trace)
    assert not groups
    fields(obj.footer,'event_footer',struct.unpack_from('<H',raw,len(raw)-2))
    counts['packets']+=h[-1];counts['source_events']+=1


def verify(app, source, out, simulated=False):
    import ROOT
    ROOT.gROOT.SetBatch(True)
    for lib in ('analysis_pipeline_core','unpacker_data_products_core','unpacker_data_products_nalu'):
        assert ROOT.gSystem.Load(str(app/'build/lib'/('lib'+lib+'.so')))>=0
    out.mkdir(parents=True,exist_ok=True)
    if not (out/'build').exists():(out/'build').symlink_to(app/'build',target_is_directory=True)
    with (out/'unpack.log').open('w') as log:
        subprocess.run([str(app/'build/bin/unpacker'),str(source),'--profile','HDSoC'],cwd=out,stdout=log,stderr=subprocess.STDOUT,check=True)
    log=(out/'unpack.log').read_text()
    assert '[error]' not in log and '[warning]' not in log,log[-3000:]
    f=ROOT.TFile.Open(str(out/'output.root'));assert f and not f.IsZombie()
    tree=f.Get('events');counts=Counter();sizes=defaultdict(list);boards=Counter()
    flags={'HB00':'builder_event','HC00':'collection_timing','HS00':'selection_summary','HD00':'event','HT00':'time'}
    for serial,banks in events(source):
        if not set(banks)&set(flags):continue
        assert tree.GetEntry(counts['events'])>0,('missing ROOT entry',serial)
        for bank,name in flags.items():assert bool(getattr(tree,'has_nalu_'+name))==(bank in banks),(serial,bank)
        for name,data in banks.items():sizes[name].append(len(data))
        if 'HB00' in banks:
            data=banks['HB00'];magic,v,fl,start,end,n,npkt=struct.unpack_from('<4sHHQQII',data)
            assert (magic,v)==(b'HEB1',1)
            obj=tree.nalu_builder_event;fields(obj,'version flags start_ns end_ns source_events packets',(v,fl,start,end,n,npkt))
            assert len(obj.sources)==n
            pos=32;packets=0
            counts['empty_events']+=n==0
            for s in obj.sources:
                board,reserved,nb,nt=struct.unpack_from('<HHII',data,pos);pos+=12
                assert reserved==0 and int(s.board)==board and bool(s.has_timing)==bool(nt)
                raw=data[pos:pos+nb];pos+=nb
                source_event(s.event,raw,counts,simulated);boards[board]+=1;packets+=int(s.event.header.num_packets)
                if nt:fields(s.timing,TIMING_FIELDS,struct.unpack_from('<QQddddQd',data,pos))
                pos+=nt
            assert pos==len(data) and packets==npkt
        if 'HC00' in banks:
            v=struct.unpack('<4sHH9Q',banks['HC00']);assert v[:3]==(b'HCT1',1,80)
            fields(tree.nalu_collection_timing,'cycle_id start_ns end_ns input_events input_packets output_events output_packets processing_ns flags',v[3:])
            counts['input_packets']+=v[7]
            if 'HB00' in banks:
                hc=tree.nalu_collection_timing;hb=tree.nalu_builder_event
                assert (int(hc.start_ns),int(hc.end_ns),int(hc.output_events),int(hc.output_packets))==(int(hb.start_ns),int(hb.end_ns),int(hb.source_events),int(hb.packets))
        if 'HS00' in banks:
            v=struct.unpack('<4sHHQ32sIi4Q',banks['HS00']);assert v[:3]==(b'HSS1',1,88)
            obj=tree.nalu_selection_summary
            fields(obj,'cycle_id',(v[3],));assert str(obj.id)==v[4].split(b'\0')[0].decode()
            fields(obj,'enabled priority evaluated_events modified_events evaluated_packets removed_packets',v[5:])
            counts['removed_packets']+=v[-1]
            if 'HC00' in banks:
                hc=tree.nalu_collection_timing
                assert int(obj.cycle_id)==int(hc.cycle_id)
                assert int(hc.output_packets)+int(obj.removed_packets)==int(hc.input_packets)
        if 'HD00' in banks:source_event(tree.nalu_event,banks['HD00'],counts,simulated)
        if 'HT00' in banks:fields(tree.nalu_time.time,TIMING_FIELDS,struct.unpack('<QQddddQd',banks['HT00']))
        counts['events']+=1
    assert tree.GetEntries()==counts['events']
    f.Close()
    result={'source':str(source),'root':str(out/'output.root'),'status':'PASS','simulated':simulated,'counts':dict(counts),'boards':dict(boards),'banks':{name:{'records':len(v),'min_bytes':min(v),'max_bytes':max(v)} for name,v in sizes.items()}}
    (out/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
    return result


def main():
    p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('--output',required=True,type=Path);p.add_argument('--simulated',action='store_true');a=p.parse_args()
    verify(Path(__file__).resolve().parents[1],a.source.resolve(),a.output.resolve(),a.simulated)

if __name__=='__main__':main()
