"""Read-only USA XBE inspection; layout follows MIT SDK runtime/xbe/xbe.h."""
from pathlib import Path
import argparse,hashlib,struct
ROOT=Path(__file__).resolve().parents[1]
class Image:
    def __init__(self,path):
        self.data=Path(path).read_bytes()
        if hashlib.sha1(self.data).hexdigest()!='16f9e41ce26c315a758f25d43ba1fa9d3809d1ad':
            raise ValueError('Unsupported owner XBE')
        u=lambda offset:struct.unpack_from('<I',self.data,offset)[0]
        self.base=u(0x104);self.headers=u(0x108)
        self.sections=[]
        for index in range(u(0x11C)):
            at=u(0x120)-self.base+index*56
            self.sections.append(struct.unpack_from('<IIII',self.data,at+4))
    def read(self,va,count):
        if self.base<=va and va+count<=self.base+self.headers:return self.data[va-self.base:va-self.base+count]
        for start,virtual_size,offset,raw_size in self.sections:
            if start<=va and va+count<=start+raw_size:return self.data[offset+va-start:offset+va-start+count]
        raise ValueError('Address is not fully backed by XBE data')
def main():
    p=argparse.ArgumentParser();p.add_argument('address',type=lambda s:int(s,16));p.add_argument('--words',type=int,default=16)
    a=p.parse_args()
    if not 1<=a.words<=256:p.error('words must be 1..256')
    data=Image(ROOT/'assets/default.xbe').read(a.address,a.words*4)
    for index in range(0,a.words,4):
        part=data[index*4:(index+4)*4]
        print('%08X: '%(a.address+index*4)+' '.join('%08X'%x for x in struct.unpack('<'+'I'*(len(part)//4),part)))
if __name__=='__main__':main()
