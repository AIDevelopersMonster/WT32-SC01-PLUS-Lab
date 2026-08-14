from __future__ import annotations

import hashlib
from dataclasses import dataclass
from pathlib import Path

from openocd import OpenOcdClient


KNOWN_FACTORY_FLASH_SHA256 = (
    "3772c1bf7d6d2b713973212ddf5c671e3c844a13a8464f675343d9aed4e7f044"
)


@dataclass(frozen=True)
class Signature:
    name: str
    address: int
    expected: bytes


@dataclass(frozen=True)
class SignatureResult:
    signature: Signature
    actual: bytes

    @property
    def matched(self) -> bool:
        return self.actual == self.signature.expected


SIGNATURES = (
    Signature(
        "factory-request-callsite",
        0x420068F9,
        bytes.fromhex(
            "a2 a0 ff e5 24 09 1c 4a 81 a8 e7 e0 08 00 0c 0b ad 01 25 2a 09"
        ),
    ),
    Signature(
        "factory-entry-gate",
        0x4200690E,
        bytes.fromhex(
            "82 a0 ff 16 ba 03 92 01 00 87 99 35 a1 9d e7 0c 12 65 41 4b"
        ),
    ),
)


def verify_target(client: OpenOcdClient) -> list[SignatureResult]:
    results: list[SignatureResult] = []
    for signature in SIGNATURES:
        actual = client.read_bytes(signature.address, len(signature.expected))
        results.append(SignatureResult(signature, actual))
    return results


def target_matches(results: list[SignatureResult]) -> bool:
    return bool(results) and all(result.matched for result in results)


def sha256_file(path: str | Path) -> str:
    digest = hashlib.sha256()
    with Path(path).open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()
