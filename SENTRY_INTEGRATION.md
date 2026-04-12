# TrackerLabs — Sentry Native + Crashpad Integration Guide

> **Scope:** This document describes how to add crash telemetry to TrackerLabs' C++ effect plugins (both AE and Resolve versions). Unlike the other three plugins (FastFX, Surveillance, Terminal) which use JavaScript-based Sentry loader scripts, TrackerLabs is pure C++ and requires a different approach: the **Sentry Native SDK** with the **Crashpad backend**.
>
> **Who can apply this:** A C++ developer with Xcode (Mac) + Visual Studio (Windows) + CMake experience. Claud cannot apply this — it's escalation territory by design. This document exists so a C++ dev (or Jakob in Xcode) can apply the integration in an afternoon without starting from scratch.
>
> **Why this matters:** Without crash reporting, a TrackerLabs crash on a customer's machine is completely invisible to us. The customer just sees "Adobe After Effects quit unexpectedly" and shrugs. With Crashpad + Sentry Native, we get a symbolicated stack trace pointing to the exact file/line of the crash — often enough to diagnose without ever talking to the customer.

## Architecture

```
┌──────────────────────────────┐
│ Customer's After Effects     │
│                              │
│  ┌────────────────────────┐  │
│  │ TrackerLabs.aex        │  │
│  │                        │  │
│  │ On startup:            │  │
│  │   sentry_options_new() │  │
│  │   sentry_init()        │  │
│  │                        │  │
│  │ On crash:              │  │
│  │   Crashpad catches     │  │
│  │   signal → writes      │  │
│  │   minidump to disk     │  │
│  └────────────────────────┘  │
│            │                 │
│            ▼                 │
│  ┌────────────────────────┐  │
│  │ crashpad_handler.exe   │  │
│  │ (ships with plugin)    │  │
│  └────────────────────────┘  │
└──────────────┬───────────────┘
               │ HTTPS
               ▼
┌──────────────────────────────┐
│ Sentry.io                    │
│ Symbolicates via dSYM/PDB    │
│ Shows readable stack trace   │
└──────────────────────────────┘
```

## Prerequisites

### DSNs (one per TrackerLabs variant)

| Plugin variant | Sentry project | DSN |
|---|---|---|
| TrackerLabs (After Effects) | `trackerlabs-ae` | `https://3775579fa0a1835e3e9b75b1eeb5a33e@o4511201510096896.ingest.us.sentry.io/4511201528840192` |
| TrackerLabs (DaVinci Resolve) | `trackerlabs-resolve` | `https://077e4ecb288fef45e443a8c6bca8e21f@o4511201510096896.ingest.us.sentry.io/4511201531330560` |

### Sentry CLI (for uploading debug symbols)

```bash
brew install getsentry/tools/sentry-cli      # Mac
# or: https://docs.sentry.io/cli/installation/
```

Authenticate with the org token already stored in `~/Plugins/_shared/sentry-secrets.md`:

```bash
export SENTRY_AUTH_TOKEN="sntrys_..."
export SENTRY_ORG="tiny-tapes"
```

## Integration Steps

### Step 1 — Download the Sentry Native SDK

Sentry Native is distributed as source. The easiest path is via CMake FetchContent or by cloning the release tarball.

```bash
cd /tmp
curl -L https://github.com/getsentry/sentry-native/releases/download/0.7.15/sentry-native.zip -o sentry-native.zip
unzip sentry-native.zip -d sentry-native
```

Copy `sentry-native/` into each TrackerLabs variant's source tree as a vendored dependency (or set up as a git submodule — the vendored approach is simpler for a small team).

Target paths:
- `TrackerLabs_AEX_MAC/TrackerLabs/third_party/sentry-native/`
- `TrackerLabs_AEX_WIN/TrackerLabs/third_party/sentry-native/`
- `TrackerLabsResolvePlugin_Mac/TrackerLabsResolvePlugin/third_party/sentry-native/`
- `TrackerLabsResolvePlugin_Win/TrackerLabsResolvePlugin/third_party/sentry-native/`

### Step 2 — Build the Sentry Native SDK with Crashpad backend

```bash
cd third_party/sentry-native
cmake -B build -DSENTRY_BACKEND=crashpad -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=./install
cmake --build build --target install --config RelWithDebInfo
```

This produces:
- `./install/lib/libsentry.a` (static lib to link into the plugin)
- `./install/bin/crashpad_handler` (executable that must ship alongside the plugin)
- `./install/include/sentry.h` (header for the plugin code)

### Step 3 — Add a new source file: `sentry_integration.cpp`

