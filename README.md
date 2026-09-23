# MEGAsyncX

MEGAsyncX is a personal, non-commercial fork of the MEGA Desktop Application for **Windows x64**.
It keeps the sync engine untouched and changes what the app does around it: several accounts in one
install, no upgrade nagging, no telemetry, no auto-updater, and no Explorer integration.

It installs alongside official MEGAsync rather than replacing it. The binary is `MEGAsyncX.exe`, the
settings live in `%LOCALAPPDATA%\Mega Limited\MEGAsyncX`, and the shell pipe is renamed, so neither
app can read or clobber the other's configuration.

The upstream MEGA Limited Code Licence still applies. See [LICENCE.md](LICENCE.md), and
[CREDITS.md](CREDITS.md) for third-party libraries.

## What this fork adds

**Multiple accounts, one live session.** Save up to 500 MEGA accounts and switch between them from
the tray menu or Settings → Account. Only one account is ever logged in at a time: switching stores
the current session, pauses its syncs, and restarts the app against the target account. Running two
simultaneous sessions would put the account at risk under MEGA's terms, so the fork does not do it.

The switcher asks for confirmation before every switch and before adding an account, refuses
duplicate emails, and shows the state in the tray tooltip:

```
MEGAsyncX 6.6.1
you@example.com (3/500)
Up to date
```

The third line is the sync state, not an update check. The tray submenu lists the active account
first and then up to ten more alphabetically, with **All accounts…** opening the full list.
Settings → Account has a filter box for finding one email among many.

**Import and export.** Settings → Account has **Import from File** and **Export**, and the tray
submenu has **Import from file** next to **Add account…**. The file format is one account per line:

```
alice@example.com:password1
bob@example.com:password2
```

Blank lines and lines starting with `#` are ignored. The password is everything after the first
colon, so colons inside passwords are safe. Importing does not log in to anything: it records the
credentials and adds the emails to your list. The first time you switch to an imported account, the
app restarts and signs in with the stored password. Re-importing an email that is already in the
list just updates its password. Import reports how many entries it added, updated, and skipped, and
tells you if the 500 limit stopped it.

Export writes the same format for every account that has a stored password, after a confirmation
dialog. Accounts you added by logging in normally have a session but no stored password, so they
are skipped and the count is reported.

> **The import and export files are plain text and are not encrypted.** Anyone who reads an
> exported file gets full access to every account in it. Passwords you import are stored in the
> app's settings under Windows DPAPI, which protects them against other Windows users but not
> against anything running as you. Delete exported files when you are done with them.

**Forget all.** Settings → Account has a **Forget all** button that deletes every stored session
and every stored password except the active account's, behind a confirmation dialog. The emails
stay in the list and ask for a password on the next switch. Nothing is deleted unless you press it.

**Recovering from a dead session.** When MEGA invalidates a session from another device, the app
keeps the email in your list, drops only that one session, and returns you to the login screen with
the address already filled in.

## What this fork removes

**Upgrade nagging.** All of it, at every surface:

- the "You're running out of storage space" panel with Dismiss and Buy more space
- the "Storage almost full" banner in Transfer Manager
- the "Your MEGA account is nearly full" banner in Settings
- the "Your account is almost full — Get Pro" desktop notification
- the upsell plan dialog and the discount campaign state machine

The genuinely informational full-storage states are kept, because "uploads are disabled" is a fact
you need rather than an advert.

**Telemetry.** Crash reports are never uploaded and the statistics event handler is a no-op.
Breakpad is disabled in the CI build.

**The auto-updater.** Automatic updates default to off, the update task is inert, and the updater
tool is excluded from the build. Nothing contacts MEGA's update server, so the app never replaces
itself behind your back. Updating means building a new binary yourself.

**Explorer integration.** The context menu extension and the sync overlay icons are forced off and
excluded from the build, so no shell extension DLL is ever loaded into Explorer.

**HTTP server and Stream from MEGA.** The embedded HTTP server and the streaming entry points are
stubbed out.

## Reported version

The app writes a version into its User-Agent. `reported-version.txt`, placed next to the exe or in
the data directory, overrides it:

```
6.7.2
```

That makes the app report `6.7.2` and version code `60702`. See
[reported-version.txt.example](reported-version.txt.example). This is cosmetic. It changes the
string the app sends, nothing about how it syncs.

## Building

Windows x64 only, with Qt 5.15.2 MSVC 2019 64-bit, MSVC v143, and vcpkg. Full local instructions
are in [HANDOVER.md](HANDOVER.md); the upstream Windows notes are still in
[README.win.md](README.win.md).

There is also a GitHub Actions workflow at `.github/workflows/windows.yml` that produces an
unsigned `MEGAsyncX-windows-x64` artifact. The first run takes around two and a half hours because
vcpkg builds every dependency from source; later runs reuse the cache.

Because the binary is unsigned, SmartScreen will warn on first launch.

# MEGA Desktop Application

Easy automated syncing and backup between your computers and your MEGA cloud drive.
This repository contains all the development history of the official MEGA Desktop Application.

MEGA Desktop is an installable application that synchronises folders between your computer and
your MEGA Cloud Drive. All files and subfolders will be replicated in both directions.
Changes that you make on your device will also be made on the MEGA Cloud Drive. Similarly,
changes made in your MEGA Cloud Drive (such as renaming, moving and deleting) will also be
made to the synced folders on your device.

https://mega.io/syncing

# Supported Platforms

The minimum version of Windows required at the moment is Windows 10.
The minimum version of Windows Server required at the moment is Windows Server 2019.
The minimum version of macOS required at the moment is macOS Catalina 10.15. 

We officially support a handful of Linux flavors based on Debian and RedHat, such as:
- Debian
- Ubuntu
- Mint
- Fedora
- CentOS
