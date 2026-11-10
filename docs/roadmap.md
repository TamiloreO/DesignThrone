# Roadmap

Seven phases from empty repository to a shipped product on two platforms. Each phase has a single gate: the thing that must be true before the next phase starts. Phases are ordered by dependency, not by excitement.

Durations are deliberately not fixed. The gate matters more than the calendar, and the honest range for a solo developer using AI assistance on a project of this size is measured in months, not weeks.

## Phase 0: Foundation

**Goal.** Prove the engine and the toolchain, and get a TestFlight build into your hands that can cut a clip and export it.

**Scope**

- C++ core: time model with rational frame rates, project document model, timeline model, edit operations (insert, overwrite, append, lift, ripple delete, split, trim, move), undo and redo, validation.
- Render graph design: source decode, effect stack, compositor, output. Defined on paper before code, in `docs/architecture.md`.
- Metal preview renderer: decode a single source with AVFoundation, composite two layers, display at 60 fps.
- Export path: AVAssetWriter with hardware H.264 and HEVC, audio mixed, A/V sync verified.
- SwiftUI shell: media import, a scrollable timeline, playhead, basic trim handles, export button.
- Toolchain: Xcode project, GitHub Actions macOS runner that builds and runs core tests, TestFlight pipeline.

**Gate.** A 60-second project with three clips, a cross dissolve, and a title exports to a playable 1080p file, and the core test suite passes in CI on every push.

**Risks.** The preview renderer is the hardest single piece and the most likely to consume time. Build it against real footage early, on the oldest supported device, not a simulator.

## Phase 1: Shippable v1

**Goal.** Everything in the P0 list from the feature spec. This is the App Store submission.

**Scope**

- Full timeline: eight video and eight audio tracks, all trim operations, snapping, groups, markers, zoom to frame.
- Transform, opacity, blend modes, keyframes on every animatable property, chroma key, speed controls, freeze frame, reverse.
- Audio: mixer, fades and crossfades, volume envelopes, voiceover recording, metering.
- Titles with styles and animation presets, on-device auto captions, the caption correction editor, SRT and VTT.
- Primary color correction, core effects, transitions.
- Export: H.264 and HEVC up to 4K60, social presets, background export, share sheet.
- Project autosave, crash recovery, version history, relinking.
- Keyboard shortcuts and command palette, accessibility baseline, VoiceOver on the timeline.
- App Store assets: icon, screenshots for both device classes, preview video, privacy nutrition label, review notes.

**Gate.** The app is submitted to App Store Review, and a stranger can complete a real edit without instructions.

**Risks.** App Review rejections are the main unknown. Guideline 4.3 (spam, indistinguishable from existing apps) and 1.2 (user-generated content, if any sharing ships) are the two to design against from the start. Privacy disclosures must match actual behavior, which is easy here because nothing leaves the device.

## Phase 2: The professional layer

**Goal.** The features that are missing across the entire iOS market. This is where the product becomes the one professionals recommend.

**Scope**

- Compound clips and nested sequences.
- Adjustment layers.
- Masking: shape, freehand, feather, expansion, and mask tracking.
- Speed ramps with a curve editor.
- Stabilization and rolling shutter correction.
- Audio: noise reduction, voice isolation, parametric EQ, compressor, limiter, loudness normalization, ducking, waveform sync, beat detection.
- Color: curves, wheels, LUT import and export, scopes, log conversion and camera LUTs.
- ProRes export, HDR export (HLG and HDR10), audio stems, image sequences.
- Proxies, external drive projects, external display, Apple Pencil tools, background export with Live Activities.
- FCPXML export.

**Gate.** A working editor completes a paid client project entirely on an iPad, and the output round-trips into Final Cut Pro or Resolve through FCPXML without manual repair.

## Phase 3: AI and automation, done properly

**Goal.** On-device intelligence that saves real time, with no credits and no uploads.

**Scope**

