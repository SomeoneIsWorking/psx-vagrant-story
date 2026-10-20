#!/usr/bin/env python3
"""Provision the measured inputs, build vagrant_port, and launch the Vagrant Story native/Lightrec product."""

from __future__ import annotations

import os
import sys
from collections.abc import Sequence
from pathlib import Path

from automation.process import ToolError
from port import launcher
from port.launch_environment import player_environment

ROOT = Path(__file__).resolve().parents[1]
RESIDENT_IMAGE = Path("scratch/bin/vagrant/SLUS_010.40")
OVERLAY_DIRECTORY = Path("scratch/bin/overlays")


def launch(disc: str | None, prepare_only: bool) -> None:
    launcher.preflight()
    for tool in ("tools/extract_exe.py", "tools/extract_overlays.py"):
        launcher.run([sys.executable, tool, *([disc] if disc else [])], os.environ, cwd=ROOT)
    missing = [str(path) for path in (RESIDENT_IMAGE, OVERLAY_DIRECTORY) if not (ROOT / path).exists()]
    if missing:
        raise ToolError(f"provisioning finished but these measured inputs are still absent: {', '.join(missing)}")
    build = launcher.build_product(ROOT, "vagrant_port", os.environ)
    if prepare_only:
        launcher.say("Vagrant Story is built and ready.")
        return
    launcher.exec_product(build / "vagrant_port", [], player_environment(os.environ, product="vagrant"), ROOT)


def main(argv: Sequence[str] | None = None) -> int:
    args = launcher.argument_parser(__doc__).parse_args(argv)
    return launcher.main(lambda: launch(args.disc, args.prepare_only))


if __name__ == "__main__":
    raise SystemExit(main())
