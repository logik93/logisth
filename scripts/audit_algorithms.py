#!/usr/bin/env python3
"""Verify vendored algorithm provenance; optionally compare pinned upstream checkouts."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
PINS = {'Signalsmith Stretch 1.3.2': '88061fd4eda56a2353c5d65c74aa3f4ccf76f9da',
        'Signalsmith Linear 0.6.6': '7bb1c8f36794d6980080ce817c91f8da8fe56a66'}

def audit(stretch=None, linear=None):
    root = Path(__file__).resolve().parents[1]
    vendor = root / 'Source/vendor/signalsmith-stretch'
    manifest = json.loads((vendor / 'UPSTREAM.json').read_text())
    assert set(manifest) == set(PINS), 'Unexpected upstream components'
    reports=[]
    for name, record in manifest.items():
        assert record['commit'] == PINS[name], 'Unexpected upstream revision'
        upstream = linear if 'Linear' in name else stretch
        if upstream:
            commit = subprocess.check_output(['git','-C',str(upstream),'rev-parse','HEAD'],text=True).strip()
            assert commit == PINS[name], 'Upstream checkout is not pinned'
        for relative, digest in record['files'].items():
            contents = (vendor / relative).read_bytes()
            assert hashlib.sha256(contents).hexdigest() == digest, 'Modified vendor file: '+relative
            if upstream:
                path = relative.removeprefix('signalsmith-linear/') if 'Linear' in name else relative
                assert contents == (upstream / path).read_bytes(), 'Different upstream bytes: '+relative
        reports.append({'component':name,'commit':record['commit'],'files_verified':len(record['files']),
                        'sha256_manifest':'passed','direct_upstream_comparison':'passed' if upstream else 'not requested'})
    return {'components':reports,
            'processing_parity':'StretchControls CTest compares stereo output with upstream at 44.1/48/96 kHz',
            'independent_models':['Kick attack/tail hybrid','WSOLA/Tonal/Transient sampler paths',
                                  'granular sample engine','two-part subtractive engine',
                                  'chord and pattern sequencer','five character reverbs and equalizers'],
            'scope':'Source identity and DSP adapter parity do not assert equivalence to proprietary plug-in binaries.'}

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--upstream-stretch',type=Path)
    p.add_argument('--upstream-linear',type=Path)
    p.add_argument('--output',type=Path)
    args=p.parse_args()
    report=audit(args.upstream_stretch,args.upstream_linear)
    text=json.dumps(report,indent=2)+'\n'
    if args.output:args.output.write_text(text)
    print(text,end='')
if __name__=='__main__':main()
