"""Window aggregates must not turn mixed klog sessions into a false FPS claim."""
from pathlib import Path
import sys,unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
from analyze_console_perf import analyze

def frame(build,run,time,count,mean):
    return (f'[Sims2Independent] [SIMS2][PERF][INFO] t={time} build={build} run={run} '
        f'tid=1 milestone= guest=00000000 native=0 perf: deep-frames window=1 '
        f'count={count} mean_ms={mean} p99_ms=30 low1_fps=20')

class ConsolePerf(unittest.TestCase):
    def test_callback_mode_requires_observed_unambiguous_policy(self):
        prefix='[Sims2Independent] [SIMS2][XDK][INFO] t=1 build=aaaa run=run tid=1 milestone= guest=00000000 native=0 '
        records=[frame('aaaa','run',1,100,20)]
        self.assertFalse(analyze(records)['run']['gpu_callback_policy_verified'])
        records.append(prefix+'d3d: GPU callback policy=ordered-write-completion (READ/unknown callbacks use synchronous flush)')
        self.assertEqual(analyze(records)['run']['gpu_callback_policies'],['ordered-write-completion'])
        self.assertTrue(analyze(records)['run']['gpu_callback_policy_verified'])
        records.append(prefix+'d3d: GPU callback policy=synchronous')
        self.assertFalse(analyze(records)['run']['gpu_callback_policy_verified'])
    def test_keeps_builds_and_sessions_separate(self):
        result=analyze([frame('aaaa','same',1,10,10),frame('bbbb','same',2,10,40),
            frame('aaaa','other',3,10,20),'unrelated private kernel service message'])
        self.assertEqual(set(result),{'same@aaaa','same@bbbb','other'})
        self.assertEqual(result['same@aaaa']['weighted_frame_rate'],100)
        self.assertEqual(result['same@bbbb']['weighted_frame_rate'],25)
        self.assertEqual(result['other']['weighted_frame_rate'],50)
    def test_weights_time_by_observed_frame_count(self):
        result=analyze([frame('aaaa','run',1,100,10),frame('aaaa','run',6,300,30)])['run']
        self.assertEqual(result['observed_frame_intervals'],400)
        self.assertEqual(result['weighted_frame_mean_ms'],25)
        self.assertEqual(result['weighted_frame_rate'],40)
        self.assertEqual(result['window_low1_fps_range'],[20,20])
        self.assertNotIn('global_low1_fps',result)
    def test_gpu_only_matches_same_window_and_errors_cannot_pass(self):
        prefix='[Sims2Independent] [SIMS2][PERF][INFO] t=2 build=aaaa run=run tid=1 milestone= guest=00000000 native=0 perf: '
        records=[frame('aaaa','run',1,100,20),prefix+'deep-gpu window=2 segments=200 sum_ms=900 max_ms=8 errors=0',
                 prefix+'deep-gpu window=1 segments=200 sum_ms=600 max_ms=7 errors=0']
        window=analyze(records)['run']['correlated_windows'][0]
        self.assertEqual(window['fps'],50);self.assertEqual(window['gpu_ms_per_frame_approx'],6)
        records[-1]=records[-1].replace('errors=0','errors=1')
        self.assertIsNone(analyze(records)['run']['correlated_windows'][0]['gpu_ms_per_frame_approx'])

    def test_no_gpu_queries_is_unknown_instead_of_zero_cost(self):
        prefix='[Sims2Independent] [SIMS2][PERF][INFO] t=2 build=aaaa run=run tid=1 milestone= guest=00000000 native=0 perf: '
        result=analyze([frame('aaaa','run',1,100,20),prefix+'deep-gpu window=1 segments=0 sum_ms=0 max_ms=0 errors=0'])['run']
        self.assertIsNone(result['correlated_windows'][0]['gpu_ms_per_frame_approx'])
        self.assertTrue(any('GPU cost is unknown' in warning for warning in result['warnings']))

    def test_pipeline_cache_timing_is_retained_for_stall_analysis(self):
        prefix='[Sims2Independent] [SIMS2][PERF][INFO] t=2 build=aaaa run=run tid=1 milestone= guest=00000000 native=0 '
        result=analyze([prefix+'perf: pipeline-cache bytes=397656 unchanged=1 written=0 read_ms=0.2 write_ms=0.0'])['run']
        record=result['pipeline_cache_records'][0]
        self.assertEqual(record['unchanged'],'1')
        self.assertEqual(record['written'],'0')
        self.assertEqual(record['t'],2)

    def test_retains_attribution_and_flags_unusable_cpu_readings(self):
        prefix='[Sims2Independent] [SIMS2][PERF][INFO] t=2 build=aaaa run=run tid=1 milestone= guest=00000000 native=0 '
        result=analyze([frame('aaaa','run',1,100,20),
            prefix+'perf: build-config schema=1 deep=1 guest_samples=1 fps_cap=60 vsync=1',
            prefix+'perf: deep-thread window=1 slot=0 cpu_supported=0 age_ms=1500',
            prefix+'perf: deep-scope window=1 slot=0 metric=fence calls=4 wall_ms=20 cpu_ms=0',
            prefix+'perf: deep-guest window=1 slot=0 target=00123456 calls=2 wall_ms=5',
            prefix+'perf: title thread per frame: recording 2.0 ms, waiting 3.0 ms',
            prefix+'vulkan: present 600 target 3840x2880 window 3840x2160'])['run']
        self.assertEqual(result['correlated_windows'][0]['scopes'][0]['metric'],'fence')
        self.assertEqual(result['correlated_windows'][0]['guest_targets'][0]['target'],'00123456')
        self.assertEqual(len(result['profiling_config_records']),1)
        self.assertEqual(len(result['renderer_details']),1)
        self.assertEqual(len(result['resolution_records']),1)
        self.assertTrue(any('unsupported' in warning for warning in result['warnings']))
        self.assertTrue(any('one second old' in warning for warning in result['warnings']))

if __name__=='__main__':unittest.main()
