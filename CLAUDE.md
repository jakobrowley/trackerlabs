# CLAUDE.md — TrackerLabs

> **For Claude Code:** This file is loaded automatically when you open any session in this folder. Always also load `~/Plugins/_shared/CUSTOMER_BUG_INVESTIGATION.md` for the bug-fix mental model.
>
> **For Claud (the support agent):** This is your reference for everything about TrackerLabs. **Heads up: this is the most technically complex plugin in the catalog and the one most likely to require escalation. Don't be discouraged.**

---

## What TrackerLabs Is

TrackerLabs is a **2D motion tracker plugin** that draws stylized visual elements (circles, lines, labels, connection nodes) attached to tracked points in a video clip. It's used for surveillance / cyberpunk / sci-fi looks where the editor wants to highlight people, objects, or movement with animated tracker overlays.

It exists in **two completely separate codebases**:

- **TrackerLabs (After Effects)** — a C++ effect plugin (`.aex`), CPU rendering only
- **TrackerLabs (DaVinci Resolve)** — an OFX 1.4 C++ plugin (`.ofx`), with full GPU compute (Metal on Mac, CUDA on Windows, OpenCL fallback)

The two versions share **only** the license verification code (`activation.cpp`, `httplib.h`). Everything else — the rendering engine, the parameter system, the entry points, the GPU code — is separate. **Most bugs will be version-specific.**

---

## ⚠️ READ THIS BEFORE TOUCHING ANYTHING ⚠️

TrackerLabs is **mostly C++ and GPU compute**. For Claud, this means:

**Files Claud should NEVER modify:**
- Any `.cpp`, `.h`, `.hpp` file (C++)
- Any `.cu` file (CUDA — NVIDIA GPU code)
- Any `.metal` or `.mm` file (Apple Metal GPU code)
- Any `.cl` file (OpenCL GPU code)
- Anything in `OpenFX-1.4/`, `Support/`, or `opencl/` (vendored SDKs)
- The Xcode `.xcodeproj` or Visual Studio `.sln`/`.vcxproj` files

**This means: for ~95% of TrackerLabs bugs, the answer is "escalate to Jakob."** That's not failure — that's the system working correctly. TrackerLabs is hard, and escalation is the right call for hard things.

**What Claud CAN do for TrackerLabs:**
1. Triage the customer's report
2. Check Sentry for any matching crashes (TrackerLabs has Crashpad enabled, so C++ crashes from real customers will appear there with symbolicated stack traces)
3. Run `/escalate` with all the gathered context, so Jakob can pick up the work in 5 minutes
4. Communicate with the customer professionally throughout

---

## Technology Stack (verified by source inspection)

### TrackerLabs (After Effects)

| Component | Technology |
|---|---|
| **Plugin type** | Adobe AE C++ effect plugin (PF_Effect, NOT AEGP) |
| **SDK** | Adobe After Effects SDK (headers in `Headers/`, includes `AE_Effect.h`, `PF_EffectCB.h`, `AEFX_SuiteHelper.h`) |
| **Language** | C++ |
| **Build system (Mac)** | Xcode 3.2+ (`TrackerLabs.xcodeproj`) |
| **Build system (Windows)** | Visual Studio 2019 toolset v142 (`TrackerLabs.sln`) |
| **Output** | `.aex` Mach-O bundle (Mac, universal x86_64+arm64) or DLL-style binary (Windows) |
| **Output install path (Win)** | `C:\Program Files\Adobe\Common\Plug-ins\7.0\MediaCore\TrackerLabs\TrackerLabs.aex` |
| **GPU compute** | **None.** OpenCL.framework is linked but unused. CPU rendering only. |
| **Frameworks** | Cocoa.framework (Mac UI alerts), Direct3D / DirectXUtils.h (Windows GPU primitives — legacy, mostly unused) |
| **Total source size** | ~15,531 lines C++ |
| **Match name** | `TrackerLabs_v3_Restore` (defined in `TrackerLabs.h:14`) |
| **Plugin entry** | `EffectMain()` declared in `TrackerLabs.h:61`, defined in `TrackerLabs.cpp:1469` |

### TrackerLabs (DaVinci Resolve)

