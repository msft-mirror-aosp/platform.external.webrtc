set(VPX_ROOT ${CMAKE_CURRENT_LIST_DIR}/source/libvpx)
include(libvpx_src.cmake)

set(libvpx_cflags
    # libvpx heavily relies on implicit enum casting.
    -Wno-conversion
    # libvpx does `if ((a == b))` in some places.
    -Wno-parentheses-equality
    # libvpx has many static functions in header, which trigger this warning.
    -Wno-unused-function)

if(DARWIN_AARCH64)
  set(libvpx_includes ${VPX_ROOT}/../config
                      ${VPX_ROOT}/../config/linux/arm64-highbd ${VPX_ROOT})
  set(libvpx_sources ${libvpx_srcs_arm64_highbd})
elseif(LINUX_AARCH64)
  set(libvpx_includes ${VPX_ROOT}/../config ${VPX_ROOT}/../config/linux/arm64
                      ${VPX_ROOT})

  android_add_library(
    TARGET libvpx_intrinsics_neon
    LICENSE "BSD-3-Clause"
    NOTICE "REPO/LICENSE"
    REPO "${VPX_ROOT}/../.."
         URL
         "https://android.googlesource.com/platform/external/webrtc/+/refs/heads/emu-master-dev"
    SRC ${libvpx_srcs_arm64_neon})
  # target_compile_options(libvpx_intrinsics_neon PRIVATE -Wall
  # -march=armv8.2-a+dotprod)

  target_include_directories(libvpx_intrinsics_neon PRIVATE ${libvpx_includes})

  android_add_library(
    TARGET libvpx_arm64_neon_dotprod
    # SHARED
    LICENSE "BSD-3-Clause"
    NOTICE "REPO/LICENSE"
    REPO "${VPX_ROOT}/../.."
         URL
         "https://android.googlesource.com/platform/external/webrtc/+/refs/heads/emu-master-dev"
    SRC ${libvpx_srcs_arm64_neon_dotprod})
  target_compile_options(libvpx_arm64_neon_dotprod
                         PRIVATE -Wall -march=armv8.2-a+dotprod)

  target_include_directories(libvpx_arm64_neon_dotprod
                             PRIVATE ${libvpx_includes})

  android_add_library(
    TARGET libvpx_arm64_highbd_neon_i8mm
    # SHARED
    LICENSE "BSD-3-Clause"
    NOTICE "REPO/LICENSE"
    REPO "${VPX_ROOT}/../.."
         URL
         "https://android.googlesource.com/platform/external/webrtc/+/refs/heads/emu-master-dev"
    SRC ${libvpx_srcs_arm64_highbd_neon_i8mm})
  target_compile_options(libvpx_arm64_highbd_neon_i8mm
                         PRIVATE -Wall -march=armv8.2-a+i8mm)

  target_include_directories(libvpx_arm64_highbd_neon_i8mm
                             PRIVATE ${libvpx_includes})
  set(libvpx_sources ${libvpx_srcs_arm64})
else()
  set(libvpx_includes ${VPX_ROOT}/../config ${VPX_ROOT}/../config/linux/x64
                      ${VPX_ROOT})

  if(DARWIN_X86_64)
    set(libvpx_cflags ${libvpx_cflags} -fno-common)
  endif()
  if(WINDOWS_MSVC_X86_64)
    set(libvpx_includes ${VPX_ROOT}/../config ${VPX_ROOT}/../config/win/x64
                        ${VPX_ROOT})
  endif()

  android_nasm_compile(
    TARGET libvpx-asm
    LICENSE "BSD-3-Clause"
    NOTICE "REPO/NOTICE"
    REPO "${ANDROID_QEMU2_TOP_DIR}/../libvpx"
         URL
         "https://android.googlesource.com/platform/external/libvpx/+/refs/heads/emu-master-dev"
         INCLUDES
         ${libvpx_includes}
    SRC ${libvpx_srcs_x86_64_assembly})
  set(libvpx_asm libvpx-asm)

  set(libvpx_sources
      ${libvpx_srcs_x86_64_avx2}
      ${libvpx_srcs_x86_64_avx512}
      ${libvpx_srcs_x86_64_avx}
      ${libvpx_srcs_x86_64_mmx}
      ${libvpx_srcs_x86_64_sse2}
      ${libvpx_srcs_x86_64_sse4_1}
      ${libvpx_srcs_x86_64_ssse3}
      ${libvpx_srcs_x86_64})

  # We need to set the proper compiler flags for a subset of files. You cannot
  # do this globally, as the compiler will make the lib unusable.
  set_source_files_properties(${libvpx_srcs_x86_64_mmx} PROPERTIES COMPILE_FLAGS
                                                                   -mmmx)
  set_source_files_properties(${libvpx_srcs_x86_64_sse2}
                              PROPERTIES COMPILE_FLAGS -msse2)
  set_source_files_properties(${libvpx_srcs_x86_64_ssse3}
                              PROPERTIES COMPILE_FLAGS -mssse3)
  set_source_files_properties(${libvpx_srcs_x86_64_sse4_1}
                              PROPERTIES COMPILE_FLAGS -msse4.1)
  set_source_files_properties(${libvpx_srcs_x86_64_avx} PROPERTIES COMPILE_FLAGS
                                                                   -mavx)
  set_source_files_properties(${libvpx_srcs_x86_64_avx2}
                              PROPERTIES COMPILE_FLAGS -mavx2)
  set_source_files_properties(
    ${libvpx_srcs_x86_64_avx512}
    PROPERTIES COMPILE_FLAGS -mavx512f -mavx512cd -mavx512bw -mavx512dq
                                                             -mavx512vl)
endif()

android_add_library(
  TARGET libvpx
  SHARED
  LICENSE "BSD-3-Clause"
  NOTICE "REPO/LICENSE"
  REPO "${VPX_ROOT}/../.."
       URL
       "https://android.googlesource.com/platform/external/webrtc/+/refs/heads/emu-master-dev"
  SRC ${libvpx_sources})

if(LINUX_AARCH64)
  target_link_libraries(
    libvpx PRIVATE libvpx_arm64_highbd_neon_i8mm libvpx_arm64_neon_dotprod
                   libvpx_intrinsics_neon)
endif()
target_include_directories(libvpx PUBLIC ${VPX_ROOT} PRIVATE ${libvpx_includes})
target_link_libraries(libvpx PRIVATE webrtc-yuv ${libvpx_asm})
target_link_options(libvpx PRIVATE -Wl,-no-undefined)
