# Third-party notices

This repository bundles the following build tools (they are not linked into the
released launcher executable):

## TinyCC 0.9.27

- Path: `tools/tcc/`
- Project: https://bellard.org/tcc/
- License: GNU Lesser General Public License, version 2.1
- Full text: `LICENSES/LGPL-2.1.txt`

The launcher is compiled with TinyCC. The resulting `NovolineLauncher.exe` links
only against Windows system DLLs; the TinyCC runtime DLL is not required at run
time.

## rcedit 2.0.0

- Path: `tools/rcedit.exe`
- Project: https://github.com/electron/rcedit
- License: MIT
- Full text: `LICENSES/MIT-rcedit.txt`

rcedit is used only at build time to embed the icon, manifest, version
information and requested execution level into the executable.
