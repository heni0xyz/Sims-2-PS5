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

if __name__=='__main__':unittest.main()
