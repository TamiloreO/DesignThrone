# Architecture

The product ships on iPhone and iPad first and Windows later. One engine, two shells. This document records the boundaries that make that possible and the decisions behind them.

## The shape

```
+-------------------+        +--------------------+
|  iOS shell        |        |  Windows shell     |
|  Swift + SwiftUI  |        |  native UI         |
+---------+---------+        +---------+----------+
          |                            |
          |  C ABI / Swift package     |
          v                            v
+--------------------------------------------------+
|  core (C++20, no platform dependencies)          |
|  document model, timeline edits, undo, render    |
|  graph, frame scheduling, project serialization  |
+--------------------------------------------------+
          |                            |
          v                            v
+-------------------+        +--------------------+
| iOS backends      |        | Windows backends   |
| Metal, AVF        |        | D3D12, MediaFndn   |
+-------------------+        +--------------------+
```

The core knows nothing about Metal, AVFoundation, UIKit, or Windows. Platform code implements a small set of interfaces: decoder, encoder, texture, and clock. That is the entire contract.

## Why C++ rather than Swift

Swift would be faster to start and would make the iOS half simpler. It cannot be reused on Windows, and a second engine written in C# or C++ later would mean two products with two sets of bugs.

C++20 with no exceptions across the ABI and no RTTI in the interface is boring, portable, and links cleanly from both Swift and C#. The cost is discipline: no Apple frameworks in the core, ever, and the core must build and test on Linux in CI.

Rust was considered. It is a better language for memory safety and its C ABI story is fine, but the video ecosystem (FFmpeg bindings, GPU interop, encoder SDKs) is more mature in C++, and every hour spent fighting bindings is an hour not spent on the editor. Revisit only if memory-safety bugs become a pattern.

## Module boundaries

`core/time` - rational time, frame rates, frame-accurate arithmetic. Nothing else may do time math.

`core/model` - project, sequence, track, clip, effects, and their invariants. Pure data plus validation.

`core/edit` - every mutation. Each operation is a command with apply and revert, which is what makes undo reliable and testable.

`core/render` - the render graph: a sequence of nodes from source to output. Deterministic given a time and a project.

`core/serialize` - the project format. Versioned, forward-compatible, documented.

Platform backends are deliberately dumb: give me a frame at this time, give me a texture, encode this frame. All decisions live in the core.

## The render graph

Preview and export must produce identical pixels. The only way to guarantee that is to drive both from the same graph.

For a given output time, the graph resolves to: for each visible track, find the clip at that time, map timeline time to source time (accounting for speed and reverse), decode, apply the clip's effect stack in order, composite with the track below using blend mode and opacity, then apply sequence-level adjustment layers, then the output transform for the display or encoder.

Preview uses a pull model driven by a display link: the core asks for the next frame, the renderer produces it. Export uses a push model: the encoder asks for frame N, the graph produces it. Same graph, two schedulers.

Cache policy: decoded frames are cached around the playhead, rendered frames are cached by graph hash and time, and export bypasses the interactive cache entirely to keep memory flat on long timelines.

## Decode and encode strategy

On iOS, use Apple's frameworks first. AVFoundation decodes HEVC, H.264, and ProRes with hardware acceleration and handles HDR and ProRes RAW. VideoToolbox encodes. This avoids codec licensing exposure and gives the best battery behavior.

FFmpeg is a Windows-side and fallback concern. If it ships on iOS at all, it must be an LGPL build, dynamically linked, with no GPL-only components (no x264, no x265), and it needs legal review before a commercial release. Record the final decision in a decision record before Phase 4.

## Project format

A project is a folder package containing a manifest (JSON, versioned), a media reference table with bookmarks and content hashes, and optional cached proxies and thumbnails. Media is never copied in by default.

The format is documented and stable, because the promise is that the project outlives the app. A documented format also makes FCPXML export tractable and gives power users a way out, which paradoxically makes them more willing to stay.

## Threading and memory

One thread owns the document. Edits are queued onto it and applied in order, which removes an entire class of race conditions. Rendering runs on its own queue and reads an immutable snapshot of the document, so a render never blocks an edit and an edit never tears a frame.

Memory is the constraint that decides device support. A 4K frame is roughly 25 MB uncompressed, so a three-layer composite plus caches can pass 200 MB before any decode buffers. The policy: bound every cache by bytes, not by count, and drop decoded frames before dropping rendered ones.

## Testing

The core is tested on Linux in CI, which is why it has no Apple dependencies. Golden-frame tests compare rendered output against checked-in reference frames with a perceptual tolerance. Edit operations are tested as property tests: apply, revert, compare to the original document, byte for byte.

## Open decisions

These are recorded now and must be settled before the phase that needs them.

1. **iOS 18 versus iOS 26 minimum.** Current plan is iOS 18 with iOS 26 features gated at runtime, because SpeechAnalyzer and Foundation Models are the on-device AI story and they require iOS 26. Revisit after measuring adoption at launch.
2. **WinUI 3 versus Qt for the Windows shell.** Decide at the start of Phase 4 with the iPad UI in hand as the reference.
3. **Whether FFmpeg ships on iOS at all.** Prefer Apple frameworks only. Decide when a format gap actually appears.
4. **Project format schema.** JSON manifest is the working assumption. Revisit if load time on large projects becomes a problem.
5. **Product name and bundle identifier.** Needed before the first TestFlight build, not before the core is written.
