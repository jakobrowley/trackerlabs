# TrackerLabs

> 2D motion tracker plugin for Adobe After Effects and Blackmagic DaVinci Resolve. Sold by [Tiny Tapes](https://tinytapes.ca/).

This repository contains the **full source code** for all four TrackerLabs plugin variants, the installer scripts that ship to customers, and the documentation needed to maintain and extend the plugins.

## If you're a future Claude Code agent: start here

Read these in order, then come back:

1. **[CLAUDE.md](CLAUDE.md)** — agent-targeted technical brief. What you can/can't safely modify, the tech stack per variant, common pitfalls. The most important file in this repo.
2. **[SECURITY.md](SECURITY.md)** — license/auth model, known threat scope.
3. **[SENTRY_INTEGRATION.md](SENTRY_INTEGRATION.md)** — crash-reporting setup. Per-variant DSNs, build flags, symbol upload.
4. This README — high-level orientation, repo layout, ship recipe.

If a customer is reporting a bug: also check `~/.claude/projects/-Users-jakob-Documents-Plugins-Vladyslav/memory/MEMORY.md` on Jakob's machine for prior incidents and recurring gotchas.

## What ships to customers

A TrackerLabs purchase delivers **one license key** that activates either the After Effects plugin, the DaVinci Resolve plugin, or both. The customer downloads a single ZIP per app from a GitHub release and runs the installer for their OS.

| Variant | Platform | Output binary | Installer (in zip) |
|---|---|---|---|
| `TrackerLabs (After Effects)` | Mac | `TrackerLabs.plugin/` (bundle) | `TrackerLabs-AE-Installer.pkg` (notarized) |
| `TrackerLabs (After Effects)` | Windows | `TrackerLabs.aex` (PE32+ DLL) | `Install TrackerLabs AE (Run as Admin).bat` |
| `TrackerLabs (DaVinci Resolve)` | Mac | `TrackerLabsPlugin.ofx.bundle/` | `TrackerLabs-Resolve-Installer.pkg` (notarized) |
| `TrackerLabs (DaVinci Resolve)` | Windows | `TrackerLabsPlugin.ofx` (PE32+ DLL) | `Install TrackerLabs Resolve (Run as Admin).bat` |

The two AE plugins are unrelated codebases from the two Resolve plugins. They share **only** the license-verification module (`activation.cpp` + `httplib.h`). Everything else (rendering, parameters, GPU compute, lifecycle) is independent.

## Repo layout

```
.
├── CLAUDE.md                              # Agent brief (read first)
├── SECURITY.md                            # License/auth threat model
├── SENTRY_INTEGRATION.md                  # Crash reporting setup
├── README.md                              # This file
├── .github/workflows/                     # CI builds (Windows currently)
│
├── TrackerLabs_AEX_MAC/                   # AE plugin source — Mac (Xcode)
│   └── TrackerLabs/
│       ├── src/                           # C++ sources, Objective-C++, Sentry hooks
│       ├── *.xcodeproj                    # Xcode project
│       └── installer/                     # Mac .pkg build inputs
│
├── TrackerLabs_AEX_WIN/                   # AE plugin source — Windows (VS)
│   └── TrackerLabs/
│       ├── src/                           # C++ sources (mirrors Mac)
│       ├── *.sln / *.vcxproj              # Visual Studio project
│       └── installer/
│           └── Install TrackerLabs AE (Run as Admin).bat
│
├── TrackerLabsResolvePlugin_Mac/          # Resolve OFX source — Mac (Xcode)
│   └── TrackerLabsResolvePlugin/
│       ├── src/                           # C++ + OFX SDK + Metal/OpenCL kernels
│       ├── installer/                     # Mac .pkg build + payload layout
│       └── Support/, OpenFX-1.4/, opencl/ # Vendored SDK / kernels
│
└── TrackerLabsResolvePlugin_Win/          # Resolve OFX source — Windows (VS)
    └── TrackerLabsResolvePlugin/
        ├── src/                           # C++ + CUDA + OpenCL kernels
        ├── *.sln / *.vcxproj
        └── installer/
            └── Install TrackerLabs Resolve (Run as Admin).bat
```

## License system (current — Railway-backed)

Both the AE and Resolve plugins phone home to the Tiny Tapes license server hosted on Railway:

```
Plugin (any variant)
   ├── On Register/Activate:
   │     POST/GET https://fxbuddy-production-eccd.up.railway.app/api/tt/lmfwc/v2/licenses/activate/<KEY>
   │
   └── On periodic re-validation:
         POST/GET https://fxbuddy-production-eccd.up.railway.app/api/tt/lmfwc/v2/licenses/validate/<KEY>
```

License key format (TrackerLabs only): `TRACK[A-Z0-9]{16}TT` — exactly **23 characters**, no dashes. The plugin's client-side regex enforces this *before* hitting the network — wrong-length keys produce `Invalid license key format` without contacting the server.

License storage paths after activation:

| Platform | Path |
|---|---|
| macOS | `~/Library/Application Support/com.tracker.lic` |
| macOS (legacy) | `~/Library/Application Support/EditLab/plugins/dt.txt` |
| Windows | `%APPDATA%\com.tracker.lic` |

> **Historical note:** Earlier versions (≤ v1.00) called `tinytapes.com/wp-json/lmfwc/v2/...` — the previous developer's domain, which is no longer responsive. Any v1.00 install on a customer machine will silently fail to activate. The Apr 2026 rewrite (`activation.cpp`/`activation.h`) cut over to Railway.

## Where the customer-facing files live

The bundled, signed, notarized customer downloads ship as **release assets** on this repo:

- Tag: `v1.0.0-new-license`
- Assets:
  - `TrackerLabs-AfterEffects.zip` — Mac `.pkg` + Win `.bat` + plugin files + READ ME FIRST.txt
  - `TrackerLabs-DaVinciResolve.zip` — same structure for Resolve

The Tiny Tapes backend (`fxbuddy_backend`) serves these via [`/api/tt/download/trackerlabs-ae`](https://fxbuddy-production-eccd.up.railway.app/api/tt/download/trackerlabs-ae) and [`/api/tt/download/trackerlabs-resolve`](https://fxbuddy-production-eccd.up.railway.app/api/tt/download/trackerlabs-resolve), which 302-redirect to the GitHub release zip URL. URLs are configured in `FALLBACK_DOWNLOAD_URLS` in `src/routes/tinytapes-admin-dashboard.ts` of the backend repo.

## Building

### Mac (both AE and Resolve)

Open the relevant `.xcodeproj` in Xcode. Each project has a release configuration that:

1. Compiles `arm64 + x86_64` universal Mach-O
2. Code-signs with `Developer ID Application: Jakob Rowley (TFQ3XTTVJC)`
3. Notarizes via `xcrun notarytool` (you need an app-specific password for `tinytapesappledev@gmail.com`)
4. Staples the notarization ticket
5. Wraps into the appropriate bundle structure

The Resolve `.pkg` install payload lives at `TrackerLabsResolvePlugin_Mac/TrackerLabsResolvePlugin/installer/Payload/Library/OFX/Plugins/`. The AE `.pkg` payload puts `TrackerLabs.plugin/` into `/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/`.

### Windows (both AE and Resolve)

Open the `.sln` in Visual Studio (2022 recommended, Windows 10 SDK 10.0.20348.0+).

- Resolve plugin requires CUDA Toolkit 12.x for the `.cu` kernel.
- AE plugin requires Adobe AE 25.6 SDK headers (vendored in `Support/`).

Build the `Release | x64` config. Output is the `.aex` or `.ofx` binary. There's no Windows code signing currently — Mark-of-the-Web is stripped at install time by the `.bat` script using `Unblock-File`.

The CI workflow at `.github/workflows/build-windows.yml` builds Windows binaries on every push.

## Shipping a new version (recipe)

1. **Build** all four binaries (Mac `.plugin`/`.ofx.bundle`, Win `.aex`/`.ofx`).
2. **Notarize** the Mac binaries (Apple Developer Program — `tinytapesappledev@gmail.com`).
3. **Wrap Mac binaries in `.pkg` installers** using the `installer/` folder layouts.
4. **Stage the customer zip layouts** matching the existing structure:
   ```
   TrackerLabs-AfterEffects/
     READ ME FIRST.txt
     Mac/TrackerLabs-AE-Installer.pkg
     Windows/Install TrackerLabs AE (Run as Admin).bat
     Windows/plugin_files/TrackerLabs.aex
   TrackerLabs-DaVinciResolve/
     READ ME FIRST.txt
     Mac/TrackerLabs-Resolve-Installer.pkg
     Windows/Install TrackerLabs Resolve (Run as Admin).bat
     Windows/plugin_files/Contents/Win64/TrackerLabsPlugin.ofx
     Windows/plugin_files/Contents/Resources/TrackerLabs.png
   ```
5. **Zip each top-level folder.**
6. **Upload to the GitHub release**, replacing the existing assets:
   ```
   gh release upload v1.0.0-new-license -R jakobrowley/trackerlabs --clobber \
     TrackerLabs-AfterEffects.zip TrackerLabs-DaVinciResolve.zip
   ```
7. The Tiny Tapes backend already points at this release tag — customers get the new build on their next download.
8. Existing customers don't auto-update. To force an update, the backend has a `/admin/send-update` endpoint that emails a notification.

## Common gotchas (the ones that will burn you)

### Resolve caches plugin failures aggressively
DaVinci Resolve maintains an OFX plugin cache at:
- macOS: `~/Library/Application Support/Blackmagic Design/DaVinci Resolve/OFXPluginCacheV2.xml`
- Windows: `%APPDATA%\Blackmagic Design\DaVinci Resolve\OFXPluginCacheV2.xml`

If a plugin fails to load once (cached as `status="2"`), Resolve will silently skip it on every subsequent launch — even if you fix the binary. The Windows `.bat` installer wipes this cache automatically. On Mac, you may need to delete it by hand if a customer reports "I installed but TrackerLabs doesn't appear in OpenFX."

### `auth.mm` uses the MotionBlur bundle identifier
The Mac Resolve plugin's `auth.mm` file looks up `CFBundleGetBundleWithIdentifier(CFSTR("com.adobe.AfterEffects.MotionBlur"))` — copy-paste leftover from the MotionBlur plugin. This returns NULL inside an OFX context (no AE bundle exists), so `getResourcePath()` and `getModelResourcePath()` return empty strings. In practice this is non-fatal at registration time, but fix it if you're cleaning up: change the identifier to one that won't be looked up at all (or remove the lookups entirely if those resource paths aren't actually needed).

### Mac notarization requires an app-specific password
The team identifier is `TFQ3XTTVJC`. The active Apple Developer account is `tinytapesappledev@gmail.com`. The `jakobrowley@hotmail.com` account's Developer Program expired 2026-05-01.

### License key format collision risk
The plugin's regex `TRACK[A-Z0-9]{16}TT` is 23 chars. The backend's `generateLicenseKey('TRACK')` produces exactly this format (4 groups of 4 random crypto bytes, no separator). If you ever change one without changing the other, every new customer's key will be silently rejected client-side with `Invalid license key format`.

### Windows `.bat` installers self-elevate
Don't strip the `NET SESSION` admin check at the top of the `.bat` files — it's what makes them work for users who double-click instead of right-clicking → Run as administrator. Without it, the install silently fails for any user who misses that step.

### `__MACOSX/` cruft from Mac zips
When zipping on macOS, hidden `__MACOSX/` folders get included. The `.gitignore` excludes them but be careful when staging release zips — strip them or use `zip -X` to avoid Windows showing them as junk folders.

## Support contact

Customer support: `tinytapeshelp@gmail.com`. Owner / decision-maker: Jakob (`jakob@tinytapes.ca`).
