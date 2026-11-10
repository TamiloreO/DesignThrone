# Feature specification

This is the complete feature set for the editor, organized by area. Each area lists what competitors ship, what we will ship, and the acceptance criteria that define "done". Priorities:

- **P0** ships in the first App Store release. Without it the app is not a credible editor.
- **P1** ships in the next two releases. This is where we overtake the mid-tier competitors.
- **P2** is the professional layer and the Windows-parity work. Later, but designed for now.

The rule for every feature: it runs on device, it does not require an account, and it does not consume credits.

## Positioning

**One line:** a professional editing engine that respects your footage and your wallet, with no watermark, no export paywall, and no upload.

**Primary user:** the creator who has outgrown iMovie and CapCut but does not want to pay a subscription for basics or fight a desktop-class interface on a phone. They shoot on an iPhone, edit on an iPhone or iPad, occasionally finish on a Mac or PC, and they care about frame accuracy, audio, and captions that are correct.

**Secondary user:** the iPad-first professional who wants a field editing station that does not lie about what it can do.

**What we deliberately do not compete on:** template feeds, trending audio, social network integration, and influencer growth tools. Those belong to apps that monetize attention. We monetize tooling.

## Product principles

1. **The free tier finishes real projects.** No watermark, no resolution cap below 4K, no export block.
2. **Buy the tool once.** The editor is a one-time purchase. Subscription is only for services that cost us money continuously.
3. **Nothing leaves the device unless the user asks.** All editing, AI analysis, and export happen locally. Cloud is opt-in and clearly labeled.
4. **Touch first, but never touch only.** Every action has a keyboard shortcut, and every shortcut is discoverable.
5. **Precision over magic.** Automation is offered as an action the user triggers and can undo, never as a silent change to their timeline.
6. **The project outlives the app.** Documented project format, plus FCPXML export.

## Platform commitments

- iPhone and iPad, universal app with a genuinely different layout per size class, not a scaled phone UI.
- Minimum iOS 18, with iOS 26 features (SpeechAnalyzer, Foundation Models, Liquid Glass) enabled at runtime when available.
- Windows later, sharing the same engine, with its own native shell.
- Portrait and landscape editing on both devices.

## 1. Project and media management

**Competitor baseline.** CapCut and InShot keep projects in a flat list with almost no media management. Final Cut Pro for iPad has no bins or folders and duplicates imported media into project storage, which reviewers single out as a serious flaw. LumaFusion and Resolve have real media pools and external drive support. Resolve has Blackmagic Cloud collaboration.

**What we ship**

| Feature | Priority | Notes |
|---|---|---|
| Project creation with aspect ratio, resolution, frame rate, and color space presets | P0 | 16:9, 9:16, 1:1, 4:5, 2.39:1, custom; 23.976 to 120 fps; SDR Rec.709, HLG, HDR10 |
| Multiple sequences per project, duplicate and reorder | P0 | Each sequence has independent settings |
| Media library with folders, smart collections, favorites, and search | P0 | Search by filename, date, resolution, frame rate, duration, and tag |
| Import from Photos, Files, iCloud Drive, and drag and drop | P0 | Photo library limited-access handling with clear explanation |
| Import from external USB-C drives and SD cards | P0 | Read directly, never copy by default |
| Non-destructive reference model | P0 | Media stays where it is; project stores bookmarks and hashes |
| Relink missing media | P0 | Batch relink by folder or by fingerprint |
| Proxy generation and toggle | P1 | HEVC proxies at half resolution, background generation, automatic switching |
| Project autosave, version history, and crash recovery | P0 | Autosave every edit, restore point every 10 minutes |
| Project package export and import | P1 | Single folder package, portable between devices |
| FCPXML 1.11 export | P1 | For handoff to Final Cut Pro and Resolve |
| Cloud sync across the user's devices | P2 | Opt-in, project-level, conflict-safe |
| Collaborative review with comments | P2 | Frame-anchored comments, read-only share links |

**Acceptance criteria.** A user imports 200 clips from an external SSD, organizes them into three folders, builds a sequence, quits the app mid-edit, reopens it, and finds the timeline exactly as they left it with no media duplicated on disk.

## 2. Timeline and core editing

**Competitor baseline.** CapCut uses a simple track stack. LumaFusion has six video and six audio tracks (twelve with paid features) and real insert and overwrite editing. Final Cut Pro uses a magnetic timeline with connected clips, which is brilliant for speed and frustrating for users who think in tracks. Premiere mobile has unlimited layers. Nobody on iOS ships compound clips.

