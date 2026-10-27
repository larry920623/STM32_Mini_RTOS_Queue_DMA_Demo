#!/usr/bin/env python3
"""Standalone GCC verification build; CubeIDE remains the supported GUI workflow."""
from pathlib import Path
import argparse
import os
import subprocess
import shlex

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default=os.environ.get('ARM_CC','arm-none-eabi-gcc'))
    parser.add_argument('--mode',choices=['debug','release'],default='debug')
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    build=root/'build'/args.mode;build.mkdir(parents=True,exist_ok=True)
    includes=['Core/Inc','Drivers/STM32F4xx_HAL_Driver/Inc','Drivers/STM32F4xx_HAL_Driver/Inc/Legacy',
              'Drivers/CMSIS/Device/ST/STM32F4xx/Include','Drivers/CMSIS/Include']
    flags=['-mcpu=cortex-m4','-mthumb','-mfpu=fpv4-sp-d16','-mfloat-abi=hard','-std=gnu11',
           '-DUSE_HAL_DRIVER','-DSTM32F407xx','-ffunction-sections','-fdata-sections',
           '-Wall','-Wextra','-fstack-usage','-g3','-O0' if args.mode=='debug' else '-O2']
    flags += ['-I'+str(root/p) for p in includes]
    flags += shlex.split(os.environ.get('ARM_EXTRA_FLAGS', ''))
    sources=sorted((root/'Core/Src').glob('*.c'))+sorted((root/'Drivers/STM32F4xx_HAL_Driver/Src').glob('*.c'))
    sources += sorted((root/'Core/Startup').glob('*.s'))
    objects=[]
    for src in sources:
        obj=build/('_'.join(src.relative_to(root).parts)+'.o')
        subprocess.run([args.cc,*flags,'-c',str(src),'-o',str(obj)],check=True)
        objects.append(str(obj))
    elf=build/'STM32_UART_Workbench.elf'
    subprocess.run([args.cc,*flags,*objects,'-T'+str(root/'STM32F407VETX_FLASH.ld'),
        '--specs=nano.specs','--specs=nosys.specs','-Wl,--gc-sections',
        '-Wl,-Map='+str(build/'STM32_UART_Workbench.map'),'-Wl,--print-memory-usage',
        '-Wl,--start-group','-lc','-lm','-Wl,--end-group','-o',str(elf)],check=True)
    size=str(Path(args.cc).with_name('arm-none-eabi-size')) if '/' in args.cc else 'arm-none-eabi-size'
    subprocess.run([size,str(elf)],check=True)
    objcopy=str(Path(args.cc).with_name('arm-none-eabi-objcopy')) if '/' in args.cc else 'arm-none-eabi-objcopy'
    subprocess.run([objcopy,'-O','binary',str(elf),str(elf.with_suffix('.bin'))],check=True)
    print('Built',elf)
if __name__=='__main__':main()
