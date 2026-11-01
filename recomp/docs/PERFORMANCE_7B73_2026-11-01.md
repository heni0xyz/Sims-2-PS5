# Owner 7b73 performance capture — 1 November 2026

Raw evidence stays private under `out/console/fps-7b73-20261101T124324Z/`.
SHA-256: 74e9b131e8da035b533cb2350653284ed4c1f2ea66c56fef140ac2bd4213e107.
The owner explicitly confirms this is FPS testing, not a reproduction of thin
Continue. Do not use its default-height read as evidence of that defect.

The file contains two run IDs of build 7b73fdc02feb0835d8a55a7d. The first
is only a 3.80-second intro fragment. The main run manual-501620565649 spans
92.77 observed seconds, 91 one-second rates, 18 five-second frame summaries
and 4645 frame intervals. Its overall weighted mean is 19.60205 ms / 51.015 FPS,
which mixes startup, menus, transitions and heavier rendering.

| Explicit five-second windows | Frames | Mean frame ms | Mean FPS | Approx GPU ms/frame |
| --- | ---: | ---: | ---: | ---: |
| 1..6 early | 1792 | 16.8493 | 59.35 | 1.145 |
| 8..14 heavier | 1513 | 23.4257 | 42.69 | 6.361 |
| 16..18 late lighter | 856 | 17.6995 | 56.50 | 4.816 |

Heavy individual windows range 38.92..45.16 FPS. Windows7 and15 are excluded
as transitions for this explicit comparison. Camera distance, paused state and
exact interaction are not independently labeled by this log. These groups are
not an automatic gameplay classifier. Window-summary lows are not a global
1% low; no complete raw frame trace or clean exit is captured.

GPU times correlate completed segments with identical frame-window IDs; they
are approximate and overlap CPU work. They cannot be subtracted from total
frame time to claim exact CPU cost. Heavy title reports show roughly 4..5 ms
recording, 6..8 ms flush waiting and substantial XDK draw/sendstate work.
The CPU/wait path deserves investigation before reducing native4K detail.

7b73 has guest-call sampling and deep timing of kernel/lock scopes enabled.
Heavy windows record hundreds of thousands of scoped kernel/wait operations.
The next native candidate keeps O2, frame cap60, 4K, handle caching, selective
object wakes and per-second renderer reports, while disabling guest/deep
instrumentation. This is a controlled probe of profiler overhead, not a
measured optimization or a promise of stable60. Ordered asynchronous WRITE
callbacks remain disabled pending stronger ordering/hardware evidence.

The owner reports thin Continue persists on7b73. Its earlier ready-geometry
a0 refresh is therefore not accepted as a fix. Expanded bounded diagnostics
also observe the controller's state-2 path (which does not call140D00) and
record item count, both dimensions and current/previous state at variable
reads. Guest controller/provider bodies and results are unchanged. A new thin
Continue capture is still needed to distinguish state and refresh failures.

For the next comparison use the same lot/save at normal game speed: one minute
far camera and one minute close camera, logging from launch. Then separately
enter/back/reenter Story and Freeplay Continue several times, retaining a
capture when the thin panel appears. No console was contacted by this work.
