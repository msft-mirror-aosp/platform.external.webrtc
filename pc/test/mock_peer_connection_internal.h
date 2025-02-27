/*
 *  Copyright 2022 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef PC_TEST_MOCK_PEER_CONNECTION_INTERNAL_H_
#define PC_TEST_MOCK_PEER_CONNECTION_INTERNAL_H_

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "api/adaptation/resource.h"
#include "api/audio/audio_device.h"
#include "api/candidate.h"
#include "api/crypto/crypto_options.h"
#include "api/data_channel_interface.h"
#include "api/dtls_transport_interface.h"
#include "api/field_trials_view.h"
#include "api/jsep.h"
#include "api/media_stream_interface.h"
#include "api/media_types.h"
#include "api/peer_connection_interface.h"
#include "api/rtc_error.h"
#include "api/rtc_event_log_output.h"
#include "api/rtp_parameters.h"
#include "api/rtp_receiver_interface.h"
#include "api/rtp_sender_interface.h"
#include "api/rtp_transceiver_interface.h"
#include "api/scoped_refptr.h"
#include "api/sctp_transport_interface.h"
#include "api/set_remote_description_observer_interface.h"
#include "api/stats/rtc_stats_collector_callback.h"
#include "api/transport/bandwidth_estimation_settings.h"
#include "api/transport/bitrate_settings.h"
#include "api/transport/network_control.h"
#include "call/call.h"
#include "call/payload_type_picker.h"
#include "p2p/base/port.h"
#include "p2p/base/port_allocator.h"
#include "pc/data_channel_utils.h"
#include "pc/jsep_transport_controller.h"
#include "pc/peer_connection_internal.h"
#include "pc/peer_connection_message_handler.h"
#include "pc/rtp_transceiver.h"
#include "pc/rtp_transmission_manager.h"
#include "pc/session_description.h"
#include "pc/transport_stats.h"
#include "pc/usage_pattern.h"
#include "rtc_base/rtc_certificate.h"
#include "rtc_base/ssl_certificate.h"
#include "rtc_base/ssl_stream_adapter.h"
#include "rtc_base/thread.h"
#include "test/gmock.h"

namespace webrtc {

class MockPeerConnectionInternal : public PeerConnectionInternal {
 public:
  MockPeerConnectionInternal() {}
  ~MockPeerConnectionInternal() = default;
  // PeerConnectionInterface
  MOCK_METHOD(rtc::scoped_refptr<StreamCollectionInterface>,
              local_streams,
              (),
              (override));
  MOCK_METHOD(rtc::scoped_refptr<StreamCollectionInterface>,
              remote_streams,
              (),
              (override));
  MOCK_METHOD(bool, AddStream, (MediaStreamInterface*), (override));
  MOCK_METHOD(void, RemoveStream, (MediaStreamInterface*), (override));
  MOCK_METHOD(RTCErrorOr<rtc::scoped_refptr<RtpSenderInterface>>,
              AddTrack,
              (rtc::scoped_refptr<MediaStreamTrackInterface>,
               const std::vector<std::string>&),
              (override));
  MOCK_METHOD(RTCErrorOr<rtc::scoped_refptr<RtpSenderInterface>>,
              AddTrack,
              (rtc::scoped_refptr<MediaStreamTrackInterface>,
               const std::vector<std::string>&,
               const std::vector<RtpEncodingParameters>&),
              (override));
  MOCK_METHOD(RTCError,
              RemoveTrackOrError,
              (rtc::scoped_refptr<RtpSenderInterface>),
              (override));
  MOCK_METHOD(RTCErrorOr<rtc::scoped_refptr<RtpTransceiverInterface>>,
              AddTransceiver,
              (rtc::scoped_refptr<MediaStreamTrackInterface>),
              (override));
  MOCK_METHOD(RTCErrorOr<rtc::scoped_refptr<RtpTransceiverInterface>>,
              AddTransceiver,
              (rtc::scoped_refptr<MediaStreamTrackInterface>,
               const RtpTransceiverInit&),
              (override));
  MOCK_METHOD(RTCErrorOr<rtc::scoped_refptr<RtpTransceiverInterface>>,
              AddTransceiver,
              (cricket::MediaType),
              (override));
  MOCK_METHOD(RTCErrorOr<rtc::scoped_refptr<RtpTransceiverInterface>>,
              AddTransceiver,
              (cricket::MediaType, const RtpTransceiverInit&),
              (override));
  MOCK_METHOD(rtc::scoped_refptr<RtpSenderInterface>,
              CreateSender,
              (const std::string&, const std::string&),
              (override));
  MOCK_METHOD(std::vector<rtc::scoped_refptr<RtpSenderInterface>>,
              GetSenders,
              (),
              (const, override));
  MOCK_METHOD(std::vector<rtc::scoped_refptr<RtpReceiverInterface>>,
              GetReceivers,
              (),
              (const, override));
  MOCK_METHOD(std::vector<rtc::scoped_refptr<RtpTransceiverInterface>>,
              GetTransceivers,
              (),
              (const, override));
  MOCK_METHOD(bool,
              GetStats,
              (StatsObserver*, MediaStreamTrackInterface*, StatsOutputLevel),
              (override));
  MOCK_METHOD(void, GetStats, (RTCStatsCollectorCallback*), (override));
  MOCK_METHOD(void,
              GetStats,
              (rtc::scoped_refptr<RtpSenderInterface>,
               rtc::scoped_refptr<RTCStatsCollectorCallback>),
              (override));
  MOCK_METHOD(void,
              GetStats,
              (rtc::scoped_refptr<RtpReceiverInterface>,
               rtc::scoped_refptr<RTCStatsCollectorCallback>),
              (override));
  MOCK_METHOD(void, ClearStatsCache, (), (override));
  MOCK_METHOD(RTCErrorOr<rtc::scoped_refptr<DataChannelInterface>>,
              CreateDataChannelOrError,
              (const std::string&, const DataChannelInit*),
              (override));
  MOCK_METHOD(SessionDescriptionInterface*,
              local_description,
              (),
              (const, override));
  MOCK_METHOD(SessionDescriptionInterface*,
              remote_description,
              (),
              (const, override));
  MOCK_METHOD(SessionDescriptionInterface*,
              current_local_description,
              (),
              (const, override));
  MOCK_METHOD(SessionDescriptionInterface*,
              current_remote_description,
              (),
              (const, override));
  MOCK_METHOD(SessionDescriptionInterface*,
              pending_local_description,
              (),
              (const, override));
  MOCK_METHOD(SessionDescriptionInterface*,
              pending_remote_description,
              (),
              (const, override));
  MOCK_METHOD(void, RestartIce, (), (override));
  MOCK_METHOD(void,
              CreateOffer,
              (CreateSessionDescriptionObserver*, const RTCOfferAnswerOptions&),
              (override));
  MOCK_METHOD(void,
              CreateAnswer,
              (CreateSessionDescriptionObserver*, const RTCOfferAnswerOptions&),
              (override));

  MOCK_METHOD(void,
              SetLocalDescription,
              (SetSessionDescriptionObserver*, SessionDescriptionInterface*),
              (override));
  MOCK_METHOD(void,
              SetRemoteDescription,
              (SetSessionDescriptionObserver*, SessionDescriptionInterface*),
              (override));
  MOCK_METHOD(void,
              SetRemoteDescription,
              (std::unique_ptr<SessionDescriptionInterface>,
               rtc::scoped_refptr<SetRemoteDescriptionObserverInterface>),
              (override));
  MOCK_METHOD(bool,
              ShouldFireNegotiationNeededEvent,
              (uint32_t event_id),
              (override));
  MOCK_METHOD(PeerConnectionInterface::RTCConfiguration,
              GetConfiguration,
              (),
              (override));
  MOCK_METHOD(RTCError,
              SetConfiguration,
              (const PeerConnectionInterface::RTCConfiguration&),
              (override));
  MOCK_METHOD(bool,
              AddIceCandidate,
              (const IceCandidateInterface*),
              (override));
  MOCK_METHOD(bool,
              RemoveIceCandidates,
              (const std::vector<cricket::Candidate>&),
              (override));
  MOCK_METHOD(RTCError, SetBitrate, (const BitrateSettings&), (override));
  MOCK_METHOD(void,
              ReconfigureBandwidthEstimation,
              (const BandwidthEstimationSettings&),
              (override));
  MOCK_METHOD(void, SetAudioPlayout, (bool), (override));
  MOCK_METHOD(void, SetAudioRecording, (bool), (override));
  MOCK_METHOD(rtc::scoped_refptr<DtlsTransportInterface>,
              LookupDtlsTransportByMid,
              (const std::string&),
              (override));
  MOCK_METHOD(rtc::scoped_refptr<SctpTransportInterface>,
              GetSctpTransport,
              (),
              (const, override));
  MOCK_METHOD(SignalingState, signaling_state, (), (override));
  MOCK_METHOD(IceConnectionState, ice_connection_state, (), (override));
  MOCK_METHOD(IceConnectionState,
              standardized_ice_connection_state,
              (),
              (override));
  MOCK_METHOD(PeerConnectionState, peer_connection_state, (), (override));
  MOCK_METHOD(IceGatheringState, ice_gathering_state, (), (override));
  MOCK_METHOD(void,
              AddAdaptationResource,
              (rtc::scoped_refptr<Resource>),
              (override));
  MOCK_METHOD(std::optional<bool>, can_trickle_ice_candidates, (), (override));
  MOCK_METHOD(bool,
              StartRtcEventLog,
              (std::unique_ptr<RtcEventLogOutput>, int64_t),
              (override));
  MOCK_METHOD(bool,
              StartRtcEventLog,
              (std::unique_ptr<RtcEventLogOutput>),
              (override));
  MOCK_METHOD(void, StopRtcEventLog, (), (override));
  MOCK_METHOD(void, Close, (), (override));
  MOCK_METHOD(rtc::Thread*, signaling_thread, (), (const, override));

  // PeerConnectionSdpMethods
  MOCK_METHOD(std::string, session_id, (), (const, override));
  MOCK_METHOD(bool, NeedsIceRestart, (const std::string&), (const, override));
  MOCK_METHOD(std::optional<std::string>, sctp_mid, (), (const, override));
  MOCK_METHOD(PeerConnectionInterface::RTCConfiguration*,
              configuration,
              (),
              (const, override));
  MOCK_METHOD(void,
              ReportSdpBundleUsage,
              (const SessionDescriptionInterface&),
              (override));
  MOCK_METHOD(PeerConnectionMessageHandler*, message_handler, (), (override));
  MOCK_METHOD(RtpTransmissionManager*, rtp_manager, (), (override));
  MOCK_METHOD(const RtpTransmissionManager*,
              rtp_manager,
              (),
              (const, override));
  MOCK_METHOD(bool, dtls_enabled, (), (const, override));
  MOCK_METHOD(const PeerConnectionFactoryInterface::Options*,
              options,
              (),
              (const, override));
  MOCK_METHOD(CryptoOptions, GetCryptoOptions, (), (override));
  MOCK_METHOD(JsepTransportController*, transport_controller_s, (), (override));
  MOCK_METHOD(JsepTransportController*, transport_controller_n, (), (override));
  MOCK_METHOD(DataChannelController*, data_channel_controller, (), (override));
  MOCK_METHOD(cricket::PortAllocator*, port_allocator, (), (override));
  MOCK_METHOD(LegacyStatsCollector*, legacy_stats, (), (override));
  MOCK_METHOD(PeerConnectionObserver*, Observer, (), (const, override));
  MOCK_METHOD(std::optional<rtc::SSLRole>, GetSctpSslRole_n, (), (override));
  MOCK_METHOD(PeerConnectionInterface::IceConnectionState,
              ice_connection_state_internal,
              (),
              (override));
  MOCK_METHOD(void,
              SetIceConnectionState,
              (PeerConnectionInterface::IceConnectionState),
              (override));
  MOCK_METHOD(void, NoteUsageEvent, (UsageEvent), (override));
  MOCK_METHOD(bool, IsClosed, (), (const, override));
  MOCK_METHOD(bool, IsUnifiedPlan, (), (const, override));
  MOCK_METHOD(bool,
              ValidateBundleSettings,
              (const cricket::SessionDescription*,
               (const std::map<std::string, const cricket::ContentGroup*>&)),
              (override));
  MOCK_METHOD(RTCErrorOr<rtc::scoped_refptr<RtpTransceiverInterface>>,
              AddTransceiver,
              (cricket::MediaType,
               rtc::scoped_refptr<MediaStreamTrackInterface>,
               const RtpTransceiverInit&,
               bool),
              (override));
  MOCK_METHOD(void, StartSctpTransport, (int, int, int), (override));
  MOCK_METHOD(void,
              AddRemoteCandidate,
              (absl::string_view, const cricket::Candidate&),
              (override));
  MOCK_METHOD(Call*, call_ptr, (), (override));
  MOCK_METHOD(bool, SrtpRequired, (), (const, override));
  MOCK_METHOD(bool,
              CreateDataChannelTransport,
              (absl::string_view),
              (override));
  MOCK_METHOD(void, DestroyDataChannelTransport, (RTCError error), (override));
  MOCK_METHOD(const FieldTrialsView&, trials, (), (const, override));

  // PeerConnectionInternal
  MOCK_METHOD(rtc::Thread*, network_thread, (), (const, override));
  MOCK_METHOD(rtc::Thread*, worker_thread, (), (const, override));
  MOCK_METHOD(bool, initial_offerer, (), (const, override));
  MOCK_METHOD(
      std::vector<
          rtc::scoped_refptr<RtpTransceiverProxyWithInternal<RtpTransceiver>>>,
      GetTransceiversInternal,
      (),
      (const, override));
  MOCK_METHOD(std::vector<DataChannelStats>,
              GetDataChannelStats,
              (),
              (const, override));
  MOCK_METHOD(std::optional<std::string>,
              sctp_transport_name,
              (),
              (const, override));
  MOCK_METHOD(cricket::CandidateStatsList,
              GetPooledCandidateStats,
              (),
              (const, override));
  MOCK_METHOD((std::map<std::string, cricket::TransportStats>),
              GetTransportStatsByNames,
              (const std::set<std::string>&),
              (override));
  MOCK_METHOD(Call::Stats, GetCallStats, (), (override));
  MOCK_METHOD(std::optional<AudioDeviceModule::Stats>,
              GetAudioDeviceStats,
              (),
              (override));
  MOCK_METHOD(bool,
              GetLocalCertificate,
              (const std::string&, rtc::scoped_refptr<rtc::RTCCertificate>*),
              (override));
  MOCK_METHOD(std::unique_ptr<rtc::SSLCertChain>,
              GetRemoteSSLCertChain,
              (const std::string&),
              (override));
  MOCK_METHOD(bool, IceRestartPending, (const std::string&), (const, override));
  MOCK_METHOD(bool,
              GetSslRole,
              (const std::string&, rtc::SSLRole*),
              (override));
  MOCK_METHOD(void, NoteDataAddedEvent, (), (override));
  MOCK_METHOD(void,
              OnSctpDataChannelStateChanged,
              (int channel_id, DataChannelInterface::DataState),
              (override));
  MOCK_METHOD(NetworkControllerInterface*,
              GetNetworkController,
              (),
              (override));
  MOCK_METHOD(PayloadTypePicker&, payload_type_picker, (), (override));
};

}  // namespace webrtc

#endif  // PC_TEST_MOCK_PEER_CONNECTION_INTERNAL_H_