Create `src/sentry_integration.cpp` in each TrackerLabs variant's source tree:

```cpp
#include "sentry_integration.h"
#include <sentry.h>
#include <string>

#ifdef __APPLE__
#include <dlfcn.h>
#include <libgen.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
  bool g_sentry_initialized = false;

  // Returns the directory containing THIS PLUGIN's binary, not the host app's
  // (After Effects / DaVinci Resolve). This is critical because Crashpad needs
  // to find `crashpad_handler` in the same directory as the plugin, not next
  // to the host app.
  //
  // Note: this was originally written with _NSGetExecutablePath / GetModuleFileNameA(NULL, ...)
  // which is wrong — those return the HOST app's path. Sentry Seer caught this
  // during PR review. The correct approach uses dladdr (Mac) and
  // GetModuleHandleEx with GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS (Win),
  // passing a function pointer from our own module so the OS resolves to our
  // own DLL/dylib instead of the host process's executable.
  std::string get_plugin_dir() {
#ifdef __APPLE__
    // dladdr looks up the shared library that contains the given symbol.
    // Passing a pointer to a function in our own plugin resolves to our
    // plugin's dylib path, not the host app.
    Dl_info info;
    if (dladdr(reinterpret_cast<void*>(&get_plugin_dir), &info) != 0 && info.dli_fname) {
      // Make a mutable copy because dirname() may modify its argument.
      char path_copy[1024];
      strncpy(path_copy, info.dli_fname, sizeof(path_copy) - 1);
      path_copy[sizeof(path_copy) - 1] = '\0';
      return std::string(dirname(path_copy));
    }
    return "/tmp";
#elif defined(_WIN32)
    // GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS with a function pointer from
    // THIS module resolves to our plugin's DLL, not the host's .exe.
    // GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT prevents ref-count changes.
    HMODULE hModule = NULL;
    if (GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&get_plugin_dir),
            &hModule)) {
      wchar_t wpath[MAX_PATH];
      if (GetModuleFileNameW(hModule, wpath, MAX_PATH) > 0) {
        // Convert wide string to narrow — simplified; in production use WideCharToMultiByte
        std::wstring ws(wpath);
        std::string path(ws.begin(), ws.end());
        return path.substr(0, path.find_last_of("\\/"));
      }
    }
    return "C:\\Temp";
#else
    return "/tmp";
#endif
  }
}

void InitSentry(const char* plugin_variant) {
  if (g_sentry_initialized) return;
  g_sentry_initialized = true;

  sentry_options_t* options = sentry_options_new();

  // DSN — use the right one per variant. Hard-coded for simplicity;
  // could be pulled from a build-time constant.
  if (strcmp(plugin_variant, "ae") == 0) {
    sentry_options_set_dsn(options,
      "https://3775579fa0a1835e3e9b75b1eeb5a33e@o4511201510096896.ingest.us.sentry.io/4511201528840192");
    sentry_options_set_release(options, "trackerlabs-ae@3.0.0");
  } else {
    sentry_options_set_dsn(options,
      "https://077e4ecb288fef45e443a8c6bca8e21f@o4511201510096896.ingest.us.sentry.io/4511201531330560");
    sentry_options_set_release(options, "trackerlabs-resolve@3.0.0");
  }

  sentry_options_set_environment(options, "production");

  // Where Crashpad writes minidumps. Must be a writable directory.
  std::string db_path = get_plugin_dir() + "/.trackerlabs-sentry-db";
  sentry_options_set_database_path(options, db_path.c_str());

  // Where crashpad_handler lives. Must be the same directory as the plugin.
  std::string handler_path = get_plugin_dir() + "/crashpad_handler";
#ifdef _WIN32
  handler_path += ".exe";
#endif
  sentry_options_set_handler_path(options, handler_path.c_str());

  // Strip IPs and other PII by default. Customer explicit opt-in is a
  // separate concern — see the EULA for telemetry disclosure.
  sentry_options_set_require_user_consent(options, 0);

  // Default tags — every event will carry these.
  sentry_options_set_symbolize_stacktraces(options, 1);

  sentry_init(options);

  // Set tags on the global scope so every event carries them.
  sentry_set_tag("plugin", strcmp(plugin_variant, "ae") == 0 ? "trackerlabs-ae" : "trackerlabs-resolve");
  sentry_set_tag("host_app", strcmp(plugin_variant, "ae") == 0 ? "ae" : "resolve");
}

void ShutdownSentry() {
  if (!g_sentry_initialized) return;
  sentry_close();
  g_sentry_initialized = false;
}

void CaptureSentryMessage(const char* message, int level) {
  if (!g_sentry_initialized) return;
  sentry_level_t sentry_level = SENTRY_LEVEL_ERROR;
  switch (level) {
    case 0: sentry_level = SENTRY_LEVEL_DEBUG; break;
    case 1: sentry_level = SENTRY_LEVEL_INFO; break;
    case 2: sentry_level = SENTRY_LEVEL_WARNING; break;
    case 3: sentry_level = SENTRY_LEVEL_ERROR; break;
    case 4: sentry_level = SENTRY_LEVEL_FATAL; break;
  }
  sentry_capture_event(sentry_value_new_message_event(sentry_level, "default", message));
}
```

