android_add_library(
  TARGET
  crc32c
  LICENSE
  "BSD-3-Clause"
  SRC
  src/src/crc32c.cc
  src/src/crc32c_portable.cc)
target_include_directories(crc32c PUBLIC config src/include)
target_compile_definitions(crc32c PRIVATE BYTE_ORDER_BIG_ENDIAN=0
                                          CRC32C_TESTS_BUILT_WITH_GLOG=0)
if(LINUX_AARCH64 OR DARWIN_AARCH64)
  target_compile_definitions(crc32c PRIVATE HAVE_MM_PREFETCH=0 HAVE_SSE42=0)
  target_sources(crc32c PRIVATE src/src/crc32c_arm64.cc)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_definitions(crc32c PRIVATE HAVE_ARM64_CRC32C=0)
    target_compile_options(
      crc32c
      PRIVATE "-march=armv8-a"
              # Some builds set -march to a different value from the above. The
              # specific feature flags below enable the instructions we need in
              # these cases. See https://crbug.com/934016 for example.
              "-Xclang -target-feature"
              "-Xclang +crc"
              "-Xclang -target-feature"
              "-Xclang +crypto")
  else()
    target_compile_options(crc32c PRIVATE "-march=armv8-a+crc+crypto")
    target_compile_definitions(crc32c PRIVATE HAVE_ARM64_CRC32C=1)
  endif()
else()
  target_compile_definitions(crc32c PRIVATE HAVE_MM_PREFETCH=1 HAVE_SSE42=1)
  target_compile_definitions(crc32c PRIVATE HAVE_ARM64_CRC32C=0)
  target_sources(crc32c PRIVATE src/src/crc32c_sse42.cc)
  if(WINDOWS_MSVC_X86_64)
    target_compile_options(crc32c PRIVATE -mavx)
  else()
    target_compile_options(crc32c PRIVATE -msse4.2)
  endif()
endif()

target_compile_definitions(crc32c PRIVATE HAVE_BUILTIN_PREFETCH=1)

if(LINUX_AARCH64 OR LINUX_X86_64)
  target_compile_definitions(crc32c PRIVATE HAVE_STRONG_GETAUXVAL=1
                                            HAVE_WEAK_GETAUXVAL=1)

else()
  target_compile_definitions(crc32c PRIVATE HAVE_STRONG_GETAUXVAL=0
                                            HAVE_WEAK_GETAUXVAL=0)
endif()

android_add_test(
  TARGET
  crc32c_tests
  SRC
  "src/src/crc32c_arm64_unittest.cc"
  "src/src/crc32c_extend_unittests.h"
  "src/src/crc32c_portable_unittest.cc"
  "src/src/crc32c_prefetch_unittest.cc"
  "src/src/crc32c_read_le_unittest.cc"
  "src/src/crc32c_round_up_unittest.cc"
  "src/src/crc32c_sse42_unittest.cc"
  "src/src/crc32c_unittest.cc")

target_link_libraries(crc32c_tests PRIVATE crc32c gmock_main)
