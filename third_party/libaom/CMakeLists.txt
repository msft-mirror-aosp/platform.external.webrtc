set(AOM_ROOT ${CMAKE_CURRENT_LIST_DIR}/source)
include(libaom_src.cmake)

if(LINUX_X86_64)
  android_nasm_compile(
    TARGET
    webrtc_libaom_asm
    LICENSE
    "BSD-3-Clause"
    INCLUDES
    ${AOM_ROOT}/libaom
    ${AOM_ROOT}/config/linux/x64/
    SRC
    ${aom_dsp_common_asm_sse2}
    ${aom_dsp_common_asm_ssse3}
    ${aom_av1_encoder_asm_sse2}
    ${aom_dsp_encoder_asm_sse2}
    ${aom_av1_encoder_asm_ssse3_x86_64}
    ${aom_dsp_encoder_asm_sse2_x86_64}
    ${aom_dsp_encoder_asm_ssse3_x86_64}
    ${aom_ports_asm_x86})

  android_add_library(
    TARGET
    webrtc_libaom
    LICENSE
    "BSD-3-Clause"
    SRC
    ${aom_av1_common_intrin_sse2}
    ${aom_dsp_common_intrin_sse2}
    ${aom_av1_encoder_intrin_sse2}
    ${aom_dsp_encoder_intrin_sse2}
    ${aom_av1_common_intrin_ssse3}
    ${aom_av1_encoder_intrin_sse3}
    ${aom_dsp_common_intrin_ssse3}
    ${aom_dsp_encoder_intrin_ssse3}
    ${aom_av1_common_intrin_sse4_1}
    ${aom_dsp_common_intrin_sse4_1}
    ${aom_av1_encoder_intrin_sse4_1}
    ${aom_dsp_encoder_intrin_sse4_1}
    ${aom_av1_encoder_intrin_sse4_2}
    ${aom_dsp_encoder_intrin_avx}
    ${aom_av1_common_intrin_avx2}
    ${aom_dsp_common_intrin_avx2}
    ${aom_av1_encoder_intrin_avx2}
    ${aom_dsp_encoder_intrin_avx2}
    ${aom_av1_common_sources}
    ${aom_av1_decoder_sources}
    ${aom_dsp_common_sources}
    ${aom_dsp_decoder_sources}
    ${aom_mem_sources}
    ${aom_rtcd_sources}
    ${aom_scale_sources}
    ${aom_sources}
    ${aom_util_sources}
    ${aom_av1_encoder_sources}
    ${aom_dsp_encoder_sources})
  target_include_directories(
    webrtc_libaom
    PRIVATE ${AOM_ROOT}/config/linux/x64 ${AOM_ROOT}/config
    PUBLIC ${AOM_ROOT}/libaom)
  # We need to set the proper compiler flags for a subset of files. You cannot
  # do this globally, as the compiler will make the lib unusable.
  set_source_files_properties(
    ${aom_av1_common_intrin_sse2} ${aom_dsp_common_intrin_sse2}
    ${aom_av1_encoder_intrin_sse2} ${aom_dsp_encoder_intrin_sse2}
    PROPERTIES COMPILE_FLAGS -msse2)
  set_source_files_properties(
    ${aom_av1_common_intrin_ssse3} ${aom_dsp_common_intrin_ssse3}
    ${aom_av1_encoder_intrin_sse3} ${aom_dsp_encoder_intrin_ssse3}
    PROPERTIES COMPILE_FLAGS -mssse3)
  set_source_files_properties(
    ${aom_av1_common_intrin_sse4_1} ${aom_dsp_common_intrin_sse4_1}
    ${aom_av1_encoder_intrin_sse4_1} ${aom_dsp_encoder_intrin_sse4_1}
    PROPERTIES COMPILE_FLAGS -msse4.1)
  set_source_files_properties(${aom_av1_encoder_intrin_sse4_2}
                              PROPERTIES COMPILE_FLAGS -msse4.2)
  set_source_files_properties(${aom_dsp_encoder_intrin_avx}
                              PROPERTIES COMPILE_FLAGS -mavx)
  set_source_files_properties(
    ${aom_av1_common_intrin_avx2} ${aom_dsp_common_intrin_avx2}
    ${aom_av1_encoder_intrin_avx2} ${aom_dsp_encoder_intrin_avx2}
    PROPERTIES COMPILE_FLAGS -mavx2)
  target_compile_options(webrtc_libaom
                         PRIVATE -Wno-implicit-function-declaration)
  target_link_libraries(webrtc_libaom PRIVATE webrtc_libaom_asm)
elseif(LINUX_AARCH64)
  android_add_library(
    TARGET
    webrtc_libaom
    LICENSE
    "BSD-3-Clause"
    SRC
    ${aom_av1_common_intrin_neon}
    ${aom_av1_common_sources}
    ${aom_av1_decoder_sources}
    ${aom_dsp_common_intrin_neon}
    ${aom_dsp_common_sources}
    ${aom_dsp_decoder_sources}
    ${aom_mem_sources}
    ${AOM_ROOT}/libaom/aom_ports/arm_cpudetect.c
    ${aom_rtcd_sources}
    ${aom_scale_sources}
    ${aom_sources}
    ${aom_util_sources})
  target_include_directories(
    webrtc_libaom
    PRIVATE ${AOM_ROOT}/config ${AOM_ROOT}/config/arm64
    PUBLIC ${AOM_ROOT}/libaom)
else()
  message(FATAL_ERROR "This can only be used in linux builds")
endif()