**What we ship**

| Feature | Priority | Notes |
|---|---|---|
| Track-based timeline with optional magnetic behavior | P0 | Tracks default to magnetic mode; a toggle switches to fixed positions with gaps |
| Eight video tracks and eight audio tracks in v1, scalable architecture | P0 | Limit is a performance setting, not a hard model constraint |
| Trim, ripple trim, roll, slip, and slide | P0 | All five, all frame accurate, all with numeric entry |
| Insert, overwrite, append, replace, and lift edits | P0 | Predictable results, undoable |
| Frame-accurate scrubbing with audio scrubbing | P0 | Jog wheel gesture on iPhone, keyboard frame stepping |
| Snapping with adjustable strength and per-track overrides | P0 | Snap to clip edges, playhead, markers, and beat markers |
| Clip groups, linked audio and video, and multi-select transforms | P0 | Group moves and trims as one unit |
| Markers, ranges, and notes | P1 | Range markers export as chapter metadata |
| Timeline zoom from full sequence to single frame | P0 | Pinch plus keyboard shortcuts, 30x or better |
| Nested sequences | P1 | A sequence used as a clip, editable in place |
| Compound clips | P1 | Flatten a selection into a reusable clip with its own internal timeline. Nobody on iOS has this |
| Adjustment layers | P1 | One effect stack applied to everything beneath it. Only LumaFusion has this |
| Multicam editing | P2 | Sync by audio waveform or timecode, angle switching, up to 9 angles |
| Timeline search and transcript-based navigation | P2 | Jump to a spoken phrase |

**Acceptance criteria.** Editing a 40-clip sequence with ripple trims, roll edits, and a compound clip stays responsive at 60 fps of UI interaction, and every operation is exactly reversible with undo, including across app restarts.

## 3. Clip tools and compositing

**Competitor baseline.** CapCut has keyframes, masks, chroma key, motion tracking, and stabilization. LumaFusion has unlimited keyframes with Bezier paths, blend modes, keying, and its own stabilizer. Final Cut Pro has keyframes but no object tracking and no post-production stabilization. Premiere mobile has keyframes and background removal but no masking.

**What we ship**

| Feature | Priority | Notes |
|---|---|---|
| Transform: position, scale, rotation, anchor point, flip, crop | P0 | Numeric entry plus gestures, with reset per property |
| Opacity and blend modes | P0 | 24 modes including multiply, screen, overlay, add, soft light |
| Speed controls from 0.1x to 100x, reverse, freeze frame | P0 | Preserve pitch option, frame blending, optical flow as P1 |
| Speed ramps with an editable curve | P1 | Bezier curve editor, presets, ramps that respect audio pitch |
| Keyframes on every animatable property | P0 | Linear, ease in, ease out, and Bezier handles; copy and paste keyframes between clips |
| Masking: shape, freehand, and matte | P1 | Rectangle, ellipse, polygon, freehand path; feather, expansion, invert; mask tracking |
| Chroma key with spill suppression | P0 | Real time preview, alpha matte view, edge refinement |
| Luma key and difference matte | P2 | For compositing without a green screen |
| Video stabilization | P1 | Gyro-assisted where metadata exists, optical fallback |
| Rolling shutter correction | P2 | Uses gyro data from iPhone captures |
| Corner pin and perspective transform | P2 | Screen replacement workflow |
| Object tracking | P2 | Point and planar tracking to attach text, stickers, or masks |
| Live Photo and portrait depth support | P2 | Depth-based effects using the embedded depth map |
| HDR-aware compositing | P1 | Correct tone mapping when HDR and SDR media mix on one timeline |

**Acceptance criteria.** A three-layer composite with a keyed subject, an animated mask, and a speed ramp previews in real time on an iPhone 15 or newer and exports with identical results.

## 4. Audio

**Competitor baseline.** KineMaster has the best mixer on mobile: per-track volume, pan, pitch, 13 EQ presets, 36 voice presets, volume envelopes, and ducking. LumaFusion has parametric EQ, voice isolation, Audio Unit plugins, and 12 audio tracks. Final Cut Pro has Logic effects and voice isolation. Premiere mobile is explicitly weak here, which reviewers call out.

**What we ship**

