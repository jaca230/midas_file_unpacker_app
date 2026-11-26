#!/usr/bin/env python3
"""Create/unpack synthetic SG/SV/SR/SB records for notebook branch coverage.
This is not a real detector acquisition. Requires the built app.
"""
from pathlib import Path
import struct
import subprocess
from check_bank_edge_cases import event, sd, sh, hit

app=Path(__file__).resolve().parents[1]
out=app/'.artifacts/bank_format_verification/runnotebook_fixture'
out.mkdir(parents=True,exist_ok=True)
if not (out/'build').exists(): (out/'build').symlink_to(app/'build',target_is_directory=True)
wave=[0]*8+[10000]*8+[-5000]*8+[0]*40
frames=[]
for i in range(4):
    n=1 if i%2==0 else 0
    banks={'SD00':sd([hit(wave,2)] if n else []),'SH00':sh(n),
        'SG00':struct.pack('<IIIIIdd',(254+i)%256,42+i,i,n,0,1000.+i*1000,1000.+i*1000+10 if n else 0),
        'SV00':struct.pack('<I',1)+struct.pack('<IIHHd',(254+i)%256,42+i,1,0,1000.+i*1000)}
    if i!=1: banks['ST00']=struct.pack('<Q12I',1000000000+i*10000000,n,1,*([1]*10))
    if i==0: banks['SR00']=struct.pack('<HHI5QII',1,56,1,0,1,0,10000000,10000001,n,1)
    if i==2: banks['SB00']=struct.pack('<HHIqdd',1,32,n,-1,-1000.,0.)
    frames.append(event(banks,i))
(out/'input.mid').write_bytes(b''.join(frames))
with (out/'unpack.log').open('w') as log:
    subprocess.run([str(app/'build/bin/unpacker'),'input.mid'],cwd=out,stdout=log,stderr=subprocess.STDOUT,check=True)
print(out/'output.root')
