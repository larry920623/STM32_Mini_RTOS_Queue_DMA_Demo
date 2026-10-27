#!/usr/bin/env python3
"""Host tests with mocked RTOS/IRQ/HAL interfaces, not a hardware simulation."""
from pathlib import Path
import os
import subprocess
import tempfile
import sys
root=Path(__file__).resolve().parents[1]
cc=os.environ.get('CC','gcc')
cases={
 'work_job':['Core/Src/work_job.c','tests/test_work_job.c'],
 'sync_io':['Core/Src/os_sync.c','Core/Src/msg_queue.c','Core/Src/uart_dma.c','tests/test_sync_io.c'],
 'application':['Core/Src/work_job.c','Core/Src/msg_queue.c','tests/test_application.c']}
with tempfile.TemporaryDirectory() as directory:
 for name,sources in cases.items():
  binary=Path(directory)/name
  subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror','-pedantic','-g',
                  '-Itests/fakes','-ICore/Inc',*sources,'-o',str(binary)],cwd=root,check=True)
  subprocess.run([str(binary)],check=True)
subprocess.run([sys.executable,str(root/'tests/test_randomized.py')],check=True)
print('All host tests passed. ARM context switch, NVIC, WFI and physical UART still require the board.')
