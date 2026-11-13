#!/usr/bin/env python3
"""Draws the 1024x1024 App Store icon.

The icon is generated rather than hand drawn so it can be regenerated at any
size, and so the repository holds the source of truth instead of a binary that
nobody can edit. Run from the repository root:

    python3 ios/tools/make_app_icon.py

Output: ios/VideoEditor/Assets.xcassets/AppIcon.appiconset/AppIcon-1024.png
"""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageOps

SIZE = 1024
OUT = (
    Path(__file__).resolve().parents[1]
    / "VideoEditor"
    / "Assets.xcassets"
    / "AppIcon.appiconset"
    / "AppIcon-1024.png"
)

TOP = (8, 13, 20)
BOTTOM = (18, 58, 82)
GLOW = (45, 212, 191)


def background() -> Image.Image:
    """Vertical gradient with a soft glow behind the mark."""
    gradient = ImageOps.colorize(Image.linear_gradient("L").resize((SIZE, SIZE)), TOP, BOTTOM)

    glow = Image.new("L", (SIZE, SIZE), 0)
    ImageDraw.Draw(glow).ellipse((212, 172, 812, 772), fill=110)
    glow = glow.filter(ImageFilter.GaussianBlur(140))
    glow_image = Image.new("RGB", (SIZE, SIZE), GLOW)
    return Image.composite(glow_image, gradient, glow.point(lambda v: v // 3))


def rounded_shape(mask: Image.Image, radius: int) -> Image.Image:
    """Softens the corners of a hard-edged mask so the mark does not look cut."""
    blurred = mask.filter(ImageFilter.GaussianBlur(radius))
    return blurred.point(lambda v: 255 if v > 127 else 0)


def draw_mark(base: Image.Image) -> Image.Image:
    # Play triangle, nudged right so it reads as optically centred.
    triangle = Image.new("L", (SIZE, SIZE), 0)
    ImageDraw.Draw(triangle).polygon([(410, 318), (410, 622), (694, 470)], fill=255)
    triangle = rounded_shape(triangle, 12)

    # A scrubber under the mark: the editing half of the idea. Deliberately a
    # track with one knob rather than stacked bars, which read as a pause icon.
    scrubber = Image.new("L", (SIZE, SIZE), 0)
    draw = ImageDraw.Draw(scrubber)
    draw.rounded_rectangle((286, 772, 738, 800), radius=14, fill=95)
    draw.ellipse((330, 750, 402, 822), fill=255)

    white = Image.new("RGB", (SIZE, SIZE), (255, 255, 255))
    base.paste(white, (0, 0), triangle)
    base.paste(white, (0, 0), scrubber)
    return base


def main() -> None:
    icon = draw_mark(background()).convert("RGB")
    OUT.parent.mkdir(parents=True, exist_ok=True)
    icon.save(OUT, format="PNG", optimize=True)
    print(f"wrote {OUT} ({icon.size[0]}x{icon.size[1]})")


if __name__ == "__main__":
    main()
