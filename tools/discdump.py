#!/usr/bin/env python3
"""discdump.py — locate (and, if needed, build) the framework's `discdump` tool, and read a disc.

`discdump` is psxport's ISO9660/CHD reader (external/psxport/tools/discdump.cpp); all disc reads go
through it. psxport's launcher builds it into build/player-tools; $PSXPORT_DISCDUMP overrides
with a prebuilt binary. Listings are never cached.
"""

import os
import subprocess
import sys
from pathlib import Path

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def psxport_dir():
    d = os.environ.get("PSXPORT_DIR") or os.path.join(ROOT, "external", "psxport")
    if not os.path.isabs(d):
        d = os.path.join(ROOT, d)
    return d


def find():
    """Return an absolute path to a usable `discdump`, or raise SystemExit(2) saying why not."""
    override = os.environ.get("PSXPORT_DISCDUMP")
    if override:
        if not os.access(override, os.X_OK):
            print(
                f"[discdump] $PSXPORT_DISCDUMP={override} is not executable",
                file=sys.stderr,
            )
            raise SystemExit(2)
        return os.path.abspath(override)

    px = psxport_dir()
    if not os.path.isfile(os.path.join(px, "tools", "port", "launcher.py")):
        print(
            f"[discdump] PSXPORT_DIR={px} is not a psxport checkout; set PSXPORT_DIR.",
            file=sys.stderr,
        )
        raise SystemExit(2)
    sys.path.insert(0, os.path.join(px, "tools"))
    from automation.process import ToolError
    from port.launcher import build_discdump

    try:
        return str(build_discdump(Path(ROOT), os.environ))
    except ToolError as error:
        print(f"[discdump] {error}", file=sys.stderr)
        raise SystemExit(2) from error


def listing(disc, dd=None):
    """Every file on the disc, as a list of (path, lba, size). Raises SystemExit(2) on a bad read."""
    dd = dd or find()
    out = subprocess.run(
        [dd, "list", disc], capture_output=True, text=True, check=False
    )
    if out.returncode != 0:
        print(
            f"[discdump] list failed on {disc}:\n{out.stdout}{out.stderr}",
            file=sys.stderr,
        )
        raise SystemExit(2)
    # `discdump list` already prints each file with its full path; do not prepend the header directory.
    files = []
    for line in out.stdout.splitlines():
        s = line.strip()
        if (
            not s
            or s.startswith(("disc:", "root dir"))
            or (s.endswith("/") and " " not in s)
        ):
            continue
        parts = s.split()
        if len(parts) >= 5 and parts[1] == "LBA":
            files.append((parts[0], int(parts[2]), int(parts[3])))
    if not files:
        print(
            f"[discdump] list produced ZERO files for {disc} — refusing to report an empty disc "
            "as a successful read",
            file=sys.stderr,
        )
        raise SystemExit(2)
    return files


def get(disc, path_on_disc, outdir, dd=None):
    """Extract one file. `path_on_disc` uses forward slashes ('BATTLE/BATTLE.PRG'), exactly as
    `discdump list` prints it — the backslash form does NOT resolve. Returns the written path."""
    dd = dd or find()
    os.makedirs(outdir, exist_ok=True)
    out = subprocess.run(
        [dd, "get", path_on_disc, disc, outdir],
        capture_output=True,
        text=True,
        check=False,
    )
    dest = os.path.join(outdir, os.path.basename(path_on_disc))
    if out.returncode != 0 or not os.path.isfile(dest):
        print(
            f"[discdump] get {path_on_disc} failed:\n{out.stdout}{out.stderr}",
            file=sys.stderr,
        )
        return None
    return dest
