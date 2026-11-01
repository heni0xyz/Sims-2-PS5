# Ordered GPU completion comparison — 1 November 2026

Owner f48a3b24684ecd4f2479a7c0 log retained privately under
out/console/fps-latest-20261101T161725Z/klog.raw.log, SHA256
81ed97d670c2584ae17a5f728d67d566bea4427e2e458799b531de9c6939ead1.
One run, manual-514201391389,79.27 observed seconds/78 rate reports. Native
3840x2880 backing,3840x2688 picture and3840x2160 output are logged. The
fragment starts minutes after boot, so its callback-policy startup is absent.
Shipped f48 settings were synchronous. Expensive guest/deep profiling is OFF.

The last15 rate reports range40.4..49.1FPS, arithmetic mean44.173FPS
(approximate one-second reports, not a complete frame trace). Last report:
43FPS/23.27ms, host draw10.33ms, title recording5.09ms, flush wait7.58ms,
XDK draws9.45ms (sendstate2.98ms),1782 draws/110k vertices/12.2MB upload.
Scopes overlap: these cannot be added or used to promise a fixed speedup.
A59FPS median across the fragment does not certify sustained60 gameplay.

## Concrete experiment

The existing opt-in SDK WRITE completion path records work without blocking
the producer. The actual render worker waits for a successful native GPU
completion, then queues the guest callback on its initialized host context.
No guest completion is fabricated and no fence/error is disabled. Both direct
USA InsertCallback call sites,0x1FC53F and0x1FCB0D, pass type1/WRITE and callback
0x1FA8C0; that function dispatches the original object's virtual method. These
are independently decoded facts, not generated-source edits. This does not
establish all indirect calls or generic mixed READ/WRITE ordering.

This path already exists in the delivered f48 binary but is disabled by default.
The isolated experiment changes only settings.toml:gpu_callbacks_async=true.
Binary build ID/eboot/debug ELF remain identical; configuration hashes identify
the variant. Native4K,16:9,60cap,fonts/prompts and owner data are unchanged.
Synchronous original settings remain preserved as rollback-settings.toml.
The default/previous deliveries are never overwritten. This is not an accepted
production default and not a newly linked executable or a measured speedup.

## Executed checks

The actual recording backend contract test rebuilt with Clang23.1.4,
-std=c++20 -O2 -march=x86-64-v2 PASS: producer nonblocking behind a real mock
GPU gate, FIFO, exactly-once across34 callbacks/batch reuse, and fail-closed
default on unsuccessful completion.20 Python tool regressions PASS, including
callback-policy attribution (missing/mixed startup cannot be treated as verified).

A new480-second Windows offscreen Vulkan test completed under the local
SwiftShader ICD:run20261101T162611Z-5ec4ed, actual recording backend and
ordered-write mode, normal controller inputs and a cloned synthetic HDD.
Old personal data and console are untouched. Earlier480-second run0a2882
reached the lot fly-through with this mode, not interactive/stable60 proof.
The new run completed480.203s/3000 reported frames with no fatal record.
Reviewed final1280x720 frame2981 shows the Sim and lot/HUD after the tutorial
dialog; frame2418 shows the control tutorial and DualSense prompts. All13
normal timed input requests were consumed. The watchdog ended the process;
this is not clean shutdown, native4K/PS5 performance or full interaction proof.
The original raw run, paired captures and summary remain private.
PC-integration-result.json in the variant folder records image hashes.

Prepared artifact: outputs/PS5-Independent/OrderedGPU-Experiment-f48a3b24.
The verified50-member package changes only settings.toml; eboot SHA256 stays
92a47c8c21296fd153dcaebde5834b5366c4e6930348b232f0767276a24525bd.
Experimental settings SHA256c6421167ab63ad75f023a00c434f38d80b3c91078eeb0f104d48f49f9e543395;
baseline17c04ed91e80a6e73fe9a9c7aa682494d0cfa87cf9d9bf847c5fef47c9f3cc64.

## Reproduce

```text
python tools/ps5/prepare_gpu_experiment.py --base <validated-f48-directory> --output <fresh-experiment-directory>
python tools/ps5/validate_delivery.py <fresh-experiment-directory>
```

If the installed title is exactly f48, copy only the experimental settings.toml
into PPSA99145. For rollback copy rollback-settings.toml as settings.toml.
Do not replace/delete assets,ISO or hdd. Native experiment performance and
correctness remain NOT TESTED. Owner test: same lot/save/normal speed, warm
scene, one minute far camera plus one minute close; capture klog from launch
so the ordered-write-completion policy is observed. Compare the same scene
with original synchronous settings. No stable60 claim before that evidence.
