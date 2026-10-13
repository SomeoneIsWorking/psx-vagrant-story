# Prior art — Vagrant Story (USA `SLUS_010.40`, SHA-1 `fababcfd4325d42f350d95b3472874affeb0e48c`)

Other work on this executable and what this repo may take from it; `docs/references.md` owns the detail on the vendored decomp. A borrowed address is a lead until measured on our image.

| project | URL | kind | license | target build | build match (addresses checked) | what we may take | notes |
|---|---|---|---|---|---|---|---|
| rood-reverse | https://github.com/ser-pounce/rood-reverse | matching decomp (splat/objdiff), vendored at `external/rood-reverse` pin `f3072188` | CC0-1.0 | `SLUS_010.40` plus overlays; upstream `SLUS_010.40/splat.yaml` `sha1: fababcfd…` | matched: SHA-1 of the resident image and all 21 configs at the pin (`docs/references.md`); `SetDefDrawEnv` `0x8002B374`, `vs_main_initHeap` `0x80043F74`, `VSync` `0x8001F6C4` are Ghidra function entries on our image (2026-10-13); `SetGeomOffset` `0x80041540`, `SetGeomScreen` `0x80041534`, `SetDefDrawEnv`, `SetDefDispEnv` `0x8002B434`, `vs_main_projectionDistance` `0x8005E248` equal the addresses in `CLAUDE.md` (`0x80041540` has no Ghidra-defined function) | vendor under license containment (CC0: code and ideas) | active, last push 2026-10-12; decomp.dev 50.12%; upstream has 89 splat configs (EFFECT, SMALL) against 21 at our pin |
| vstools | https://github.com/morris/vstools | asset/model viewer (JS) | MIT | disc files, no executable addresses | not checkable: names no addresses | evidence only (format knowledge) | last push 2022-12-30 |
| GodHands | https://github.com/collinsmichael/GodHands | modding tool | MIT | disc files | not checkable: no addresses checked | evidence only | last push 2020-02-22 |
| VSViewer, Vagrant-Story-Unity-Parser, Vagrant-Story-Noesis-Python-Plugin, blender-vagrant-story | https://github.com/MercurialForge/VSViewer, https://github.com/korobetski | format viewers/importers | none (all rights reserved) | disc files | not checkable | evidence only | no license file |
| vagrantstory_recomp | https://github.com/riotashley81-jpg/vagrantstory_recomp | static recompilation via psxrecomp (R.A.I.D.) | none | `SLUS-01040` per its README | not checked | nothing | created 2026-10-12; no license; R.A.I.D. community is not engaged (user decision 2026-08-12, `megamanx4/docs/references.md`) |
| VagrantStoryRandomizer, VagrantStoryImporter | https://github.com/ChaoticBrave/VagrantStoryRandomizer, https://github.com/scurest/VagrantStoryImporter | randomizer; Blender model importer | none | disc files | not checked | evidence only | |
| Data Crystal | https://datacrystal.tcrf.net/wiki/Vagrant_Story | wiki | CC BY-SA (wiki) | RAM map subpage covers prototype builds | not checkable: no `SLUS_010.40` addresses | names only | credited by rood-reverse |

Not found (2026-10-13): another decompilation or symbol map for `SLUS_010.40` on GitHub, GitLab, Codeberg or decomp.dev; a PC port other than the psxrecomp build; a `SLUS_010.40` RAM map.
