#!/usr/bin/env python3
"""Execute every maintained notebook on both real collection modes.

Activate the app's ROOT/Jupyter environment first. Executed copies and logs live
under .artifacts/notebook_verification; source notebooks stay output-free.
"""
import argparse
import json
import os
from pathlib import Path
import time
import nbformat
from nbclient import NotebookClient


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--notebook', help='Optional path relative to notebooks/')
    parser.add_argument('--run', help='Optional run number, e.g. 00266')
    args=parser.parse_args()
    app=Path(__file__).resolve().parents[1]
    out=app/'.artifacts/notebook_verification';out.mkdir(parents=True,exist_ok=True)
    paths=[app/'notebooks'/args.notebook] if args.notebook else sorted(p for p in (app/'notebooks').glob('*/*.ipynb') if not p.parent.name.startswith('.'))
    results=[]
    for path in paths:
        template=nbformat.read(path,as_version=4)
        sampic=template.metadata['daq_profile']=='SAMPIC'
        runs=[args.run] if args.run else ['00266','00267'] if sampic else ['121479','121485']
        for run in runs:
            source=app/'.artifacts/bank_format_verification'/('run'+run)/'output.root'
            if not source.is_file(): raise FileNotFoundError(source)
            os.environ['MPLBACKEND']='module://matplotlib_inline.backend_inline'
            nb=nbformat.read(path,as_version=4)
            selection=[c for c in nb.cells if 'data-file' in c.metadata.get('tags',[])]
            if len(selection) != 1: raise ValueError(f'Expected one data-file cell in {path}')
            selection[0].source='DATA_FILE = '+repr(str(source))
            start=time.monotonic()
            row=dict(notebook=str(path.relative_to(app)),run=run,status='PASS')
            try:
                NotebookClient(nb,timeout=300,kernel_name='python3',resources={'metadata':{'path':str(path.parent)}}).execute()
            except Exception as exc:
                row.update(status='FAIL',error=str(exc))
            row['seconds']=round(time.monotonic()-start,2)
            row['figures']=sum('image/png' in o.get('data',{}) for c in nb.cells if c.cell_type=='code' for o in c.get('outputs',[]))
            dest=out/('run'+run)/path.parent.name;dest.mkdir(parents=True,exist_ok=True)
            nbformat.write(nb,dest/path.name)
            results.append(row)
            (out/('results_'+(path.stem if args.notebook else 'all')+'.json')).write_text(json.dumps(results,indent=2)+'\n')
            print(f"{row['status']} {run} {path.parent.name}/{path.name}: {row['seconds']}s, {row['figures']} figures",flush=True)
            if row['status']=='FAIL': print(row['error'],flush=True)
    if any(r['status']!='PASS' for r in results): raise SystemExit(1)

if __name__=='__main__': main()
