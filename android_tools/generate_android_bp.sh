#!/bin/bash

set -o errexit

if [[ "${ANDROID_BUILD_TOP}" == "" ]]; then
  echo "Run source build/envsetup.sh && lunch to be able to format Android.bp"
  exit 2
fi

DIR=$(dirname $0)

PYTHONHASHSEED=31 "${DIR}"/generate_bp.py \
        "${DIR}"/project_x64.json \
        "${DIR}"/project_x86.json \
        "${DIR}"/project_arm64.json \
        "${DIR}"/project_arm.json \
        >"${DIR}"/../Android.bp

bpfmt -w "${DIR}"/../Android.bp

