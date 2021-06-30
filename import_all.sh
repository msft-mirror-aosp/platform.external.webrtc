#!/bin/bash
# Converts google3 bazel to cmake.
# $1 should point to your bazel directory containig webrtc.
# for example:  ./transform.sh G3_ROOT/google3/third_party/webrtc/files/stable/webrtc
# #2 should point to the webrtc source base

# Next we rsync the dependencies. We expect you to have a matching gclient

function generate_from_build {
  for platform in Darwin Darwin-aarch64  Linux Windows; do
    echo "Generating cmake for ${platform}"
    python ./import-webrtc.py \
      --target webrtc_api_video_codecs_builtin_video_decoder_factory \
      --target webrtc_api_video_codecs_builtin_video_encoder_factory \
      --target webrtc_api_libjingle_peerconnection_api \
      --target webrtc_pc_peerconnection \
      --target webrtc_api_create_peerconnection_factory \
      --target webrtc_api_audio_codecs_builtin_audio_decoder_factory \
      --target webrtc_api_audio_codecs_builtin_audio_encoder_factory \
      --target webrtc_common_audio_common_audio_unittests \
      --target webrtc_common_video_common_video_unittests \
      --target webrtc_media_rtc_media_unittests \
      --target webrtc_modules_audio_coding_audio_decoder_unittests \
      --target webrtc_pc_peerconnection_unittests \
      --target webrtc_pc_rtc_pc_unittests \
      --root $1 \
      --platform $platform BUILD .

        # These tests require a large set of dependencies, but do not introduce new tests.
        # --target webrtc__rtc_unittests \
        # --target webrtc__video_engine_tests \
        # --target webrtc__webrtc_perf_tests \
        # --target webrtc__webrtc_nonparallel_tests \
        # --target webrtc__voip_unittests \
  done

  # ARM64 is using gcc.. not all the tests compile
  for platform in Linux-aarch64; do
    echo "Generating cmake for ${platform}"
    python ./import-webrtc.py \
      --target webrtc_api_video_codecs_builtin_video_decoder_factory \
      --target webrtc_api_video_codecs_builtin_video_encoder_factory \
      --target webrtc_api_libjingle_peerconnection_api \
      --target webrtc_pc_peerconnection \
      --target webrtc_api_create_peerconnection_factory \
      --target webrtc_api_audio_codecs_builtin_audio_decoder_factory \
      --target webrtc_api_audio_codecs_builtin_audio_encoder_factory \
      --target webrtc_common_audio_common_audio_unittests \
      --target webrtc_modules_audio_coding_audio_decoder_unittests \
      --root $1 \
      --platform $platform BUILD .
  done
}

function sync_deps {
  for dep in libaom libvpx jsoncpp pffft opus rnnoise usrsctp libsrtp libyuv crc32c
  do
    rsync -rav  \
      --exclude 'third_party' \
      --include '*/' \
      --include '*.asm' --include '*.S' --include 'NOTICE' \
      --include '*.proto' --include '*.inl' \
      --include '*.h' --include '*.cc' --include '*.c' --include '*.cpp' \
      --exclude '*' \
      $1/third_party/${dep} third_party/
    done
  # extra for libaom/vpx
  for dep in libaom libvpx
  do
    rsync -rav  \
      --include='**/' --include='**/x86inc/**' --include='**/vector/**' \
      --include='**/fastfeat/**' \
      --exclude='*' \
      --include '*.asm' --include '*.S' \
      --exclude '*' \
      $1/third_party/${dep} third_party/
  done
  # abseil
  rsync -rav  \
    --exclude 'third_party' \
    --include '*/' \
    --include '*.asm' --include '*.S' --include 'NOTICE' \
    --include '*.proto' --include '*.inc' \
    --include '*.h' --include '*.cc' --include '*.c' --include '*.cpp' \
    --include 'CMakeLists.txt' --include '*.cmake' \
    --exclude '*' \
    --delete \
      $1/third_party/abseil-cpp  third_party/

  # catapult
  rsync -rav  \
    --exclude 'third_party' \
    --include '*/' \
    --include '*.asm' --include '*.S' --include 'NOTICE' \
    --include '*.proto' \
    --include '*.h' --include '*.cc' --include '*.c' --include '*.cpp' \
    --exclude '*' \
    $1/third_party/catapult/tracing third_party/catapult
  }

git merge aosp/upstream-master
generate_from_build $1
sync_deps $2

