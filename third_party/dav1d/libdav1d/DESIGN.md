# DESIGN: DAV1D Bazel Build Configuration

## Objective
Make the DAV1D decoder buildable using Bazel (Bzlmod) across Linux (ARM/x64), Windows (x64/ARM64), and macOS (ARM64), utilizing `BUILD.bazel` and `MODULE.bazel` as starting points and `meson.build` as guidance.

## Context & Current State
- **Module Root:** `libdav1d` currently contains `MODULE.bazel`, establishing it as the root of the Bazel module (`dav1d`).
- **Legacy Configuration:** The starting `BUILD.bazel` originated from a monorepo setup (`//buildenv/platforms/...`, `//third_party/bazel_platforms/...`), referencing absolute paths like `third_party/dav1d/...`.
- **Config Files:** WebRTC pre-generates platform-specific `config.h` and `config.asm` files located in `../config` (relative to `libdav1d`).
- **Dependencies:** `MODULE.bazel` defines dependencies on `rules_cc`, `rules_nasm`, `bazel_skylib`, `platforms`, and `rules_license`. `BUILD.bazel` references `//third_party/xxhash` (which is mapped to `@xxhash` in `cc_binary`).

## Architectural Analysis: Placement of MODULE.bazel and BUILD.bazel

Determining whether `MODULE.bazel` and `BUILD.bazel` should remain in `libdav1d` or move to `third_party/webrtc/third_party/dav1d` involves multi-dimensional trade-offs. Below are the conflicting perspectives to evaluate:

### Perspective A: Move `MODULE.bazel` and `BUILD.bazel` to Parent Directory (`third_party/webrtc/third_party/dav1d`)
- **Advantages:**
  1. **Direct Access to Configurations:** Bazel operates strictly within the directory tree of `MODULE.bazel`. The WebRTC pre-generated configuration files are located in `third_party/webrtc/third_party/dav1d/config`. If `MODULE.bazel` is at the parent level, Bazel can directly access `config/...` via standard labels and `glob()` without needing symlinks, file copying, or duplication into `libdav1d`.
  2. **Alignment with GN Build:** The existing `BUILD.gn` is located at the parent level (`third_party/webrtc/third_party/dav1d/BUILD.gn`) and treats `libdav1d` as a subdirectory (`libdav1d/src/...`). Placing `BUILD.bazel` alongside `BUILD.gn` establishes a consistent boundary between the wrapper repository and the upstream submodule.
- **Disadvantages:**
  1. **Path Adjustments:** All `srcs`, `hdrs`, and include paths in `BUILD.bazel` must be prepended with `libdav1d/` (e.g., `libdav1d/src/*.c`).
  2. **Upstream Divergence:** Upstream projects typically expect build definitions at the root of the source tree (`libdav1d`), not in a parent wrapper directory.

### Perspective B: Keep `MODULE.bazel` and `BUILD.bazel` in `libdav1d` (Current State)
- **Advantages:**
  1. **Upstream Alignment:** Maintaining `MODULE.bazel` inside `libdav1d` mirrors how an upstream Bazel module would be structured. If upstream `dav1d` eventually adopts Bzlmod, keeping the files in `libdav1d` minimizes structural divergence.
  2. **Pristine Paths:** Source file references in `BUILD.bazel` remain clean and direct (`src/*.c`, `include/dav1d/*.h`) without `libdav1d/` prefixes.
- **Disadvantages:**
  1. **Boundary Isolation:** Bazel cannot directly reference files in `../config`. Therefore, the build system must duplicate/symlink `third_party/dav1d/config` into `libdav1d/config` (or define a local repository), adding complexity and maintenance overhead.

### Perspective C: Hybrid Approach
- **Advantages:** `MODULE.bazel` at `third_party/webrtc/third_party/dav1d` defines the workspace root, allowing access to `config/`. A top-level `BUILD.bazel` exports `config/`, while `libdav1d/BUILD.bazel` contains the actual target definitions (`cc_library(name = "dav1d_lib", ...)`).
- **Disadvantages:** Spreads the build logic across multiple packages, increasing cognitive load for maintainers.

## Architectural Plan for Completing the Module

Regardless of the chosen placement, the following actions will be executed to make the Bazel module complete:

### 1. Modernizing Platform Selects (Bzlmod)
Replace all legacy platform conditions with standard Bazel `@platforms//os:...` and `@platforms//cpu:...` constraints:
```starlark
config_setting(name = "linux_x86_64", constraint_values = ["@platforms//os:linux", "@platforms//cpu:x86_64"])
config_setting(name = "linux_arm64", constraint_values = ["@platforms//os:linux", "@platforms//cpu:aarch64"])
config_setting(name = "linux_armv7", constraint_values = ["@platforms//os:linux", "@platforms//cpu:armv7"])
config_setting(name = "windows_x86_64", constraint_values = ["@platforms//os:windows", "@platforms//cpu:x86_64"])
config_setting(name = "windows_arm64", constraint_values = ["@platforms//os:windows", "@platforms//cpu:aarch64"])
config_setting(name = "macos_arm64", constraint_values = ["@platforms//os:macos", "@platforms//cpu:aarch64"])
```

### 2. Adjusting Include Paths and Copts
- **Include Paths:** Ensure `copts` and `nasm_library` include paths correctly map to relative paths (`.`, `include`, `include/dav1d`, `config/...`).
- **Platform-specific Copts & Headers:**
  - `linux_x86_64`: `-Iconfig/linux/x64`, `config/linux/x64/config.h`, `config/linux/x64/config.asm`
  - `linux_arm64`: `-Iconfig/linux/arm64`, `config/linux/arm64/config.h`
  - `linux_armv7`: `-Iconfig/linux/arm`, `config/linux/arm/config.h`
  - `windows_x86_64`: `-Iconfig/win/x64`, `config/win/x64/config.h`, `config/win/x64/config.asm`
  - `windows_arm64`: `-Iconfig/win/arm64`, `config/win/arm64/config.h`
  - `macos_arm64`: `-Iconfig/apple/arm64`, `config/apple/arm64/config.h`

### 3. Target Structure & NASM
- `x86_asm` (`nasm_library`): Only built/linked on x86_64 platforms (Linux x64, Windows x64).
- `dav1d_hdrs`, `dav1d_lib_8bit`, `dav1d_lib_16bit`, `dav1d_lib`: Core static libraries.
- `dav1d` (CLI binary): Verify `@xxhash` dependency resolution within the Bzlmod workspace.

## Four Pillars of Reviewability
1. **Semantic Atomicity:** The implementation will be delivered as a single cohesive commit enabling Bazel builds for `dav1d`.
2. **No-Surprise Diffs:** Changes to `BUILD.bazel` and `MODULE.bazel` will strictly serve the goal of porting to standard Bzlmod platforms and correcting include paths.
3. **Skeleton-First Progression:** We will first establish the core library targets (`dav1d_lib`) across all platforms before addressing optional binaries.
4. **Proactive Justification:** Using `@platforms//` ensures compatibility with modern Bzlmod ecosystems without relying on legacy monorepo definitions.
