#!/usr/bin/env python3
"""Optional Windows/macOS/Linux desktop client. Install pyserial; close other COM users."""
import queue
import threading
import tkinter as tk
from tkinter import messagebox, scrolledtext, ttk
try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None

class WorkbenchGUI:
    def __init__(self, root):
        self.root = root
        self.link = None
        self.stop = threading.Event()
        self.events = queue.Queue()
        root.title('STM32 Data Workbench')
        root.geometry('960x720')
        root.minsize(800, 600)
        style = ttk.Style(root)
        if 'clam' in style.theme_names():
            style.theme_use('clam')
        style.configure('Title.TLabel', font=('Segoe UI', 19, 'bold'))
        style.configure('TButton', padding=6)
        frame = ttk.Frame(root, padding=18)
        frame.pack(fill='both', expand=True)
        ttk.Label(frame, text='STM32 Data Workbench', style='Title.TLabel').pack(anchor='w')
        ttk.Label(frame, text='資料統計與排序 — 由 STM32 工作佇列處理').pack(anchor='w', pady=(3, 14))
        bar = ttk.Frame(frame)
        bar.pack(fill='x')
        ttk.Label(bar, text='串口').pack(side='left')
        self.port = ttk.Combobox(bar, width=20)
        self.port.pack(side='left', padx=8)
        ttk.Button(bar, text='重新掃描', command=self.refresh).pack(side='left')
        self.connect_button = ttk.Button(bar, text='連線', command=self.toggle)
        self.connect_button.pack(side='left', padx=8)
        self.status = tk.StringVar(value='未連線 · 115200 8N1')
        ttk.Label(bar, textvariable=self.status).pack(side='left', padx=6)
        jobs = ttk.LabelFrame(frame, text='提交資料處理工作', padding=12)
        jobs.pack(fill='x', pady=14)
        self.inputs = {}
        definitions = [('stats', '數值統計', '10 20 30'), ('sort', '數值排序', '9 -3 4 4 0')]
        for row, (command, title, default) in enumerate(definitions):
            ttk.Label(jobs, text=title, width=20).grid(row=row, column=0, sticky='w', pady=4)
            entry = ttk.Entry(jobs)
            entry.insert(0, default)
            entry.grid(row=row, column=1, sticky='ew', padx=8, pady=4)
            entry.bind('<Return>', lambda event, c=command: self.send_job(c))
            self.inputs[command] = entry
            ttk.Button(jobs, text='提交', command=lambda c=command: self.send_job(c)).grid(row=row, column=2)
        jobs.columnconfigure(1, weight=1)
        actions = ttk.Frame(frame)
        actions.pack(fill='x', pady=(0, 8))
        ttk.Button(actions, text='查詢狀態', command=lambda: self.send('status')).pack(side='left')
        ttk.Button(actions, text='命令說明', command=lambda: self.send('help')).pack(side='left', padx=8)
        ttk.Button(actions, text='清除畫面', command=self.clear).pack(side='right')
        self.log = scrolledtext.ScrolledText(frame, font=('Consolas', 10), wrap='word', state='disabled',
                                           background='#102131', foreground='#d8e8f5', insertbackground='white')
        self.log.pack(fill='both', expand=True)
        for name, color in [('sent', '#89b8e8'), ('result', '#83d8a7'), ('error', '#ffa79b'), ('status', '#e1ce90')]:
            self.log.tag_configure(name, foreground=color)
        raw = ttk.Frame(frame)
        raw.pack(fill='x', pady=(10, 0))
        self.raw = ttk.Entry(raw)
        self.raw.pack(side='left', fill='x', expand=True)
        self.raw.bind('<Return>', lambda event: self.send_raw())
        ttk.Button(raw, text='傳送命令', command=self.send_raw).pack(side='right', padx=(8, 0))
        ttk.Label(frame, text='請先燒錄韌體；USB-to-TTL TX → PA10，RX → PA9，GND 共地。連線時請關閉 Tera Term。').pack(anchor='w', pady=(8, 0))
        self.refresh()
        root.protocol('WM_DELETE_WINDOW', self.close)
        root.after(80, self.poll)

    def refresh(self):
        ports = [item.device for item in list_ports.comports()] if serial else []
        self.port['values'] = ports
        if not self.port.get() and ports:
            self.port.set(ports[0])

    def append(self, text, tag=''):
        self.log.configure(state='normal')
        self.log.insert('end', text + '\n', tag)
        self.log.see('end')
        self.log.configure(state='disabled')

    def clear(self):
        self.log.configure(state='normal')
        self.log.delete('1.0', 'end')
        self.log.configure(state='disabled')

    def toggle(self):
        if self.link:
            self.disconnect()
            return
        if not serial:
            messagebox.showerror('缺少套件', '請先執行：python -m pip install pyserial')
            return
        try:
            self.link = serial.Serial(self.port.get().strip(), 115200, timeout=0.2, write_timeout=1)
        except (serial.SerialException, OSError, ValueError) as exc:
            self.link = None
            messagebox.showerror('連線失敗', str(exc))
            return
        self.stop = threading.Event()
        link, stop = self.link, self.stop
        self.connect_button.configure(text='斷線')
        self.status.set('已連線 · ' + link.port + ' · 115200 8N1')
        self.append('[PC] 已連線。可按板上 RESET 看開機說明，或直接提交工作。', 'status')
        def reader():
            while not stop.is_set():
                try:
                    data = link.readline()
                    if data:
                        self.events.put(('line', data.decode('utf-8', errors='replace').rstrip()))
                except (serial.SerialException, OSError) as exc:
                    if not stop.is_set():
                        self.events.put(('error', str(exc)))
                    break
        threading.Thread(target=reader, daemon=True).start()

    def disconnect(self):
        self.stop.set()
        if self.link:
            self.link.close()
        self.link = None
        self.connect_button.configure(text='連線')
        self.status.set('未連線 · 115200 8N1')

    def send(self, command):
        if not self.link:
            messagebox.showinfo('尚未連線', '請先選擇串口並按「連線」。')
            return
        payload = command.encode('utf-8')
        if not payload or b'\r' in payload or b'\n' in payload or len(payload) >= 256:
            messagebox.showerror('輸入錯誤', '單行命令需為 1–255 bytes，不能包含換行。')
            return
        try:
            self.link.write(payload + b'\r\n')
            self.append('> ' + command, 'sent')
        except (serial.SerialException, OSError) as exc:
            self.append('[PC] ' + str(exc), 'error')
            self.disconnect()

    def send_job(self, command):
        self.send(command + ' ' + self.inputs[command].get())

    def send_raw(self):
        command = self.raw.get()
        if command:
            self.send(command)
            self.raw.delete(0, 'end')

    def poll(self):
        try:
            while True:
                kind, text = self.events.get_nowait()
                if kind == 'error':
                    self.append('[PC] ' + text, 'error')
                    self.disconnect()
                else:
                    tag = 'result' if text.startswith('RESULT') else 'error' if text.startswith(('ERR', 'BUSY')) else 'status' if text.startswith(('STATUS', 'COUNTERS')) else ''
                    self.append(text, tag)
        except queue.Empty:
            pass
        self.root.after(80, self.poll)

    def close(self):
        self.disconnect()
        self.root.destroy()

if __name__ == '__main__':
    root = tk.Tk()
    WorkbenchGUI(root)
    root.mainloop()
