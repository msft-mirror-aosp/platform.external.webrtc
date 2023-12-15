/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "media/engine/webrtc_media_engine.h"

#include <memory>
#include <string>
#include <utility>

#include "media/engine/webrtc_media_engine_defaults.h"
#include "test/gtest.h"
#include "test/scoped_key_value_config.h"

using webrtc::RtpExtension;

namespace cricket {
namespace {

std::vector<RtpExtension> MakeUniqueExtensions() {
  std::vector<RtpExtension> result;
  char name[] = "a";
  for (int i = 0; i < 7; ++i) {
    result.push_back(RtpExtension(name, 1 + i));
    name[0]++;
    result.push_back(RtpExtension(name, 255 - i));
    name[0]++;
  }
  return result;
}

std::vector<RtpExtension> MakeRedundantExtensions() {
  std::vector<RtpExtension> result;
  char name[] = "a";
  for (int i = 0; i < 7; ++i) {
    result.push_back(RtpExtension(name, 1 + i));
    result.push_back(RtpExtension(name, 255 - i));
    name[0]++;
  }
  return result;
}

bool SupportedExtensions1(absl::string_view name) {
  return name == "c" || name == "i";
}

bool SupportedExtensions2(absl::string_view name) {
  return name != "a" && name != "n";
}

bool IsSorted(const std::vector<webrtc::RtpExtension>& extensions) {
  const std::string* last = nullptr;
  for (const auto& extension : extensions) {
    if (last && *last > extension.uri) {
      return false;
    }
    last = &extension.uri;
  }
  return true;
}
}  // namespace

TEST(WebRtcMediaEngineTest, ValidateRtpExtensionsEmptyList) {
  std::vector<RtpExtension> extensions;
  EXPECT_TRUE(ValidateRtpExtensions(extensions, {}));
}

TEST(WebRtcMediaEngineTest, ValidateRtpExtensionsAllGood) {
  std::vector<RtpExtension> extensions = MakeUniqueExtensions();
  EXPECT_TRUE(ValidateRtpExtensions(extensions, {}));
}

TEST(WebRtcMediaEngineTest, ValidateRtpExtensionsOutOfRangeId_Low) {
  std::vector<RtpExtension> extensions = MakeUniqueExtensions();
  extensions.push_back(RtpExtension("foo", 0));
  EXPECT_FALSE(ValidateRtpExtensions(extensions, {}));
}

TEST(WebRtcMediaEngineTest, ValidateRtpExtensionsOutOfRangeIdHigh) {
  std::vector<RtpExtension> extensions = MakeUniqueExtensions();
  extensions.push_back(RtpExtension("foo", 256));
  EXPECT_FALSE(ValidateRtpExtensions(extensions, {}));
}

TEST(WebRtcMediaEngineTest, ValidateRtpExtensionsOverlappingIdsStartOfSet) {
  std::vector<RtpExtension> extensions = MakeUniqueExtensions();
  extensions.push_back(RtpExtension("foo", 1));
  EXPECT_FALSE(ValidateRtpExtensions(extensions, {}));
}

TEST(WebRtcMediaEngineTest, ValidateRtpExtensionsOverlappingIdsEndOfSet) {
  std::vector<RtpExtension> extensions = MakeUniqueExtensions();
  extensions.push_back(RtpExtension("foo", 255));
  EXPECT_FALSE(ValidateRtpExtensions(extensions, {}));
}

TEST(WebRtcMediaEngineTest, ValidateRtpExtensionsEmptyToEmpty) {
  std::vector<RtpExtension> extensions;
  EXPECT_TRUE(ValidateRtpExtensions(extensions, extensions));
}

TEST(WebRtcMediaEngineTest, ValidateRtpExtensionsNoChange) {
  std::vector<RtpExtension> extensions = MakeUniqueExtensions();
  EXPECT_TRUE(ValidateRtpExtensions(extensions, extensions));
}

TEST(WebRtcMediaEngineTest, ValidateRtpExtensionsChangeIdNotUrl) {
  std::vector<RtpExtension> old_extensions = MakeUniqueExtensions();
  std::vector<RtpExtension> new_extensions = old_extensions;
  std::swap(new_extensions[0].id, new_extensions[1].id);

  EXPECT_FALSE(ValidateRtpExtensions(new_extensions, old_extensions));
}

TEST(WebRtcMediaEngineTest, ValidateRtpExtensionsChangeIdForUrl) {
  std::vector<RtpExtension> old_extensions = MakeUniqueExtensions();
  std::vector<RtpExtension> new_extensions = old_extensions;
  // Change first extension to something not generated by MakeUniqueExtensions
  new_extensions[0].id = 123;

  EXPECT_FALSE(ValidateRtpExtensions(new_extensions, old_extensions));
}

TEST(WebRtcMediaEngineTest, FilterRtpExtensionsEmptyList) {
  std::vector<RtpExtension> extensions;
  webrtc::test::ScopedKeyValueConfig trials;
  std::vector<webrtc::RtpExtension> filtered =
      FilterRtpExtensions(extensions, SupportedExtensions1, true, trials);
  EXPECT_EQ(0u, filtered.size());
}

TEST(WebRtcMediaEngineTest, FilterRtpExtensionsIncludeOnlySupported) {
  std::vector<RtpExtension> extensions = MakeUniqueExtensions();
  webrtc::test::ScopedKeyValueConfig trials;
  std::vector<webrtc::RtpExtension> filtered =
      FilterRtpExtensions(extensions, SupportedExtensions1, false, trials);
  EXPECT_EQ(2u, filtered.size());
  EXPECT_EQ("c", filtered[0].uri);
  EXPECT_EQ("i", filtered[1].uri);
}

TEST(WebRtcMediaEngineTest, FilterRtpExtensionsSortedByName1) {
  std::vector<RtpExtension> extensions = MakeUniqueExtensions();
  webrtc::test::ScopedKeyValueConfig trials;
  std::vector<webrtc::RtpExtension> filtered =
      FilterRtpExtensions(extensions, SupportedExtensions2, false, trials);
  EXPECT_EQ(12u, filtered.size());
  EXPECT_TRUE(IsSorted(filtered));
}

TEST(WebRtcMediaEngineTest, FilterRtpExtensionsSortedByName2) {
  std::vector<RtpExtension> extensions = MakeUniqueExtensions();
  webrtc::test::ScopedKeyValueConfig trials;
  std::vector<webrtc::RtpExtension> filtered =
      FilterRtpExtensions(extensions, SupportedExtensions2, true, trials);
  EXPECT_EQ(12u, filtered.size());
  EXPECT_TRUE(IsSorted(filtered));
}

TEST(WebRtcMediaEngineTest, FilterRtpExtensionsDontRemoveRedundant) {
  std::vector<RtpExtension> extensions = MakeRedundantExtensions();
  webrtc::test::ScopedKeyValueConfig trials;
  std::vector<webrtc::RtpExtension> filtered =
      FilterRtpExtensions(extensions, SupportedExtensions2, false, trials);
  EXPECT_EQ(12u, filtered.size());
  EXPECT_TRUE(IsSorted(filtered));
  EXPECT_EQ(filtered[0].uri, filtered[1].uri);
}

TEST(WebRtcMediaEngineTest, FilterRtpExtensionsRemoveRedundant) {
  std::vector<RtpExtension> extensions = MakeRedundantExtensions();
  webrtc::test::ScopedKeyValueConfig trials;
  std::vector<webrtc::RtpExtension> filtered =
      FilterRtpExtensions(extensions, SupportedExtensions2, true, trials);
  EXPECT_EQ(6u, filtered.size());
  EXPECT_TRUE(IsSorted(filtered));
  EXPECT_NE(filtered[0].uri, filtered[1].uri);
}

TEST(WebRtcMediaEngineTest, FilterRtpExtensionsRemoveRedundantEncrypted1) {
  std::vector<RtpExtension> extensions;
  extensions.push_back(webrtc::RtpExtension("b", 1));
  extensions.push_back(webrtc::RtpExtension("b", 2, true));
  extensions.push_back(webrtc::RtpExtension("c", 3));
  extensions.push_back(webrtc::RtpExtension("b", 4));
  webrtc::test::ScopedKeyValueConfig trials;
  std::vector<webrtc::RtpExtension> filtered =
      FilterRtpExtensions(extensions, SupportedExtensions2, true, trials);
  EXPECT_EQ(3u, filtered.size());
  EXPECT_TRUE(IsSorted(filtered));
  EXPECT_EQ(filtered[0].uri, filtered[1].uri);
  EXPECT_NE(filtered[0].encrypt, filtered[1].encrypt);
  EXPECT_NE(filtered[0].uri, filtered[2].uri);
  EXPECT_NE(filtered[1].uri, filtered[2].uri);
}

TEST(WebRtcMediaEngineTest, FilterRtpExtensionsRemoveRedundantEncrypted2) {
  std::vector<RtpExtension> extensions;
  extensions.push_back(webrtc::RtpExtension("b", 1, true));
  extensions.push_back(webrtc::RtpExtension("b", 2));
  extensions.push_back(webrtc::RtpExtension("c", 3));
  extensions.push_back(webrtc::RtpExtension("b", 4));
  webrtc::test::ScopedKeyValueConfig trials;
  std::vector<webrtc::RtpExtension> filtered =
      FilterRtpExtensions(extensions, SupportedExtensions2, true, trials);
  EXPECT_EQ(3u, filtered.size());
  EXPECT_TRUE(IsSorted(filtered));
  EXPECT_EQ(filtered[0].uri, filtered[1].uri);
  EXPECT_NE(filtered[0].encrypt, filtered[1].encrypt);
  EXPECT_NE(filtered[0].uri, filtered[2].uri);
  EXPECT_NE(filtered[1].uri, filtered[2].uri);
}

TEST(WebRtcMediaEngineTest, FilterRtpExtensionsRemoveRedundantBwe1) {
  webrtc::test::ScopedKeyValueConfig trials(
      "WebRTC-FilterAbsSendTimeExtension/Enabled/");
  std::vector<RtpExtension> extensions;
  extensions.push_back(
      RtpExtension(RtpExtension::kTransportSequenceNumberUri, 3));
  extensions.push_back(RtpExtension(RtpExtension::kTimestampOffsetUri, 9));
  extensions.push_back(RtpExtension(RtpExtension::kAbsSendTimeUri, 6));
  extensions.push_back(
      RtpExtension(RtpExtension::kTransportSequenceNumberUri, 1));
  extensions.push_back(RtpExtension(RtpExtension::kTimestampOffsetUri, 14));
  std::vector<webrtc::RtpExtension> filtered =
      FilterRtpExtensions(extensions, SupportedExtensions2, true, trials);
  EXPECT_EQ(1u, filtered.size());
  EXPECT_EQ(RtpExtension::kTransportSequenceNumberUri, filtered[0].uri);
}

TEST(WebRtcMediaEngineTest,
     FilterRtpExtensionsRemoveRedundantBwe1KeepAbsSendTime) {
  std::vector<RtpExtension> extensions;
  extensions.push_back(
      RtpExtension(RtpExtension::kTransportSequenceNumberUri, 3));
  extensions.push_back(RtpExtension(RtpExtension::kTimestampOffsetUri, 9));
  extensions.push_back(RtpExtension(RtpExtension::kAbsSendTimeUri, 6));
  extensions.push_back(
      RtpExtension(RtpExtension::kTransportSequenceNumberUri, 1));
  extensions.push_back(RtpExtension(RtpExtension::kTimestampOffsetUri, 14));
  webrtc::test::ScopedKeyValueConfig trials;
  std::vector<webrtc::RtpExtension> filtered =
      FilterRtpExtensions(extensions, SupportedExtensions2, true, trials);
  EXPECT_EQ(2u, filtered.size());
  EXPECT_EQ(RtpExtension::kTransportSequenceNumberUri, filtered[0].uri);
  EXPECT_EQ(RtpExtension::kAbsSendTimeUri, filtered[1].uri);
}

TEST(WebRtcMediaEngineTest, FilterRtpExtensionsRemoveRedundantBweEncrypted1) {
  webrtc::test::ScopedKeyValueConfig trials(
      "WebRTC-FilterAbsSendTimeExtension/Enabled/");
  std::vector<RtpExtension> extensions;
  extensions.push_back(
      RtpExtension(RtpExtension::kTransportSequenceNumberUri, 3));
  extensions.push_back(
      RtpExtension(RtpExtension::kTransportSequenceNumberUri, 4, true));
  extensions.push_back(RtpExtension(RtpExtension::kTimestampOffsetUri, 9));
  extensions.push_back(RtpExtension(RtpExtension::kAbsSendTimeUri, 6));
  extensions.push_back(
      RtpExtension(RtpExtension::kTransportSequenceNumberUri, 1));
  extensions.push_back(
      RtpExtension(RtpExtension::kTransportSequenceNumberUri, 2, true));
  extensions.push_back(RtpExtension(RtpExtension::kTimestampOffsetUri, 14));
  std::vector<webrtc::RtpExtension> filtered =
      FilterRtpExtensions(extensions, SupportedExtensions2, true, trials);
  EXPECT_EQ(2u, filtered.size());
  EXPECT_EQ(RtpExtension::kTransportSequenceNumberUri, filtered[0].uri);
  EXPECT_EQ(RtpExtension::kTransportSequenceNumberUri, filtered[1].uri);
  EXPECT_NE(filtered[0].encrypt, filtered[1].encrypt);
}

TEST(WebRtcMediaEngineTest,
     FilterRtpExtensionsRemoveRedundantBweEncrypted1KeepAbsSendTime) {
  std::vector<RtpExtension> extensions;
  extensions.push_back(
      RtpExtension(RtpExtension::kTransportSequenceNumberUri, 3));
  extensions.push_back(
      RtpExtension(RtpExtension::kTransportSequenceNumberUri, 4, true));
  extensions.push_back(RtpExtension(RtpExtension::kTimestampOffsetUri, 9));
  extensions.push_back(RtpExtension(RtpExtension::kAbsSendTimeUri, 6));
  extensions.push_back(
      RtpExtension(RtpExtension::kTransportSequenceNumberUri, 1));
  extensions.push_back(
      RtpExtension(RtpExtension::kTransportSequenceNumberUri, 2, true));
  extensions.push_back(RtpExtension(RtpExtension::kTimestampOffsetUri, 14));
  webrtc::test::ScopedKeyValueConfig trials;
  std::vector<webrtc::RtpExtension> filtered =
      FilterRtpExtensions(extensions, SupportedExtensions2, true, trials);
  EXPECT_EQ(3u, filtered.size());
  EXPECT_EQ(RtpExtension::kTransportSequenceNumberUri, filtered[0].uri);
  EXPECT_EQ(RtpExtension::kTransportSequenceNumberUri, filtered[1].uri);
  EXPECT_EQ(RtpExtension::kAbsSendTimeUri, filtered[2].uri);
  EXPECT_NE(filtered[0].encrypt, filtered[1].encrypt);
}

TEST(WebRtcMediaEngineTest, FilterRtpExtensionsRemoveRedundantBwe2) {
  std::vector<RtpExtension> extensions;
  extensions.push_back(RtpExtension(RtpExtension::kTimestampOffsetUri, 1));
  extensions.push_back(RtpExtension(RtpExtension::kAbsSendTimeUri, 14));
  extensions.push_back(RtpExtension(RtpExtension::kTimestampOffsetUri, 7));
  webrtc::test::ScopedKeyValueConfig trials;
  std::vector<webrtc::RtpExtension> filtered =
      FilterRtpExtensions(extensions, SupportedExtensions2, true, trials);
  EXPECT_EQ(1u, filtered.size());
  EXPECT_EQ(RtpExtension::kAbsSendTimeUri, filtered[0].uri);
}

TEST(WebRtcMediaEngineTest, FilterRtpExtensionsRemoveRedundantBwe3) {
  std::vector<RtpExtension> extensions;
  extensions.push_back(RtpExtension(RtpExtension::kTimestampOffsetUri, 2));
  extensions.push_back(RtpExtension(RtpExtension::kTimestampOffsetUri, 14));
  webrtc::test::ScopedKeyValueConfig trials;
  std::vector<webrtc::RtpExtension> filtered =
      FilterRtpExtensions(extensions, SupportedExtensions2, true, trials);
  EXPECT_EQ(1u, filtered.size());
  EXPECT_EQ(RtpExtension::kTimestampOffsetUri, filtered[0].uri);
}

// Deprecated as part of the bugs.webrtc.org/15574 effort.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
TEST(WebRtcMediaEngineTest, Create) {
  MediaEngineDependencies deps;
  webrtc::DeprecatedSetMediaEngineDefaults(&deps);
  webrtc::test::ScopedKeyValueConfig trials;
  deps.trials = &trials;

  std::unique_ptr<MediaEngineInterface> engine =
      CreateMediaEngine(std::move(deps));

  EXPECT_TRUE(engine);
}
#pragma clang diagnostic pop

}  // namespace cricket
