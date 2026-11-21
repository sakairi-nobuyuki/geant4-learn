#!/usr/bin/env python3
"""Run each aluminum thickness in a fresh batch output directory."""
import argparse
import os
import subprocess
import tempfile
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable', default='./lunar_device')
    parser.add_argument('--macro', default='../macros/run_log.mac')
    parser.add_argument('--thicknesses', nargs='+', type=float,
                        default=[0, 1, 2, 5, 10])
    parser.add_argument('--outdir', default='sweep_results')
    args = parser.parse_args()
    output_root = Path(args.outdir).resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    for t in args.thicknesses:
        directory = Path(tempfile.mkdtemp(prefix=f'al_{t:g}mm_', dir=output_root))
        env = os.environ.copy()
        env['LUNAR_AL_THICKNESS_MM'] = str(t)
        env['LUNAR_OUTPUT_DIR'] = str(directory)
        print(f'Running t={t:g} mm; output -> {directory}', flush=True)
        with open(directory / 'run.log', 'w') as log:
            subprocess.run([args.executable, args.macro], env=env,
                           stdout=log, stderr=subprocess.STDOUT, check=True)
    print('Completed. Summarize each case with scripts/analyze.py --dir PATH.')

if __name__ == '__main__':
    main()
