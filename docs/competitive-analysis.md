# Competitive analysis: iPhone and iPad video editors

Research date: November 10, 2026. Ten products were reviewed from official documentation, App Store listings, and recent user discussion. Prices are US App Store prices unless noted.

## Why this market is hard

The category is crowded and the floor keeps rising. Auto captions, background removal, noise removal, and 4K export are now baseline expectations, not differentiators. A new entrant cannot win by shipping a shorter feature list than CapCut at a lower price.

The openings are specific. Users consistently complain about the same things: watermarks, surprise paywalls, subscriptions that charge for basics, editors that treat the iPad as a companion to a desktop rather than a workspace, and AI that requires uploading their footage. Those complaints are the product thesis, not just marketing copy.

## Where each product sits

**CapCut (ByteDance).** Free tier with Standard at $9.99/month and Pro at $19.99/month. The strongest social workflow in the category: templates, trending audio, auto captions, AutoCut beat sync, background removal, speed curves from 0.1x to 100x, keyframes, shape masks, chroma key, motion tracking, stabilization. The dedicated iPad build has been described in its own help center as beta and unavailable in the US store, so iPad users mostly run the phone app scaled up. Recurring complaints are that previously free effects moved behind Pro and that export gets interrupted by paywall prompts. Also a privacy-sensitive choice for some users given its data disclosure and its 2025 US availability scare.

**LumaFusion (LumaTouch).** $29.99 one time, with a $9.99/month or $69.99/year Creator Pass for Storyblocks and enhanced tools. Six video tracks and six audio tracks base, twelve and twelve with enhanced features. The deepest "real editor" feel on iPad: insert and overwrite editing, magnetic timeline, adjustment clips, unlimited keyframes with Bezier paths, speed ramps, scopes, LUT import, Audio Unit plugin support, ducking, external drive and SMB editing, FCPXML export to Final Cut Pro. It is the clearest proof that people will pay once for depth. Its weaknesses are the flip side: no native auto captions, no machine learning features at all, aging interface, and no iPhone-first ergonomics.

**Final Cut Pro for iPad (Apple).** Now sold through Apple Creator Studio at $12.99/month or $129/year, with legacy $4.99/month plans grandfathered. The best touch editing interaction in the category: jog wheel, Magnetic Timeline, Apple Pencil live drawing, Live Multicam with up to four iPhones, ProRes RAW controls, on-device transcript search, visual search, beat detection, Montage Maker, scene removal mask, background export on M3 and later. Its gaps are equally clear: no compound clips, no bins or folders, no custom LUTs, no curves, no object tracking, no adjustment layers, no post-production stabilization, and imported media gets duplicated into project storage.

**DaVinci Resolve for iPad (Blackmagic).** Free, with Studio at $94.99 one time. Node-based color grading, HDR, Power Windows, qualifiers, tracking, Magic Mask, professional codecs including BRAW and ProRes on M-series. The most capable finishing tool on a tablet and the cheapest route to professional color. The cost is complexity, chip-dependent feature gating (A-series loses a lot), thermal throttling on long timelines, and no full Edit, Fusion, or Fairlight pages.

**Adobe Premiere mobile.** Free core editing with unlimited tracks and watermark-free 4K export, plus a $7.99/month or $69.99/year tier for generative AI credits and cloud. Launched September 2025 and has been shipping fast: keyframes in February 2026, speed curves and audio keyframes mid-2026, LUT import, stabilization, Auto Reframe, Beat Detection, background removal, and a one-way project handoff to desktop Premiere. Its weak spots are audio depth, masking, multicam, compound clips, and a fairly generic feel.

**iMovie (Apple).** Free, no in-app purchases, no watermark. Magic Movie and Storyboards remain the best guided editing experience for beginners, plus solid 4K60 export and easy handoff to Final Cut Pro for iPad. It caps out fast: three foreground audio clips, transitions limited to two seconds, no auto captions, no keyframes, no multicam, no color controls beyond filters.

**VN Video Editor.** Free with Pro at $7.99/month or $69.99/year. Punches far above its price: watermark-free 4K60 export, 1 Mbps to 300 Mbps bitrate control, HDR and Dolby Vision export on supported iPhones, custom keyframe curves, nine mask shapes, chroma key, LUT import, stabilization, proxy previews, editable project files that move between iOS, Android, Mac, and Windows. Weak on templates, social integration, and audio post.

**InShot.** Free with Pro at $4.99/month, $19.99/year, or $49.99 lifetime. The most accessible all-in-one editor: photo and collage tools alongside video, 0.1x to 100x speed curves, keyframes for filters and effects, smart tracking, chroma key, stabilization, auto captions with bilingual subtitles, AI silence removal, voice enhancement, background removal. Thin on audio processing, and its AI features upload media to cloud providers.

