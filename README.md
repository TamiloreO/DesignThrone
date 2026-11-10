# Video Editor (working title)

A professional-grade video editor for iPhone, iPad, and later Windows. Native UI on each platform over one shared, portable editing engine.

The App Store build comes first. Windows is a second front end over the same engine, not a rewrite.

## Status

Planning is done and the engine skeleton is working: timeline model, edit operations, and undo/redo are implemented and covered by 615 assertions that pass on GCC, with no compiler warnings. The iOS app, the renderer, and the Windows shell do not exist yet, so there is no app binary or TestFlight build.

## Documents

- [docs/features.md](docs/features.md) - the full feature specification, with priorities and competitor baselines
- [docs/roadmap.md](docs/roadmap.md) - phased build plan with exit criteria
- [docs/competitive-analysis.md](docs/competitive-analysis.md) - what CapCut, LumaFusion, Final Cut Pro, Resolve, Premiere, iMovie, VN, InShot, KineMaster, and Edits actually ship, and where the gaps are
- [docs/architecture.md](docs/architecture.md) - repository layout, module boundaries, and the decisions behind them

## Repository layout

```
core/       Portable C++20 editing engine: timeline model, edit operations, render graph
ios/        SwiftUI app (added when the Xcode project is scaffolded)
windows/    Native Windows shell (later phase)
docs/       Specifications, roadmap, and decision records
```

## Build and test the core

```
cd core
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

The core has no platform dependencies and no external libraries, so it builds on macOS, Windows, and Linux. CI runs the same suite on all three, in Debug and Release.

## Constraints that shape the design

- App Store uploads must be built with Xcode 26 or later against the iOS 26 SDK (required since April 28, 2026). The deployment target can still be lower, so the plan is iOS 18 minimum with iOS 26 features behind availability checks.
- Everything that edits footage runs on device. No uploads, no per-use AI credits, no watermarks.
- The engine is written in portable C++ with no Apple frameworks inside it, so the Windows build reuses it unchanged.

## Not yet decided

- Product name and bundle identifier
- Monetization split between one-time purchase and subscription
- Whether a template marketplace ships in v1 (it triggers extra App Store review obligations)