| Feature | Priority | Notes |
|---|---|---|
| Multitrack mixer with per-clip and per-track volume, pan, mute, solo | P0 | 8 tracks in v1, metering with peak and RMS |
| Audio fades and crossfades, automatic and manual | P0 | Drag handles plus numeric duration |
| Volume automation envelopes | P0 | Drawable, with keyframes |
| Voiceover recording into the timeline | P0 | Countdown, monitoring, takes management, punch-in |
| Noise reduction and voice isolation | P1 | On-device ML, adjustable strength, real-time preview |
| Parametric EQ, compressor, limiter, and gate | P1 | Four-band EQ minimum, presets plus manual |
| Loudness normalization to a target LUFS | P1 | Per clip and per export, with true peak limiting |
| Automatic ducking of music under dialogue | P1 | Sidechain-style, adjustable depth and release |
| Beat detection and beat markers | P1 | Drives snapping and AutoCut |
| Audio sync by waveform | P1 | Align separate recorder audio to camera audio |
| Audio Unit plugin support | P2 | Third-party effects and instruments |
| Stereo and multichannel handling | P2 | Dual mono, channel mapping, 5.1 passthrough for export |
| Pitch shift and time stretch | P2 | Independent of speed, formant preservation |

**Acceptance criteria.** A two-person interview with separate lav recordings can be synced, cleaned, leveled to minus 14 LUFS, and mixed with music that ducks automatically, entirely on device.

## 5. Text and captions

**Competitor baseline.** CapCut, Premiere, VN, InShot, KineMaster, and Edits all ship auto captions. LumaFusion has none, which its users complain about. Final Cut Pro's Generate Captions is English only. InShot and VN support bilingual subtitles. Every competitor's caption editor is described as tedious to correct.

**What we ship**

| Feature | Priority | Notes |
|---|---|---|
| Title system with fonts, styles, stroke, shadow, background, and animation presets | P0 | Custom font import, saved styles, per-project brand kit |
| Automatic captions using SpeechAnalyzer | P0 | On device, iOS 26 and later; Whisper-class fallback bundled for iOS 18 |
| Caption editor built for correction, not just generation | P0 | Full transcript view, edit text once and it re-times, find and replace, filler word removal, speaker labels |
| Caption styling with word-level highlighting | P0 | Karaoke-style highlight, per-word animation, safe area guides for vertical video |
| Subtitle import and export: SRT, VTT, and TTML | P0 | Also burn-in or sidecar choice at export |
| Translation of existing captions | P1 | On-device where supported, optional cloud with explicit consent |
| Text behind subject | P1 | Uses the person segmentation mask to place text between subject and background |
| Motion text templates | P2 | Parameterized presets, user-savable |
| Live text recognition in footage | P2 | Detect and replace signage or screens in frame |

**Acceptance criteria.** Correcting a misheard sentence takes two taps, with the following words re-timed automatically. Transcription speed is a target to be measured on device, not a promise: the goal is faster than real time on an iPhone 15, with the exact budget set once SpeechAnalyzer is benchmarked in Phase 3.

## 6. Color

**Competitor baseline.** Resolve owns this category with node-based grading, Power Windows, qualifiers, and HDR. LumaFusion has scopes and LUT import. Final Cut Pro for iPad has no curves and no custom LUTs. CapCut has basic sliders. Most phone editors have no scopes at all.

**What we ship**

| Feature | Priority | Notes |
|---|---|---|
| Primary correction: exposure, contrast, highlights, shadows, whites, blacks, saturation, vibrance, temperature, tint | P0 | Non-destructive, keyframable |
| Curves: luma, red, green, blue | P1 | Bezier control points |
| Color wheels: lift, gamma, gain, offset | P1 | With per-wheel master |
| HSL qualifier and secondary correction | P2 | Isolate a color range and grade only that |
| LUT import (.cube, .3dl) with intensity | P1 | Also LUT export of the current grade |
| Scopes: waveform, vectorscope, histogram, parade | P1 | Real time, GPU rendered |
| Shot matching and auto balance | P2 | Match a clip to a reference frame |
| HDR grading with HLG and PQ output | P2 | Correct HDR preview on supported displays |
| Log conversion and camera LUTs | P1 | Apple Log, S-Log, V-Log, D-Log presets |
| Color space management | P2 | Rec.709, Rec.2020, ACES-style pipeline |
| Grade copy and paste across clips | P0 | Paste attributes with per-property selection |

**Acceptance criteria.** A user can match two clips from different cameras, apply a LUT, verify skin tones on a vectorscope, and export in HDR without leaving the iPad.

## 7. Effects and transitions

**Competitor baseline.** CapCut's effects library is enormous but increasingly paywalled. KineMaster sells a 75,000-asset store. LumaFusion lets users build and share effect presets. VN supports custom transitions.

