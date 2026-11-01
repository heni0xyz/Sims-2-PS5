"""Summarize private UI bridge XML; never distribute the captured game data."""
from pathlib import Path
import argparse,json,re,xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser();p.add_argument('log',type=Path);p.add_argument('--details',action='store_true');a=p.parse_args()
    out=ROOT/'out/ui-layout'/a.log.parent.name;out.mkdir(parents=True,exist_ok=True)
    records=[];chunks={};legacy=[]
    for line in a.log.read_text(errors='replace').splitlines():
        if '[UI_LAYOUT][LoadWidget' not in line or ' xml=' not in line:continue
        text=line.split(' xml=',1)[1]
        parts=re.search(r'\[UI_LAYOUT\]\[(LoadWidget(?:Patched)?)\] record=(\d+) part=(\d+)/(\d+)',line)
        if parts:
            operation,key,part,count=parts.groups();item=chunks.setdefault(key,{'operation':operation,'parts':{},'count':int(count)})
            item['parts'][int(part)]=text
        else:legacy.append(('LoadWidget',text))
    assembled=legacy+[(v['operation'],''.join(v['parts'].get(i,'') for i in range(1,v['count']+1))) for v in chunks.values()]
    for operation,text in assembled:
        label=re.search(r'<scrnid>([^<]+)',text)
        if not label:continue
        label=label.group(1);(out/(str(len(records))+'-'+label+'.xml')).write_text(text)
        record={'screen':label,'operation':operation,'nodes':[],'truncated':False}
        try:root=ET.fromstring(text)
        except ET.ParseError:
            record['truncated']=True
            record['prefix']=text[:1500]
        else:
            for node in root.iter():
                values={child.tag:child.text for child in node if not len(child)}
                if any(key.endswith('id') for key in values):record['nodes'].append({'kind':node.tag,**values})
        records.append(record)
    (out/'summary.json').write_text(json.dumps(records,indent=2)+'\n')
    for record in records:
        if a.details and record['screen'] in ('0MOD','0MOT','0ACT','0CXM'):print(json.dumps(record,indent=2))
        elif not a.details:print(record['operation'],record['screen'],'nodes',len(record['nodes']),'truncated',record['truncated'])
    print('Private summary:',out/'summary.json')
if __name__=='__main__':main()
