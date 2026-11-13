# iOS app (Phase 0 scaffold)

The SwiftUI shell over the portable engine. It imports footage from the photo
library, builds a timeline through the engine's C ABI, previews it, trims and
splits clips, and exports an MP4.

## Status: compiles, not yet run

CI builds this app for the iOS simulator on every push, so the Swift and
AVFoundation code is known to compile with no warnings against the engine it
embeds. Nobody has run it on a device or simulator yet, so runtime behaviour
(gestures, playback, export) is unverified.

`Engine.swift`, `EditorModel.swift`, and the AVFoundation glue were written
without a Mac available, which is why the build step matters more than usual
here.

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
- `ProjectStore.swift` and the save and load calls in `EditorModel.swift` keep
  one working project: edits autosave through `vxe_save` about half a second
  after the last change, the app saves again when it leaves the foreground,
  and a relaunch reopens the timeline through `vxe_load`.
- `Assets.xcassets` carries the app icon, drawn by `tools/make_app_icon.py` so it
  can be regenerated instead of edited as a binary. `PrivacyInfo.xcprivacy`
  declares no tracking, no collected data, and no required-reason APIs.

## Distribution

TestFlight uploads run in GitHub Actions, not from a Mac. `.github/workflows/testflight.yml`
archives, signs with an App Store Connect API key, and uploads to App Store
Connect. The account setup it needs is in `../docs/testflight.md`. Until that
setup exists, the workflow fails at its first step with a message naming the
missing secret.

## Known gaps, on purpose

- The timeline strip is read-only. Dragging handles need the gesture layer, which
  should be built against the engine's ripple semantics rather than beside them.
- Trims run in one second steps from buttons, not frame steps from a drag.
- Speed is carried through the model but playback and export assume 1x.
- Export uses `AVAssetExportPresetHighestQuality`, so the codec is whatever
  AVFoundation picks, which may be HEVC. Pinning H.264 needs the custom exporter.
- One project only. There is no project browser, no duplicate, and no way to
  start a second project without clearing the first one's timeline.
- Footage is stored under a file named after its media id. If the user deletes
  stored footage, the clip stays on the timeline with no video behind it; there
  is no relink or replace yet.
- `CompositionBuilder` is temporary. The Metal render pipeline replaces it rather
  than extending it, because effects, masks, and speed ramps cannot be expressed
  as AVFoundation layer instructions.
