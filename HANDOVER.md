# Compile this Windows MEGAsync fork on Apple M4 Pro (24 GB)

This PC has no Visual Studio. Build on the M4 in a **Windows 11 ARM** VM (Parallels or VMware Fusion). Output is **x64** `MEGAsyncX.exe` (ARM Windows runs it emulated). Do **not** build native ARM64 and do **not** compile this app on macOS. Data dir is `%LOCALAPPDATA%\Mega Limited\MEGAsyncX` so official MEGAsync never shares config.

Licence: MEGA Limited Code Licence, non-commercial. Keep `LICENCE.md`. Do not publish a GitHub fork unless you decide to.

## 1. VM size (24 GB Mac — do this first)

Give Windows enough RAM or vcpkg will OOM.

| Resource | Set to | Leave for macOS |
| --- | --- | --- |
| RAM | **12 GB** (max 14 GB) | 10–12 GB |
| CPU | 6 cores | rest |
| Disk | **80 GB** minimum (100 GB safer) | — |
| Display | 2+ GB VRAM if Parallels asks | — |

vcpkg first configure: **1–3 hours**. Keep `VCPKG_MAX_CONCURRENCY=2` so 12 GB RAM holds.

Copy **this whole tree** into the VM (includes `src/MEGASync/mega` SDK). USB, SMB, or zip. Do not clone stock `meganz/MEGAsync` or you lose these patches.

Suggested Windows path: `C:\mega\MegaSync`

## 2. Software to install (Windows VM)

Install in this order. All **x64** unless noted.

1. **Git for Windows** — https://git-scm.com/download/win  
   Checkout as-is, commit as-is. Add Git to PATH.

2. **Python 3.12+ x64** — https://www.python.org/downloads/windows/  
   Tick “Add python.exe to PATH”. vcpkg needs it.

3. **CMake 4.2 or newer** (Windows x64 installer) — https://cmake.org/download/  
   Add CMake to PATH. Project minimum is 3.18, but **Visual Studio 18 2026** needs CMake ≥ 4.2. Do not rely only on the CMake bundled in VS.

4. **Visual Studio 2026 Community** (or 2022 if the installer still offers it)  
   https://visualstudio.microsoft.com/vs/community/  
   Workload: **Desktop development with C++**.  
   Individual components:
   - MSVC **v143** (x64/x86 build tools) — MEGAsync hard-codes `-T v143`
   - Windows 11 SDK (10.0.22621 or 10.0.26100)
   - C++ CMake tools for Windows (optional)
   Put the product on a drive with **≥ 40 GB free**. Installer engine stays on `C:`.

5. **Qt 5.15 MSVC 64-bit** — https://www.qt.io/download-qt-installer  
   Qt account required. In Maintenance Tool open **Archive** and install **Qt 5.15.2** → **MSVC 2019 64-bit** (`msvc2019_64`).  
   Path will look like `C:\Qt\5.15.2\msvc2019_64`.  
   Do **not** install ARM64 Qt. Do not use Qt 6.

6. **vcpkg** (clone, do not install a random binary):

```bat
mkdir C:\mega
cd C:\mega
git clone https://github.com/microsoft/vcpkg
cd vcpkg
bootstrap-vcpkg.bat -disableMetrics
```

No extra NASM/Perl unless you build Qt from source (skip that).

## 3. Configure + compile (x64)

Developer PowerShell or “x64 Native Tools” is optional. Plain PowerShell is enough if `cmake` and `git` are on PATH.

```bat
cd C:\mega\MegaSync

set VCPKG_MAX_CONCURRENCY=2
set CMAKE_BUILD_PARALLEL_LEVEL=2

cmake -G "Visual Studio 18 2026" -A x64 -T v143 ^
  -DCMAKE_PREFIX_PATH=C:\Qt\5.15.2\msvc2019_64 ^
  -DVCPKG_ROOT=C:\mega\vcpkg ^
  -DVCPKG_TARGET_TRIPLET=x64-windows-mega ^
  -DENABLE_EXPLORER_EXT=OFF ^
  -DENABLE_DESKTOP_UPDATER=OFF ^
  -DENABLE_DESKTOP_UPDATE_GEN=OFF ^
  -DENABLE_DESKTOP_APP_TESTS=OFF ^
  -DENABLE_DESIGN_TOKENS_IMPORTER=OFF ^
  -S C:\mega\MegaSync ^
  -B C:\mega\build-x64
```

First `cmake` **builds third-party libs**. If RAM dies, close browsers and rerun the same command (vcpkg resumes).

If CMake says generator `Visual Studio 18 2026` unknown: upgrade CMake to 4.2+ **or** install VS 2022 and use `-G "Visual Studio 17 2022" -A x64 -T v143`.

Build only the app:

```bat
cmake --build C:\mega\build-x64 --config RelWithDebInfo --target MEGAsync -j 2
```