| Component | Technology |
|---|---|
| **Plugin type** | **OFX 1.4 C++ plugin — VERIFIED** |
| **SDK** | OpenFX 1.4 (headers in `OpenFX-1.4/include/`) + OFX C++ Support Library (`Support/Library/`) |
| **Language** | C++ + Objective-C++ (Metal kernel) + CUDA (NVIDIA) + OpenCL |
| **Build system (Mac)** | Xcode (`TrackerLabsResolvePlugin.xcodeproj`) |
| **Build system (Windows)** | Visual Studio (`TrackerLabsResolvePlugin.sln`) |
| **Output** | `.ofx` bundle (Mach-O 64-bit on Mac, ~1.0MB universal binary) |
| **GPU compute** | **Metal (Mac, primary), CUDA (Windows), OpenCL (fallback)** |
| **Frameworks** | Metal.framework, OpenCL.framework, Foundation.framework |
| **Total source size** | ~17,961 lines |
| **Plugin entry** | OFX factory pattern via `OFX::PluginFactoryHelper` constructor at `TrackerLabsResolvePlugin.cpp:1583` |
| **Plugin class** | `class TrackerLabsResolvePlugin : public OFX::ImageEffect` at `TrackerLabsResolvePlugin.cpp:1201` |
| **Factory class** | `class TrackerLabsResolvePluginFactory : public OFX::PluginFactoryHelper<TrackerLabsResolvePluginFactory>` at `TrackerLabsResolvePlugin.h:5` |

---

## OFX Verification Proof (for the Resolve version)

For the record, here's the definitive proof that TrackerLabs Resolve is an OFX 1.4 plugin:

1. **Header include:** `TrackerLabsResolvePlugin.h:3` → `#include "ofxsImageEffect.h"`
2. **Factory class:** `TrackerLabsResolvePlugin.h:5` → `class TrackerLabsResolvePluginFactory : public OFX::PluginFactoryHelper<TrackerLabsResolvePluginFactory>`
3. **Image effect class:** `TrackerLabsResolvePlugin.cpp:1201` → `class TrackerLabsResolvePlugin : public OFX::ImageEffect`
4. **Factory constructor:** `TrackerLabsResolvePlugin.cpp:1583` → `OFX::PluginFactoryHelper(...)`
5. **OFX entry methods:**
   - `describe()` at line 1587
   - `describeInContext()` at line 1628
   - `createInstance()` at line 1988
6. **OFX SDK headers** present at `OpenFX-1.4/include/`: `ofxCore.h`, `ofxImageEffect.h`, `ofxParam.h`, `ofxMultiThread.h`, etc. (all standard OFX 1.4 API headers, copyright 2003-2015)
7. **OFX C++ Support Library** present at `Support/Library/ofxsImageEffect.cpp`
8. **Compiled output:** `TrackerLabsPlugin.ofx` is a Mach-O 64-bit bundle (1.0MB universal binary, x86_64 + arm64)

**Conclusion:** OFX 1.4 plugin using the OFX::ImageEffect C++ wrapper. `OFX.md` (in `~/Plugins/_shared/api-reference/`) is the correct primary reference.

---

## File Structure

### TrackerLabs (After Effects) — `TrackerLabs_AEX_MAC/TrackerLabs/`

```
TrackerLabs/
├── src/
│   ├── TrackerLabs.cpp              # 🔧 1,498 lines — main effect logic
│   ├── TrackerLabs.h                # Header with 59 parameter enum definitions
│   ├── TrackerLabsPiPL.r            # PiPL resource (plugin metadata)
│   ├── activation.cpp & .h          # ⚠️ OFF-LIMITS — license verification (shared with Resolve)
│   ├── bundle.h & .mm               # Objective-C++ bundle utilities
│   ├── httplib.h                    # ⚠️ OFF-LIMITS — HTTP client for license server
│   └── Win/                         # Windows-specific build files
│       ├── TrackerLabs.sln
│       └── TrackerLabs.vcxproj
├── Util/                            # AE framework utilities
│   ├── AEFX_SuiteHelper.cpp/.h
│   ├── AEGP_SuiteHandler.cpp/.h
│   ├── Param_Utils.h                # Parameter macros
│   ├── DirectXUtils.cpp/.h          # Windows GPU primitives (legacy)
│   └── [12 other AE framework headers]
├── Headers/                         # Adobe AE SDK headers (44 files)
│   ├── AE_Effect.h
│   ├── AE_EffectCB.h
│   └── [...]
└── Resources/
    ├── TrackerLabs.plugin-Info.plist
    └── Mach-O_prefix.h
```

