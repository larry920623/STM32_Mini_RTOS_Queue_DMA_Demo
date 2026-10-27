#!/usr/bin/env python3
"""Optional PC helper: python tools/send_commands.py --port COM3 --demo
Install pyserial first. Close Tera Term before using this helper.
"""
import argparse
import threading
import time

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', required=True)
    parser.add_argument('--baud', type=int, default=115200)
    parser.add_argument('--demo', action='store_true')
    args = parser.parse_args()
    try:
        import serial
    except ImportError:
        parser.error('pyserial is required: python -m pip install pyserial')
    with serial.Serial(args.port, args.baud, timeout=0.2, write_timeout=2) as link:
        stop = threading.Event()
        def reader():
            while not stop.is_set():
                try:
                    line = link.readline()
                    if line:
                        print(line.decode('utf-8', errors='replace').rstrip(), flush=True)
                except (serial.SerialException, OSError):
                    stop.set()
        thread = threading.Thread(target=reader, daemon=True)
        thread.start()
        def send(command):
            print('> ' + command, flush=True)
            link.write((command + '\r\n').encode('utf-8'))
        try:
            time.sleep(0.3)
            if args.demo:
                for command in ('help', 'stats 10 20 30', 'sort 9 -3 4 4 0', 'status'):
                    send(command); time.sleep(0.2)
            print('Interactive mode: enter commands; use quit or Ctrl+C to close.')
            while not stop.is_set():
                line = input()
                if line.strip() == 'quit':break
                send(line)
        except (KeyboardInterrupt, EOFError):
            pass
        finally:
            stop.set(); thread.join(timeout=1)
if __name__ == '__main__':
    main()
