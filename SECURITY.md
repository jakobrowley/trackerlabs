# Security Notes — Known Issues

## Hardcoded WooCommerce API credentials in license code (HIGH PRIORITY)

**Files:**
- `TrackerLabs_AEX_MAC/TrackerLabs/src/activation.cpp` (and the Win equivalent)
- `TrackerLabsResolvePlugin_Mac/TrackerLabsResolvePlugin/src/activation.cpp` (and the Win equivalent)
- `TrackerLabs_AEX_MAC/TrackerLabs/src/httplib.h` (HTTP client used to call the license server)

The license verification code calls `tinytapes.com/wp-json/lmfwc/v2/licenses/...` with hardcoded WooCommerce API credentials. The credentials are the same as the FastFX, Surveillance, and Terminal plugins. Since these plugins are compiled C++, the credentials live in the binary as string literals — they can be recovered with `strings` on the `.aex` or `.ofx` file.

**Followup work:** see the FastFX or Terminal SECURITY.md for the credential rotation plan. For C++ plugins, the credentials should be moved to compile-time constants set via the build system (e.g., Xcode build settings, MSVC preprocessor defines) so they don't live in source files.

## Metal private buffer workaround (NOT a security issue, but critical to know)

**File:** `TrackerLabsResolvePlugin_Mac/TrackerLabsResolvePlugin/src/MetalKernel.mm` lines 5-28

DaVinci Resolve hands plugins MTLBuffers with `MTLStorageModePrivate`. The `[buffer contents]` API returns nil for private buffers, so naive `memcpy` crashes. The current workaround uses a staging buffer protected by a mutex (`gStagingMutex`).

**Risk:** if the staging buffer logic is broken (mutex not acquired, buffer size miscalculated, threading bug), Resolve crashes when applying TrackerLabs effects on Mac. This is documented in the source comments. Any change to MetalKernel.mm should preserve this workaround.

## CUDA kernel disabled in AE version (NOT a security issue)

**File:** `TrackerLabsResolvePlugin_Mac/TrackerLabsResolvePlugin/src/TrackerLabsResolvePlugin.cpp` line 129

```cpp
//RunCudaKernel(...)
```

The CUDA path is commented out for the AE version. AE falls back to CPU rendering. If GPU support is ever added to the AE version, this needs to be uncommented and tested.

## Build Output Caveats

The `.gitignore` excludes:
- `*.aex` (compiled AE plugins)
- `*.ofx` (compiled OFX plugins)
- `*.plugin/` and `*.bundle/` (Mac plugin bundles)
- `*.dSYM/` (debug symbols — needed for Sentry symbolication)

**Important:** the `.dSYM` files are excluded from git but ARE needed for Sentry to symbolicate crash minidumps. They should be uploaded to Sentry separately during the release build using `sentry-cli upload-dif`. Don't commit them to git (they're large), but don't lose them either.
