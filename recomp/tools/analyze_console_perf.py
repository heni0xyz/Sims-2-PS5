"""Summarize app-only PS5 performance windows, keeping different runs separate."""
from pathlib import Path
import argparse,json,re,statistics

def analyze(lines):
    groups={}
    for line in lines:
        if not line.startswith('[Sims2Independent]'):continue
        match=re.search(r't=([\d.]+) build=([a-f0-9]+) run=(\S+).*? native=\S+ (.*)',line)
        if not match:continue
        timestamp,build,run,text=match.groups()
        g=groups.setdefault((build,run),{'build':build,'times':[],'rates':[],'frames':[],'gpu':[],'threads':[],'guest_samples':[],'records':[],'flags':[],'callback_policies':[],
            'configs':[],'scopes':[],'guest_targets':[],'renderer_details':[],'resolution_records':[],'pipeline_cache_records':[]})
        g['times'].append(float(timestamp))
        if re.search(r'perf: (?:build-config|deep-config|deep-gpu-config|deep-limits) ',text):g['configs'].append(text)
        if 'perf: deep-scope ' in text:g['scopes'].append(dict(re.findall(r'(\w+)=([^\s]+)',text)))
        if 'perf: deep-guest ' in text:g['guest_targets'].append(dict(re.findall(r'(\w+)=([^\s]+)',text)))
        if re.search(r'perf: (?:title thread per frame|per frame |framebuffer-cache)',text):
            g['renderer_details'].append(dict(t=float(timestamp),text=text))
        if 'vulkan: present ' in text:g['resolution_records'].append(dict(t=float(timestamp),text=text))
        if text.startswith('perf: pipeline-cache '):
            g['pipeline_cache_records'].append(dict(t=float(timestamp),**dict(re.findall(r'(\w+)=([^\s]+)',text))))
        policy=re.search(r'd3d: GPU callback policy=(synchronous|ordered-write-completion)\b',text)
        if policy and policy[1] not in g['callback_policies']:g['callback_policies'].append(policy[1])
        if re.search(r'handle-cache lifetime|dispatcher-wakes lifetime|deep-frames |perf: [\d.]+ fps|title thread per frame|per frame [\d.]+ draws|deep-gpu window',text):g['records'].append(text)
        rate=re.search(r'perf: ([\d.]+) fps ([\d.]+) ms/frame \| host ([\d.]+) \(draw ([\d.]+).*?other ([\d.]+)',text)
        if rate:g['rates'].append(dict(zip(('fps','frame_ms','host_ms','draw_ms','other_ms'),map(float,rate.groups())),t=float(timestamp)))
        if 'perf: deep-frames ' in text:
            values=dict(re.findall(r'(\w+)=([\d.]+)',text));g['frames'].append({k:float(v) for k,v in values.items()})
        if 'perf: deep-gpu window=' in text:g['gpu'].append(dict(re.findall(r'(\w+)=([\d.]+)',text)))
        if 'perf: deep-thread ' in text:
            values=dict(re.findall(r'(\w+)=([^\s]+)',text));g['threads'].append(values)
        if 'perf: guest-profile-entry ' in text:
            values=dict(re.findall(r'(\w+)=([^\s]+)',text));g['guest_samples'].append(values)
        if 'handle-cache lifetime' in text or 'dispatcher-wakes lifetime' in text:g['flags'].append(text)
    output={}
    for (build,run),g in groups.items():
        key=run if sum(1 for b,r in groups if r==run)==1 else run+'@'+build
        frames=g['frames'];count=sum(w.get('count',0) for w in frames)
        mean=sum(w.get('mean_ms',0)*w.get('count',0) for w in frames)/count if count else None
        rates=g['rates']
        # Correlate only identical five-second window IDs. Never combine GPU
        # segment count with an unrelated frame window or hide missing queries.
        windows=[]
        for frame in frames:
            window=int(frame['window']);entry={'window':window,'frames':int(frame.get('count',0)),
                'frame_mean_ms':frame.get('mean_ms'),'fps':1000/frame['mean_ms'] if frame.get('mean_ms') else None}
            gpu=next((v for v in g['gpu'] if int(v['window'])==window),None)
            if gpu:
                entry['gpu_segments']=int(gpu['segments']);entry['gpu_errors']=int(gpu['errors'])
                entry['gpu_ms_per_frame_approx']=float(gpu['sum_ms'])/frame['count'] if frame.get('count') and entry['gpu_segments']>0 and not entry['gpu_errors'] else None
            else:entry['gpu_ms_per_frame_approx']=None
            entry['threads']=[v for v in g['threads'] if int(v['window'])==window]
            entry['scopes']=[v for v in g['scopes'] if int(v['window'])==window]
            entry['guest_targets']=[v for v in g['guest_targets'] if int(v['window'])==window]
            windows.append(entry)
        warnings=[]
        if not frames:warnings.append('No detailed frame intervals; averages cannot prove stable 60 FPS.')
        if not any(v['gpu_ms_per_frame_approx'] is not None for v in windows):
            warnings.append('No usable GPU timestamps correlated with frames; GPU cost is unknown, not zero.')
        if any(v.get('cpu_supported')!='1' for v in g['threads']):
            warnings.append('Some thread CPU readings are unsupported; their wall time is not CPU utilization.')
        if any(float(v.get('age_ms',0))>1000 for v in g['threads']):
            warnings.append('Some thread snapshots are over one second old; inspect age_ms before attribution.')
        output[key]={'build':g['build'],'run_id':run,'gpu_callback_policies':g['callback_policies'],
            'gpu_callback_policy_verified':len(g['callback_policies'])==1,
            'observed_seconds':max(g['times'])-min(g['times']),
            'rate_window_count':len(rates),'rate_fps_range':[min(v['fps'] for v in rates),max(v['fps'] for v in rates)] if rates else None,
            'rate_fps_median':statistics.median(v['fps'] for v in rates) if rates else None,
            'frame_window_count':len(frames),'observed_frame_intervals':int(count),'weighted_frame_mean_ms':mean,
            'weighted_frame_rate':1000/mean if mean else None,
            'window_p99_ms_range':[min(v['p99_ms'] for v in frames),max(v['p99_ms'] for v in frames)] if frames else None,
            'window_low1_fps_range':[min(v['low1_fps'] for v in frames),max(v['low1_fps'] for v in frames)] if frames else None,
            'rate_windows':rates,'frame_windows':frames,'gpu_windows':g['gpu'],'optimization_records':g['flags'],
            'correlated_windows':windows,'sampled_guest_calls':g['guest_samples'],
            'profiling_config_records':g['configs'],'scope_windows':g['scopes'],'guest_target_windows':g['guest_targets'],
            'renderer_details':g['renderer_details'],'resolution_records':g['resolution_records'],'warnings':warnings,
            'pipeline_cache_records':g['pipeline_cache_records'],
            'supporting_records':g['records'],
            'limitations':'Window summaries, not a complete raw frame trace; do not merge different runs or call the minimum a global 1% low. GPU/frame ratios are approximate window correlations; thread snapshots can be stale. Guest call wall samples include waits and are not CPU attribution.'}
    return output

def main():
    p=argparse.ArgumentParser();p.add_argument('log',type=Path)
    p.add_argument('--output',type=Path,help='JSON summary destination (default: beside the input log)');a=p.parse_args()
    output=analyze(a.log.read_text(errors='replace').splitlines())
    for record in output.values():
        print(json.dumps({k:v for k,v in record.items() if k not in ('rate_windows','frame_windows','gpu_windows','supporting_records','optimization_records','correlated_windows','sampled_guest_calls','scope_windows','guest_target_windows','renderer_details','resolution_records','pipeline_cache_records')},indent=2))
        print('Last optimization records:',record['optimization_records'][-2:])
        print('Last timing records:',record['supporting_records'][-8:])
    (a.output or a.log.parent/'perf-summary.json').write_text(json.dumps(output,indent=2)+'\n')
if __name__=='__main__':main()
