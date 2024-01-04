set(DAV1D_ROOT ${CMAKE_CURRENT_LIST_DIR})

android_add_library(
  TARGET dav1d
  LICENSE "BSD-2-Clause"
  SOURCE_DIR libdav1d/src
  SRC cdf.c
      cpu.c
      data.c
      decode.c
      dequant_tables.c
      getbits.c
      intra_edge.c
      itx_1d.c
      lf_mask.c
      lib.c
      log.c
      mem.c
      msac.c
      obu.c
      pal.c
      picture.c
      qm.c
      ref.c
      refmvs.c
      scan.c
      tables.c
      thread_task.c
      warpmv.c
      wedge.c
  WINDOWS win32/thread.c)

if(WINDOWS_MSVC_X86_64 OR LINUX_X86_64 OR DARWIN_X86_64)
  target_sources(dav1d PRIVATE "libdav1d/src/x86/cpu.c")
endif()

if(LINUX_AARCH64)
  # TODO No darwin aarch64...
  enable_language(ASM)
  target_sources(
    dav1d
    PRIVATE libdav1d/src/arm/64/cdef16.S
            libdav1d/src/arm/64/cdef.S
            libdav1d/src/arm/64/filmgrain16.S
            libdav1d/src/arm/64/filmgrain.S
            libdav1d/src/arm/64/ipred16.S
            libdav1d/src/arm/64/ipred.S
            libdav1d/src/arm/64/itx16.S
            libdav1d/src/arm/64/itx.S
            libdav1d/src/arm/64/loopfilter16.S
            libdav1d/src/arm/64/loopfilter.S
            libdav1d/src/arm/64/looprestoration16.S
            libdav1d/src/arm/64/looprestoration_common.S
            libdav1d/src/arm/64/looprestoration.S
            libdav1d/src/arm/64/mc16.S
            libdav1d/src/arm/64/mc.S
            libdav1d/src/arm/64/msac.S
            libdav1d/src/arm/64/refmvs.S)
endif()

target_compile_options(
  dav1d
  PRIVATE "-Wall"
          "-Winvalid-pch"
          "-Wextra"
          "-std=c99"
          "-Wundef"
          "-w"
          "-fomit-frame-pointer"
          "-ffast-math")

android_target_include_directories(dav1d linux-x86_64 PRIVATE config/linux/x64)
android_target_include_directories(dav1d linux-aarch64 PRIVATE
                                   config/linux/arm64)
android_target_include_directories(dav1d windows PRIVATE config/win/x64)
android_target_compile_definitions(dav1d linux PRIVATE _GNU_SOURCE)
android_target_compile_definitions(dav1d darwin PRIVATE _GNU_SOURCE)
target_include_directories(
  dav1d PRIVATE libdav1d PUBLIC libdav1d/include libdav1d/include/dav1d version)

target_link_libraries(dav1d PRIVATE dav1d_lib_8bit dav1d_lib_16bit)
target_compile_definitions(dav1d PRIVATE "-D_FILE_OFFSET_BITS=64"
                                         "-D_ISOC99_SOURCE")
target_compile_options(
  dav1d
  PRIVATE "-Wall"
          "-Winvalid-pch"
          "-Wextra"
          "-std=c99"
          "-Wundef"
          "-w"
          "-fomit-frame-pointer"
          "-ffast-math")

android_target_compile_options(dav1d linux-x86_64
                               PRIVATE -mavx2 -mssse3 -msse4.1 -msse2 -msse3)

android_add_library(
  TARGET dav1d_lib_8bit
  LICENSE "BSD-2-Clause"
  SOURCE_DIR libdav1d/src
  SRC cdef_apply_tmpl.c
      cdef_tmpl.c
      fg_apply_tmpl.c
      filmgrain_tmpl.c
      ipred_prepare_tmpl.c
      ipred_tmpl.c
      itx_tmpl.c
      lf_apply_tmpl.c
      loopfilter_tmpl.c
      looprestoration_tmpl.c
      lr_apply_tmpl.c
      mc_tmpl.c
      recon_tmpl.c)

target_compile_definitions(dav1d_lib_8bit PRIVATE BITDEPTH=8)
android_target_include_directories(dav1d_lib_8bit linux-x86_64 PRIVATE
                                   config/linux/x64)
android_target_include_directories(dav1d_lib_8bit linux-aarch64 PRIVATE
                                   config/linux/arm64)
android_target_include_directories(dav1d_lib_8bit windows PRIVATE
                                   config/win/x64)
target_compile_definitions(dav1d_lib_8bit PRIVATE "-D_FILE_OFFSET_BITS=64"
                                                  "-D_ISOC99_SOURCE")
target_compile_options(
  dav1d_lib_8bit
  PRIVATE "-Wall"
          "-Winvalid-pch"
          "-Wextra"
          "-std=c99"
          "-Wundef"
          "-w"
          "-fomit-frame-pointer"
          "-ffast-math")
target_include_directories(
  dav1d_lib_8bit PRIVATE libdav1d PUBLIC libdav1d/include
                                         libdav1d/include/dav1d version)

android_target_compile_options(dav1d_lib_8bit linux-x86_64
                               PRIVATE -mavx2 -mssse3 -msse4.1 -msse2 -msse3)

