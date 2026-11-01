# Earlier-port performance audit — 1 November 2026

Read-only comparison requested by szampan. No previous project, native binary,
settings, owner files or console was modified. No game hook/config/generated
code was copied. Exact local evidence:out/audits/old-port-performance-20261101/
comparison.json. This audit does not change the current GPU comparison variant.

Old game project HEAD3ceb21967889ad83781c3595c0c8139444df817f; actual clean
SDK8f5b2796ce7577d5264ed60505af3bac8f506551. Independent project before this
audit2b85c90f74eca96efe59ea8abc36c38c66487efa; runtime163c80f2a9a2ac5b35b71eeebacae5526988b3de.
Git merge-base confirms that exact old SDK revision is an ancestor of the
current runtime. objects.cpp,files.cpp,guest_address.h,x86_ops.h remain byte
identical after line-ending normalization. Inheritance is checked against
actual code/current pins, not inferred from feature names.

## Optimization inventory

| Earlier change | Current independent implementation | Decision |
| --- | --- | --- |
| O0 to O2/g1/NDEBUG, IEEE arithmetic, no strict-aliasing assumptions | Delivered f48 profile audits121 compile commands with O2; no fast-math | Already applied |
| Physical-mirror dereference canonicalization and sequential REP overlap behavior (8b28550) | Same guest_address/x86_ops code; kernel helpers unchanged | Already applied; correctness prerequisite for optimization |
| Vulkan framebuffer reuse within a fenced segment (42088fd) | BeginTargetPass checks compatible attachments/pass/extent and retains fence-owned lifetime; current klog reports reuse | Already applied |
| Sparse release performance logging (b64b6e7) | Per-second renderer reports ON; noisy diagnostic, guest and deep scope profiling OFF in f48 | Already applied; no measured standalone FPS gain established |
| Single-object waits without vector allocation; polling without clock queries; no broadcast when no sleepers (3b5901f) | Same objects.cpp,including span-based waits/poll deadline handling | Already applied |
| Bounded weak handle cache/invalidation (92f7059) | Same objects.cpp; FAST_HANDLES ON in f48; current klog has live hits/misses | Already applied |
| Object-specific wake channels (757133f) | Same objects.cpp; OBJECT_WAIT_CHANNELS ON; current klog selective=1 and avoided-target counters | Already applied; avoided targets are not measured OS wakeups |
| Case-alias path lookup/cache (8f5b279) | Same files.cpp and inherited platform lookup helpers | Already applied; mostly I/O/path work, not a fix for every frame |
| Shader-cache/binding fixes (26dc3a1), optimized glslang and release RADV | Same dependency ancestry and established cached toolchain/driver; current pins are recorded in repositories.lock.json | Retained; rebuilding RADV is not a newly identified optimization |

This comparison found no missing proven generic optimization to reapply.
Duplicating these patches would not add another speed benefit. Current f48
logs show cache hits,selective wakes and framebuffer reuse actually active.

## Material differences

### GPU completion

The old title's src/game/d3d_callback.cpp calls the guest callback immediately,
without waiting for a GPU completion. The current SDK's synchronous default
waits for native work before CallGuest. In the owner's latest f48 heavy report,
render-queue flush wait is7.58ms/frame, total23.27ms/43FPS. That wait includes
queue/worker/GPU completion; it is not a pure measured GPU execution time.
The isolated ordered-WRITE configuration overlaps producer work while keeping
a real successful GPU completion before the guest signal. It is the current
controlled experiment,not accepted sustained60. See
PERFORMANCE_ORDERED_GPU_2026-11-01.md for contract/PC evidence and rollback.
The immediate callback shortcut is not transplanted.

### Render load

The old owner-accepted SelectiveWake e456 test in docs/ps5/SELECTIVE_WAKE.md
has29 late reports with median59.9FPS; individual34..65ms frame outliers still
occurred. Its output was1920x1080,visible source1440x1008. It was not native4K.
Current f48 reports3840x2880 backing /3840x2688 picture /3840x2160 output.
The source picture has64/9 =7.111 times as many pixels. This is not a claim
that runtime cost scales7.111x,or that GPU resolution is the sole bottleneck.
The old later4K candidates require separate evidence; an earlier1080p result
cannot be used as a same-resolution/same-scene reference for them.
No resolution reduction is introduced by this audit.

### Game timing and input repeat

The earlier inherited game hooks wrap the engine time-step update and held-pad
repeat. The independent project has a host60FPS cap/pacer but no matching
strong game-specific wrappers at0x1C2400/0x1BCA50. These address elapsed game
seconds/input repeat at varying frame rates,not rendering throughput. Their
absence is an item for independent reanalysis and real-time simulation/input
rate tests at multiple caps. This audit does not assert that current simulation
speed is broken or copy/relabel inherited game hooks. Implement only after
measuring/rederiving the required behavior from the supported owner executable.

### Shader-constant fast-entry binding

One analysis binding also differs: old config binds SetVertexShaderConstant1Fast
to0x272FD0; the independent reviewed config leaves it unbound. As recorded in
OPEN_UI_ISSUES.md,the independently generated wrapper preserves its conditional
guest scratch-mirror copy and reaches the already bound0x272EF0 implementation.
Sixteen earlier ABI cases pass. A direct replacement at0x272FD0 would skip that
copy; it is not an established safe speedup. Do not substitute an old signature
just because its name contains Fast. A native equivalent would need independent
semantic/copy/ABI validation and evidence that this entry is a material hotspot.

## Next action

Keep the settings-only ordered GPU comparison isolated. Measure the same
lot/save/camera/game speed with both policies at the same native4K settings.
If remaining time is CPU recording/draw work,profile that path before selecting
a new cache/batching change. Add low-overhead frame-interval measurements for
sustained60 acceptance; one-second averages alone cannot exclude stutter.
Keep timing/input rate validation separate from a throughput speedup claim.
No extra native build or console run was needed for this read-only audit.
