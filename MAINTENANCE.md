# WebRTC Bazel Maintenance Guide

This document describes the design decisions, architecture, and manual adjustments required to integrate WebRTC (`third_party/webrtc`) into the Android Emulator (`aemu`) Bazel workspace using Bzlmod.

Use this as a guide for future WebRTC upgrades and maintenance rounds.

---

## 1. Architecture Overview

To maintain compatibility with upstream WebRTC while fitting into the `aemu` monorepo, we use a hybrid approach:
1.  **Registry-resolved module**: WebRTC is defined as a local module (`webrtc`) in our local Bazel registry (`build/bazel/registry`).
2.  **Local path source**: The registry entry points to the local `third_party/webrtc` folder.
3.  **Forwarding Shims**: External dependencies (like `libyuv`, `aom`, `opus`) are resolved from the registry (BCR), but WebRTC accesses them through local **forwarding shims** to satisfy legacy include paths without modifying the upstream BCR modules.

```mermaid
graph TD
    Root[Root Workspace] -->|bazel_dep| WebRTC[@webrtc]
    WebRTC -->|dep| ShimOpus[//third_party/opus shim]
    WebRTC -->|dep| ShimYuv[//third_party/libyuv shim]
    WebRTC -->|dep| ShimAom[//third_party/libaom shim]
    
    ShimOpus -->|wraps| BCROpus[@opus]
    ShimYuv -->|wraps| BCRYuv[@libyuv]
    ShimAom -->|wraps| BCRAom[@aom]
    
    BCROpus -.-> Registry[Local Registry]
    BCRYuv -.-> Registry
    BCRAom -.-> Registry
```

---

## 2. WebRTC Bzlmod Registration & Versioning

To ensure relative labels (e.g. `//rtc_base:checks` inside WebRTC) resolve correctly to the `third_party/webrtc` directory (instead of the emulator root), WebRTC MUST be treated as a separate Bazel repository.

### Versioning Scheme:
WebRTC does not use traditional SemVer (e.g. `1.2.3`). Instead, we map the Chromium Milestone to the SemVer major version, following the format:
`[Milestone].[Minor].[Patch].aemu`

*   **Current Version**: `152.0.0.aemu` (maps to Chromium Milestone **M152**, upstream revision **48058**, commit `732e34399f`).
*   **Rationale**:
    *   **SemVer Compatible**: Allows Bazel to compare versions correctly (e.g., `152.0.0.aemu` < `153.0.0.aemu`).
    *   **Upgradable**: If we need to release patches for the same milestone, we can increment minor/patch (e.g., `152.0.1.aemu`).
    *   **Identifiable**: The `.aemu` suffix flags it as our custom emulator integration build.
    *   *Note on Commit Hashes*: We do not include the git commit hash in the version string to keep it simple and stable across local patches. The commit hash is documented here and in `call/version.cc`.

### Configuration:
1.  **Registry Entry**: Defined in `build/bazel/registry/modules/webrtc/`:
    *   `metadata.json`: Declares version `152.0.0.aemu`.
    *   `152.0.0.aemu/source.json`: Points to the local path:
        ```json
        {
          "type": "local_path",
          "path": "third_party/webrtc"
        }
        ```
    *   `152.0.0.aemu/MODULE.bazel`: A copy of `third_party/webrtc/MODULE.bazel`. **These two files must always be kept identical.**
2.  **Root Dependency**: The root `MODULE.bazel` depends on it via:
    ```python
    bazel_dep(name = "webrtc", version = "152.0.0.aemu")
    ```
    No `local_path_override` is needed in the root `MODULE.bazel`, as the registry handles the redirection.

---

## 3. Dependency Alignment

To avoid version conflicts (where different modules request different versions of the same dependency), we align versions in the registry and remove local overrides.

*   **Registry resolution**: All external dependencies (`pffft`, `rnnoise`, `libgav1`, `dav1d`, `libsrtp2`, `libyuv`, `aom`, `opus`, `ffmpeg`) must be present in the local registry.
*   **No Overrides**: `third_party/webrtc/MODULE.bazel` should contain **zero** `local_path_override` or `single_version_override` declarations for these dependencies. It should rely entirely on Bzlmod version resolution.
*   **gRPC**: Both the root module and WebRTC must depend on the same custom patched version (e.g., `1.81.1.aemu`). The root `MODULE.bazel` uses `single_version_override` to pin this version workspace-wide.

---

## 4. Include Path Shims (Starlark Forwarding Shims)

Upstream BCR modules (like `libyuv` and `aom`) export their headers at the root of their repositories. However, WebRTC source code contains hardcoded include paths expecting them under specific subdirectories (e.g., `#include "third_party/libyuv/include/libyuv.h"` or `#include "third_party/opus/src/include/opus.h"`).

