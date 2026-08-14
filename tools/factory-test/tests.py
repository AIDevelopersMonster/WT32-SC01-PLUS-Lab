from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class FactoryStage:
    key: str
    title: str
    entry: int
    next_pc: int
    mode: str
    timeout: float
    note: str


STAGES = {
    "audio": FactoryStage(
        "audio",
        "Audio launcher",
        0x42007060,
        0x42007063,
        "async",
        5.0,
        "Launches an asynchronous MP3 task; physical playback is operator-observed.",
    ),
    "display": FactoryStage(
        "display",
        "Interactive LCD test",
        0x42007063,
        0x42007066,
        "interactive",
        600.0,
        "RGB/grayscale inspection followed by touch-advanced BLUE/GREEN/RED screens.",
    ),
    "touch": FactoryStage(
        "touch",
        "44-point touch test",
        0x42007066,
        0x42007069,
        "interactive",
        900.0,
        "Complete all 44 touch targets; the command returns only when the stage returns.",
    ),
    "io": FactoryStage(
        "io",
        "External IO fixture test",
        0x42007069,
        0x4200706C,
        "refused",
        0.0,
        "Requires the external one-hot GPIO fixture for GPIO10/11/12/13/14/21.",
    ),
    "sd": FactoryStage(
        "sd",
        "SD factory stage",
        0x4200706C,
        0x4200706F,
        "sd-write",
        120.0,
        "May create/replace /sdcard/hello.txt and /sdcard/foo.txt.",
    ),
    "usb": FactoryStage(
        "usb",
        "USB Con / USB Dis fixture test",
        0x4200706F,
        0x42007072,
        "refused",
        0.0,
        "Refused in v0.1 because the current debug transport uses GPIO19/GPIO20.",
    ),
    "ok": FactoryStage(
        "ok",
        "Final OK screen",
        0x42007072,
        0x42007075,
        "safe",
        15.0,
        "Displays the recovered green OK function in isolation.",
    ),
}


def list_stages() -> list[FactoryStage]:
    return [STAGES[key] for key in ("audio", "display", "touch", "io", "sd", "usb", "ok")]
