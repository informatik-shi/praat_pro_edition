# Praat Custom Script IDE — Windows build

Target: Windows 10/11 x64, x86-64-v1. Official Praat Makefiles are retained.
Toolchain used: MSYS2 CLANG64, Clang 22.1.8, GNU Make 4.4.1, pkg-config 3.0.7.

## Prerequisites

Install Git for Windows and [MSYS2](https://www.msys2.org/) to `C:\msys64`.
In the MSYS2 CLANG64 terminal:

```bash
pacman -Syu
# Restart the terminal if the runtime update requests it, then repeat:
pacman -Syu
pacman -S --needed make mingw-w64-clang-x86_64-clang mingw-w64-clang-x86_64-pkgconf
```

No separate editor package is required: Windows supplies Msftedit.dll
(RichEdit); the C++ runtime and Praat libraries are statically linked.

## Checkout

The custom repository is currently local, branch `custom/main`. To make a
second checkout on this machine:

```powershell
git clone --branch custom/main C:\Users\PC\Documents\job\praatrus praat-custom
cd praat-custom
```

When a remote fork is published, replace the local path with that fork URL.
Cloning official upstream alone does not include these custom changes.

## One-command build

From PowerShell at the checkout root:

```powershell
.\build-windows.ps1
# Optional complete verification:
.\build-windows.ps1 -Jobs 4 -Test
# Non-default MSYS2 path:
.\build-windows.ps1 -MsysRoot D:\msys64
```

Alternatively, from CLANG64:

```bash
cd /c/Users/PC/Documents/job/praatrus
bash scripts/build-windows.sh
bash scripts/test-debugger.sh
bash scripts/test-windows.sh
```

The underlying command is `make PRAAT_ARCH=x64v1 EXECUTABLE_FILE=Praat-custom.exe -j4`.
Result: **dist/praat-custom.exe**; intermediate executable: Praat-custom.exe.
Close an existing custom EXE before rebuilding that same file. Baseline
Praat.exe is kept separately. Logs and tool versions are in `.local-build/`.
Generated binaries and logs are ignored by Git.

See docs/debugger-architecture.md and tests/debugger/README.md for behaviour,
verification and limitations. Preserve upstream authorship and GPL notices
when distributing; provide the corresponding modified source code.
