"""Libraries that require the use of different targets per platform."""

def libvpx_dependency():
    return select({
        "//tools/cc_target_os:android": [
            "@libvpx//:vpx_android_static_realtime",
        ],
        "//configs:ios_arm64": [
            "@libvpx//:vpx_ios_arm64_lowbd_realtime",
        ],
        "//configs:ios_x86_64": [
            "@libvpx//:vpx_ios_x86_64_lowbd_realtime",
        ],
        "//configs:darwin_x86_64": [
            "@libvpx//:vpx_macos_x86_64_lowbd_realtime",
        ],
        "//configs:darwin_arm64": [
            "@libvpx//:vpx_ios_arm64_lowbd_realtime",
        ],
        "//configs:windows_x86": ["@libvpx//:vpx_win32"],
        "//configs:windows_x86_64": ["@libvpx//:vpx_win64"],
        "//conditions:default": ["@libvpx//:vpx"],
    })

def libaom_encoder_dependency():
    # This build target selector exists because there is no libaom AV1
    # RTC optimized encoder only build target. For Android and iOS there are
    # optmized build targets, but for other platforms we still have to fall
    # back to `libaom:aom_highbd`.
    # TODO(b/224761746): Remove this and map directly to the libaom RTC optimized
    # encoder build target instead.
    return []
    select({
        "@platforms//os:android": ["@libaom//:aom_android_native_realtime-encoder-only"],
        "//configs:ios_arm64": ["@libaom//:aom_ios_arm64_lowbd_realtime"],
        "//configs:ios_x86_64": ["@libaom//:aom_ios_x86_64_lowbd_realtime"],
        "//conditions:default": ["@libaom//:aom_highbd"],
    })
