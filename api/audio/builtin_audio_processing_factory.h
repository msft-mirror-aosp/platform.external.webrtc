/*
 *  Copyright (c) 2024 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef API_AUDIO_BUILTIN_AUDIO_PROCESSING_FACTORY_H_
#define API_AUDIO_BUILTIN_AUDIO_PROCESSING_FACTORY_H_

#include <memory>
#include <utility>

#include "absl/base/nullability.h"
#include "api/audio/audio_processing.h"
#include "api/audio/echo_control.h"
#include "api/environment/environment.h"
#include "api/scoped_refptr.h"
#include "rtc_base/system/rtc_export.h"

namespace webrtc {

class RTC_EXPORT BuiltinAudioProcessingFactory : public AudioProcessingFactory {
 public:
  BuiltinAudioProcessingFactory() = default;
  explicit BuiltinAudioProcessingFactory(const AudioProcessing::Config& config)
      : config_(config) {}
  BuiltinAudioProcessingFactory(const BuiltinAudioProcessingFactory&) = delete;
  BuiltinAudioProcessingFactory& operator=(
      const BuiltinAudioProcessingFactory&) = delete;
  ~BuiltinAudioProcessingFactory() override = default;

  // Sets the APM configuration.
  BuiltinAudioProcessingFactory& SetConfig(
      const AudioProcessing::Config& config) {
    config_ = config;
    return *this;
  }

  // Sets the echo controller factory to inject when APM is created.
  BuiltinAudioProcessingFactory& SetEchoControlFactory(
      std::unique_ptr<EchoControlFactory> echo_control_factory) {
    echo_control_factory_ = std::move(echo_control_factory);
    return *this;
  }

  // Sets the capture post-processing sub-module to inject when APM is created.
  BuiltinAudioProcessingFactory& SetCapturePostProcessing(
      std::unique_ptr<CustomProcessing> capture_post_processing) {
    capture_post_processing_ = std::move(capture_post_processing);
    return *this;
  }

  // Sets the render pre-processing sub-module to inject when APM is created.
  BuiltinAudioProcessingFactory& SetRenderPreProcessing(
      std::unique_ptr<CustomProcessing> render_pre_processing) {
    render_pre_processing_ = std::move(render_pre_processing);
    return *this;
  }

  // Sets the echo detector to inject when APM is created.
  BuiltinAudioProcessingFactory& SetEchoDetector(
      rtc::scoped_refptr<EchoDetector> echo_detector) {
    echo_detector_ = std::move(echo_detector);
    return *this;
  }

  // Sets the capture analyzer sub-module to inject when APM is created.
  BuiltinAudioProcessingFactory& SetCaptureAnalyzer(
      std::unique_ptr<CustomAudioAnalyzer> capture_analyzer) {
    capture_analyzer_ = std::move(capture_analyzer);
    return *this;
  }

  // Creates an APM instance with the specified config or the default one if
  // unspecified. Injects the specified components transferring the ownership
  // to the newly created APM instance. This implementation of the
  // AudioProcessingFactory interface is not designed to be used more than once.
  // Calling `Create` second time would return an unspecified object.
  absl::Nullable<scoped_refptr<AudioProcessing>> Create(
      const Environment& env) override;

 private:
  bool called_create_ = false;
  AudioProcessing::Config config_;
  std::unique_ptr<EchoControlFactory> echo_control_factory_;
  std::unique_ptr<CustomProcessing> capture_post_processing_;
  std::unique_ptr<CustomProcessing> render_pre_processing_;
  scoped_refptr<EchoDetector> echo_detector_;
  std::unique_ptr<CustomAudioAnalyzer> capture_analyzer_;
};

}  // namespace webrtc

#endif  // API_AUDIO_BUILTIN_AUDIO_PROCESSING_FACTORY_H_
