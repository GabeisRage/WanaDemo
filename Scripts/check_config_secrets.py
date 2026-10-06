#!/usr/bin/env python3
"""Fail if Config ini files contain strings that look like provider API keys.

Matches sk- and xai- (any case). Unreal's project SecurityToken is ignored
because it does not use those prefixes.

Usage:
  python3 Scripts/check_config_secrets.py
  python3 Scripts/check_config_secrets.py --self-test
"""

from __future__ import annotations

import argparse
import sys
import tempfile
from pathlib import Path

NEEDLES = ("sk-", "xai-")


def find_hits(config_dir: Path) -> list[str]:
    hits: list[str] = []
    if not config_dir.is_dir():
        return hits
    for path in sorted(config_dir.rglob("*.ini")):
        text = path.read_text(encoding="utf-8", errors="replace")
        for number, line in enumerate(text.splitlines(), start=1):
            lowered = line.lower()
            if any(needle in lowered for needle in NEEDLES):
                hits.append(f"{path}:{number}: provider-key prefix in config")
    return hits


def self_test() -> int:
    with tempfile.TemporaryDirectory() as raw:
        root = Path(raw)
        clean = root / "clean"
        clean.mkdir()
        (clean / "DefaultEngine.ini").write_text(
            "[/Script/Engine.Engine]\nSecurityToken=ECDD16B34C81DF8950F57CAD3AC68B3D\n",
            encoding="utf-8",
        )
        dirty = root / "dirty"
        dirty.mkdir()
        (dirty / "DefaultEditor.ini").write_text(
            "OpenAIApiKey=sk-test-should-fail\n",
            encoding="utf-8",
        )
        xai = root / "xai"
        xai.mkdir()
        (xai / "User.ini").write_text("token=XAI-abc\n", encoding="utf-8")
        if find_hits(clean):
            print("self-test failed: SecurityToken was flagged", file=sys.stderr)
            return 1
        if len(find_hits(dirty)) != 1 or len(find_hits(xai)) != 1:
            print("self-test failed: sk- or xai- was not flagged", file=sys.stderr)
            return 1
    print("config secret check self-test passed")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--config", default="Config", help="Directory of ini files to scan")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    hits = find_hits(Path(args.config))
    if hits:
        print("Refusing config that looks like it contains an API key:", file=sys.stderr)
        for hit in hits:
            print(hit, file=sys.stderr)
        return 1
    print(f"config secret check passed ({args.config})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
