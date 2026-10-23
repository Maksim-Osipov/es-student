"""Generate explicitly labelled classroom examples, never hardware measurements.

2.1 memory addresses come from the locally built ELF/bin. Stack/heap locations,
USB identity, clocks and execution times are model inputs. 2.3 runs production
C command/task code in tests/host/example_runtime with a deterministic clock.
Real board capture remains available through the unchanged check-*.py scripts.
"""
from pathlib import Path
import argparse
import ast
import hashlib
import math
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SERIAL = '0000000000000000'  # Reserved example identity, not a measured board ID.

def script_constants(path):
    values = {}
    for node in ast.parse(path.read_text(encoding='utf-8')).body:
        if isinstance(node, ast.Assign) and all(isinstance(t, ast.Name) and t.id.isupper() for t in node.targets):
            code = compile(ast.Module(body=[node], type_ignores=[]), str(path), 'exec')
            exec(code, {'__builtins__': {}}, values)
    return values

def write_log(project, task, events, provenance=''):
    text = [
        'источник: учебный пример, без платы; время и USB-идентичность модельные',
        f'задание: {task}', f'проект: {project}', 'устройство: 2e8a:000a',
        f'серийный номер: {SERIAL}', 'порт: SIMULATED',
        'начало: 2026-10-23T00:00:00+03:00 (условная эпоха примера)',
    ]
    if provenance:
        text.append('модель: ' + provenance)
    for t, direction, line in sorted(events, key=lambda x: x[0]):
        text.append(f'{t:8.3f} {direction} {line}')
    text.append(f'итог: отправлено кадров {sum(e[1]=="-->" for e in events)}, принято строк {sum(e[1]=="<--" for e in events)}')
    path = ROOT / project / f'device-{task.replace(".", "-")}.log'
    path.write_text('\n'.join(text)+'\n', encoding='utf-8', newline='\n')
    print(path.relative_to(ROOT))

class Image:
    def __init__(self, project):
        build = ROOT / project / 'build'
        self.elf = next(build.glob('*.elf'))
        self.binary = next(build.glob('*.bin')).read_bytes()
        self.symbols = {}
        table = subprocess.check_output(['arm-none-eabi-nm', '-n', str(self.elf)], text=True)
        for row in table.splitlines():
            fields = row.split()
            if len(fields) == 3:
                self.symbols[fields[2]] = int(fields[0], 16)
        self.digest = hashlib.sha256(self.binary).hexdigest()

    def __getitem__(self, name):
        return self.symbols[name]

    def string(self, text):
        return 0x10000000 + self.binary.index(text.encode() + b'\0')

    def halfword(self, address):
        return struct.unpack_from('<H', self.binary, (address & ~1)-0x10000000)[0]

    def boot(self, on):
        top, reset = struct.unpack_from('<II', self.binary, 0x100)
        return ['vector table   0x10000100', f'  stack top    0x{top:08x}',
                f'  reset        0x{reset:08x}', f'  reset (even) 0x{reset & ~1:08x}',
                'gpio in        0xd0000004', f'  led bit      {int(on)}', f'  gpio_get     {int(on)}']

    def memory(self):
        s = self.symbols
        data = s['__data_end__']-s['__data_start__']
        regions = [
            ('flash', 0x10000000, 0x10200000), ('sram', 0x20000000, 0x20042000), ('rom', 0, 16384),
            ('image', s['__flash_binary_start'], s['__flash_binary_end']),
            ('free', s['__flash_binary_end'], 0x10200000),
            ('boot2', s['__boot2_start__'], s['__boot2_end__']), ('text', s['__boot2_end__'], s['__etext']),
            ('data flash', s['__etext'], s['__etext']+data), ('data ram', s['__data_start__'], s['__data_end__']),
            ('bss', s['__bss_start__'], s['__bss_end__']), ('heap', s['__bss_end__'], s['__HeapLimit']),
            ('stack', s['__StackBottom'], s['__StackTop']),
        ]
        sizes = {n: b-a for n, a, b in regions}
        lines = ['area       start      end        size']
        lines += [f'{n:10} 0x{a:08x} 0x{b:08x} {b-a:8}' for n, a, b in regions]
        lines += ['total', f'  flash image {sizes["image"]} = boot2 {sizes["boot2"]} + text {sizes["text"]} + data {data}',
                  f'  flash free {sizes["free"]} of 2097152', f'  ram used {data+sizes["bss"]} = data {data} + bss {sizes["bss"]}',
                  f'  ram free {sizes["heap"]} for heap and {sizes["stack"]} for stack']
        return lines

    def firmware(self, count):
        lines = ['object          address     value']
        for name in ['main', 'fw_info']:
            addr = self[name] | 1
            lines.append(f'{name:15} 0x{addr:08x} 0x{self.halfword(addr):04x}')
        lines.append(f'commands        0x{self["commands"]:08x}')
        source = (ROOT/'211-command-usb/main.c').read_text()
        for command, handler in re.findall(r'\{"(\w+)", (\w+)\}', source):
            lines.append(f'- {command:13} 0x{self[handler] | 1:08x}')
        for name, value in [('DEVICE_PROJECT','211-command-usb'), ('DEVICE_BOARD','pico')]:
            lines.append(f'{name:15} 0x{self.string(value):08x} {value}')
        # Model of one fixed call depth and a freed/reused allocation, not a runtime observation.
        for name, addr, value in [('data_variable', self['data_variable'], 100+count),
                                  ('bss_variable', self['bss_variable'], count),
                                  ('stack_variable', self['__StackTop']-128, 1946),
                                  ('heap_variable', (self['__bss_end__']+15)&~7, 1951)]:
            lines.append(f'{name:15} 0x{addr:08x} {value}')
        return lines

    def card(self):
        addr = self['device_card']
        return ['struct          address     size offset value', f'device_card     0x{addr:08x}    20',
                f'- version       0x{addr:08x}     4      0 0x00010000',
                f'- name          0x{addr+4:08x}    13      4 es-cmd-usb',
                f'- revision      0x{addr+17:08x}     1     17 2', 'fields 18, sizeof 20, padding 2']

