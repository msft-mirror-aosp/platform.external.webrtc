/*
 *  Copyright (c) 2021 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "net/dcsctp/packet/crc32c.h"

#include <cstdint>
#include <string_view>

#include "absl/crc/crc32c.h"

namespace dcsctp {

uint32_t GenerateCrc32C(rtc::ArrayView<const uint8_t> data) {
  auto view = std::string_view((const char*)data.data(), data.size());
  uint32_t crc32c = (uint32_t) absl::ComputeCrc32c(view);

  // // Byte swapping for little endian byte order:
  uint8_t byte0 = crc32c;
  uint8_t byte1 = crc32c >> 8;
  uint8_t byte2 = crc32c >> 16;
  uint8_t byte3 = crc32c >> 24;
  crc32c = ((byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3);
  return crc32c;
}
}  // namespace dcsctp
