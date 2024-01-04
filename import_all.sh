#!/bin/bash


if [ $# -ne 2 ]; then
  echo "Exports the google3 webrtc build to cmake that can be consumed by the android emulator"
  echo "Param #1 should point to your bazel directory containig webrtc."
  echo "Param #2 should point to the webrtc source base (chromium)"
  echo "for example:  ./import_all.sh G3_ROOT/google3/third_party/webrtc/files/stable/webrtc ~/webrtc-checkout/src"
  echo
  echo "Make sure to merge in the latest source!"
  echo "i.e 'git merge aosp/upstream-master'"
  echo "Error: This script must be run with two parameters."
  exit 1
fi

function generate_from_build {
    # Invokes the python script that translates all the build files --> cmake
    for platform in Darwin Darwin-aarch64 Linux Windows; do
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
        --platform $platform BUILD . || exit 1

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

function sync_aom {
     # extra for libaom/vpx
    for dep in libaom
    do
        rsync -rav  \
        --exclude '**/.git' \
        --include '*.inc' \
        --include 'README*' \
        --include '*.gn' \
        --include '*.h' --include '*.cc' --include '*.c' --include '*.cpp' \
        --include '*.asm' --include '*.S' \
        $1/third_party/${dep} third_party/
    done
}

function sync_deps {
    # Synchronizes all the third party dependencies that are needed for
    # a successful compilation.
    for dep in  libvpx jsoncpp pffft opus rnnoise libsrtp libyuv crc32c dav1d
    do
        echo "Syncing $dep"
        rsync -rav  \
        --exclude 'third_party' \
        --exclude '**/.git' \
        --include '*/' \
        --include '*.asm' --include '*.S' --include 'NOTICE' \
        --include 'README*' \
        --include '*.gn' \
        --include '*.proto' --include '*.inl' \
        --include '*.h' --include '*.cc' --include '*.c' --include '*.cpp' \
        --exclude '*' \
        $1/third_party/${dep} third_party/
    done
    # abseil
    rsync -rav  \
    --exclude 'third_party' \
    --exclude 'OWNERS' \
    --include '*/' \
    --include '*.asm' --include '*.S' --include 'NOTICE' \
    --include 'README*' \
    --include '*.proto' --include '*.inc' \
    --include '*.h' --include '*.cc' --include '*.c' --include '*.cpp' \
    --include 'CMakeLists.txt' --include '*.cmake' \
    --delete \
    $1/third_party/abseil-cpp  third_party/

    # catapult
    rsync -rav  \
    --exclude 'third_party' \
    --include '*/' \
    --include '*.asm' --include '*.S' --include 'NOTICE' \
    --include '*.proto' \
    --include '*.h' --include '*.cc' --include '*.c' --include '*.cpp' \
    --include '*.gn' \
    --include 'README*' \
    --exclude '*' \
    --delete \
    $1/third_party/catapult/tracing third_party/catapult

    (cd third_party/libaom && ./import_libaom_srcs.sh $1)
    (cd third_party/libvpx && ./import_libvpx_srcs.sh $1)
}

#
# git merge aosp/upstream-master
generate_from_build $1 && sync_deps $2 && sync_aom $2


