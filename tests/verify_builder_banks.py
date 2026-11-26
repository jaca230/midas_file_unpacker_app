#!/usr/bin/env python3
"""Unpack builder banks and compare every ROOT field with an independent wire parse."""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
import struct
import subprocess
from verify_current_banks import events, fields

SUMMARY_FIELDS = 'cycle_id read_count hit_count prepare_us_sum read_us_sum decode_us_sum total_us_sum prepare_us_max read_us_max decode_us_max total_us_max retries_sum retries_max first_acquisition_id last_acquisition_id'
CYCLE_FIELDS = 'cycle_id start_ns input_events input_hits output_events output_hits selected_events rejected_events input_read_ns group_build_ns cuts_ns finalize_ns output_wait_ns total_ns flags'


def verify(app, source, out, manifest=None):
    import ROOT
    ROOT.gROOT.SetBatch(True)
    for lib in ['unpacker_data_products_core', 'unpacker_data_products_sampic', 'unpacker_data_products_nalu']:
        assert ROOT.gSystem.Load(str(app/'build/lib'/('lib'+lib+'.so'))) >= 0
    out.mkdir(parents=True, exist_ok=True)
    if not (out/'build').exists(): (out/'build').symlink_to(app/'build', target_is_directory=True)
    with (out/'unpack.log').open('w') as log:
        subprocess.run([str(app/'build/bin/unpacker'),str(source),'--profile','SAMPIC'],cwd=out,stdout=log,stderr=subprocess.STDOUT,check=True)
    log=(out/'unpack.log').read_text()
    assert '[error]' not in log and '[warning]' not in log, f'Unpacker diagnostics in {out}/unpack.log'
    file=ROOT.TFile.Open(str(out/'output.root')); assert file and not file.IsZombie()
    tree=file.Get('events'); assert tree
    counts=Counter(); waveforms=Counter(); reads=Counter(); cycles=Counter(); bank_counts=Counter(); cut_totals={}; cut_config={}
    previous_ids={}; previous_cycle=0
    for index,(serial,banks) in enumerate(events(source)):
        assert index < tree.GetEntries(), 'Missing ROOT entry'
        tree.GetEntry(index); counts['entries']+=1; bank_counts.update(banks.keys())
        assert bool(tree.has_sampic_builder_event)==('BD00' in banks)
        diag_names=[name for name in banks if name.startswith('BT') or name in ('BC00','BS00')]
        assert bool(tree.has_sampic_builder_diagnostics)==bool(diag_names)
        # Builder banks must never populate legacy branches.
        for flag in ['event','event_timing','collector_timing']:
            assert not getattr(tree,'has_sampic_'+flag)
        if 'BD00' in banks:
            payload=banks['BD00']; magic,version,reserved,n,start,end=struct.unpack_from('<IHHIQQ',payload)
            assert (magic,version,reserved)==(0x31424553,4,0)
            obj=tree.sampic_builder_event; fields(obj,'start_ns end_ns',(start,end)); assert len(obj.hits)==n
            offset=28
            for hit in obj.hits:
                crate=struct.unpack_from('<H',payload,offset)[0]; offset+=2
                values=struct.unpack_from('<BBi6B64hHfd',payload,offset); offset+=154
                data=hit.data
                assert int(hit.crate)==crate
                fields(data,'fe_board_index channel hit_number sampic_index channel_index',values[:5])
                nsamples,flags,cell,first=values[5:9]
                fields(data,'flags cell_info first_cell_physical_index',(flags,cell,first))
                for bit,name in enumerate(['inl_corrected','adc_corrected','residual_pedestal_corrected',
                                           'waveform_clipped','waveform_nonfinite','raw_tot_clipped']):
                    assert bool(getattr(data,name))==bool(flags & (1<<bit))
                samples=values[9:9+nsamples]
                assert list(data.encoded_waveform)==list(samples)
                assert len(data.corrected_waveform)==nsamples
                for raw,decoded in zip(samples,data.corrected_waveform):
                    assert math.isclose(decoded,raw/10000 if flags & 2 else raw,rel_tol=1e-7,abs_tol=1e-8)
                fields(data,'raw_tot_value tot_value first_cell_timestamp',values[-3:])
                metadata=struct.unpack_from('<diiQQ',payload,offset); offset+=32
                fields(hit.metadata,'physical_cell0_timestamp_ns sampic_timestamp_a sampic_timestamp_b fpga_timestamp trigger_position_mask',metadata)
                waveforms[str(crate)]+=1; counts['samples']+=nsamples
                counts['adc_corrected_waveforms']+=bool(flags & 2)
                counts['fractional_timestamps']+=values[-1]!=math.floor(values[-1])
            assert offset==len(payload)
            counts['physics_events']+=1
        if diag_names:
            obj=tree.sampic_builder_diagnostics
            assert bool(obj.has_collector_cycle)==('BC00' in banks)
            assert bool(obj.has_cut_summary)==('BS00' in banks)
            summaries={int(summary.crate):summary for summary in obj.acquisition_summaries}
            assert len(summaries)==len([name for name in diag_names if name.startswith('BT')])
            if 'BS00' in banks:
                payload=banks['BS00']; magic,version,size,cycle,n,reserved=struct.unpack_from('<4sHHQII',payload)
                assert (magic,version,size,reserved)==(b'BSS1',1,72,0) and len(payload)==24+72*n
                assert int(obj.cut_cycle_id)==cycle and len(obj.cut_summaries)==n
                remaining_events=(int(obj.collector_cycle.selected_events)+int(obj.collector_cycle.rejected_events)) if obj.has_collector_cycle else None
                remaining_hits=None; rejected_events=0; order=[]
                for i,cut in enumerate(obj.cut_summaries):
                    ident,enabled,priority,*values=struct.unpack_from('<32sIi4Q',payload,24+72*i)
                    ident=ident.split(b'\0')[0].decode()
                    assert str(cut.id)==ident and bool(cut.enabled)==bool(enabled) and int(cut.priority)==priority
                    fields(cut,'evaluated_events rejected_events evaluated_hits rejected_hits',values)
                    order.append((priority,ident))
                    assert cut_config.setdefault(ident,(enabled,priority))==(enabled,priority)
                    cut_totals.setdefault(ident,Counter()).update(dict(zip(['evaluated_events','rejected_events','evaluated_hits','rejected_hits'],values)))
                    evaluated,rejected,evaluated_hits,rejected_hits=values
                    if enabled:
                        if remaining_events is not None: assert evaluated<=remaining_events
                        if remaining_hits is not None: assert evaluated_hits==remaining_hits
                        remaining_events=evaluated; remaining_hits=evaluated_hits-rejected_hits
                    else: assert not any(values)
                    rejected_events+=rejected
                assert order==sorted(order)
                if obj.has_collector_cycle:
                    assert cycle==int(obj.collector_cycle.cycle_id)
                    if remaining_events is not None: assert remaining_events<=int(obj.collector_cycle.output_events)
                    if remaining_hits is not None: assert remaining_hits==int(obj.collector_cycle.output_hits)
                    assert rejected_events>=int(obj.collector_cycle.rejected_events)
                counts['cut_summary_events']+=1
            for name in diag_names:
                if name=='BS00': continue
                magic,version,size,*values=struct.unpack('<4sHH15Q',banks[name]); assert version==1 and size==128
                if name.startswith('BT'):
                    assert magic==b'BTS1'; crate=int(name[2:]); fields(summaries[crate],SUMMARY_FIELDS,values)
                    reads[name]+=values[1]
                    assert values[14]-values[13]+1==values[1]
                    assert values[13]==previous_ids.get(crate,-1)+1
                    previous_ids[crate]=values[14]
                else:
                    assert magic==b'BCT1'; fields(obj.collector_cycle,CYCLE_FIELDS,values)
                    assert values[0]>previous_cycle; previous_cycle=values[0]
                    for key,i in [('input_events',2),('input_hits',3),('output_events',4),('output_hits',5)]: cycles[key]+=values[i]
            counts['diagnostic_events']+=1
    assert tree.GetEntries()==counts['entries'], 'Extra ROOT entries'
    file.Close()
    if cycles:
        assert sum(waveforms.values())==cycles['output_hits']
        assert sum(waveforms.values())+sum(c['rejected_hits'] for c in cut_totals.values())==cycles['input_hits']
    if cycles: assert sum(reads.values())==cycles['input_events']
    if cycles: assert counts['physics_events']==cycles['output_events']
    if manifest and json.loads(manifest.read_text())['configuration'].get('collection_strategy')=='periodic_readout' and cycles:
        assert bank_counts['BD00']==bank_counts['BC00']==bank_counts['BS00']

    if manifest:
        expected=json.loads(manifest.read_text())['validation']
        assert dict(waveforms)==expected['waveforms_by_crate']
        assert dict(reads)==expected['acquisition_reads_by_bank']
        assert dict(bank_counts)==expected['bank_counts']
        if 'cut_totals' in expected: assert {k:dict(v) for k,v in cut_totals.items()}==expected['cut_totals']
    result=dict(status='PASS',input=str(source),root_file=str(out/'output.root'),counts=dict(counts),
                bank_counts=dict(bank_counts),waveforms_by_crate=dict(waveforms),acquisition_reads_by_bank=dict(reads),cycle_totals=dict(cycles),cut_totals={k:dict(v) for k,v in cut_totals.items()},cut_configuration=cut_config)
    (out/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input',type=Path)
    parser.add_argument('--output',type=Path,default=Path('.artifacts/builder_qa'))
    parser.add_argument('--manifest',type=Path)
    args=parser.parse_args()
    verify(Path(__file__).resolve().parents[1],args.input.resolve(),args.output.resolve(),args.manifest)