To support this without patching the BCR modules:
1.  We define forwarding shim targets in `third_party/webrtc/third_party/`.
2.  We use a Starlark macro `generate_shim_headers` (defined in `third_party/shims.bzl`) to generate forwarding headers that `#include` the actual BCR headers.
3.  We redirect WebRTC target dependencies from `@external_module` to `//third_party/external_module`.
4.  **Package Marker**: An empty `third_party/BUILD.bazel` file must be present to mark the `third_party` directory as a package (enabling `load` statements from `//third_party:shims.bzl`). Since `third_party/` is gitignored by default, this file must be force-added to git (`git add -f third_party/BUILD.bazel`).

### The Shim Macro (`third_party/shims.bzl`):
The macro takes a list of headers and a target subdirectory path, generates empty forwarding files that redirect to the BCR headers using `#include`, and exposes a `cc_library` with `include_prefix` set to the legacy path.

### Implemented Shims:
*   **`//third_party/opus`**: Replaces `@opus` dependency. Generates forwarding headers for `opus.h`, `opus_defines.h`, `opus_multistream.h`, `opus_types.h`.
*   **`//third_party/libyuv`**: Replaces `@libyuv` dependency. Generates 25+ forwarding headers (e.g., `libyuv.h`, `libyuv/basic_types.h`, etc.) under `third_party/libyuv/include/`.
*   **`//third_party/libaom`**: Replaces `@aom` dependency. Generates forwarding headers under `third_party/libaom/source/libaom/`.

*Maintenance action*: When upgrading WebRTC, if compilation fails due to missing headers from these libraries, add the missing header names to the macro instantiation in the respective `BUILD.bazel` file under `third_party/webrtc/third_party/<library>/BUILD.bazel`.

---

## 5. Catapult (Tracing) Integration

WebRTC depends on `catapult` for tracing, which is not available in Bzlmod and too complex to import.
1.  **Disabled Bzlmod Dependency**: We do not declare `catapult` in `MODULE.bazel`.
2.  **Local Compilation**: We compile the subset of Catapult C++ and Proto sources needed by WebRTC locally.
3.  **Local Targets**:
    *   `third_party/catapult/tracing/tracing/BUILD.bazel` defines C++ targets (e.g., `histogram`, `trace_event`).
    *   `third_party/catapult/tracing/tracing/proto/BUILD.bazel` defines proto targets.
4.  **Redirection**: In `test/BUILD.bazel`, all dependencies on `@catapult` targets are rewritten to point to these local targets (e.g. `//third_party/catapult/tracing/tracing:histogram`).

---

## 6. GL and Xorg Integration

To avoid external Bzlmod repository dependencies for graphics/windowing:
1.  **GL**: Replaced `@GL` references in `test/BUILD.bazel` with local path `//third_party/GL`.
2.  **Xorg**: Replaced `@Xorg` references in `modules/desktop_capture/BUILD.bazel` and `test/BUILD.bazel` with local path `//third_party/Xorg`.

---

## 7. Local Build Shims (tools/build_defs, third_party/bazel_rules, and third_party/protobuf)

Upstream WebRTC's generated `BUILD.bazel` files load various local rules from `//tools/build_defs/...`, `//third_party/bazel_rules/...`, and `//third_party/protobuf/...`.
1.  **Purpose**: These are shims to stub out external toolchain dependencies (Go, Swift, Kotlin, Android, Kotlin Protobuf) or to vendor build rules (`rules_cc`, `rules_java`, `rules_python`, standard protobuf rules) locally so that the generated build files can resolve them without external repository downloads.
2.  **Tracking**: Because `tools/` and `third_party/` are ignored by upstream WebRTC's `.gitignore`, these directories/files must be **force-added** to git (`git add -f tools/build_defs`, `git add -f third_party/bazel_rules`, and `git add -f third_party/protobuf`) to ensure they are committed.
3.  **Contents**:
    *   `tools/build_defs/android/`: Stub rules for Android.
    *   `tools/build_defs/go/`: Stub rules for Go (e.g. `go_proto_library`).
    *   `tools/build_defs/swift/`: Stub rules for Swift.
    *   `tools/build_defs/js/`, `tools/build_defs/kotlin/`, `tools/build_defs/build_test/`.
    *   `third_party/bazel_rules/rules_cc/`: Local shims for C++ rules (`cc_library`, `cc_binary`, etc.).
    *   `third_party/bazel_rules/rules_java/`: Local shims for Java rules (`java_import`).
    *   `third_party/bazel_rules/rules_python/`: Local shims for Python rules (`py_binary`).
    *   `third_party/protobuf/bazel/`: Local shims for protobuf rules (`cc_proto_library`, `proto_library`).
    *   `third_party/protobuf/build_defs/`: Local shims for Kotlin protobuf (`kt_jvm_proto_library`).