### TrackerLabs (Resolve) — `TrackerLabsResolvePlugin_Mac/TrackerLabsResolvePlugin/`

```
TrackerLabsResolvePlugin/
├── src/
│   ├── TrackerLabsResolvePlugin.cpp        # 🔧 1,997 lines — main OFX plugin
│   ├── TrackerLabsResolvePlugin.h          # Factory class declaration
│   ├── TrackerLabsResolvePlugin.xcodeproj  # Mac build
│   ├── TrackerLabsResolvePlugin.sln        # Windows build
│   ├── Header.h                            # Parameter macro definitions
│   ├── MetalKernel.mm                      # 🔥 141 lines — Mac GPU + critical workaround
│   ├── CudaKernel.cu                       # 🔥 353 lines — Windows GPU (CUDA)
│   ├── TrackerLabsResolvePluginCLKernel.cl # 🔥 342 lines — OpenCL fallback (color spaces)
│   ├── OpenCLKernel.cpp                    # OpenCL host code
│   ├── activation.cpp & .h                 # ⚠️ OFF-LIMITS — shared license code
│   ├── Rijndael.cpp & .h                   # ⚠️ OFF-LIMITS — AES encryption
│   ├── auth.h & auth.mm                    # ⚠️ OFF-LIMITS — authentication helpers
│   ├── httplib.h                           # ⚠️ OFF-LIMITS — HTTP client
│   ├── TrackerLabsPlugin.ofx               # Compiled binary (1.0MB universal)
│   └── TrackerLabsPlugin.ofx.bundle/Contents/
│       ├── MacOS/TrackerLabsPlugin.ofx     # Executable
│       └── Resources/Info.plist
├── OpenFX-1.4/include/                     # OFX SDK headers (vendored)
│   ├── ofxCore.h
│   ├── ofxImageEffect.h
│   ├── ofxParam.h
│   └── [...]
├── Support/Library/                        # OFX C++ Support library (vendored)
│   └── ofxsImageEffect.cpp                 # Contains an OFX HACK (see Gotchas)
├── opencl/inc/CL/                          # OpenCL SDK headers (vendored)
└── installer/                              # Deployment scripts
```

---

## Architecture

### TrackerLabs (After Effects)

**Plugin entry:** `EffectMain()` function. Adobe's effect plugin model uses a single entry point that receives a command (`PF_Cmd_GLOBAL_SETUP`, `PF_Cmd_PARAMS_SETUP`, `PF_Cmd_RENDER`, `PF_Cmd_USER_CHANGED_PARAM`, etc.) and handles each one.

**Parameter system:** 59 parameters defined as an enum in `TrackerLabs.h`, instantiated as `PF_ParamDef` structures during `PF_Cmd_PARAMS_SETUP`.

Major parameter categories:
- Tracking (master intensity, target color, tolerance, tracking speed)
- Visual elements (lines enabled, node size, line thickness, etc.)
- Connections (connection mode, line style)
- License (license key input field)

**Drawing functions** (CPU-based, in `TrackerLabs.cpp`):
- `DrawFilledCircle()` (line 114)
- `DrawCircleOutline()` (line 130)
- `DrawLine()` (line 166)
- `DrawCurvedLine()` (line 191)
- `PlotPixel()` and `PlotPixelAlpha()` — handle ARGB32 (8-bit), ARGB64 (16-bit), and ARGB128 (float) pixel formats

**Activation gate:** A global flag `bool g_IsActivated = false;` (line 8) is checked at the top of every render function. If false, render returns early and the user sees nothing.

### TrackerLabs (Resolve)

**Plugin entry:** OFX factory pattern. The OFX host (Resolve) discovers plugins via the OFX bundle format and calls these methods on `TrackerLabsResolvePluginFactory`:

