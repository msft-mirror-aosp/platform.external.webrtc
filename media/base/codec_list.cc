/*
 *  Copyright (c) 2024 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "media/base/codec_list.h"

#include <cstddef>
#include <map>

#include "media/base/codec.h"
#include "media/base/media_constants.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/string_encode.h"

namespace cricket {

using webrtc::RTCError;
using webrtc::RTCErrorOr;
using webrtc::RTCErrorType;

namespace {

RTCError CheckInputConsistency(const std::vector<Codec>& codecs) {
  std::map<int, int> pt_to_index;
  // Create a map of payload type to index, and ensure
  // that there are no duplicates.
  for (size_t i = 0; i < codecs.size(); i++) {
    const Codec& codec = codecs[i];
    if (codec.id != Codec::kIdNotSet) {
      // Not true - the test PeerConnectionMediaTest.RedFmtpPayloadMixed
      // fails this check. In that case, the duplicates are identical.
      // TODO: https://issues.webrtc.org/384756621 - fix test and enable check.
      // RTC_DCHECK(pt_to_index.count(codec.id) == 0);
      if (pt_to_index.count(codec.id) != 0) {
        RTC_LOG(LS_WARNING) << "Surprising condition: Two codecs on same PT. "
                            << "First: " << codecs[pt_to_index[codec.id]]
                            << " Second: " << codec;
        // Skip this codec in the map, and go on.
        continue;
      }
      pt_to_index.insert({codec.id, i});
    }
  }
  for (const Codec& codec : codecs) {
    switch (codec.GetResiliencyType()) {
      case Codec::ResiliencyType::kRed:
        // Check that the target codec exists
        break;
      case Codec::ResiliencyType::kRtx: {
        // Check that the target codec exists
        const auto apt_it = codec.params.find(kCodecParamAssociatedPayloadType);
        // Not true - there's a test that deliberately injects a wrong
        // RTX codec (MediaSessionDescriptionFactoryTest.RtxWithoutApt)
        // TODO: https://issues.webrtc.org/384756622 - reject codec earlier and
        // enable check. RTC_DCHECK(apt_it != codec.params.end()); Until that is
        // fixed:
        if (apt_it == codec.params.end()) {
          RTC_LOG(LS_WARNING) << "Surprising condition: RTX codec without"
                              << " apt parameter: " << codec;
          break;
        }
        int associated_pt;
        if (!(rtc::FromString(apt_it->second, &associated_pt))) {
          LOG_AND_RETURN_ERROR(RTCErrorType::INVALID_PARAMETER,
                               "Non-numeric argument to rtx apt parameter");
        }
        if (pt_to_index.count(associated_pt) != 1) {
          RTC_LOG(LS_WARNING)
              << "Surprising condition: RTX codec APT not found: " << codec
              << " points to a PT that occurs "
              << pt_to_index.count(associated_pt) << " times";
          LOG_AND_RETURN_ERROR(
              RTCErrorType::INVALID_PARAMETER,
              "PT pointed to by rtx apt parameter does not exist");
        }
        // const Codec& referred_codec = codecs[pt_to_index[associated_pt]];
        // Not true:
        // RTC_DCHECK(referred_codec.type == Codec::Type::kVideo);
        // Not true:
        // RTC_DCHECK(referred_codec.GetResiliencyType() ==
        // Codec::ResiliencyType::kNone);
        // TODO: https://issues.webrtc.org/384756623 - figure out if this is
        // expected or not.
        break;
      }
      case Codec::ResiliencyType::kNone:
        break;  // nothing to see here
      default:
        break;  // don't know what to check yet
    }
  }
  return RTCError::OK();
}

}  // namespace

// static
RTCErrorOr<CodecList> CodecList::Create(const std::vector<Codec>& codecs) {
  RTCError error = CheckInputConsistency(codecs);
  if (!error.ok()) {
    return error;
  }
  return CodecList(codecs);
}

void CodecList::CheckConsistency() {
  RTC_DCHECK(CheckInputConsistency(codecs_).ok());
}

}  // namespace cricket