And the header `src/sentry_integration.h`:

```cpp
#pragma once

/**
 * Initialize Sentry error telemetry with Crashpad backend.
 * Call ONCE at plugin startup (PF_Cmd_GLOBAL_SETUP for AE, describe() for OFX).
 *
 * @param plugin_variant "ae" or "resolve" — determines which DSN to use.
 */
void InitSentry(const char* plugin_variant);

/**
 * Flush and shut down Sentry. Call at plugin unload.
 */
void ShutdownSentry();

/**
 * Report a non-fatal message/error to Sentry with a severity level.
 * @param level 0=debug 1=info 2=warning 3=error 4=fatal
 */
void CaptureSentryMessage(const char* message, int level);
```

### Step 4 — Wire Sentry into the plugin entry points

**For TrackerLabs AE** — edit `TrackerLabs.cpp:EffectMain()`:

```cpp
#include "sentry_integration.h"

PF_Err EffectMain(PF_Cmd cmd, PF_InData* in_data, PF_OutData* out_data,
                  PF_ParamDef* params[], PF_LayerDef* output, void* extra) {
    PF_Err err = PF_Err_NONE;

    switch (cmd) {
        case PF_Cmd_GLOBAL_SETUP:
            InitSentry("ae");
            // ... existing setup code
            break;

        case PF_Cmd_GLOBAL_SETDOWN:
            ShutdownSentry();
            // ... existing teardown code
            break;

        // ... other cases unchanged
    }

    return err;
}
```

**For TrackerLabs Resolve** — edit `TrackerLabsResolvePlugin.cpp` factory:

```cpp
#include "sentry_integration.h"

void TrackerLabsResolvePluginFactory::load() {
    InitSentry("resolve");
    // ... existing load code
}

void TrackerLabsResolvePluginFactory::unload() {
    ShutdownSentry();
    // ... existing unload code
}
```

### Step 5 — Update Xcode / Visual Studio projects

**Xcode (Mac):**

1. Add `src/sentry_integration.cpp` and `src/sentry_integration.h` to the target
2. Build Settings → Header Search Paths → add `$(SRCROOT)/third_party/sentry-native/install/include`
3. Build Settings → Library Search Paths → add `$(SRCROOT)/third_party/sentry-native/install/lib`
4. Build Phases → Link Binary With Libraries → add `libsentry.a`
5. Build Phases → Copy Files → add new phase "Copy Sentry Crashpad Handler" → destination: Wrapper, add `crashpad_handler` from the install dir
6. Build Phases → Run Script (new) → add:
   ```bash
   # Upload debug symbols to Sentry after successful build
   if [ "${CONFIGURATION}" == "Release" ] && [ -n "${SENTRY_AUTH_TOKEN}" ]; then
       sentry-cli upload-dif --org tiny-tapes --project trackerlabs-ae \
           "${BUILT_PRODUCTS_DIR}/${WRAPPER_NAME}"
   fi
   ```

**Visual Studio (Windows):**

1. Add `sentry_integration.cpp` and `sentry_integration.h` to the project
2. Properties → C/C++ → General → Additional Include Directories → add `third_party\sentry-native\install\include`
3. Properties → Linker → General → Additional Library Directories → add `third_party\sentry-native\install\lib`
4. Properties → Linker → Input → Additional Dependencies → add `sentry.lib`
5. Properties → Build Events → Post-Build Event → add:
   ```
   copy "$(ProjectDir)third_party\sentry-native\install\bin\crashpad_handler.exe" "$(OutDir)"
   ```
6. Post-build symbol upload:
   ```
   if exist "%SENTRY_AUTH_TOKEN%" sentry-cli upload-dif --org tiny-tapes --project trackerlabs-ae "$(TargetPath)"
   ```

### Step 6 — Ship the crashpad_handler alongside the plugin

The `crashpad_handler` executable MUST be in the same directory as `TrackerLabs.aex` / `TrackerLabsPlugin.ofx` for Crashpad to work. The build steps above handle this.