**What we ship**

| Feature | Priority | Notes |
|---|---|---|
| GPU effect stack per clip, reorderable | P0 | Every effect has intensity and is keyframable |
| Core effect set: blur, sharpen, glow, grain, vignette, glitch, light leak, chromatic aberration, mirror, mosaic | P0 | Written as Metal shaders in the shared engine |
| Transitions: dissolve, dip to color, wipe, slide, zoom, blur, and 3D | P0 | Duration and direction editable on the timeline |
| Custom transitions built from a mini-composition | P2 | Advanced, for power users |
| Effect presets, save and share | P1 | Preset files exportable |
| Motion blur and optical flow retiming | P2 | Quality tiers with preview fallback |
| Particle and light generators | P2 | Procedural, GPU driven |
| Third-party effect plugin format | P2 | Metal-based, documented |

**Acceptance criteria.** Any effect can be applied, adjusted, keyframed, reordered, and removed without a frame drop in preview, and export matches preview pixel for pixel.

## 8. AI and automation

**Competitor baseline.** AI is now table stakes, and it is where competitors differ most in philosophy. CapCut, InShot, and VN push AI to cloud credits. Final Cut Pro and Resolve keep it on device but gate it behind subscriptions or Studio. Adobe sells Firefly credits.

**Our position.** On-device first, triggered by the user, always undoable, never uploaded by default. Where an on-device model cannot do the job, the cloud option is a clearly labeled, consent-gated, one-time action.

| Feature | Priority | Engine | Notes |
|---|---|---|---|
| Auto captions and transcription | P0 | SpeechAnalyzer (iOS 26), bundled Whisper for iOS 18 | Offline, unlimited |
| Transcript-based editing | P1 | Same transcript | Delete words in the transcript and the video cuts follow |
| Filler word and silence removal | P1 | On-device | "Um", "uh", long pauses, with adjustable aggressiveness |
| Subject and background removal | P1 | Vision person segmentation | No green screen required |
| Auto reframe for aspect ratios | P1 | Vision saliency and face detection | Keyframed result the user can adjust |
| Scene and shot detection | P1 | On-device | Splits long takes at cuts |
| Auto color balance and shot matching | P2 | On-device | Matches reference frame |
| Object tracking | P2 | Vision tracking | Drives masks, text, and stickers |
| Super resolution upscaling | P2 | On-device ML | 1080p to 4K, quality tiers |
| Script-to-edit | P2 | Foundation Models | Paste a script, get a rough cut from footage that matches it |
| Natural language timeline search | P2 | Foundation Models plus transcripts and visual embeddings | "Find the shot where he opens the door" |
| Text-based video generation | P2 | Optional cloud, consent gated | Explicitly not a core feature |
| AI voice generation for narration | P2 | Optional, licensed voices | Must respect Apple's rules on cloning real people |

**Acceptance criteria.** Every AI feature works in airplane mode, produces a result the user can inspect before applying, and can be undone in one tap.

## 9. Export and delivery

**Competitor baseline.** CapCut charges for long exports and premium assets. VN exposes bitrate up to 300 Mbps. KineMaster exports H.264 or HEVC with bitrate control. Final Cut Pro exports ProRes, HEVC, and H.264. Premiere mobile is free and watermark free at 4K.

**What we ship**

| Feature | Priority | Notes |
|---|---|---|
| H.264 and HEVC export up to 4K60 | P0 | Hardware encoder, quality and bitrate controls |
| ProRes 422 and 422 HQ export | P1 | On devices that support it |
| HDR export: HLG and HDR10 | P1 | Correct metadata, verified on a reference display |
| Audio-only export: AAC, WAV, AIFF | P1 | Plus stems export per track |
| Still frame and image sequence export | P1 | PNG, JPEG, TIFF, ProRes 4444 with alpha |
| Social presets with safe areas | P0 | Vertical, square, and landscape with platform overlays |
| Direct share sheet and Save to Photos | P0 | Never a mandatory app account |
| Background export with progress notifications | P0 | Continues when the app is backgrounded |
| Export queue with per-item settings | P1 | Multiple deliverables from one sequence |
| XML, FCPXML, and EDL export | P1 | Interchange for finishing elsewhere |
| Chapter markers in MP4 | P2 | From timeline range markers |
| Direct upload to YouTube and Vimeo | P2 | Optional, OAuth based |
| Watermark | never | Not in the free tier, not as an upsell |

