/*
 *  Copyright (c) 2004 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "media/base/codec.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/strings/match.h"
#include "api/audio_codecs/audio_format.h"
#include "api/media_types.h"
#include "api/rtp_parameters.h"
#include "api/video_codecs/h264_profile_level_id.h"
#include "api/video_codecs/sdp_video_format.h"
#ifdef RTC_ENABLE_H265
#include "api/video_codecs/h265_profile_tier_level.h"  // IWYU pragma: keep
#endif
#include "media/base/codec_comparators.h"
#include "media/base/media_constants.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/string_encode.h"
#include "rtc_base/strings/string_builder.h"

namespace cricket {

FeedbackParams::FeedbackParams() = default;
FeedbackParams::~FeedbackParams() = default;

bool FeedbackParam::operator==(const FeedbackParam& other) const {
  return absl::EqualsIgnoreCase(other.id(), id()) &&
         absl::EqualsIgnoreCase(other.param(), param());
}

bool FeedbackParams::operator==(const FeedbackParams& other) const {
  return params_ == other.params_;
}

bool FeedbackParams::Has(const FeedbackParam& param) const {
  return absl::c_linear_search(params_, param);
}

void FeedbackParams::Add(const FeedbackParam& param) {
  if (param.id().empty()) {
    return;
  }
  if (Has(param)) {
    // Param already in `this`.
    return;
  }
  params_.push_back(param);
  RTC_CHECK(!HasDuplicateEntries());
}

bool FeedbackParams::Remove(const FeedbackParam& param) {
  if (!Has(param)) {
    return false;
  }
  params_.erase(std::remove(params_.begin(), params_.end(), param),
                params_.end());
  return true;
}

void FeedbackParams::Intersect(const FeedbackParams& from) {
  std::vector<FeedbackParam>::iterator iter_to = params_.begin();
  while (iter_to != params_.end()) {
    if (!from.Has(*iter_to)) {
      iter_to = params_.erase(iter_to);
    } else {
      ++iter_to;
    }
  }
}

bool FeedbackParams::HasDuplicateEntries() const {
  for (std::vector<FeedbackParam>::const_iterator iter = params_.begin();
       iter != params_.end(); ++iter) {
    for (std::vector<FeedbackParam>::const_iterator found = iter + 1;
         found != params_.end(); ++found) {
      if (*found == *iter) {
        return true;
      }
    }
  }
  return false;
}

Codec::Codec(Type type, int id, const std::string& name, int clockrate)
    : Codec(type, id, name, clockrate, 0) {}
Codec::Codec(Type type,
             int id,
             const std::string& name,
             int clockrate,
             size_t channels)
    : type(type),
      id(id),
      name(name),
      clockrate(clockrate),
      bitrate(0),
      channels(channels) {}

Codec::Codec(Type type) : Codec(type, kIdNotSet, "", 0) {}

Codec::Codec(const webrtc::SdpAudioFormat& c)
    : Codec(Type::kAudio, kIdNotSet, c.name, c.clockrate_hz, c.num_channels) {
  params = c.parameters;
}

Codec::Codec(const webrtc::SdpVideoFormat& c)
    : Codec(Type::kVideo, kIdNotSet, c.name, kVideoCodecClockrate) {
  params = c.parameters;
  scalability_modes = c.scalability_modes;
}

Codec::Codec(const Codec& c) = default;
Codec::Codec(Codec&& c) = default;
Codec::~Codec() = default;
Codec& Codec::operator=(const Codec& c) = default;
Codec& Codec::operator=(Codec&& c) = default;

bool Codec::operator==(const Codec& c) const {
  return type == c.type && this->id == c.id &&  // id is reserved in objective-c
         name == c.name && clockrate == c.clockrate && params == c.params &&
         feedback_params == c.feedback_params &&
         (type == Type::kAudio
              ? (bitrate == c.bitrate && channels == c.channels)
              : (packetization == c.packetization));
}

bool Codec::Matches(const Codec& codec) const {
  return webrtc::MatchesWithCodecRules(*this, codec);
}

bool Codec::MatchesRtpCodec(const webrtc::RtpCodec& codec_capability) const {
  webrtc::RtpCodecParameters codec_parameters = ToCodecParameters();

  return codec_parameters.name == codec_capability.name &&
         codec_parameters.kind == codec_capability.kind &&
         codec_parameters.num_channels == codec_capability.num_channels &&
         codec_parameters.clock_rate == codec_capability.clock_rate &&
         (codec_parameters.name == cricket::kRtxCodecName ||
          codec_parameters.parameters == codec_capability.parameters);
}

bool Codec::GetParam(const std::string& name, std::string* out) const {
  webrtc::CodecParameterMap::const_iterator iter = params.find(name);
  if (iter == params.end())
    return false;
  *out = iter->second;
  return true;
}

bool Codec::GetParam(const std::string& name, int* out) const {
  webrtc::CodecParameterMap::const_iterator iter = params.find(name);
  if (iter == params.end())
    return false;
  return rtc::FromString(iter->second, out);
}

void Codec::SetParam(const std::string& name, const std::string& value) {
  params[name] = value;
}

void Codec::SetParam(const std::string& name, int value) {
  params[name] = rtc::ToString(value);
}

bool Codec::RemoveParam(const std::string& name) {
  return params.erase(name) == 1;
}

void Codec::AddFeedbackParam(const FeedbackParam& param) {
  feedback_params.Add(param);
}

bool Codec::HasFeedbackParam(const FeedbackParam& param) const {
  return feedback_params.Has(param);
}

void Codec::IntersectFeedbackParams(const Codec& other) {
  feedback_params.Intersect(other.feedback_params);
}

webrtc::RtpCodecParameters Codec::ToCodecParameters() const {
  webrtc::RtpCodecParameters codec_params;
  codec_params.payload_type = id;
  codec_params.name = name;
  codec_params.clock_rate = clockrate;
  codec_params.parameters.insert(params.begin(), params.end());

  switch (type) {
    case Type::kAudio: {
      codec_params.num_channels = static_cast<int>(channels);
      codec_params.kind = MEDIA_TYPE_AUDIO;
      break;
    }
    case Type::kVideo: {
      codec_params.kind = MEDIA_TYPE_VIDEO;
      break;
    }
  }

  return codec_params;
}

bool Codec::IsMediaCodec() const {
  return !IsResiliencyCodec() &&
         !absl::EqualsIgnoreCase(name, kComfortNoiseCodecName);
}

bool Codec::IsResiliencyCodec() const {
  return GetResiliencyType() != ResiliencyType::kNone;
}

Codec::ResiliencyType Codec::GetResiliencyType() const {
  if (absl::EqualsIgnoreCase(name, kRedCodecName)) {
    return ResiliencyType::kRed;
  }
  if (absl::EqualsIgnoreCase(name, kUlpfecCodecName)) {
    return ResiliencyType::kUlpfec;
  }
  if (absl::EqualsIgnoreCase(name, kFlexfecCodecName)) {
    return ResiliencyType::kFlexfec;
  }
  if (absl::EqualsIgnoreCase(name, kRtxCodecName)) {
    return ResiliencyType::kRtx;
  }
  return ResiliencyType::kNone;
}

bool Codec::ValidateCodecFormat() const {
  if (id < 0 || id > 127) {
    RTC_LOG(LS_ERROR) << "Codec with invalid payload type: " << ToString();
    return false;
  }
  if (IsResiliencyCodec()) {
    return true;
  }

  int min_bitrate = -1;
  int max_bitrate = -1;
  if (GetParam(kCodecParamMinBitrate, &min_bitrate) &&
      GetParam(kCodecParamMaxBitrate, &max_bitrate)) {
    if (max_bitrate < min_bitrate) {
      RTC_LOG(LS_ERROR) << "Codec with max < min bitrate: " << ToString();
      return false;
    }
  }
  return true;
}

std::string Codec::ToString() const {
  char buf[256];

  rtc::SimpleStringBuilder sb(buf);
  switch (type) {
    case Type::kAudio: {
      sb << "AudioCodec[" << id << ":" << name << ":" << clockrate << ":"
         << bitrate << ":" << channels << "]";
      break;
    }
    case Type::kVideo: {
      sb << "VideoCodec[" << id << ":" << name;
      if (packetization.has_value()) {
        sb << ":" << *packetization;
      }
      sb << "]";
      break;
    }
  }
  return sb.str();
}

Codec CreateAudioRtxCodec(int rtx_payload_type, int associated_payload_type) {
  Codec rtx_codec = CreateAudioCodec(rtx_payload_type, kRtxCodecName, 0, 1);
  rtx_codec.SetParam(kCodecParamAssociatedPayloadType, associated_payload_type);
  return rtx_codec;
}

Codec CreateVideoRtxCodec(int rtx_payload_type, int associated_payload_type) {
  Codec rtx_codec = CreateVideoCodec(rtx_payload_type, kRtxCodecName);
  rtx_codec.SetParam(kCodecParamAssociatedPayloadType, associated_payload_type);
  return rtx_codec;
}

const Codec* FindCodecById(const std::vector<Codec>& codecs, int payload_type) {
  for (const auto& codec : codecs) {
    if (codec.id == payload_type)
      return &codec;
  }
  return nullptr;
}

bool HasLntf(const Codec& codec) {
  return codec.HasFeedbackParam(
      FeedbackParam(kRtcpFbParamLntf, kParamValueEmpty));
}

bool HasNack(const Codec& codec) {
  return codec.HasFeedbackParam(
      FeedbackParam(kRtcpFbParamNack, kParamValueEmpty));
}

bool HasRemb(const Codec& codec) {
  return codec.HasFeedbackParam(
      FeedbackParam(kRtcpFbParamRemb, kParamValueEmpty));
}

bool HasRrtr(const Codec& codec) {
  return codec.HasFeedbackParam(
      FeedbackParam(kRtcpFbParamRrtr, kParamValueEmpty));
}

const Codec* FindMatchingVideoCodec(const std::vector<Codec>& supported_codecs,
                                    const Codec& codec) {
  webrtc::SdpVideoFormat sdp_video_format{codec.name, codec.params};
  for (const Codec& supported_codec : supported_codecs) {
    if (sdp_video_format.IsSameCodec(
            {supported_codec.name, supported_codec.params})) {
      return &supported_codec;
    }
  }
  return nullptr;
}

std::vector<const Codec*> FindAllMatchingCodecs(
    const std::vector<Codec>& supported_codecs,
    const Codec& codec) {
  std::vector<const Codec*> result;
  webrtc::SdpVideoFormat sdp(codec.name, codec.params);
  for (const Codec& supported_codec : supported_codecs) {
    if (sdp.IsSameCodec({supported_codec.name, supported_codec.params})) {
      result.push_back(&supported_codec);
    }
  }
  return result;
}

// If a decoder supports any H264 profile, it is implicitly assumed to also
// support constrained base line even though it's not explicitly listed.
void AddH264ConstrainedBaselineProfileToSupportedFormats(
    std::vector<webrtc::SdpVideoFormat>* supported_formats) {
  std::vector<webrtc::SdpVideoFormat> cbr_supported_formats;

  // For any H264 supported profile, add the corresponding constrained baseline
  // profile.
  for (auto it = supported_formats->cbegin(); it != supported_formats->cend();
       ++it) {
    if (it->name == cricket::kH264CodecName) {
      const std::optional<webrtc::H264ProfileLevelId> profile_level_id =
          webrtc::ParseSdpForH264ProfileLevelId(it->parameters);
      if (profile_level_id &&
          profile_level_id->profile !=
              webrtc::H264Profile::kProfileConstrainedBaseline) {
        webrtc::SdpVideoFormat cbp_format = *it;
        webrtc::H264ProfileLevelId cbp_profile = *profile_level_id;
        cbp_profile.profile = webrtc::H264Profile::kProfileConstrainedBaseline;
        cbp_format.parameters[cricket::kH264FmtpProfileLevelId] =
            *webrtc::H264ProfileLevelIdToString(cbp_profile);
        cbr_supported_formats.push_back(cbp_format);
      }
    }
  }

  size_t original_size = supported_formats->size();
  // ...if it's not already in the list.
  std::copy_if(cbr_supported_formats.begin(), cbr_supported_formats.end(),
               std::back_inserter(*supported_formats),
               [supported_formats](const webrtc::SdpVideoFormat& format) {
                 return !format.IsCodecInList(*supported_formats);
               });

  if (supported_formats->size() > original_size) {
    RTC_LOG(LS_WARNING) << "Explicitly added H264 constrained baseline to list "
                           "of supported formats.";
  }
}

Codec CreateAudioCodec(int id,
                       const std::string& name,
                       int clockrate,
                       size_t channels) {
  return Codec(Codec::Type::kAudio, id, name, clockrate, channels);
}

Codec CreateAudioCodec(const webrtc::SdpAudioFormat& c) {
  return Codec(c);
}

Codec CreateVideoCodec(const std::string& name) {
  return CreateVideoCodec(Codec::kIdNotSet, name);
}

Codec CreateVideoCodec(int id, const std::string& name) {
  Codec c(Codec::Type::kVideo, id, name, kVideoCodecClockrate);
  if (absl::EqualsIgnoreCase(kH264CodecName, name)) {
    // This default is set for all H.264 codecs created because
    // that was the default before packetization mode support was added.
    // TODO(hta): Move this to the places that create VideoCodecs from
    // SDP or from knowledge of implementation capabilities.
    c.SetParam(kH264FmtpPacketizationMode, "1");
  }
  return c;
}

Codec CreateVideoCodec(const webrtc::SdpVideoFormat& c) {
  return Codec(c);
}

Codec CreateVideoCodec(int id, const webrtc::SdpVideoFormat& sdp) {
  Codec c = CreateVideoCodec(sdp);
  c.id = id;
  return c;
}

}  // namespace cricket
