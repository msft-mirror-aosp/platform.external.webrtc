"""WebRTC Skylark macros and constants."""

load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")
load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_python//python:proto.bzl", "py_proto_library")

def platform_select(
        _all = None,
        posix = None,
        linux_kernel = None,
        linux = None,
        linux_x64 = None,
        android = None,
        android_armv7 = None,
        android_arm64 = None,
        android_x86 = None,
        android_x64 = None,
        apple = None,
        ios = None,
        ios_arm64 = None,
        ios_armv7 = None,
        ios_x86 = None,
        ios_x64 = None,
        mac = None,
        mac_x64 = None,
        mac_arm64 = None,
        windows_x86 = None,
        windows_x64 = None,
        windows = None,
        intel_cpu = None,
        armv7 = None,
        arm64 = None):
    """Generates a Bazel `select` for each platform by collapsing categories of dependencies.

    Inputs to this function represent the dependencies associated with each category.
    E.g. Dependencies associated with `android` are those that apply to all platforms that include
    the `android` category below.

    Args:
      _all: Dependencies common to all platforms.
      posix: Dependencies common to posix.
      linux_kernel: Dependencies common to platforms that use the Linux kernel.
      linux: Dependencies common to platforms that use Linux headers.
      linux_x64: Dependencies common to Linux x86_64.
      linux_arm64: Dependencies common to Linux ARM64.
      chromeos: Dependencies common to Chrome OS.
      chromeos_armv7: Dependencies common to Chrome OS ARMv7.
      chromeos_arm64: Dependencies common to Chrome OS ARM64.
      chromeos_x64: Dependencies common to Chrome OS x86_64.
      android: Dependencies common to Android.
      android_armv7: Dependencies common to Android ARMv7.
      android_arm64: Dependencies common to Android ARM64.
      android_x86: Dependencies common to Android x86.
      android_x64: Dependencies common to Android x86_64.
      android_riscv64: Dependencies common to Android RISCV64.
      apple: Dependencies common to Apple platforms.
      ios: Dependencies common to iOS platforms.
      ios_arm64: Dependencies common to iOS ARM64.
      ios_armv7: Dependencies common to iOS ARMv7.
      ios_x86: Dependencies common to iOS x86.
      ios_x64: Dependencies common to iOS x86_64.
      fuchsia: Dependencies common to Fuchsia platforms.
      mac: Dependencies common to Mac OS.
      mac_x64: Dependencies common to Mac OS x64.
      mac_arm64: Dependencies common to Mac OS arm64.
      windows_x86: Dependencies common to Windows x86.
      windows_x64: Dependencies common to Windows x86_64.
      windows: Dependencies common to Windows.
      intel_cpu: Dependencies common to Intel CPUs.
      armv7: Dependencies common to ARMv7 CPUs.
      arm64: Dependencies common to ARM64 CPUs.
      riscv64: Dependencies common to RISCV64 CPUs.
      wasm: Dependencies common to Web Assembly.
      fuchsia_x64: Dependencies common to Fuchsia x86_64.
      nestcam_ambarella_armv7: Dependencies common to NestCam Amberalla SDK, for ARMv7.
      nestcam_ambarella_armv6_nofp: Dependencies common to NestCam Amberalla SDK, for ARMv6 with no
      floating point support (corresponding to QV1).

    Returns:
      A Bazel `select` statement that maps platforms of interest to the dependencies, computed by
      collapsing the values of subcategories of these platforms.
    """
    result = {}
    for condition, lists in {
        # Keep this synced with generate_project.cfg - collapse_categories
        "//configs:linux": [_all, posix, linux_kernel, intel_cpu, linux, linux_x64],
        "//configs:android_arm": [_all, posix, linux_kernel, android, armv7, android_armv7],
        "//configs:android_arm64": [_all, posix, linux_kernel, android, arm64, android_arm64],
        "//configs:android_x86": [_all, posix, linux_kernel, android, intel_cpu, android_x86],
        "//configs:android_x86_64": [_all, posix, linux_kernel, android, intel_cpu, android_x64],
        "//configs:ios_x86_32": [_all, posix, apple, ios, intel_cpu, ios_x86],
        "//configs:ios_x86_64": [_all, posix, apple, ios, intel_cpu, ios_x64],
        "//configs:ios_armv7": [_all, posix, apple, ios, armv7, ios_armv7],
        "//configs:ios_arm64": [_all, posix, apple, ios, arm64, ios_arm64],
        "//configs:darwin_x86_64": [_all, posix, apple, intel_cpu, mac, mac_x64],
        "//configs:darwin_arm64": [_all, posix, apple, arm64, mac, mac_arm64],
        "//configs:windows_x86": [_all, windows, intel_cpu, windows_x86],
        "//configs:windows_x86_64": [_all, windows, intel_cpu, windows_x64],
    }.items():
        for lst in lists:
            # Branches that are not covered by any category will not be in the select.
            if lst != None:
                result.setdefault(condition, []).extend(lst)

    return select(result)

def webrtc_proto_library(
        name,
        srcs,
        deps = [],
        visibility = ["//:__subpackages__"]):
    if not name.endswith("_proto"):
        fail("The target name must end with '_proto'.")
    proto_library(
        name = "%s_lib" % name,
        srcs = srcs,
        visibility = visibility,
        deps = deps,
    )

    cc_proto_library(
        name = name,
        visibility = visibility,
        deps = ["%s_lib" % name],
    )

    if hasattr(native, "py_proto_library"):
        py_proto_library(
            name = "%s_py_pb2" % name,
            api_version = 2,
            deps = [":%s_lib" % name],
            visibility = visibility,
        )

def webrtc_grpc_library(
        name,
        srcs,
        deps = [],
        visibility = ["//:__subpackages__"]):
    """Generates targets for grpc_library.

    Generates the proto_library, cc_proto_library and cc_grpc_library targets
    for the matching srcs proto files.

    Args:
      name: The name of the library.
      srcs: The list of proto files.
      deps: Optional dependencies for the proto files.
      visibility: Visibility of the library.
    """
    proto_library(
        name = "%s_proto" % name,
        srcs = srcs,
        cc_api_version = 2,
        has_services = True,
        visibility = visibility,
        deps = deps,
    )

    cc_proto_library(
        name = "%s_cc_proto" % name,
        visibility = visibility,
        deps = ["%s_proto" % name],
    )

    cc_grpc_library(
        name = name,
        srcs = ["%s_proto" % name],
        visibility = visibility,
        deps = ["%s_cc_proto" % name],
    )