- Transcript-based editing: delete words, the timeline follows.
- Filler word and silence removal with adjustable aggressiveness.
- Background removal, auto reframe, scene detection.
- Optional cloud features behind explicit consent, clearly labeled and priced.
- Foundation Models integration for script-to-edit and natural language search, on iOS 26 and later only.

**Gate.** Editing a 20-minute interview to a 3-minute cut takes under 15 minutes of user time, entirely offline, and every automated change is visible and reversible.

## Phase 4: Windows

**Goal.** A native Windows application over the same core, at feature parity with the iPad build for editing, with Windows-specific strengths.

**Scope**

- Native Windows shell. The decision between WinUI 3 and a Qt shell is recorded in `docs/architecture.md` and should be revisited at the start of this phase, not before.
- Core compiles unchanged. Only the render backend, decoder, and encoder are new: Direct3D 12 and Media Foundation, with FFmpeg as the codec fallback.
- Shared project format, so a project opens on iPad and Windows with identical results.
- Windows-specific: multi-monitor, mouse and keyboard conventions, drag and drop from Explorer, large file handling, GPU vendor variance.
- Optional sync between devices.

**Gate.** The same project file opens on an iPad and a Windows PC and exports byte-comparable output within encoder tolerance.

## Phase 5: Post-launch

**Goal.** Retention, reputation, and revenue.

**Scope**

- Performance work driven by real telemetry the user opts into.
- Localization into the top ten languages.
- Collaboration and review links.
- Mac app via Catalyst or a shared SwiftUI target.
- Plugin format for third-party effects.
- Template sharing between users, if and only if the moderation obligations are acceptable.

**Gate.** Sustained four-star-plus rating, and the top support requests are all shipping features rather than defects.

## Cross-cutting work, every phase

- **Testing.** Unit tests on the core for every edit operation, golden-frame image tests on the renderer, and a device test matrix covering the oldest supported iPhone and iPad.
- **Performance budget.** Preview at 60 fps for 1080p with three layers on an iPhone 13, 4K with three layers on an iPhone 15. Any change that breaks the budget is a bug.
- **Memory discipline.** Large media handled by reference and memory-mapped where possible. The app should survive a 30-minute 4K timeline on a device with 6 GB of RAM.
- **Crash-free rate target.** 99.8 percent or better before any feature work in a release cycle.
- **Documentation.** Every architectural decision recorded as a short decision record in `docs/`.
- **Release cadence.** Two-week TestFlight builds, App Store releases every four to six weeks.

## Risk register

**The engine takes longer than expected.** The single largest risk. Mitigation: Phase 0 is scoped to one export path and three clips. Nothing else starts until that works.

**App Review rejects the app as a copycat or as spam.** Mitigation: distinct interaction design, no borrowed brand language, original icon and naming, and a review note that explains what is genuinely new. Do not describe the app as "CapCut but better" anywhere in the metadata.

**Thermal throttling makes 4K editing unpleasant on older devices.** Mitigation: proxies, quality tiers in preview, and honest device gating. Resolve's reviews show what happens when you ignore this.

**Codec and licensing exposure.** Mitigation: use Apple's hardware codecs on iOS, keep any FFmpeg build LGPL-compliant with dynamic linking and no GPL-only components, and get legal review before shipping HDR and ProRes export commercially.

**The one-time purchase does not fund ongoing development.** Mitigation: keep ongoing costs near zero by design (on-device AI, no hosted rendering), and reserve subscription for cloud and licensed content only.

**Scope creep from the feature list.** Mitigation: the P0 list is closed. New ideas go into P1 or P2 or they wait.

**Windows becomes a second product.** Mitigation: the core has no platform code, and the Windows phase starts only after the iOS product is stable and reviewed.

## What happens next, concretely

1. Finish the core timeline model and its tests (in progress).
2. Write the render graph design into `docs/architecture.md`.
3. Scaffold the Xcode project and the Metal preview prototype.
4. Get a TestFlight build with import, trim, and export working.
5. Then, and only then, start on the P0 feature list in earnest.