Exe:

`C:\mega\build-x64\src\MEGASync\RelWithDebInfo\MEGAsyncX.exe`

(Debug: `--config Debug` → `...\Debug\MEGAsyncX.exe`)

Do **not** pass `-A ARM64`. `-A x64` on Windows 11 ARM is correct; Prism/x64 emulation runs the exe.

## 4. What this fork already changed (no extra flags needed)

Defaults in `cmake/modules/desktopapp_options.cmake`: Explorer ext OFF, updater OFF, update generator OFF.

Runtime:

- One process, one MegaApi session, **max 500 saved accounts**
- Tray **Accounts** submenu + Settings → Account list (Switch / Add / Remove)
- Switch = save session, restart, FastLogin. Other accounts’ syncs stay idle
- Add account = keep sessions, restart into login (cancel + quit restores previous account next launch)
- No HTTPServer, Stream, Upgrade/upsell/discount, overlay icons, Explorer context menu, updater UI

## 5. After first launch

1. Log in account 1, set a sync, wait until green.
2. Settings → Account → **Add account** (app restarts to login).
3. Log in account 2. Confirm only account 2 syncs.
4. Tray → Accounts → switch back to account 1.
5. Cap: fifth add works, sixth shows “Maximum of 5”.
6. Second `MEGAsyncX.exe` does not start (lock is in the MEGAsyncX data dir). Official MEGAsync can still run; different folder.

Time budget: toolchain install ~1–2 h; vcpkg ~1–3 h; MEGAsync link ~15–40 min.

## 6. If it fails

| Symptom | Fix |
| --- | --- |
| C: disk full during VS | Move VS install cache + shared to D:/VM disk; need ~40 GB |
| `v143` toolset missing | VS Installer → Individual components → MSVC v143 |
| Qt not found | `CMAKE_PREFIX_PATH` must be the `msvc2019_64` folder, not Qt root |
| Triplet `arm64-windows-mega` | You used `-A ARM64`. Reconfigure with `-A x64` and a **new** `-B` dir |
| Linker OOM / cl.exe killed | `VCPKG_MAX_CONCURRENCY=1` and `-j 1`; VM RAM 14 GB |
| CMake generator 18 unknown | CMake 4.2+ or VS 2022 generator 17 |
| Submodule empty `src/MEGASync/mega` | Copy failed. Need the SDK files; `git submodule update --init src/MEGASync/mega` only if this repo’s `.git` came along |

## 7. Reported version (no rebuild)

Encoding is `major * 10000 + minor * 100 + micro`. **6.6.1 → 60601. 6.7.2 → 60702.** User-Agent becomes `MEGAsync/6.7.2.0`.

Copy `reported-version.txt.example` to **one** of:

1. Next to `MEGAsyncX.exe` as `reported-version.txt` (this wins)
2. `%LOCALAPPDATA%\Mega Limited\MEGAsyncX\reported-version.txt`

Put one line: `6.7.2`. Restart. Check GitHub tags: https://github.com/meganz/MEGAsync/releases

Spoof is camouflage only. It does not add MEGA’s new SDK code. If a future MEGA server **requires** a new protocol, bumping the number will not save you — you would need to merge/rebase this tree.

Updater thread is **off**. Official `v.txt` cannot overwrite this build.

## 8. SmartScreen (unsigned exe)

MEGA’s installer is signed with **their** Authenticode cert. You cannot use it. A local VS build is unsigned. Windows will show “Windows protected your PC”.

Do this on the VM (personal use):

1. First run: **More info → Run anyway**. Warns again when the **file hash** changes (every rebuild).
2. Unblock after copy: `Unblock-File .\MEGAsyncX.exe` (Zone.Identifier from zip/USB).
3. Optional self-signed cert (same PC, quieter after you trust the cert):

```bat
New-SelfSignedCertificate -Type CodeSigningCert -Subject "CN=MEGAsync local" -CertStoreLocation Cert:\CurrentUser\My
```

Then `signtool sign /a /fd SHA256 MEGAsyncX.exe` from a VS Developer Prompt. SmartScreen still nags until reputation exists; importing your cert into **Trusted Publishers** on that Windows user reduces the prompt.

4. Paid Authenticode (DigiCert/Sectigo, hundreds USD/year) is the only way to look like a “normal” downloaded app. Not worth it for a private fork.

SmartScreen is **local Windows**, not MEGA’s API.

Do not `git push` this fork unless you explicitly ask to.

## 9. GitHub Actions (private repo MEGAsyncX)

Workflow: `.github/workflows/windows.yml` on `windows-2022`.
First run: 1–3 h (vcpkg). Later runs use GitHub cache.
Download: Actions → Windows → artifact `MEGAsyncX-windows-x64`.
Trigger: push to `master` or Actions → Run workflow.
