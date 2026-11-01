# SPDX-License-Identifier: GPL-3.0-only
"""PyInstaller entry point; GUI normally, CLI when explicit arguments are given."""
import sys
from pathlib import Path

def bundled_kit():
    beside=Path(sys.executable).parent/'kit'
    return beside if beside.is_dir() else Path(getattr(sys,'_MEIPASS',Path(__file__).resolve().parents[1]))/'kit'

if __name__=='__main__':
    if len(sys.argv)==3 and sys.argv[1]=='--verify-kit':
        import json
        import tkinter as tk
        from builder.core import verify_kit
        result=verify_kit(bundled_kit())
        window=tk.Tk();window.withdraw();window.update();window.destroy()
        Path(sys.argv[2]).write_text(json.dumps(dict(status='PASS',host=result['host'],
            files=len(result['files']),tk_initialization='PASS'))+'\n')
        raise SystemExit(0)
    if len(sys.argv)>1:
        if '--kit' not in sys.argv:sys.argv.extend(['--kit',str(bundled_kit())])
        from builder.__main__ import main
        raise SystemExit(main())
    from builder.gui import main
    main()