**Acceptance criteria.** A 12-minute 4K60 HDR project exports in the background in under the duration of the clip on an iPhone 15, with audio and video in sync within one frame, and the file passes verification in QuickTime and Premiere.

## 10. iPad and pro workflow

**Competitor baseline.** Final Cut Pro has the best Pencil and jog wheel interaction. LumaFusion and Resolve support external drives, external displays, and desktop round-trip. Resolve has cloud collaboration. Premiere mobile has none of this.

**What we ship**

| Feature | Priority | Notes |
|---|---|---|
| Keyboard shortcut set with a searchable command palette | P0 | Every action reachable, remappable |
| Apple Pencil: scrubbing, precision trim, mask drawing, live drawing on frames | P1 | Hover preview on supported iPads |
| External display with a clean preview feed and a separate workspace | P1 | Stage Manager aware |
| Trackpad and mouse with hover states and right click menus | P0 | Not an afterthought |
| Background export and rendering with Live Activities | P1 | Requires iOS 26 on supported hardware |
| External drive project storage | P1 | Project and media live on the drive |
| Multi-window with a second sequence open | P2 | Compare two cuts side by side |
| Mac Catalyst or native Mac app | P2 | Shares the iPad UI code |
| Collaboration with frame-anchored comments | P2 | Review links, no account required for reviewers |

**Acceptance criteria.** A user edits an entire project on an iPad with a Magic Keyboard, an external SSD, and a monitor, never touching the on-screen keyboard, and cannot name a common action that lacks a shortcut.

## 11. Accessibility and localization

| Feature | Priority | Notes |
|---|---|---|
| VoiceOver labels on every control, including timeline clips | P0 | Timeline must be navigable without sight |
| Dynamic Type up to accessibility sizes | P0 | Layout reflows rather than clips |
| Reduce Motion and Reduce Transparency support | P0 | |
| High contrast and color-blind safe accents | P0 | Not color alone to convey state |
| Full keyboard navigation with focus rings | P0 | |
| Caption and audio description support in export | P1 | |
| Localization framework from day one, English first | P0 | Right-to-left layout ready |
| Haptics with intensity settings | P1 | |

**Acceptance criteria.** A VoiceOver user can import a clip, trim it, add a title, and export, without sighted assistance.

## 12. Monetization

The model has to be decided before the first release, so it is specified here as a working proposal.

- **Free tier:** full editing, full export, no watermark, 4K60, captions. This is the product, not a demo.
- **Pro unlock, one-time purchase:** the professional layer. Compound clips, adjustment layers, multicam, curves and wheels, scopes, ProRes and HDR export, external drive projects, FCPXML export, Audio Unit support.
- **Optional subscription:** only for services with ongoing cost. Cloud sync and storage, licensed stock music and footage, optional cloud AI for models that cannot run on device. Cancel and the app keeps working.
- **No credits.** No per-caption, per-export, or per-generation pricing.
- **Family Sharing** enabled for the one-time unlock.

The one-time unlock is the differentiator. It should be priced as a professional tool, not as a 99 cent utility, and it should be honest about what it includes.

## 13. Deliberate non-goals for v1

- A template marketplace or social feed. It invites App Store review obligations around user-generated content, moderation, reporting, and blocking, and it is not our fight.
- Trending audio licensing. Licensing music for social use is a legal minefield and a permanent cost center.
- Live streaming.
- A web editor.
- Android.
- Desktop-class node compositing (Fusion-style).
- Being a camera app. Capture integration comes later, if at all.

## Priority summary

**P0, first release.** Project and media model, 8 video and 8 audio track timeline, all core trim operations, transform, opacity, blend modes, keyframes, chroma key, speed controls, audio mixer with voiceover and fades, titles, on-device auto captions with a real caption editor, primary color correction, core effects and transitions, H.264 and HEVC export up to 4K60 with background export, keyboard shortcuts, accessibility baseline.

**P1, releases two and three.** Compound clips, adjustment layers, masking with tracking, speed ramps, stabilization, noise reduction and EQ, loudness normalization, ducking, beat detection, transcript editing, silence and filler removal, background removal, auto reframe, scene detection, LUT import, curves and wheels, scopes, ProRes and HDR export, proxies, external drives, external display, Pencil tools, FCPXML, cloud sync.

**P2, professional layer and Windows.** Multicam, object tracking, qualifiers, HDR grading pipeline, color space management, Audio Units, collaboration and review, Mac app, Windows app, third-party plugin format, generative features.
