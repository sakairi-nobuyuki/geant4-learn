#!/usr/bin/env python3
"""Run thickness cases with optional parametric DetectorConstruction.cc.

Must replace src/DetectorConstruction.cc with extras/DetectorConstruction_parametric.cc
and rebuild before running. Batch-only. Each thickness goes to a separate output directory.
"""
import argparse
import os
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable', default='./exampleB1')
    parser.add_argument('--macro', default='../macros/run_log.mac')
    parser.add_argument('--thicknesses', nargs='+', type=float,
                        default=[0, 1, 2, 5, 10])
    parser.add_argument('--outdir', default='sweep_results')
    args = parser.parse_args()
    for t in args.thicknesses:
        directory = (Path(args.outdir) / f'al_{t:g}mm').resolve()
        directory.mkdir(parents=True, exist_ok=True)
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