**Distribution:**
- Mac: `crashpad_handler` (~500KB binary) goes inside the `.aex` / `.ofx` bundle's Contents folder
- Windows: `crashpad_handler.exe` goes in the same folder as `TrackerLabs.aex`

Update the installer to include these.

### Step 7 — Upload debug symbols (dSYM / PDB)

For Sentry to show readable stack traces, the debug symbols must be uploaded after each release build:

```bash
# Mac — upload dSYM
sentry-cli upload-dif --org tiny-tapes --project trackerlabs-ae \
    build/Release/TrackerLabs.plugin

# Windows — upload PDB
sentry-cli upload-dif --org tiny-tapes --project trackerlabs-ae \
    x64/Release/TrackerLabs.aex x64/Release/TrackerLabs.pdb
```

Do the same for `trackerlabs-resolve` with the Resolve variant.

These upload steps are baked into the Xcode/VS post-build phases above so they happen automatically on release builds (as long as `SENTRY_AUTH_TOKEN` is set).

### Step 8 — Test it works

1. Build a Debug variant of the plugin with Sentry enabled
2. Install into AE (or Resolve)
3. Manually trigger a crash — e.g., add a line `*((int*)0) = 42;` somewhere in the render path temporarily
4. Apply the effect → AE/Resolve crashes
5. Restart AE/Resolve → Crashpad should upload the minidump on next launch
6. Check https://tiny-tapes.sentry.io/projects/trackerlabs-ae/ for the event
7. Remove the intentional crash, rebuild

## Gotchas

1. **Metal kernel crashes on Mac** — the Metal private-buffer workaround in `MetalKernel.mm:5-28` may or may not be caught by Crashpad depending on how Metal surfaces the fault. Test specifically for this case after integration.

2. **CUDA crashes on Windows** — CUDA errors may not produce a crash; they return error codes. Use `CaptureSentryMessage()` explicitly in CUDA error paths to report them.

3. **OFX host catches signals** — Resolve may install its own signal handlers that compete with Crashpad. Initialize Sentry as early as possible in the plugin load sequence (before any OFX suites are used) so Crashpad's handler installs first.

4. **Notarization on Mac** — Including `crashpad_handler` in the plugin bundle means it must be signed and notarized along with the plugin. If you're not currently notarizing, you'll need to start.

5. **Debug symbols must match** — if the `.dSYM` or `.pdb` uploaded to Sentry doesn't exactly match the shipped binary, stack traces will be gibberish. The Xcode/VS post-build steps make sure they match.

## What this integration does NOT do

- Does not catch crashes in **shared libraries** outside the plugin (e.g., if Metal itself crashes)
- Does not catch crashes that happen BEFORE `InitSentry()` is called (plugin load failures won't be reported)
- Does not capture CUDA error codes — those are returned, not raised as signals. Use `CaptureSentryMessage()` explicitly.
- Does not add user context (no customer email on crashes) — to add later, call `sentry_set_user()` after license verification succeeds, using the same hash pattern as FastFX

## Estimated effort

For a C++ dev familiar with Xcode + VS + CMake: **6-8 hours** to apply this end-to-end across all 4 TrackerLabs variants (AE Mac, AE Win, Resolve Mac, Resolve Win), including testing and first successful crash report appearing in Sentry.

For someone unfamiliar with the Adobe AE SDK or OFX build: add another 4-8 hours to orient.

## Followup: Sentry user context

Once the basic crash reporting is working, add this to the license success handler in `activation.cpp`:

```cpp
#include <sentry.h>

void OnLicenseVerified(const std::string& email, const std::string& license_key) {
    // ... existing license state updates

    // Hash the license key so we never send the raw value
    std::string hashed = "lic_" + std::to_string(std::hash<std::string>{}(license_key));

    sentry_value_t user = sentry_value_new_object();
    sentry_value_set_by_key(user, "email", sentry_value_new_string(email.c_str()));
    sentry_value_set_by_key(user, "id", sentry_value_new_string(hashed.c_str()));
    sentry_set_user(user);
}
```

This unblocks the `/sentry-check <customer-email>` workflow for TrackerLabs — Claud can search crashes by customer email once this is wired up.

## References

- Sentry Native SDK docs: https://docs.sentry.io/platforms/native/
- Crashpad backend docs: https://docs.sentry.io/platforms/native/configuration/backends/crashpad/
- After Effects C++ SDK Guide (in the API reference library): `~/Plugins/_shared/api-reference/AE_CPP.md`
- OpenFX 1.4 Programming Guide: `~/Plugins/_shared/api-reference/OFX.md`
