#!/usr/bin/env python3
"""Exercise compact SAMPIC bank boundaries through the complete ROOT pipeline.
Requires the same environment as verify_current_banks.py.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import verify_current_banks as verify


def event(banks,serial=0):
    body=b''
    for name,data in banks.items():
        body+=struct.pack('<4sII',name.encode(),11,len(data))+data+b'\0'*((-len(data))%8)
    payload=struct.pack('<II',len(body),17)+body
    return struct.pack('<HHIII',1,0,serial,0,len(payload))+payload


def hit(samples,flags):
    return struct.pack('<BBiBBBBBB64hHfd',3,63,-123,3,15,len(samples),flags,63,62,
                       *(samples+[1234]*(64-len(samples))),65535,12.5,123456789.25)


def sd(records):
    return struct.pack('<4sHHI',b'SDC1',1,154,len(records))+b''.join(records)


def sh(count):
    return struct.pack('<HHI',2,32,count)+struct.pack('<diiQQ',123.5,-5,6,2**63+9,2**63+1)*count


def run(app,out):
    import ROOT
    for lib in ['unpacker_data_products_core','unpacker_data_products_sampic','unpacker_data_products_nalu']:
        assert ROOT.gSystem.Load(str(app/'build/lib'/('lib'+lib+'.so')))>=0
    out.mkdir(parents=True,exist_ok=True)
    tick=struct.pack('<HHI5QII',1,56,7,2**40,3,10,20,25,2,1)
    timebin=struct.pack('<HHIqdd',1,32,0,-7,-70.,-60.)
    full=sd([hit([-32768,-1,0,32767],63),hit([0,4095,17],1)])
    cases=[
        {'SD01':full,'SH01':sh(2),'SR01':tick},
        {'SD00':sd([]),'SH00':sh(0),'SB00':timebin},
        {'SR00':struct.pack('<HHI5QII',1,56,4,5,0,50,55,55,0,0)},
        {'SB01':timebin},
        {'SD00':sd([hit([],0)])},
    ]
    fixture=out/'edge_valid.mid'
    fixture.write_bytes(b''.join(event(b,i) for i,b in enumerate(cases)))
    result=verify.verify(app,out,fixture,'SAMPIC')
    # Each malformed bank is followed by a valid event to detect stale products.
    malformed={
        'sd_short':{'SD00':b'SDC1'},
        'sd_magic':{'SD00':b'XXXX'+full[4:]},
        'sd_version':{'SD00':full[:4]+struct.pack('<H',2)+full[6:]},
        'sd_record_size':{'SD00':full[:6]+struct.pack('<H',153)+full[8:]},
        'sd_count':{'SD00':full[:8]+struct.pack('<I',0xffffffff)+full[12:]},
        'sd_truncated':{'SD00':full[:-1]},
        'sd_trailing':{'SD00':full+b'\0'},
        'sd_waveform_size':{'SD00':full[:20]+bytes([65])+full[21:]},
        'sh_version':{'SH00':struct.pack('<HHI',1,32,0)},
        'sh_record_size':{'SH00':struct.pack('<HHI',2,54,0)},
        'sh_count':{'SH00':struct.pack('<HHI',2,32,0xffffffff)},
        'sh_short':{'SH00':b'\0'},
        'sr_version':{'SR00':struct.pack('<H',2)+tick[2:]},
        'sr_record_size':{'SR00':tick[:2]+struct.pack('<H',55)+tick[4:]},
        'sr_truncated':{'SR00':tick[:-1]},
        'sb_version':{'SB00':struct.pack('<H',2)+timebin[2:]},
        'sb_record_size':{'SB00':timebin[:2]+struct.pack('<H',31)+timebin[4:]},
        'sb_truncated':{'SB00':timebin[:-1]},
    }
    for name,banks in malformed.items():
        dest=out/name;dest.mkdir(exist_ok=True)
        if not (dest/'build').exists(): (dest/'build').symlink_to(app/'build',target_is_directory=True)
        source=dest/'input.mid'
        source.write_bytes(event(cases[0],0)+event(banks,1)+event(cases[1],2))
        with (dest/'unpack.log').open('w') as log:
            subprocess.run([str(app/'build/bin/unpacker'),str(source)],cwd=dest,stdout=log,stderr=subprocess.STDOUT,check=True)
        log=(dest/'unpack.log').read_text()
        assert '[error]' in log or '[warning]' in log,name
        f=ROOT.TFile.Open(str(dest/'output.root'));tree=f.Get('events')
        assert tree.GetEntries()==2,(name,tree.GetEntries())
        for i,b in enumerate([cases[0],cases[1]]):
            tree.GetEntry(i)
            verify.sampic(tree,b,verify.Counter())
        f.Close()
    summary={'valid_fixture':result,'malformed_cases_rejected':list(malformed),'status':'PASS'}
    (out/'edge_cases.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(f'PASS: 5 valid edge-case events and {len(malformed)} malformed banks rejected.')


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--app',type=Path,default=Path(__file__).resolve().parents[1]);p.add_argument('--output',type=Path,default=Path('.artifacts/bank_format_verification/edge_cases'))
    args=p.parse_args();run(args.app.resolve(),args.output.resolve())
