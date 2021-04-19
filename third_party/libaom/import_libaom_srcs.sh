#!/bin/bash
# $1 should point to the webrtc source root
sed 's/^aom_/set(aom_/g;s/\[/ /g;s/"\/\/third_party\/libaom\/source/"${AOM_ROOT}/g;s/\]/)/g;s/,/ /g;s/=/ /g;s/".*h"/ /g' $1/third_party/libaom/libaom_srcs.gni > libaom_src.cmake
