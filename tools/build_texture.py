#!/usr/bin/env python3
"""Generate source PNG artwork for the shared gameplay HUD speedometer."""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path


SPRITES = {
    "needle": (8, 64),
    "gear": (64, 128),
    "rev": (16, 16),
    "speed": (60, 100),
    "unit": (64, 32),
}
GEARS = ("1", "2", "3", "4", "5", "6", "7", "8", "B", "F", "H", "MA", "N", "PM", "R")
REV_VALUES = tuple(str(value) for value in range(21))
DIGITS = tuple(str(value) for value in range(10))
UNITS = {"KPH1": "KM/H", "MPH1": "MPH"}
TEXTURE_ROOT = Path("HUD") / "Speedometer"
HUD_ZIP_PATH = "ui/textures/Anthem"

COLORS = {
    "white": (225, 249, 251, 255),
    "cyan": (42, 229, 232, 255),
    "cyan_dim": (17, 103, 117, 255),
    "red": (255, 82, 78, 255),
}

FONT = {
    "0": ("01110", "10001", "10011", "10101", "11001", "10001", "01110"),
    "1": ("00100", "01100", "00100", "00100", "00100", "00100", "01110"),
    "2": ("01110", "10001", "00001", "00010", "00100", "01000", "11111"),
    "3": ("11110", "00001", "00001", "01110", "00001", "00001", "11110"),
    "4": ("00010", "00110", "01010", "10010", "11111", "00010", "00010"),
    "5": ("11111", "10000", "10000", "11110", "00001", "00001", "11110"),
    "6": ("01110", "10000", "10000", "11110", "10001", "10001", "01110"),
    "7": ("11111", "00001", "00010", "00100", "01000", "01000", "01000"),
    "8": ("01110", "10001", "10001", "01110", "10001", "10001", "01110"),
    "9": ("01110", "10001", "10001", "01111", "00001", "00001", "01110"),
    "A": ("01110", "10001", "10001", "11111", "10001", "10001", "10001"),
    "B": ("11110", "10001", "10001", "11110", "10001", "10001", "11110"),
    "C": ("01111", "10000", "10000", "10000", "10000", "10000", "01111"),
    "D": ("11110", "10001", "10001", "10001", "10001", "10001", "11110"),
    "E": ("11111", "10000", "10000", "11110", "10000", "10000", "11111"),
    "F": ("11111", "10000", "10000", "11110", "10000", "10000", "10000"),
    "G": ("01111", "10000", "10000", "10111", "10001", "10001", "01111"),
    "H": ("10001", "10001", "10001", "11111", "10001", "10001", "10001"),
    "I": ("01110", "00100", "00100", "00100", "00100", "00100", "01110"),
    "J": ("00111", "00010", "00010", "00010", "00010", "10010", "01100"),
    "K": ("10001", "10010", "10100", "11000", "10100", "10010", "10001"),
    "L": ("10000", "10000", "10000", "10000", "10000", "10000", "11111"),
    "M": ("10001", "11011", "10101", "10101", "10001", "10001", "10001"),
    "N": ("10001", "11001", "10101", "10011", "10001", "10001", "10001"),
    "O": ("01110", "10001", "10001", "10001", "10001", "10001", "01110"),
    "P": ("11110", "10001", "10001", "11110", "10000", "10000", "10000"),
    "Q": ("01110", "10001", "10001", "10001", "10101", "10010", "01101"),
    "R": ("11110", "10001", "10001", "11110", "10100", "10010", "10001"),
    "S": ("01111", "10000", "10000", "01110", "00001", "00001", "11110"),
    "T": ("11111", "00100", "00100", "00100", "00100", "00100", "00100"),
    "U": ("10001", "10001", "10001", "10001", "10001", "10001", "01110"),
    "V": ("10001", "10001", "10001", "10001", "10001", "01010", "00100"),
    "W": ("10001", "10001", "10001", "10101", "10101", "10101", "01010"),
    "X": ("10001", "10001", "01010", "00100", "01010", "10001", "10001"),
    "Y": ("10001", "10001", "01010", "00100", "00100", "00100", "00100"),
    "Z": ("11111", "00001", "00010", "00100", "01000", "10000", "11111"),
    "/": ("00001", "00010", "00010", "00100", "01000", "01000", "10000"),
    " ": ("00000", "00000", "00000", "00000", "00000", "00000", "00000"),
}

Color = tuple[int, int, int, int]


class Canvas:
    def __init__(self, width: int, height: int):
        self.width = width
        self.height = height
        self.pixels = bytearray(width * height * 4)

    def pixel(self, x: int, y: int, color: Color) -> None:
        if 0 <= x < self.width and 0 <= y < self.height:
            offset = (y * self.width + x) * 4
            self.pixels[offset : offset + 4] = bytes(color)

    def rectangle(self, x: int, y: int, width: int, height: int, color: Color) -> None:
        left = max(0, x)
        right = min(self.width, x + width)
        top = max(0, y)
        bottom = min(self.height, y + height)
        if left >= right or top >= bottom:
            return
        row = bytes(color) * (right - left)
        for row_y in range(top, bottom):
            offset = (row_y * self.width + left) * 4
            self.pixels[offset : offset + len(row)] = row

    def line(self, x0: int, y0: int, x1: int, y1: int, color: Color) -> None:
        dx = abs(x1 - x0)
        dy = -abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        error = dx + dy
        while True:
            self.pixel(x0, y0, color)
            if x0 == x1 and y0 == y1:
                break
            twice_error = 2 * error
            if twice_error >= dy:
                error += dy
                x0 += sx
            if twice_error <= dx:
                error += dx
                y0 += sy

    def text(self, value: str, x: int, y: int, scale: int, color: Color) -> None:
        for index, character in enumerate(value.upper()):
            glyph = FONT.get(character, FONT[" "])
            for row, bits in enumerate(glyph):
                for column, bit in enumerate(bits):
                    if bit == "1":
                        self.rectangle(
                            x + index * 6 * scale + column * scale,
                            y + row * scale,
                            scale,
                            scale,
                            color,
                        )