def passport(project, sdk=False):
    result = [f'project: {project}', 'repo: https://github.com/Maksim-Osipov/es-student',
              'board: pico', f'serial: {SERIAL}', 'chip: manufacturer 0x927, part 0x0002, revision 2']
    if sdk:
        result.append('pico-sdk: 2.3.0')
    return result

def legacy():
    write_log('131-hello-usb','1.3.2',[(float(i),'<--','Hello, world!') for i in range(10)])
    write_log('133-led-button-usb','1.3.3',[(1.0+2*i,'<--','led '+('on' if i%2==0 else 'off')) for i in range(6)])
    for project, task, commands in [
        ('133-led-button-usb','1.3.4',['e','d','e','d','x']),
        ('134-led-module','1.3.6',['v','e','d','q']),
        ('134-led-module','1.3.7',['i'])]:
        events=[]
        for i, cmd in enumerate(commands):
            t=float(i*2); events.append((t,'-->',cmd))
            if cmd=='i': reply=passport(project)
            elif cmd=='v': reply=['device: es-led-module 1.0.0, built Oct 23 2026 00:00:00, log level 3']
            elif cmd in 'ed': reply=[('inf handle_command:22 ' if task=='1.3.6' else '')+'led '+('on' if cmd=='e' else 'off')]
            else: reply=[('err handle_command:43 ' if task=='1.3.6' else '')+'unknown command: '+cmd]
            events += [(t+.02+j*.001,'<--',line) for j,line in enumerate(reply)]
        write_log(project,task,events)

def lesson21():
    project='211-command-usb'; image=Image(project)
    for n in range(1,7):
        cfg=script_constants(ROOT/project/f'check-2-1-{n}.py')
        events=[]; on=False; calls=0
        for i,cmd in enumerate(cfg.get('COMMANDS',[cfg.get('COMMAND')])):
            t=i*cfg.get('ANSWER_S',2); events.append((t,'-->',cmd)); reply=[cmd]
            if cmd=='enable': on=True; reply+=['inf cmd_enable:18 led on']
            elif cmd=='disable': on=False; reply+=['inf cmd_disable:19 led off']
            elif cmd=='info': reply+=passport(project,True)
            elif cmd=='ping': reply+=['pong']
            elif cmd=='mem_info': reply+=image.memory()
            elif cmd=='fw_info': calls+=1; reply+=image.firmware(calls)
            elif cmd=='dev_info': reply+=image.card()
            elif cmd=='boot_info': reply+=image.boot(on)
            else: reply+=['err handle_command:45 unknown command: '+cmd]
            events += [(t+.01+j*.001,'<--',line) for j,line in enumerate(reply)]
        write_log(project,f'2.1.{n}',events,'ELF/bin SHA256 '+image.digest+'; стек/куча — модель размещения')

