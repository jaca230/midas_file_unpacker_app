#!/usr/bin/env python3
"""Exercise builder parser boundaries, signed samples and legacy timing compatibility."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
from check_bank_edge_cases import event


def run(app,out):
    import ROOT
    for lib in ['unpacker_data_products_core','unpacker_data_products_sampic','unpacker_data_products_nalu']:
        assert ROOT.gSystem.Load(str(app/'build/lib'/('lib'+lib+'.so'))) >= 0
    out.mkdir(parents=True,exist_ok=True)
    if not (out/'build').exists(): (out/'build').symlink_to(app/'build',target_is_directory=True)
    stamp=2**63+123
    def hit(crate, flags, count, timestamp):
        samples=[-32768,-1,0,32767]+[0]*60
        sd=struct.pack('<BBi6B64hHfd',2,63,-7,3,15,count,flags,63,61,*samples,65535,12.375,timestamp)
        sh=struct.pack('<diiQQ',123450.0625,-1234,1235,(1<<60)+3,(1<<63)|1)
        return struct.pack('<H',crate)+sd+sh
    bd=struct.pack('<IHHIQQ',0x31424553,4,0,3,stamp,stamp+100)
    bd+=hit(1,63,4,123456.875)+hit(0,61,4,123457.125)+hit(0,0,0,123458.5)
    empty=struct.pack('<IHHIQQ',0x31424553,4,0,0,100,200)
    bt_values=[2,1,2,1,2,3,6,1,2,3,6,2,2,0,0]
    bt=struct.pack('<4sHH15Q',b'BTS1',1,128,*bt_values)
    bc_values=[2,0,1,2,1,2,1,0,1,2,3,4,5,20,1]
    bc=struct.pack('<4sHH15Q',b'BCT1',1,128,*bc_values)
    legacy={'ST00':struct.pack('<Q12I',stamp,*range(1,13)), 'SC00':struct.pack('<Q6I',stamp,*range(1,7))}
    bs_header=struct.pack('<4sHHQII',b'BSS1',1,72,2,2,0)
    def cut(name,enabled=0,priority=100,evaluated=0,rejected=0,hits=0,dropped=0):
        return struct.pack('<32sIi4Q',name.encode(),enabled,priority,evaluated,rejected,hits,dropped)
    bs=bs_header+cut('topology')+cut('dummy',priority=200)
    bs_only=bs_header+cut('dummy',priority=-10)+cut('topology')
    valid=[{'BD00':bd},{'BT00':bt,'BT01':bt,'BC00':bc,'BS00':bs},{'BD00':empty},legacy,{'BS00':bs_only}]
    bad={
        'bd_magic': {'BD00':b'BAD!'+bd[4:]},
        'bd_version': {'BD00':bd[:4]+struct.pack('<H',3)+bd[6:]},
        'bd_reserved': {'BD00':bd[:6]+b'\x01\x00'+bd[8:]},
        'bd_count': {'BD00':bd[:8]+struct.pack('<I',0xffffffff)+bd[12:]},
        'bd_sample_count': {'BD00':bd[:38]+b'\x41'+bd[39:]},
        'bd_channel': {'BD00':bd[:31]+b'\x40'+bd[32:]},
        'bd_flags': {'BD00':bd[:39]+b'\x80'+bd[40:]},
        'bd_nonfinite_timestamp': {'BD00':bd[:176]+struct.pack('<d',float('nan'))+bd[184:]},
        'bd_trailing': {'BD00':bd+b'\0'},
        'bd_truncated': {'BD00':bd[:-1]},
        'bd_interval': {'BD00':struct.pack('<IHHIQQ',0x31424553,4,0,0,200,100)},
        'bt_magic': {'BT00':b'BAD!'+bt[4:]},
        'bt_version': {'BT00':bt[:4]+b'\x02\x00'+bt[6:]},
        'bt_size': {'BT00':bt[:6]+struct.pack('<H',127)+bt[8:]},
        'bt_truncated': {'BT00':bt[:-1]},
        'bt_range': {'BT00':bt[:-8]+struct.pack('<Q',2)},
        'bc_magic': {'BC00':b'BAD!'+bc[4:]},
        'bc_version': {'BC00':bc[:4]+b'\x02\x00'+bc[6:]},
        'bc_size': {'BC00':bc[:6]+struct.pack('<H',127)+bc[8:]},
        'bc_truncated': {'BC00':bc[:-1]},
        'bc_flags': {'BC00':bc[:-8]+struct.pack('<Q',2)},
        'cycle_mismatch': {'BT00':bt,'BC00':bc[:8]+struct.pack('<Q',3)+bc[16:]},
    }
    bad.update({
        'bs_magic':{'BS00':b'BAD!'+bs[4:]},
        'bs_version':{'BS00':bs[:4]+struct.pack('<H',2)+bs[6:]},
        'bs_size':{'BS00':bs[:6]+struct.pack('<H',71)+bs[8:]},
        'bs_reserved':{'BS00':bs[:20]+struct.pack('<I',1)+bs[24:]},
        'bs_count':{'BS00':bs[:16]+struct.pack('<I',3)+bs[20:]},
        'bs_truncated':{'BS00':bs[:-1]},
        'bs_name':{'BS00':bs_header+cut('Bad!')+cut('dummy',priority=200)},
        'bs_duplicate':{'BS00':bs_header+cut('dummy')+cut('dummy',priority=200)},
        'bs_enabled':{'BS00':bs_header+cut('topology',enabled=2)+cut('dummy',priority=200)},
        'bs_disabled_counts':{'BS00':bs_header+cut('topology',evaluated=1)+cut('dummy',priority=200)},
        'bs_bad_counts':{'BS00':bs_header+cut('topology',enabled=1,evaluated=1,rejected=2)+cut('dummy',priority=200)},
        'bs_order':{'BS00':bs_header+cut('topology',priority=200)+cut('dummy',priority=100)},
        'bs_cycle':{'BS00':bs,'BC00':bc[:8]+struct.pack('<Q',3)+bc[16:]},
    })
    # Follow each rejected input by a valid event to check no stale products escape.
    fixture=out/'edge_cases.mid'
    sequence=list(valid)
    for malformed in bad.values(): sequence += [malformed,valid[0]]
    fixture.write_bytes(b''.join(event(banks,i) for i,banks in enumerate(sequence)))
    with (out/'unpack.log').open('w') as log:
        subprocess.run([str(app/'build/bin/unpacker'),str(fixture),'--profile','SAMPIC'],cwd=out,stdout=log,stderr=subprocess.STDOUT,check=True)
    f=ROOT.TFile.Open(str(out/'output.root'));tree=f.Get('events')
    assert tree.GetEntries()==len(valid)+len(bad),tree.GetEntries()
    for i in [0,*range(len(valid),int(tree.GetEntries()))]:
        tree.GetEntry(i)
        assert tree.has_sampic_builder_event and not tree.has_sampic_builder_diagnostics
        obj=tree.sampic_builder_event
        assert int(obj.start_ns)==stamp and int(obj.end_ns)==stamp+100
        assert len(obj.hits)==3
        for j in [0,1]:
            hit=obj.hits[j]; data=hit.data; metadata=hit.metadata
            assert float(data.first_cell_timestamp)==[123456.875,123457.125][j]
            assert list(data.encoded_waveform)==[-32768,-1,0,32767]
            assert bool(data.adc_corrected)==(j==0)
            assert bool(data.waveform_clipped) and bool(data.waveform_nonfinite) and bool(data.raw_tot_clipped)
            expected=[-32768,-1,0,32767]
            for actual,raw in zip(data.corrected_waveform,expected):
                assert abs(actual-(raw/10000 if j==0 else raw))<1e-6
            assert int(data.hit_number)==-7 and float(data.tot_value)==12.375
            assert int(metadata.sampic_timestamp_a)==-1234
            assert int(metadata.fpga_timestamp)==(1<<60)+3
            assert int(metadata.trigger_position_mask)==(1<<63)|1
            assert float(metadata.physical_cell0_timestamp_ns)==123450.0625
        assert len(obj.hits[2].data.encoded_waveform)==0
    tree.GetEntry(1)
    assert tree.has_sampic_builder_diagnostics and not tree.has_sampic_builder_event
    assert len(tree.sampic_builder_diagnostics.acquisition_summaries)==2
    tree.GetEntry(2); assert tree.has_sampic_builder_event and len(tree.sampic_builder_event.hits)==0
    tree.GetEntry(3)
    assert tree.has_sampic_event_timing and tree.has_sampic_collector_timing
    assert not tree.has_sampic_builder_event and not tree.has_sampic_builder_diagnostics
    assert int(tree.sampic_event_timing.fe_timestamp_ns)==stamp
    assert int(tree.sampic_event_timing.sp_acq_retry_sum)==12
    assert int(tree.sampic_collector_timing.collector_timestamp_ns)==stamp
    assert int(tree.sampic_collector_timing.total_us)==6
    tree.GetEntry(4)
    assert tree.has_sampic_builder_diagnostics and not tree.has_sampic_builder_event
    assert tree.sampic_builder_diagnostics.has_cut_summary and not tree.sampic_builder_diagnostics.has_collector_cycle
    assert str(tree.sampic_builder_diagnostics.cut_summaries[0].id)=='dummy'
    assert int(tree.sampic_builder_diagnostics.cut_summaries[0].priority)==-10
    f.Close()
    result=dict(status='PASS',valid_cases=len(valid),malformed_cases_rejected=list(bad))
    (out/'edge_cases.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=Path('.artifacts/builder_edge_cases'))
    args=parser.parse_args()
    run(Path(__file__).resolve().parents[1],args.output.resolve())
