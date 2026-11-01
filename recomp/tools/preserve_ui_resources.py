"""Private owner UI research archive: original ARC records and real GPU targets.

No source assets are modified. Output is always under ignored private/ and
must never be included in a public source/build archive.
"""
from pathlib import Path
import argparse,datetime,hashlib,json,re,shutil,struct,sys
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/ps5'))
from patch_prompts import read_index,digest

def save_json(path,value):path.write_text(json.dumps(value,indent=2)+'\n')

def main():
    p=argparse.ArgumentParser();p.add_argument('--run',help='Include a completed one-shot GPU frame capture from this local run')
    a=p.parse_args()
    archive=ROOT/'private/owner-assets/DATA/textures.arc'
    source_hash=digest(archive)
    if source_hash!='c4b8efc39156b9091ed7050810cedc4bb5c2080775bb208e8ab77447a802304e':raise ValueError('Expected untouched supported owner archive')
    label=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    out=ROOT/'private/ui-research'/label;out.mkdir(parents=True,exist_ok=False)
    resources=[]
    with archive.open('rb') as stream:
        entries=read_index(stream)
        save_json(out/'complete-texture-index.json',{name:{'offset':v[0],'bytes':v[1]} for name,v in entries.items()})
        prompts=json.loads((ROOT/'mods/dualsense/manifest.json').read_text())
        named={r['resource'] for r in prompts['replacements']}
        named.update({'head_backdrop','s2c_logo','ui_logo256_english_light','ui_logo256_english_dark','maxis_logo_black_clean'})
        selected=[name for name in entries if name in named or name.startswith(('hud_','mood_','cat_motive_','ui_dialog_','dialog_','dialogs_'))]
        records=out/'original-arc-records';records.mkdir()
        for i,name in enumerate(sorted(selected)):
            offset,size=entries[name];stream.seek(offset);data=stream.read(size)
            if len(data)!=size:raise ValueError('Truncated owner resource')
            path=records/(f'{i:04d}.arc-record');path.write_bytes(data)
            record={'resource':name,'file':str(path.relative_to(out)),'offset':offset,'bytes':size,
                    'sha256':hashlib.sha256(data).hexdigest(),'kind':'static owner ARC record'}
            # Preserve the complete native record and header, not a resampled
            # preview. Generic TXFL payload dimensions are at +16/+18.
            if size>=20:
                kind,tag,unknown,name_length=struct.unpack_from('<4I',data)
                start=20+name_length
                if kind==9 and tag==0x5458464c and start+32<=size:
                    payload=data[start:];w,h=struct.unpack_from('<HH',payload,16)
                    record.update(payload_header_hex=payload[:32].hex(),width=w,height=h)
            resources.append(record)
    # Flash widgets have their own native geometry/paletted artwork. Preserve
    # complete original records as well, so later 4K work can identify their
    # source and alpha instead of modifying a screenshot or guessing a texture.
    flash=ROOT/'private/owner-assets/DATA/flashes.arc'
    flash_records=[]
    with flash.open('rb') as stream:
        flash_index=read_index(stream)
        folder=out/'original-flash-records';folder.mkdir()
        for i,name in enumerate(('s2c_comp_dpad_up','s2c_comp_dpad_down','s2c_comp_motives',
                's2c_comp_motivebar','s2c_comp_dialogbox','s2c_comp_waf',
                's2c_comp_hud_ff','s2c_comp_hud_pause','s2c_comp_hud_interaction')):
            offset,size=flash_index[name];stream.seek(offset);data=stream.read(size)
            if len(data)!=size:raise ValueError('Truncated owner Flash resource')
            path=folder/(f'{i:04d}.arc-record');path.write_bytes(data)
            flash_records.append({'resource':name,'file':str(path.relative_to(out)),
                'offset':offset,'bytes':size,'sha256':hashlib.sha256(data).hexdigest(),
                'kind':'complete original owner Flash ARC record'})
    targets=[];run_info=None
    if a.run:
        if not re.fullmatch(r'\d{8}T\d{6}Z-[a-f0-9]{6}',a.run):p.error('Use a local run ID')
        run=ROOT/'out/runs'/a.run
        run_info=json.loads((run/'process.json').read_text())
        dest=out/'gpu-frame-captures';dest.mkdir()
        for trace in sorted((run/'cap/vulkan').glob('*/frame.txt')):
            folder=dest/trace.parent.name;folder.mkdir()
            shutil.copyfile(trace,folder/'frame.txt')
            for source in sorted(trace.parent.glob('*.bmp')):
                data=source.read_bytes();w,h=struct.unpack_from('<ii',data,18)
                bits=struct.unpack_from('<H',data,28)[0]
                offset=struct.unpack_from('<I',data,10)[0]
                if w<=0 or h==0 or bits!=32 or len(data)-offset!=w*abs(h)*4:raise ValueError('Unexpected capture format')
                raw=folder/source.name;raw.write_bytes(data)
                image=Image.frombytes('RGBA',(w,abs(h)),data[offset:],'raw','BGRA')
                if h>0:image=image.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
                png=folder/(source.stem+'.png');image.save(png)
                targets.append({'file':str(raw.relative_to(out)),'png':str(png.relative_to(out)),
                    'sha256':hashlib.sha256(data).hexdigest(),'width':w,'height':abs(h),
                    'kind':'cached GPU render target','target_128x128':w==128 and abs(h)==128,
                    'pixel_format':'BGRA8 bytes; lossless RGBA PNG preserves alpha',
                    'resolution_note':'captured current guest target, not an upscaled replacement'})
    manifest={'schema':1,'private_owner_content':True,'distribution':'DO_NOT_PUBLISH',
        'source_archive_sha256':source_hash,'complete_index_entries':len(entries),
        'static_resources':resources,'flash_source_archive_sha256':digest(flash),
        'flash_resources':flash_records,'gpu_targets':targets,'run':run_info,
        'scope':'4K research inventory; the observed 128x128 cache is a full-body Sim preview, not an identified HUD head texture'}
    save_json(out/'manifest.json',manifest)
    (out/'PRIVATE-OWNER-CONTENT.txt').write_text('Original owner game artwork and real GPU render targets. Keep private.\n'
        'Zidentyfikowany target 128x128 to miniatura calej postaci, nie sam portret HUD-u.\n'
        'Zachowane oryginaly i metadane; zadnych tekstur nie powiekszano.\n')
    print(json.dumps({'directory':str(out),'static_records':len(resources),'flash_records':len(flash_records),'index_entries':len(entries),
        'gpu_targets':len(targets),'targets_128x128':sum(t['target_128x128'] for t in targets)},indent=2))
if __name__=='__main__':main()