def lesson22():
    project='221-command-time'; image=Image(project)
    # Cost estimate from lesson 2.2: 585 cycles per term at 125 MHz.
    pi=sum((1.0 if k%2==0 else -1.0)/(2.0*k+1.0) for k in range(1000000))*4
    for n in range(1,6):
        cfg=script_constants(ROOT/project/f'check-2-2-{n}.py')
        steps=cfg.get('STEPS',[(c,cfg.get('ANSWER_S',2)) for c in cfg.get('COMMANDS',[])])
        events=[]; t=0.; busy=0.; low=False; peri=125000; maximum=500; calc_end=-100.
        for cmd,wait in steps:
            events.append((t,'-->',cmd)); at=max(t+.01,busy+.01); reply=[cmd]
            if cmd=='info': reply+=passport(project,True)
            elif cmd=='clk_info':
                reply+=['clock      configured measured']
                for name,hz in [('clk_ref',12000),('clk_sys',62500 if low else 125000),('clk_peri',peri),('clk_usb',48000),('clk_adc',48000)]:
                    reply.append(f'{name:10} {hz:8} {hz:8} kHz')
                reply+=['rosc 6453']
            elif cmd=='uptime': reply+=[f'uptime: {int((at+.001+5)*1000)} ms']
            elif cmd=='boot_info': reply+=image.boot(int((at+5)/.5)%2)
            elif cmd=='clk_sys_low': low=True; peri=48000; reply+=['inf clk_sys_set:33 clk_sys 62500 kHz']
            elif cmd=='clk_sys_default': low=False; peri=48000; reply+=['inf clk_sys_set:33 clk_sys 125000 kHz']
            elif cmd=='main_time_reset': maximum=500; reply+=['max reset']
            elif cmd=='main_time_exec':
                average=(4.8 if low else 2.4)+9*math.exp(-(at-calc_end))
                reply+=[f'iteration avg {average:.2f} us, max {maximum} us']
            elif cmd=='calc_pi':
                events.append((at,'<--',cmd))
                duration=9.36 if low else 4.68
                calc_end=busy=at+duration; maximum=int(duration*1000000)+500
                at=busy; reply=[f'pi: {pi:.8f}',f'time: {int(duration*1000)} ms']
            else: raise ValueError(cmd)
            events += [(at+j*.001,'<--',line) for j,line in enumerate(reply)]
            t+=wait
        write_log(project,f'2.2.{n}',events,'585 тактов/член; ROSC=6453 кГц — пример, не замер; ELF/bin SHA256 '+image.digest)

def lesson23(runtime):
    project='231-firmware-template'
    cfg=script_constants(ROOT/project/'check-2-3-8.py')
    events=[]; script=[]; t=0.
    def printable(s): return s.replace('\x7f','<BS>').replace('\b','<BS>').replace('\r','').replace('\n','').strip()
    for frame,wait in cfg['STEPS']:
        events.append((t,'-->',printable(frame)))
        script.append(f'{round(t*1000000)} {frame.encode().hex()}')
        t+=wait
    script.append(f'{round(t*1000000)} ')
    run=subprocess.run([str(runtime)],input='\n'.join(script)+'\n',text=True,capture_output=True,check=True)
    for line in run.stdout.splitlines():
        stamp, text=line.split('\t',1)
        if text.strip(): events.append((float(stamp),'<--',printable(text)))
    write_log(project,'2.3.8',events,'исполнен C-код задач/API; часы и GPIO подменены; 300 мкс/64 члена π')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime',type=Path,required=True,help='Native example_runtime executable')
    args=parser.parse_args()
    legacy(); lesson21(); lesson22(); lesson23(args.runtime.resolve())