android_add_library(
  TARGET dav1d_lib_16bit
  LICENSE "BSD-2-Clause"
  SOURCE_DIR libdav1d/src
  SRC cdef_apply_tmpl.c
      cdef_tmpl.c
      fg_apply_tmpl.c
      filmgrain_tmpl.c
      ipred_prepare_tmpl.c
      ipred_tmpl.c
      itx_tmpl.c
      lf_apply_tmpl.c
      loopfilter_tmpl.c
      looprestoration_tmpl.c
      lr_apply_tmpl.c
      mc_tmpl.c
      recon_tmpl.c)

target_compile_definitions(dav1d_lib_16bit PRIVATE BITDEPTH=16)
android_target_include_directories(dav1d_lib_16bit linux-x86_64 PRIVATE
                                   config/linux/x64)
android_target_include_directories(dav1d_lib_16bit linux-aarch64 PRIVATE
                                   config/linux/arm64)
android_target_include_directories(dav1d_lib_16bit windows PRIVATE
                                   config/win/x64)
target_include_directories(
  dav1d_lib_16bit PRIVATE libdav1d PUBLIC libdav1d/include
                                          libdav1d/include/dav1d version)
target_compile_definitions(dav1d_lib_16bit PRIVATE "-D_FILE_OFFSET_BITS=64"
                                                   "-D_ISOC99_SOURCE")

android_target_compile_options(dav1d_lib_16bit linux-x86_64
                               PRIVATE -mavx2 -mssse3 -msse4.1 -msse2 -msse3)

target_compile_options(
  dav1d_lib_16bit
  PRIVATE "-Wall"
          "-Winvalid-pch"
          "-Wextra"
          "-std=c99"
          "-Wundef"
          "-w"
          "-fomit-frame-pointer"
          "-ffast-math")

if(LINUX_X86_64 OR WINDOWS)
  if(LINUX_X86_64)
    set(INCLUDE_ASM_DIR "${DAV1D_ROOT}/config/linux/x64")
  else()
    set(INCLUDE_ASM_DIR "${DAV1D_ROOT}/config/win/x64")
  endif()
  android_nasm_compile(
    TARGET dav1d_asm
    LICENSE "BSD-2-Clause" INCLUDES ${INCLUDE_ASM_DIR}
            ${DAV1D_ROOT}/libdav1d/src
    SRC libdav1d/src/x86
        libdav1d/src/x86/cdef16_avx2.asm
        libdav1d/src/x86/cdef16_avx512.asm
        libdav1d/src/x86/cdef16_sse.asm
        libdav1d/src/x86/cdef_avx2.asm
        libdav1d/src/x86/cdef_avx512.asm
        libdav1d/src/x86/cdef_sse.asm
        libdav1d/src/x86/cpuid.asm
        libdav1d/src/x86/filmgrain16_avx2.asm
        libdav1d/src/x86/filmgrain16_avx512.asm
        libdav1d/src/x86/filmgrain16_sse.asm
        libdav1d/src/x86/filmgrain_avx2.asm
        libdav1d/src/x86/filmgrain_avx512.asm
        libdav1d/src/x86/filmgrain_sse.asm
        libdav1d/src/x86/ipred16_avx2.asm
        libdav1d/src/x86/ipred16_avx512.asm
        libdav1d/src/x86/ipred16_sse.asm
        libdav1d/src/x86/ipred_avx2.asm
        libdav1d/src/x86/ipred_avx512.asm
        libdav1d/src/x86/ipred_sse.asm
        libdav1d/src/x86/itx16_avx2.asm
        libdav1d/src/x86/itx16_avx512.asm
        libdav1d/src/x86/itx16_sse.asm
        libdav1d/src/x86/itx_avx2.asm
        libdav1d/src/x86/itx_avx512.asm
        libdav1d/src/x86/itx_sse.asm
        libdav1d/src/x86/loopfilter16_avx2.asm
        libdav1d/src/x86/loopfilter16_avx512.asm
        libdav1d/src/x86/loopfilter16_sse.asm
        libdav1d/src/x86/loopfilter_avx2.asm
        libdav1d/src/x86/loopfilter_avx512.asm
        libdav1d/src/x86/loopfilter_sse.asm
        libdav1d/src/x86/looprestoration16_avx2.asm
        libdav1d/src/x86/looprestoration16_avx512.asm
        libdav1d/src/x86/looprestoration16_sse.asm
        libdav1d/src/x86/looprestoration_avx2.asm
        libdav1d/src/x86/looprestoration_avx512.asm
        libdav1d/src/x86/looprestoration_sse.asm
        libdav1d/src/x86/mc16_avx2.asm
        libdav1d/src/x86/mc16_avx512.asm
        libdav1d/src/x86/mc16_sse.asm
        libdav1d/src/x86/mc_avx2.asm
        libdav1d/src/x86/mc_avx512.asm
        libdav1d/src/x86/mc_sse.asm
        libdav1d/src/x86/msac.asm
        libdav1d/src/x86/pal.asm
        libdav1d/src/x86/refmvs.asm)
  target_link_libraries(dav1d PRIVATE dav1d_asm)
endif()