**KineMaster.** Free with Premium at $8.99/month or $59.99/year. The most generous "no tool locked" positioning, with a genuine audio mixer (per-track volume, pan, pitch, 13 EQ presets, 36 voice-changer presets, volume envelopes, ducking), 24 blend modes, corner pin, chroma key, keyframes with custom easing, on-device captions in 36 languages, and a 75,000-asset store. The free tier watermark and ads are the main friction, along with no HDR export and an aging interface.

**Edits (Instagram/Meta).** Free, no watermark, 4K60 SDR and HDR export, built-in camera, teleprompter, green screen, keyframes, auto captions, insights tied to Reels performance. iPhone only, no iPad layout, still missing basic precision tools, and locked to the Meta ecosystem.

## Feature coverage snapshot

Legend: yes means shipped and documented, partial means limited or gated, no means not found in current documentation.

| Capability | CapCut | LumaFusion | FCP iPad | Resolve | Premiere | iMovie | VN | InShot | KineMaster | Edits |
|---|---|---|---|---|---|---|---|---|---|---|
| Multitrack timeline | yes | yes | yes | yes | yes | partial | yes | partial | yes | partial |
| Auto captions on device | yes | no | yes | yes (Studio) | yes | no | yes (local model) | yes | yes | yes |
| Keyframes with curves | yes | yes | yes | yes | yes | no | yes | partial | yes | partial |
| Masking | yes | no | yes (scene removal) | yes | no | partial | yes | partial | yes | partial |
| Chroma key | yes | yes | yes | yes | partial | yes | yes | yes | yes | yes |
| Speed ramps | yes | yes | partial | yes | yes | no | yes | yes | yes | partial |
| Multicam | no | yes (paid) | yes | no | no | no | no | no | no | no |
| Compound clips | no | no | no | no | no | no | no | no | no | no |
| Adjustment layers | no | yes (paid) | no | partial | no | no | no | no | no | no |
| Color curves or wheels | partial | partial | no | yes | partial | no | partial | no | partial | partial |
| Audio EQ and ducking | partial | yes | yes | partial | no | ducking only | no | no | yes | no |
| LUT import | no | yes | no | yes | yes | no | yes | no | no | no |
| External drive editing | no | yes | yes | yes | no | no | no | no | no | no |
| Project interchange | no | FCPXML | .fcpproj | Resolve, FCPXML | Premiere desktop | FCP, iMovie | VN files | no | .kine | no |
| Watermark free free tier | partial | n/a (paid app) | n/a (paid) | yes | yes | yes | yes | partial | no | yes |
| One-time purchase option | no | yes | no | yes | no | n/a | no | yes | no | no |
| Native iPad layout | partial | yes | yes (iPad only) | yes (iPad only) | partial | yes | yes | partial | yes | no |

## What the table says

Nobody ships compound clips, and only LumaFusion ships adjustment layers. Color grading on phones is weak almost everywhere except Resolve. Audio post is weak everywhere except LumaFusion and KineMaster. Project interchange is fragmented and proprietary.

That is a real opening for a product that behaves like a desktop NLE on a touch screen without looking like one.

## The complaints that repeat

Watermarks and paywalls on basic exports. Features that were free moving behind a subscription. AI features that upload footage without clear consent, or that charge per use in credits. iPad apps that are phone apps scaled up. Media management that duplicates footage and has no folders. Export failures on long projects. Battery drain and thermal throttling during 4K editing. Crashes in the trim tools. Caption editors that are tedious to correct.

## Implications for our product

1. **No watermark, ever, and no export paywall.** The free tier must be able to finish a real project and export it.
2. **One-time purchase for the editor.** Charge for the tool once, and reserve subscription for genuinely ongoing services (cloud, licensed stock, hosted AI). This is the single clearest wedge against CapCut, and LumaFusion's longevity proves the demand.
3. **On-device AI as the default.** Apple's SpeechAnalyzer and Foundation Models frameworks make captions, transcript search, and script-based editing free to run, private, and offline. Most competitors either lack this or push it to cloud credits.
4. **Treat the iPad as the primary workspace.** Apple Pencil, keyboard, external display, external drives, background export, and real media management with folders and proxies.
5. **Ship the pro features that are missing everywhere.** Compound clips and adjustment layers first, then multicam and proper color.
6. **Keep the door open.** FCPXML and a documented project format, so users are never trapped and can round-trip to desktop tools.
7. **Do not copy CapCut's surface.** Beyond the App Store copycat rule, matching a template-driven feed app on its own terms is a losing fight. Win on precision, reliability, and ownership.
