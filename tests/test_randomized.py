#!/usr/bin/env python3
"""Independent reference checks against Python sort/sum."""
from pathlib import Path
import os
import random
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
rng=random.Random(407)
with tempfile.TemporaryDirectory() as directory:
    runner=Path(directory)/'job_runner'
    subprocess.run([os.environ.get('CC','gcc'),'-std=c11','-O2','-Wall','-Wextra','-Werror',
                    '-ICore/Inc','Core/Src/work_job.c','tests/job_runner.c','-o',str(runner)],cwd=root,check=True)
    def run(command):
        return subprocess.check_output([str(runner),command],text=True).strip()
    for _ in range(100):
        values=[rng.randint(-2147483648,2147483647) for _ in range(rng.randint(1,16))]
        assert run('sort '+' '.join(map(str,values)))==' '.join(map(str,sorted(values)))
        result=run('stats '+' '.join(map(str,values)))
        assert f'sum={sum(values)} ' in result
print('PASS: 100 randomized sort/stats inputs vs Python references')
