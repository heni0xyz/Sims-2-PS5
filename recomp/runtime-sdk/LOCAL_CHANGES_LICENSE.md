# Project extensions, 31 October 2026

The original XboxXBERecomp sources and earlier MIT integration retain their
MIT license and notices in LICENSE. This does not change their authorship.

The new optional host texture replacement interface/implementation in
runtime/gpu/texture_replacement.h, runtime/gpu/vulkan.h and vulkan.cpp,
and the project-gated ISO bootstrap call in harness/main_test.cpp were
authored by OpenAI Codex for szampan. These new contributions are licensed
GPL-3.0-only, as selected by szampan. The full GPL text is in the parent
project LICENSE, and travels with packaged titles/source snapshots.
The combined project therefore includes GPL-covered code while the
underlying MIT grants remain available for the original components.

No original game source, translated guest output, executable or data is
covered by this license declaration. The callback defaults to null; the
project-gated harness extension leaves other SDK users unchanged.

The subsequent project callback-policy guard/startup reports in xdk/d3d8.cpp
and gpu/render_thread.cpp are also authored GPL-3.0-only extensions. The
experimental ordered-write path remains disabled by title defaults.
The optional replacement callback's palette parameter is a GPL-3.0-only
extension for exact owner-font identification/reconstruction in the title.
The first-present CPU-front upload correction is a GPL-3.0-only contribution
by OpenAI Codex for szampan, verified with an asset-free Vulkan installer probe.

Native startup presentation bypasses guest crop/render scaling only while a host
startup display owns the backend; default game behavior is unchanged. GPL-3.0-only.
