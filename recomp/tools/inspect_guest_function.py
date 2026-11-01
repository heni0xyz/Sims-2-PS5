"""Read the recompiler's decoded annotations; never edit game-derived output."""
from pathlib import Path
import argparse,re
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser();p.add_argument('address',type=lambda x:int(x,16))
    p.add_argument('--full',action='store_true');a=p.parse_args()
    name=f'__imp__sub_{a.address:08X}'
    for source in (ROOT/'generated/independent').glob('x86_recomp.*.cpp'):
        text=source.read_text();needle='X86_FUNC('+name+')'
        pos=text.find(needle)
        if pos<0:continue
        end=text.find('\nX86_FUNC(',pos+len(needle));body=text[pos:end if end>=0 else len(text)]
        private=ROOT/'out/inspection';private.mkdir(parents=True,exist_ok=True)
        (private/(name+'.cpp.txt')).write_text(body)
        lines=body.splitlines()
        print('source:',source.name,'lines:',len(lines))
        comments=[line.strip() for line in lines if '// ' in line]
        if a.full:print('\n'.join(comments))
        else:print('\n'.join(comments[:35]+['...branches...']+[x for x in comments if re.search(r'\b(j\w+|loop|pause)\b',x)]+comments[-10:]))
        return
    raise SystemExit('Function not present in this independent generation')
if __name__=='__main__':main()