- `describe()` — declare plugin metadata (name, version, supported contexts)
- `describeInContext()` — declare parameters and clips
- `createInstance()` — instantiate the actual `TrackerLabsResolvePlugin` object for each effect on each clip

**Per-instance class:** `TrackerLabsResolvePlugin` (subclass of `OFX::ImageEffect`) handles per-instance state and the `render()` call.

**ImageScaler class:** Custom subclass of `OFX::ImageProcessor` (line 30) that implements the actual pixel processing:

```cpp
class ImageScaler : public OFX::ImageProcessor {
    virtual void processImagesCUDA();      // Windows NVIDIA GPU
    virtual void processImagesOpenCL();    // Cross-platform fallback
    virtual void processImagesMetal();     // Mac GPU
    virtual void multiThreadProcessImages(OfxRectI p_ProcWindow);  // CPU fallback
    // Drawing methods...
};
```

**Render dispatch:** OFX host (Resolve) calls `render()`, which selects the appropriate GPU backend based on platform and what Resolve hands the plugin. Mac → Metal. Windows → CUDA. Fallback → OpenCL or CPU.

---

## ⚠️ The Metal Private-Buffer Bug (critical, documented in source)

This is a **known Resolve quirk** that's already worked around in the source. If a customer reports a crash when applying TrackerLabs to clips in Resolve on Mac, this is almost certainly the cause — and the fix already exists, so the issue is most likely a regression.

**Source comment** at `MetalKernel.mm:5-6`:
> "Resolve often hands plugins MTLBuffers with MTLStorageModePrivate. [buffer contents] is nil; memcpy then crashes."

**Workaround** at `MetalKernel.mm:15-28`: A staging buffer with mutex protection (`gStagingMutex`) and on-demand resizing.

**If you see Metal crashes in Sentry from TrackerLabs Resolve:**
1. Check if the customer is on Mac with Resolve 19+
2. Read the `MetalKernel.mm` workaround code
3. Verify the staging buffer logic hasn't been broken
4. **Then escalate to Jakob** — this is C++/Metal code and not Claud's territory

---

## ⚠️ OFF-LIMITS FILES — NEVER MODIFY

**Effectively ALL of TrackerLabs is off-limits to Claud.** The plugins are pure C++ with GPU compute kernels. Specifically:

| File / area | Why |
|---|---|
| Every `.cpp`, `.h`, `.hpp` file | C++ — not safe for non-developer modification |
| Every `.cu` file | CUDA GPU code — extremely specialized |
| Every `.mm` file | Objective-C++ / Metal GPU code |
| Every `.cl` file | OpenCL GPU code |
| `activation.cpp` / `activation.h` | Shared license verification code |
| `httplib.h` | HTTP client for license server |
| `Rijndael.cpp` / `Rijndael.h` | AES encryption (Resolve version) |
| `auth.h` / `auth.mm` | Authentication helpers (Resolve version) |
| `OpenFX-1.4/` | Vendored OFX SDK |
| `Support/Library/` | Vendored OFX C++ Support library |
| `opencl/` | Vendored OpenCL SDK |
| `Headers/` | Vendored Adobe AE SDK |
| `Util/` | AE framework utilities (vendored) |
| Any `.xcodeproj`, `.sln`, `.vcxproj`, Makefile | Build system files |

**The escalation tripwire is: any change to any file in this folder → escalate.** TrackerLabs requires C++ expertise, GPU knowledge, and access to a working Adobe AE SDK / OFX SDK build environment. Claud doesn't have those, and trying to fix C++/GPU bugs without them is dangerous.

---

## Common Bug Categories (and how to escalate them properly)

### 1. Crashes (most common)

The customer applies TrackerLabs and Premiere/AE/Resolve crashes.

**What Claud does:**
1. Run `/sentry-check <customer-email>` → Crashpad should have captured a minidump with a symbolicated stack trace (assuming Component L instrumentation is in place)
2. Note the file/line of the crash
3. Note the platform (Mac/Win), host app, plugin version
4. Run `/escalate` with the Sentry event ID and the crash location

