# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Build the host audio converter and full, looping Home sound."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import wave

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'artwork/audio/LTqj8VkTC8z-tTY7vztlF.mp3'

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def validate(at9,preview):
    import struct
    data=at9.read_bytes()
    if len(data)>2*1024*1024 or data[:4]!=b'RIFF' or data[8:12]!=b'WAVE':
        raise ValueError('Invalid PS5 Home sound container')
    if struct.unpack_from('<I',data,4)[0]+8!=len(data):raise ValueError('Invalid RIFF length')
    chunks={};at=12
    while at+8<=len(data):
        tag=data[at:at+4];size=struct.unpack_from('<I',data,at+4)[0];at+=8
        if at+size>len(data):raise ValueError('Truncated RIFF chunk')
        chunks[tag]=data[at:at+size];at+=size+(size&1)
    if at!=len(data) or b'smpl' not in chunks:raise ValueError('Home sound must loop')
    if not {b'fmt ',b'fact',b'data'}<=chunks.keys():raise ValueError('Missing ATRAC9 chunks')
    fmt=chunks[b'fmt ']
    if struct.unpack_from('<HHI',fmt)!=(0xfffe,2,48000):raise ValueError('Wrong ATRAC9 channel/rate profile')
    frames=5760000
    if struct.unpack_from('<I',chunks[b'fact'])[0]!=frames:raise ValueError('Wrong Home clip length')
    loop=chunks[b'smpl']
    if len(loop)!=60 or struct.unpack_from('<I',loop,28)[0]!=1:raise ValueError('Wrong loop count')
    if struct.unpack_from('<II',loop,44)!=(256,frames+255):raise ValueError('Wrong loop bounds')
    with wave.open(str(preview),'rb') as wav:
        if (wav.getframerate(),wav.getnchannels(),wav.getsampwidth(),wav.getnframes())!=(48000,2,2,frames):
            raise ValueError('Wrong preview PCM format')
        pcm=wav.readframes(frames)
        # Actual processed fade must reach digital silence at its final sample.
        if pcm[-4:]!=b'\0'*4:raise ValueError('Fade did not finish at silence')
    return dict(bytes=len(data),sample_rate=48000,channels=2,frames=frames,loop=True)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx',required=True,type=Path)
    parser.add_argument('--output',type=Path,default=ROOT/'out/audio')
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    expected=json.loads((SOURCE.parent/'source-metadata.json').read_text())['sha256']
    if sha(SOURCE)!=expected:raise ValueError('Original MP3 changed, including its provenance tags')
    vendor=ROOT/'tools/audio/third_party/psgfx_at9'
    executable=out/'make_home_sound.exe'
    sources=[ROOT/'tools/audio/make_home_sound.cpp']+[vendor/n for n in
             ('at9_encoder.cpp','at9_snd0.cpp','at9_decode.cpp','at9_tables.cpp')]
    subprocess.run([str(args.cxx),'-std=c++17','-O2','-static','-pthread',*map(str,sources),'-o',str(executable)],check=True)
    target=SOURCE.parent/'snd0.at9';preview=out/'home-preview.wav'
    result=subprocess.run([str(executable),str(SOURCE),str(target),str(preview)],check=True,capture_output=True,text=True)
    report=json.loads(result.stdout);report.update(validate(target,preview))
    report.update(source_sha256=sha(SOURCE),snd0_sha256=sha(target),encoder_revision='5ee7aa9e7d91c9f518322847d13a6bda5ba97837',
                  validation='HOST_CONTAINER_AND_PCM_PASS',console_home_playback='NOT_TESTED')
    (SOURCE.parent/'home-sound.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))

if __name__=='__main__':main()
