# iOS app (Phase 0 scaffold)

The SwiftUI shell over the portable engine. It imports footage from the photo
library, builds a timeline through the engine's C ABI, previews it, trims and
splits clips, and exports an MP4.

## Status: written, not yet built

This scaffold was authored on Linux, where Xcode cannot run. Nothing here has
been compiled or run yet. Treat the first build on a Mac as a debugging session,
not a formality. The C++ engine underneath it is tested (see `../core`), so
mistakes are most likely to be in the Swift and AVFoundation glue.

## Build

Requires macOS with Xcode 26 or later, plus [XcodeGen](https://github.com/yonaskolb/XcodeGen).

```sh
brew install xcodegen
cd ios
xcodegen generate
open VideoEditor.xcodeproj
```

Pick an iPhone or iPad simulator and run. On a device, set your development team
in the target's Signing and Capabilities tab first.

The `.xcodeproj` is generated and never committed. `project.yml` is the source of
truth, so merge conflicts in project settings stay readable.

## What is wired up

- `Engine.swift` is the only place that touches the C ABI. The rest of the app
  works with `TimelineClip` values.
- `EditorModel.swift` sends every edit to the engine first, then refreshes its
  published state from the engine. Swift never reorders clips itself.
- `CompositionBuilder.swift` turns the clip list into an `AVMutableComposition`
  for preview and export.
- `EditorView.swift` has import, preview, scrub, undo and redo, trim buttons for
  the selected clip, split at the playhead, delete, and export.

## Known gaps, on purpose

- The timeline strip is read-only. Dragging handles need the gesture layer, which
  should be built against the engine's ripple semantics rather than beside them.
- Trims run in one second steps from buttons, not frame steps from a drag.
- Speed is carried through the model but playback and export assume 1x.
- Export uses `AVAssetExportPresetHighestQuality`, so the codec is whatever
  AVFoundation picks, which may be HEVC. Pinning H.264 needs the custom exporter.
- No persistence wired up yet. The engine can save and load a project through
  `vxe_save` and `vxe_load`, but the app does not call them, so a relaunch still
  starts an empty project.
- `CompositionBuilder` is temporary. The Metal render pipeline replaces it rather
  than extending it, because effects, masks, and speed ramps cannot be expressed
  as AVFoundation layer instructions.