**The escalation handoff is what matters here, not the fix.** Make Jakob's life as easy as possible by collecting:
- The Sentry minidump link
- The customer's exact host app and plugin version
- A description of what they were doing when it crashed
- Whether it's reproducible on demand or random

### 2. Tracker drift / inaccurate tracking

The tracker doesn't follow the subject correctly.

**What Claud does:**
1. Get a Loom/Jam recording showing the bug
2. Get the customer's project + footage via WeTransfer (Layer 3)
3. Try to reproduce locally (this DOES work for non-C++ debugging — the source compiles to a `.aex`/`.ofx` Claud can install)
4. **Don't try to fix the tracking algorithm** — escalate with the reproduction steps

### 3. Visual elements rendering wrong (lines, circles, labels)

The tracker overlays render but look wrong (colors off, lines broken, labels missing).

**Where to look** (read-only, for context — DON'T modify):
- AE: drawing functions in `TrackerLabs.cpp:114-191`
- Resolve: drawing functions inside `ImageScaler` class
- For color issues on Resolve: `TrackerLabsResolvePluginCLKernel.cl` handles color space conversions (ACEScct, ACEScc, DaVinciIntermediate, LogC, Log3G10, Cineon)

**Then escalate.**

### 4. GPU-specific issues (Resolve only)

Symptoms: works fine on Windows but broken on Mac, or vice versa. Or works fine on integrated GPU but crashes on discrete.

**What this almost certainly is:**
- Mac → Metal private-buffer issue (see above)
- Windows → CUDA driver compatibility issue
- Either platform → OpenCL fallback edge case

**Escalate immediately** — these are platform-specific GPU bugs that need hands-on debugging.

### 5. License activation bugs

**Always escalate.**

---

## Build Pipeline

### TrackerLabs (After Effects) — Mac

```bash
# Open the Xcode project
open src/TrackerLabs.xcodeproj
# Build via Xcode UI: Cmd+B
# Output: build/Release/TrackerLabs.plugin (Mach-O bundle)
```

**Frameworks linked:** Cocoa.framework, OpenCL.framework
**Code signing:** Configured in Xcode preferences (not in `.xcodeproj`)
**Output:** Universal binary (x86_64 + arm64)

### TrackerLabs (After Effects) — Windows

```bash
# Open in Visual Studio
src/Win/TrackerLabs.sln
# Build via VS UI
# Output: $(AE_PLUGIN_BUILD_DIR)\TrackerLabs.aex
```

**Requires:** `AE_PLUGIN_BUILD_DIR` environment variable set
**Default output path:** `C:\Program Files\Adobe\Common\Plug-ins\7.0\MediaCore\TrackerLabs\TrackerLabs.aex`
**Toolset:** v142 (VS 2019)
**Configurations:** x64 and Win32, Debug and Release

### TrackerLabs (Resolve) — Mac

```bash
open src/TrackerLabsResolvePlugin.xcodeproj
# Build → produces TrackerLabsPlugin.ofx bundle
```

**Frameworks:** Metal.framework, OpenCL.framework, Foundation.framework
**Output:** Universal binary

### TrackerLabs (Resolve) — Windows

```bash
# Open src/TrackerLabsResolvePlugin.sln in VS
# Build → produces TrackerLabsPlugin.ofx
```

**Requires:** NVIDIA CUDA toolkit (for `CudaKernel.cu`), OpenCL SDK
**Toolset:** v142 or later

**Note:** All of these builds require specialized toolchains that Claud may not have. **Building TrackerLabs is escalation territory by itself.**

---

## API References

```
~/Plugins/_shared/api-reference/AE_CPP.md   # After Effects C++ SDK Guide
~/Plugins/_shared/api-reference/OFX.md      # OpenFX 1.4 plugin SDK programming guide
```

These are the reference docs. Claud probably won't use them directly — they're for Jakob (or a contract C++ dev) when working on escalated TrackerLabs tickets.

`AE.md` (the ExtendScript API reference) is **NOT relevant** for TrackerLabs because TrackerLabs has no scripting layer — it's pure C++ effect code.

---

## Diagnostic Tools

### Sentry + Crashpad (the only practical telemetry for TrackerLabs)

Once Component L instrumentation is in place, Crashpad runs alongside the plugin and captures any C++ crash as a minidump. Sentry's Native SDK uploads the minidump and symbolicates it into a readable stack trace.

**This is the ONLY way to debug TrackerLabs without screenshare.** Without it, "TrackerLabs crashed" is unactionable. With it, you sometimes get the file/line of the crash + a stack trace + variable values at crash time.

**Run `/sentry-check <customer-email>` first, every time, on every TrackerLabs ticket.**

### Local debug builds

For Jakob (or a contract dev) working on an escalated ticket:
- Mac: Xcode → Debug build → attach to After Effects or Resolve → set breakpoints
- Windows: Visual Studio → Debug build → attach to After Effects or Resolve → set breakpoints

Claud is not expected to do this.

### GPU debugging (Resolve only)

For Metal issues on Mac: Xcode's Metal debugger
For CUDA issues on Windows: NVIDIA Nsight
For OpenCL issues: GPU vendor tools

All of this is escalation territory.

---

## Hard Rules (no exceptions)

1. **Default to escalation for TrackerLabs.** This is the one plugin where "I don't know, let's escalate" is the most common correct answer.
2. **NEVER modify any C++/CUDA/Metal/OpenCL file.** Period.
3. **NEVER attempt to build TrackerLabs without Jakob's guidance** — the toolchains are specialized.
4. **ALWAYS run `/sentry-check` first** — Crashpad is your best friend here.
5. **Reproduce locally only if no source changes are needed** — i.e., to confirm the bug exists before escalating with a clean repro.
6. **One-customer beta is mandatory** for any change Jakob ships through Claud.
7. **Run `/learn` at ticket close** — even on escalated tickets, log what was learned.

---

## Known Tech Debt / Gotchas

1. **`g_IsActivated` is in-memory only** — resets on plugin reload. If license verification silently fails, customers see "TrackerLabs does nothing" with no error message.
2. **Metal private-buffer workaround** is mutex-protected — any threading bug here causes Mac crashes.
3. **CUDA kernel commented out in AE version** — `//RunCudaKernel(...)` at `TrackerLabsResolvePlugin.cpp:129`. AE version is CPU-only as a result.
4. **httplib.h has TODO comments** about Brotli encoding parsing being incomplete and needing refactoring.
5. **OFX Support library has a HACK comment** in `Support/Library/ofxsImageEffect.cpp` ("HACK need to throw something to cause a failure") — exception handling is incomplete.
6. **String handling for license keys** — `strip_zero_width(license)` is called at `TrackerLabs.cpp:1413-1414` to remove invisible Unicode characters. Suggests there's been past abuse via copy-paste of license keys with hidden chars.
7. **No platform-independent CMake build** — Mac and Windows have completely separate project files with different conventions.
8. **The PiPL resource file** (`TrackerLabsPiPL.r`) is an old Mac/AE format that few people understand. Treat it as untouchable.
9. **Plugin install path on Windows is hardcoded** to `C:\Program Files\Adobe\Common\Plug-ins\7.0\MediaCore\TrackerLabs\` — won't work for users with non-standard installs.

---

## Escalation Tripwires (auto-escalate immediately — basically everything)

For TrackerLabs, the escalation tripwires are extremely broad. **Run `/escalate` and STOP if any of these apply:**

- Any C++ code change is needed (`.cpp`, `.h`, `.hpp`, `.mm`, `.cu`, `.metal`, `.cl`)
- The build needs to run (Xcode or Visual Studio)
- Anything related to GPU compute (Metal, CUDA, OpenCL)
- Any crash from a Crashpad minidump
- The bug involves the OFX SDK, the AE SDK, or any vendored library
- The bug involves license activation, validation, or `tinytapes.com`
- The bug involves code signing or notarization
- The customer reports the plugin "does nothing" or "isn't activated" (license issue)

**Effectively the only TrackerLabs tickets Claud handles end-to-end are:**
- Customer questions about how to use the plugin (no code involved)
- Customer asking for documentation
- Customer asking about supported versions
- Triage and information collection before escalating

That's OK. This is by design. TrackerLabs is the C++ ceiling.
