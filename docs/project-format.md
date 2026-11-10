# Project format

A project is a single JSON manifest, written by `vx::toJson` and read by
`vx::parseProjectJson` in `core/src/project_io.cpp`. Files use the extension
`.vxproj`.

The format exists so a project outlives the app: it is documented, versioned, and
readable without the editor. A bug in the reader costs a support ticket, not
someone's edit.

## Rules

1. **Times are integer nanoseconds.** Fields ending in `Ns` are signed 64 bit
   integers. Seconds as decimals would round, and a rounded trim comes back one
   frame short. Nanoseconds survive a save and reload exactly.
2. **Unknown fields are ignored.** A newer build can add optional fields without
   breaking older builds.
3. **Unknown versions are refused.** A reader that sees a higher `version` stops
   with a clear message instead of guessing at a schema it does not know.
4. **The id counter is repaired on load.** The saved `nextId` is replaced when it
   is not higher than every id in the file, so a hand-edited project cannot hand
   out a colliding id.
5. **An invalid project is rejected.** The same `validate` pass that guards edits
   runs on load, so overlapping clips or an inverted source range never reach the
   timeline.
6. **Saved text is stable.** Loading and saving an untouched project produces
   byte-identical JSON, which keeps version control and diffing sane.

## Schema, version 1

```json
{
  "format": "vx.project",
  "version": 1,
  "name": "Untitled",
  "nextId": 103,
  "media": [
    {
      "id": 1,
      "path": "Media/1B4F.mov",
      "name": "Beach clip",
      "durationNs": 12345678901,
      "width": 3840,
      "height": 2160,
      "frameRate": { "numerator": 30000, "denominator": 1001 },
      "hash": "sha256:..."
    }
  ],
  "sequences": [
    {
      "id": 1,
      "name": "Sequence 1",
      "width": 1920,
      "height": 1080,
      "frameRate": { "numerator": 30, "denominator": 1 },
      "tracks": [
        {
          "id": 10,
          "kind": "video",
          "name": "V1",
          "muted": false,
          "hidden": false,
          "locked": false,
          "clips": [
            {
              "id": 101,
              "media": 1,
              "startNs": 0,
              "sourceInNs": 2000000000,
              "sourceOutNs": 7000000000,
              "speedNumerator": 1,
              "speedDenominator": 1,
              "enabled": true,
              "name": ""
            }
          ]
        }
      ]
    }
  ]
}
```

Notes on fields:

- `kind` is `"video"` or `"audio"`. Anything else is rejected.
- `speedNumerator` and `speedDenominator` express speed as an exact fraction, so
  0.8x is `4/5` and never a repeating decimal.
- `media` entries describe footage, not edits. Clips reference them by `media`.
- Frame rates are fractions for the same reason as speed: 29.97 is `30000/1001`.

## Where the iOS app keeps projects and footage

The working project is `Application Support/Projects/current.vxproj`. The app
writes it to a temporary file and then replaces the previous one, so an
interrupted save cannot leave a truncated project behind.

Footage lives in `Application Support/Media/`, with each file named after the
media id it belongs to, for example `Media/7.mov`. That is the whole mapping: a
`media` entry's id is the file name, so a reopened project finds its footage
without an index file that could drift out of sync. Footage the user removed
from storage leaves its clips without a video source instead of pointing them at
someone else's clip.

## Deliberately not in version 1

Effects, keyframes, transitions, compound clips, adjustment layers, colour, and
audio mixing. Each is a schema change, which means a version bump and a migration
decision, and none of them exist in the engine yet. Adding them before the
engine supports them would produce a format that lies about what it stores.

Also missing: media bookmarks and security-scoped references, proxies, and
thumbnails. The architecture calls for those in the project package; the current
manifest stores a `path` relative to the app's media folder, which is enough for
Phase 0 but not for external-drive workflows.

## Compatibility policy

- Adding an optional field: keep `version`, older builds ignore it.
- Changing a field's meaning or removing one: bump `version` and add a migration.
- Refusing to open a project must always say why. "This project was saved by a
  newer version of the app" is actionable. "Invalid file" is not.