---

## 8. Step-by-Step Upgrade Checklist

When a new version of WebRTC is translated from `google3` and merged:

1.  **Regenerate BUILD files**: Run the translation scripts (in google3) to generate the base `BUILD.bazel` files.
    *   *Note on Empty Globs*: The google3 `BUILD.gn` -> Bazel generator should be updated to output `allow_empty = True` for globs that may be empty (specifically `glob(["**/*.gni"])` and `glob(["build/**"])`). If the generator does not support this yet, you must manually run a replacement script (like `scratch/fix_empty_globs.py`) to add `allow_empty = True` to all empty globs, otherwise modern Bazel (Bazel 8+) will fail during loading phase.
    *   *Note on RTTI*: The generator outputs `"$(WEBRTC_RTTI)"` in `copts` for Apple platforms, referencing a make variable defined in google3 but not in standard Bzlmod. The generator should be updated to output `"-fno-rtti"` directly (or handle it via select). If it still outputs the variable, you must run a script (like `scratch/fix_webrtc_rtti.py`) to inline `"-fno-rtti"`, otherwise builds on macOS will fail.
    *   *Note on macOS Target Compatibility*: The generator may output incorrect `target_compatible_with` constraints (e.g., marking macOS-specific `objc_library` targets like `//test:test_renderer_objc` as compatible only with Windows/Linux, or excluding macOS from `//test:perf_test`). You must ensure these are corrected (typically using `@platforms//os:macos`) to allow tests to compile on macOS.
2.  **Apply Include Path Shims**:
    *   Ensure `third_party/shims.bzl` and `third_party/BUILD.bazel` (empty package marker) are preserved.
    *   Verify `third_party/opus/BUILD.bazel`, `third_party/libyuv/BUILD.bazel`, and `third_party/libaom/BUILD.bazel` are intact and use the shim macro.
    *   In WebRTC `BUILD.bazel` files, search for dependencies on `@opus`, `@libyuv`, `@aom` and rewrite them to `//third_party/opus`, `//third_party/libyuv`, and `//third_party/libaom` respectively.
3.  **Apply Catapult Rewrites**:
    *   Ensure the local BUILD files under `third_party/catapult/` are preserved.
    *   In `test/BUILD.bazel`, rewrite any `@catapult//...` dependencies to `//third_party/catapult/tracing/tracing:...`.
4.  **Apply GL/Xorg Rewrites**:
    *   Rewrite `@GL` -> `//third_party/GL`.
    *   Rewrite `@Xorg` -> `//third_party/Xorg`.
5.  **Preserve Local Build Shims**:
    *   Ensure `tools/build_defs/`, `third_party/bazel_rules/`, and `third_party/protobuf/` are intact. If any new shims are added, they must be force-added to git (`git add -f tools/build_defs/...`, `git add -f third_party/bazel_rules/...`, or `git add -f third_party/protobuf/...`).
6.  **Sync MODULE.bazel**:
    *   Ensure `third_party/webrtc/MODULE.bazel` has no overrides for registry modules (`dav1d`, `libgav1`, `ffmpeg`, `libsrtp2`, `pffft`, `rnnoise`, `opus`, `libyuv`, `aom`).
    *   Copy `third_party/webrtc/MODULE.bazel` to `build/bazel/registry/modules/webrtc/[Version]/MODULE.bazel`.
7.  **Verify**: Follow the verification steps in Section 9.

---

## 9. Build & Test Verification

To validate that the WebRTC integration is working correctly after a merge or upgrade, you can run the following Bazel commands.

### Core Build Targets
Validate that the primary WebRTC libraries used by the emulator compile successfully:
```bash
bazel build \
    @webrtc//api:libjingle_peerconnection_api \
    @webrtc//pc:peer_connection \
    @webrtc//api:create_peerconnection_factory \
    @webrtc//api/audio_codecs:builtin_audio_decoder_factory \
    @webrtc//api/audio_codecs:builtin_audio_encoder_factory \
    @webrtc//api/video_codecs:builtin_video_decoder_factory \
    @webrtc//api/video_codecs:builtin_video_encoder_factory
```

### Basic Unit Tests
Run the following unit tests (which do not require downloading large external media assets):
```bash
bazel test \
    @webrtc//examples:examples_unittests \
    @webrtc//net/dcsctp:dcsctp_unittests
```

