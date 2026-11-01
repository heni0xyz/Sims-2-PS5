# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Small standard-library GUI; all building happens in a worker thread."""
from pathlib import Path
import queue
import sys
import threading
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from builder.core import Builder

def main():
    root=tk.Tk();root.title('The Sims 2 Builder');root.geometry('760x440');root.minsize(650,400)
    root.columnconfigure(1,weight=1)
    iso=tk.StringVar();output=tk.StringVar();status=tk.StringVar(value='Select your Xbox USA ISO and a NEW output folder.')
    bundle=Path(sys.executable).parent if getattr(sys,'frozen',False) else Path(__file__).resolve().parents[1]
    kit=bundle/'kit'
    if not kit.is_dir() and hasattr(sys,'_MEIPASS'):kit=Path(sys._MEIPASS)/'kit'
    ttk.Label(root,text='The Sims 2 — PS5',font=('Segoe UI',19)).grid(row=0,column=0,columnspan=3,padx=22,pady=20)
    def select_iso():
        value=filedialog.askopenfilename(title='Your Xbox USA ISO',filetypes=[('Xbox ISO','*.iso')])
        if value:iso.set(value)
    def select_output():
        value=filedialog.askdirectory(title='Choose parent folder for a NEW PPSA99145 folder')
        if value:output.set(str(Path(value)/'PPSA99145'))
    for row,label,var,command in ((1,'Xbox ISO',iso,select_iso),(2,'Output folder',output,select_output)):
        ttk.Label(root,text=label).grid(row=row,column=0,padx=15,pady=8,sticky='w')
        ttk.Entry(root,textvariable=var).grid(row=row,column=1,sticky='ew',pady=8)
        ttk.Button(root,text='Browse…',command=command).grid(row=row,column=2,padx=15)
    ttk.Label(root,text='Includes 16:9, 4K rendering and DualSense prompts. Your ISO is kept.\n'
              'No VM or development setup required. Build time depends on your PC.',justify='left').grid(
                  row=3,column=0,columnspan=3,padx=20,pady=14,sticky='w')
    ttk.Label(root,textvariable=status,wraplength=710).grid(row=4,column=0,columnspan=3,padx=20,pady=14,sticky='w')
    progress=ttk.Progressbar(root,mode='indeterminate');progress.grid(row=5,column=0,columnspan=3,padx=20,sticky='ew')
    messages=queue.Queue();cancel=threading.Event();worker=None;closing=False;resume=None;resume_key=None
    def start():
        nonlocal worker
        if worker and worker.is_alive():return
        cancel.clear();build.configure(state='disabled');progress.start(15)
        selected_iso,selected_output=iso.get(),output.get()
        def work():
            previous=resume if resume_key==(selected_iso,selected_output) else None
            job=Builder(kit,selected_iso,selected_output,lambda s,d:messages.put(('status',s+': '+d)),cancel,resume_work=previous)
            try:
                job.build();messages.put(('done','Build complete. Close the PS5 title before copying PPSA99145.\n'
                                          'Keep the existing hdd folder and saves when updating.'))
            except Exception as error:
                detail=str(error)
                if job.work:detail+='\n\nDiagnostics kept in: '+str(job.work)
                if job.work and (job.work/'build-cache.json').is_file():
                    messages.put(('resume',(job.work,(selected_iso,selected_output))))
                messages.put(('error',detail))
        worker=threading.Thread(target=work,daemon=False);worker.start()
    def close():
        nonlocal closing
        closing=True;cancel.set()
        if not worker or not worker.is_alive():root.destroy()
        else:status.set('Stopping the current tool safely…')
    def poll():
        nonlocal resume,resume_key
        while not messages.empty():
            kind,text=messages.get_nowait()
            if kind=='resume':resume,resume_key=text;continue
            status.set(text)
            if kind in ('done','error'):
                progress.stop();build.configure(state='normal')
                if not closing:
                    (messagebox.showinfo if kind=='done' else messagebox.showerror)('The Sims 2 Builder',text)
        if closing and (not worker or not worker.is_alive()):root.destroy();return
        root.after(150,poll)
    build=ttk.Button(root,text='Build',command=start);build.grid(row=6,column=1,pady=20,sticky='e')
    ttk.Button(root,text='Cancel',command=cancel.set).grid(row=6,column=2,padx=15)
    root.protocol('WM_DELETE_WINDOW',close);root.after(150,poll);root.mainloop()

if __name__=='__main__':main()