def build_needle() -> bytes:
    canvas = Canvas(*SPRITES["needle"])
    center_x = canvas.width // 2
    canvas.line(center_x, 3, center_x, 53, COLORS["cyan_dim"])
    canvas.line(center_x, 1, center_x, 50, COLORS["cyan"])
    canvas.line(center_x, 2, center_x, 14, COLORS["white"])
    canvas.line(center_x - 1, 51, center_x + 1, 51, COLORS["red"])
    canvas.rectangle(center_x - 1, 54, 3, 5, COLORS["cyan"])
    return bytes(canvas.pixels)


def build_label(
    text: str,
    size: tuple[int, int],
    scale: int,
    color: Color,
    accent: bool = False,
) -> bytes:
    canvas = Canvas(*size)
    text_width = (len(text) * 6 - 1) * scale
    x = max(0, (canvas.width - text_width) // 2)
    text_height = 7 * scale
    y = max(0, (canvas.height - text_height) // 2)
    if accent:
        canvas.line(0, canvas.height - 2, canvas.width - 1, canvas.height - 2, COLORS["cyan_dim"])
    canvas.text(text, x, y, scale, color)
    return bytes(canvas.pixels)


def draw_pixel_gauge_preview() -> bytes:
    width, height = 768, 384
    canvas = Canvas(width, height)
    canvas.rectangle(0, 0, width, height, (7, 15, 25, 255))

    for x in range(16, width, 32):
        canvas.line(x, 0, x, height - 1, (12, 24, 37, 255))
    for y in range(16, height, 32):
        canvas.line(0, y, width - 1, y, (12, 24, 37, 255))

    canvas.text("GAMEPLAY HUD  /  DIGITAL GAUGES", 150, 35, 3, COLORS["white"])
    canvas.text("RPM", 106, 120, 6, COLORS["cyan"])
    canvas.text("6.4", 96, 190, 10, COLORS["white"])
    canvas.text("SPEED", 441, 120, 6, COLORS["cyan"])
    canvas.text("186", 420, 190, 10, COLORS["white"])
    canvas.text("KM/H", 478, 315, 4, COLORS["cyan"])
    canvas.text("GEAR", 143, 315, 4, COLORS["cyan"])

    for tick in range(16):
        x = 43 + tick * 42
        color = COLORS["red"] if tick >= 13 else COLORS["cyan_dim"]
        canvas.rectangle(x, 92, 3, 16 + (tick % 4 == 0) * 12, color)

    return bytes(canvas.pixels)


def named_sprites() -> dict[str, tuple[int, int, bytes]]:
    width, height = SPRITES["needle"]
    assets = {"HUD_Dial_Analogue_Needle.swatchbin": (width, height, build_needle())}

    for gear in GEARS:
        size = SPRITES["gear"]
        scale = 8 if len(gear) == 1 else 5
        pixels = build_label(gear, size, scale, COLORS["cyan"], accent=True)
        assets[f"HUD_Dial_Digital_Gear_{gear}.swatchbin"] = (*size, pixels)

    for number in REV_VALUES:
        size = SPRITES["rev"]
        pixels = build_label(number, size, 1, COLORS["white"])
        assets[f"HUD_Dial_RevNumbers_{number}.swatchbin"] = (*size, pixels)

    for number in DIGITS:
        size = SPRITES["speed"]
        pixels = build_label(number, size, 12, COLORS["white"], accent=True)
        assets[f"HUD_Dial_SpeedDigit_{number}.swatchbin"] = (*size, pixels)

    for suffix, label in UNITS.items():
        size = SPRITES["unit"]
        pixels = build_label(label, size, 2, COLORS["cyan"])
        assets[f"HUD_Dial_Unit_{suffix}.swatchbin"] = (*size, pixels)

    if len(assets) != 49:
        raise ValueError(f"Expected 49 shared HUD source images, got {len(assets)}.")
    return assets


def encode_png(width: int, height: int, rgba: bytes) -> bytes:
    def chunk(kind: bytes, data: bytes) -> bytes:
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body))

    stride = width * 4
    scanlines = b"".join(
        b"\x00" + rgba[row * stride : (row + 1) * stride] for row in range(height)
    )
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(scanlines, level=9))
        + chunk(b"IEND", b"")
    )


def build(output_directory: Path) -> list[Path]:
    target_directory = output_directory / "SourcePNGs" / TEXTURE_ROOT
    target_directory.mkdir(parents=True, exist_ok=True)
    written = []
    for filename, (width, height, pixels) in named_sprites().items():
        path = target_directory / f"{Path(filename).stem}.png"
        path.write_bytes(encode_png(width, height, pixels))
        written.append(path)

    preview_path = output_directory / "ForzaGaugeSwap-HUD-preview.png"
    preview = draw_pixel_gauge_preview()
    preview_path.write_bytes(encode_png(768, 384, preview))
    written.append(preview_path)
    return written


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "build" / "research-textures",
    )
    args = parser.parse_args()
    paths = build(args.output_dir)
    print(f"Created {len(paths) - 1} source PNGs for the shared HUD textures.")
    print(f"Created preview: {paths[-1]}")


if __name__ == "__main__":
    main()
