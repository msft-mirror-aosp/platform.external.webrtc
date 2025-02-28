/*
 *  Copyright 2004 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// Types and classes used in media session descriptions.

#ifndef PC_MEDIA_SESSION_H_
#define PC_MEDIA_SESSION_H_

#include <memory>
#include <string>
#include <vector>

#include "api/media_types.h"
#include "api/rtc_error.h"
#include "call/payload_type.h"
#include "media/base/codec.h"
#include "media/base/stream_params.h"
#include "p2p/base/ice_credentials_iterator.h"
#include "p2p/base/transport_description.h"
#include "p2p/base/transport_description_factory.h"
#include "p2p/base/transport_info.h"
#include "pc/codec_vendor.h"
#include "pc/media_options.h"
#include "pc/session_description.h"
#include "rtc_base/memory/always_valid_pointer.h"
#include "rtc_base/unique_id_generator.h"

namespace webrtc {

// Forward declaration due to circular dependecy.
class ConnectionContext;

}  // namespace webrtc

namespace cricket {

class MediaEngineInterface;

// Creates media session descriptions according to the supplied codecs and
// other fields, as well as the supplied per-call options.
// When creating answers, performs the appropriate negotiation
// of the various fields to determine the proper result.
class MediaSessionDescriptionFactory {
 public:
  // This constructor automatically sets up the factory to get its configuration
  // from the specified MediaEngine (when provided).
  // The TransportDescriptionFactory, the UniqueRandomIdGenerator, and the
  // PayloadTypeSuggester are not owned by MediaSessionDescriptionFactory, so
  // they must be kept alive by the user of this class.
  MediaSessionDescriptionFactory(cricket::MediaEngineInterface* media_engine,
                                 bool rtx_enabled,
                                 rtc::UniqueRandomIdGenerator* ssrc_generator,
                                 const TransportDescriptionFactory* factory,
                                 webrtc::PayloadTypeSuggester* pt_suggester);

  RtpHeaderExtensions filtered_rtp_header_extensions(
      RtpHeaderExtensions extensions) const;

  void set_enable_encrypted_rtp_header_extensions(bool enable) {
    enable_encrypted_rtp_header_extensions_ = enable;
  }

  void set_is_unified_plan(bool is_unified_plan) {
    is_unified_plan_ = is_unified_plan;
  }

  webrtc::RTCErrorOr<std::unique_ptr<SessionDescription>> CreateOfferOrError(
      const MediaSessionOptions& options,
      const SessionDescription* current_description) const;
  webrtc::RTCErrorOr<std::unique_ptr<SessionDescription>> CreateAnswerOrError(
      const SessionDescription* offer,
      const MediaSessionOptions& options,
      const SessionDescription* current_description) const;

  CodecVendor* CodecVendorForTesting() { return codec_vendor_.get(); }

 private:
  struct AudioVideoRtpHeaderExtensions {
    RtpHeaderExtensions audio;
    RtpHeaderExtensions video;
  };

  AudioVideoRtpHeaderExtensions GetOfferedRtpHeaderExtensionsWithIds(
      const std::vector<const ContentInfo*>& current_active_contents,
      bool extmap_allow_mixed,
      const std::vector<MediaDescriptionOptions>& media_description_options)
      const;
  webrtc::RTCError AddTransportOffer(
      const std::string& content_name,
      const TransportOptions& transport_options,
      const SessionDescription* current_desc,
      SessionDescription* offer,
      IceCredentialsIterator* ice_credentials) const;

  std::unique_ptr<TransportDescription> CreateTransportAnswer(
      const std::string& content_name,
      const SessionDescription* offer_desc,
      const TransportOptions& transport_options,
      const SessionDescription* current_desc,
      bool require_transport_attributes,
      IceCredentialsIterator* ice_credentials) const;

  webrtc::RTCError AddTransportAnswer(
      const std::string& content_name,
      const TransportDescription& transport_desc,
      SessionDescription* answer_desc) const;

  // Helpers for adding media contents to the SessionDescription.
  webrtc::RTCError AddRtpContentForOffer(
      const MediaDescriptionOptions& media_description_options,
      const MediaSessionOptions& session_options,
      const ContentInfo* current_content,
      const SessionDescription* current_description,
      const RtpHeaderExtensions& header_extensions,
      const CodecList& codecs,
      StreamParamsVec* current_streams,
      SessionDescription* desc,
      IceCredentialsIterator* ice_credentials) const;

  webrtc::RTCError AddDataContentForOffer(
      const MediaDescriptionOptions& media_description_options,
      const MediaSessionOptions& session_options,
      const ContentInfo* current_content,
      const SessionDescription* current_description,
      StreamParamsVec* current_streams,
      SessionDescription* desc,
      IceCredentialsIterator* ice_credentials) const;

  webrtc::RTCError AddUnsupportedContentForOffer(
      const MediaDescriptionOptions& media_description_options,
      const MediaSessionOptions& session_options,
      const ContentInfo* current_content,
      const SessionDescription* current_description,
      SessionDescription* desc,
      IceCredentialsIterator* ice_credentials) const;

  webrtc::RTCError AddRtpContentForAnswer(
      const MediaDescriptionOptions& media_description_options,
      const MediaSessionOptions& session_options,
      const ContentInfo* offer_content,
      const SessionDescription* offer_description,
      const ContentInfo* current_content,
      const SessionDescription* current_description,
      const TransportInfo* bundle_transport,
      const CodecList& codecs,
      const RtpHeaderExtensions& header_extensions,
      StreamParamsVec* current_streams,
      SessionDescription* answer,
      IceCredentialsIterator* ice_credentials) const;

  webrtc::RTCError AddDataContentForAnswer(
      const MediaDescriptionOptions& media_description_options,
      const MediaSessionOptions& session_options,
      const ContentInfo* offer_content,
      const SessionDescription* offer_description,
      const ContentInfo* current_content,
      const SessionDescription* current_description,
      const TransportInfo* bundle_transport,
      StreamParamsVec* current_streams,
      SessionDescription* answer,
      IceCredentialsIterator* ice_credentials) const;

  webrtc::RTCError AddUnsupportedContentForAnswer(
      const MediaDescriptionOptions& media_description_options,
      const MediaSessionOptions& session_options,
      const ContentInfo* offer_content,
      const SessionDescription* offer_description,
      const ContentInfo* current_content,
      const SessionDescription* current_description,
      const TransportInfo* bundle_transport,
      SessionDescription* answer,
      IceCredentialsIterator* ice_credentials) const;

  rtc::UniqueRandomIdGenerator* ssrc_generator() const {
    return ssrc_generator_.get();
  }

  bool is_unified_plan_ = false;
  // This object may or may not be owned by this class.
  webrtc::AlwaysValidPointer<rtc::UniqueRandomIdGenerator> const
      ssrc_generator_;
  bool enable_encrypted_rtp_header_extensions_ = true;
  const TransportDescriptionFactory* transport_desc_factory_;
  // Payoad type tracker interface. Must live longer than this object.
  webrtc::PayloadTypeSuggester* pt_suggester_;
  bool payload_types_in_transport_trial_enabled_;
  std::unique_ptr<CodecVendor> codec_vendor_;
};

// Convenience functions.
bool IsMediaContent(const ContentInfo* content);
bool IsAudioContent(const ContentInfo* content);
bool IsVideoContent(const ContentInfo* content);
bool IsDataContent(const ContentInfo* content);
bool IsUnsupportedContent(const ContentInfo* content);
const ContentInfo* GetFirstMediaContent(const ContentInfos& contents,
                                        MediaType media_type);
const ContentInfo* GetFirstAudioContent(const ContentInfos& contents);
const ContentInfo* GetFirstVideoContent(const ContentInfos& contents);
const ContentInfo* GetFirstDataContent(const ContentInfos& contents);
const ContentInfo* GetFirstMediaContent(const SessionDescription* sdesc,
                                        MediaType media_type);
const ContentInfo* GetFirstAudioContent(const SessionDescription* sdesc);
const ContentInfo* GetFirstVideoContent(const SessionDescription* sdesc);
const ContentInfo* GetFirstDataContent(const SessionDescription* sdesc);
const AudioContentDescription* GetFirstAudioContentDescription(
    const SessionDescription* sdesc);
const VideoContentDescription* GetFirstVideoContentDescription(
    const SessionDescription* sdesc);
const SctpDataContentDescription* GetFirstSctpDataContentDescription(
    const SessionDescription* sdesc);
// Non-const versions of the above functions.
// Useful when modifying an existing description.
ContentInfo* GetFirstMediaContent(ContentInfos* contents, MediaType media_type);
ContentInfo* GetFirstAudioContent(ContentInfos* contents);
ContentInfo* GetFirstVideoContent(ContentInfos* contents);
ContentInfo* GetFirstDataContent(ContentInfos* contents);
ContentInfo* GetFirstMediaContent(SessionDescription* sdesc,
                                  MediaType media_type);
ContentInfo* GetFirstAudioContent(SessionDescription* sdesc);
ContentInfo* GetFirstVideoContent(SessionDescription* sdesc);
ContentInfo* GetFirstDataContent(SessionDescription* sdesc);
AudioContentDescription* GetFirstAudioContentDescription(
    SessionDescription* sdesc);
VideoContentDescription* GetFirstVideoContentDescription(
    SessionDescription* sdesc);
SctpDataContentDescription* GetFirstSctpDataContentDescription(
    SessionDescription* sdesc);

}  // namespace cricket

#endif  // PC_MEDIA_SESSION_H_
