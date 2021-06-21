set(WEBRTC_ROOT ${CMAKE_CURRENT_LIST_DIR})

# Extract the test files needed for the unit tests.
message(STATUS "Extracting webrtc test resources")
if(${CMAKE_VERSION} VERSION_LESS "3.18.0")
    execute_process(
        COMMAND unzip -n "${WEBRTC_ROOT}/test_resources.zip" -d ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${WEBRTC_ROOT})
else()
    file(ARCHIVE_EXTRACT INPUT ${WEBRTC_ROOT}/test_resources.zip DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

add_library(
  webrtc_spl_sqrt_floor
  ${WEBRTC_ROOT}/common_audio/third_party/spl_sqrt_floor/spl_sqrt_floor.c)
target_include_directories(webrtc_spl_sqrt_floor PUBLIC ${WEBRTC_ROOT})

add_library(
  webrtc_fft_size_128
  ${WEBRTC_ROOT}/common_audio/third_party/ooura/fft_size_128/ooura_fft.cc)
target_include_directories(webrtc_fft_size_128 PUBLIC ${WEBRTC_ROOT})

if(NOT LINUX_AARCH64 AND NOT DARWIN_AARCH64)
  target_sources(
    webrtc_fft_size_128
    PRIVATE
      ${WEBRTC_ROOT}/common_audio/third_party/ooura/fft_size_128/ooura_fft_sse2.cc
  )
endif()

add_library(webrtc_fft_size_256
            ${WEBRTC_ROOT}/common_audio/third_party/ooura/fft_size_256/fft4g.cc)
target_include_directories(webrtc_fft_size_256 PUBLIC ${WEBRTC_ROOT})

add_library(webrtc_fft ${WEBRTC_ROOT}/modules/third_party/fft/fft.c)
target_include_directories(webrtc_fft PUBLIC ${WEBRTC_ROOT})

android_add_library(TARGET emulator_test_overrides NODISTRIBUTE
                    SRC emu_file_utils_override.cc)
target_link_libraries(
  emulator_test_overrides
  PRIVATE android-emu-base webrtc_test_fileutils_override_api absl::optional)
target_compile_definitions(
  emulator_test_overrides
  PRIVATE "EMU_THIRD_PARTY_WEBRTC=\"${CMAKE_CURRENT_LIST_DIR}\""
          "EMU_BIN_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\""
          "EMU_OBJS_DIR=\"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\"")

if(LINUX_X86_64 OR LINUX_AARCH64)
  add_subdirectory(third_party/libaom)
endif()

add_subdirectory(third_party/libvpx)
add_subdirectory(third_party/jsoncpp)
add_subdirectory(third_party/pffft)
add_subdirectory(third_party/opus)
add_subdirectory(third_party/rnnoise)
add_subdirectory(third_party/usrsctp)
add_subdirectory(third_party/libsrtp)
add_subdirectory(third_party/catapult/tracing)
add_subdirectory(third_party/libyuv)

if(DARWIN_X86_64)
  include(darwin_x86_64.cmake)
elseif(DARWIN_AARCH64)
  include(darwin_aarch64.cmake)
elseif(LINUX_X86_64)
  # We are too aggressive for the webrtc source tree.
  add_compile_options("-Wno-final-dtor-non-final-class")
  include(linux_x86_64.cmake)
elseif(LINUX_AARCH64)
  # We are too aggressive for the webrtc source tree.
  add_compile_options("-Wno-final-dtor-non-final-class")
  include(linux_aarch64.cmake)
elseif(WINDOWS_MSVC_X86_64)
  include(windows_x86_64.cmake)
endif()

# We manually register the tests, as some of time can be flaky..
set(WEBRTC_TESTS
    webrtc_common_audio_common_audio_unittests
    webrtc_common_video_common_video_unittests
    webrtc_media_rtc_media_unittests
    webrtc_modules_audio_coding_audio_decoder_unittests
    # webrtc_pc_peerconnection_unittests  These are very expensive, and have
    # some timing flakiness.
    webrtc_pc_rtc_pc_unittests)

if(OPTION_ASAN)
  message("Disabling webrtc tests in asan, they are too expensive/slow to run.")
  set(WEBRTC_TESTS "")
endif()

foreach(WEBRTC_TEST ${WEBRTC_TESTS})
  add_test(NAME ${WEBRTC_TEST} COMMAND $<TARGET_FILE:${WEBRTC_TEST}>
           WORKING_DIRECTORY $<TARGET_FILE_DIR:${WEBRTC_TEST}>)

  android_install_as_debug_info(${WEBRTC_TEST})
  android_add_default_test_properties(${WEBRTC_TEST})
endforeach()
