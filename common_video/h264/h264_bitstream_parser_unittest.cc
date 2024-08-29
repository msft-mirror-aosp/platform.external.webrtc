/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "common_video/h264/h264_bitstream_parser.h"

#include "test/gtest.h"

namespace webrtc {

// SPS/PPS part of below chunk.
uint8_t kH264SpsPps[] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x80, 0x20, 0xda,
                         0x01, 0x40, 0x16, 0xe8, 0x06, 0xd0, 0xa1, 0x35, 0x00,
                         0x00, 0x00, 0x01, 0x68, 0xce, 0x06, 0xe2};

// Contains enough of the image slice to contain slice QP.
uint8_t kH264BitstreamChunk[] = {
    0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x80, 0x20, 0xda, 0x01, 0x40, 0x16,
    0xe8, 0x06, 0xd0, 0xa1, 0x35, 0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x06,
    0xe2, 0x00, 0x00, 0x00, 0x01, 0x65, 0xb8, 0x40, 0xf0, 0x8c, 0x03, 0xf2,
    0x75, 0x67, 0xad, 0x41, 0x64, 0x24, 0x0e, 0xa0, 0xb2, 0x12, 0x1e, 0xf8,
};

uint8_t kH264BitstreamChunkCabac[] = {
    0x00, 0x00, 0x00, 0x01, 0x27, 0x64, 0x00, 0x0d, 0xac, 0x52, 0x30,
    0x50, 0x7e, 0xc0, 0x5a, 0x81, 0x01, 0x01, 0x18, 0x56, 0xbd, 0xef,
    0x80, 0x80, 0x00, 0x00, 0x00, 0x01, 0x28, 0xfe, 0x09, 0x8b,
};

// Contains enough of the image slice to contain slice QP.
uint8_t kH264BitstreamNextImageSliceChunk[] = {
    0x00, 0x00, 0x00, 0x01, 0x41, 0xe2, 0x01, 0x16, 0x0e, 0x3e, 0x2b, 0x86,
};

// Contains enough of the image slice to contain slice QP.
uint8_t kH264BitstreamNextImageSliceChunkCabac[] = {
    0x00, 0x00, 0x00, 0x01, 0x21, 0xe1, 0x05, 0x11, 0x3f, 0x9a, 0xae, 0x46,
    0x70, 0xbf, 0xc1, 0x4a, 0x16, 0x8f, 0x51, 0xf4, 0xca, 0xfb, 0xa3, 0x65,
};

uint8_t kH264BitstreamWeightedPred[] = {
    0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x28, 0xac, 0xb4, 0x03, 0xc0,
    0x11, 0x3f, 0x2e, 0x02, 0xd4, 0x04, 0x04, 0x05, 0x00, 0x00, 0x03, 0x00,
    0x01, 0x00, 0x00, 0x03, 0x00, 0x30, 0x8f, 0x18, 0x32, 0xa0, 0x00, 0x00,
    0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x68, 0xef, 0x3c, 0xb0, 0x00, 0x00,
    0x00, 0xc0, 0x00, 0x00, 0x00, 0x01, 0x41, 0x9a, 0x26, 0x21, 0xf7, 0xff,
    0xfe, 0x9e, 0x10, 0x00, 0x00, 0x08, 0x78, 0x00, 0x00, 0x00, 0x12};

// First 4 P frames of CVWP1_TOSHIBA_E test file.
uint8_t H264BitstreamCVWP1SPS[] = {0x00, 0x00, 0x00, 0x01, 0x27, 0x4d, 0x40,
                                   0x14, 0xd9, 0x81, 0x60, 0x94, 0x40};

uint8_t H264BitstreamCVWP1PFrame1[] = {
    0x00, 0x00, 0x00, 0x01, 0x28, 0xcf, 0x1b, 0x88, 0x00, 0x00, 0x00,
    0x01, 0x21, 0x9a, 0x21, 0x8f, 0x02, 0xd8, 0x1b, 0xe0, 0x2c, 0xc3,
    0x80, 0x20, 0x00, 0xe4, 0xcd, 0x72, 0xfe, 0x1c, 0xfc, 0x2a, 0x00,
    0x02, 0x00, 0x26, 0x09, 0x04, 0xc1, 0x38, 0xe2, 0x9b, 0xcc, 0x60,
    0x54, 0xee, 0x62, 0x6b, 0x00, 0x28, 0x86, 0xce, 0x81, 0x0f, 0xd2,
    0x17, 0x26, 0x0d, 0x2f, 0x1c, 0x1d, 0xe3, 0x80, 0x01};

uint8_t H264BitstreamCVWP1PFrame2[] = {
    0x00, 0x00, 0x00, 0x01, 0x28, 0xca, 0xc6, 0xe2, 0x00, 0x00, 0x00,
    0x01, 0x21, 0x9a, 0x41, 0xcb, 0x01, 0x8e, 0x02, 0x76, 0x28, 0x68,
    0x20, 0x01, 0x9a, 0x33, 0x60, 0x58, 0xc3, 0x0d, 0x7c, 0x32, 0x00,
    0x02, 0x00, 0x7c, 0x5d, 0xf7, 0x22, 0x6c, 0x3d, 0xa3, 0xcc, 0x60,
    0x5a, 0x3d, 0x98, 0x3b, 0xf0, 0x14, 0x48, 0x1b, 0xa0, 0xdf, 0x69,
    0xfc, 0xf2, 0x66, 0x21, 0x4d, 0x72, 0x99, 0xc2, 0x1c};

uint8_t H264BitstreamCVWP1PFrame3[] = {
    0x00, 0x00, 0x00, 0x01, 0x28, 0xcb, 0xc6, 0xe2, 0x00, 0x00, 0x00,
    0x01, 0x21, 0x9a, 0x61, 0xcf, 0x04, 0xc0, 0x24, 0x20, 0x33, 0xc0,
    0x5d, 0x80, 0x80, 0x05, 0x08, 0x0a, 0xb0, 0x30, 0x81, 0xf8, 0x0d,
    0x70, 0x13, 0xa0, 0x31, 0x8e, 0x86, 0x94, 0x6c, 0x43, 0xbb, 0x58,
    0x44, 0xc2, 0x41, 0x7c, 0x92, 0x04, 0x7e, 0x9f, 0xbf, 0x01, 0xe9,
    0xab, 0x53, 0xfe, 0x8f, 0x1c, 0x00, 0x04, 0x1f, 0x23};

uint8_t H264BitstreamCVWP1PFrame4[] = {
    0x00, 0x00, 0x00, 0x01, 0x28, 0xc9, 0x31, 0xb8, 0x80, 0x00, 0x00,
    0x00, 0x01, 0x21, 0x9a, 0x81, 0xe1, 0x04, 0xe0, 0x4f, 0x0f, 0x12,
    0xc6, 0x58, 0x74, 0x34, 0x06, 0x73, 0x9f, 0x43, 0xa7, 0xd0, 0x3c,
    0x9c, 0x9c, 0x92, 0x4f, 0x84, 0x4f, 0xd6, 0x36, 0x63, 0xff, 0xa0,
    0x5b, 0x1c, 0x6f, 0x01, 0x0b, 0xc2, 0x5e, 0x7b, 0xb0, 0xd7, 0x8f,
    0x19, 0x70, 0x81, 0xfa, 0x93, 0x4d, 0x48, 0x4f, 0xd2};

// First 2 B frames of CVWP2_TOSHIBA_E test file.
uint8_t H264BitstreamCVWP2SPS[] = {0x00, 0x00, 0x00, 0x01, 0x27, 0x4d, 0x40,
                                   0x14, 0xec, 0xc0, 0xb0, 0x4a, 0x20};

uint8_t H264BitstreamCVWP2BFrame1[] = {
    0x00, 0x00, 0x00, 0x01, 0x28, 0xce, 0x1b, 0x88, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x9a, 0x3e, 0x19, 0x69, 0xa1, 0xc4, 0x1e, 0x5d, 0xea,
    0x84, 0x1c, 0x10, 0x65, 0x87, 0xc0, 0x25, 0x1b, 0x6d, 0x1e, 0xcf,
    0xf9, 0x8d, 0xf1, 0x2f, 0xec, 0xf8, 0xc2, 0x07, 0xfe, 0x02, 0x27,
    0xec, 0xcb, 0x74, 0x75, 0x59, 0xd5, 0x6e, 0xc0, 0x01, 0x4b, 0xb2,
    0xe7, 0x68, 0xfe, 0xef, 0xaf, 0xb6, 0x76, 0xc6, 0xc5};

uint8_t H264BitstreamCVWP2BFrame2[] = {
    0x00, 0x00, 0x00, 0x01, 0x28, 0xce, 0x1b, 0x88, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x9a, 0x3e, 0x19, 0x69, 0xa1, 0xc4, 0x1e, 0x5d, 0xea,
    0x84, 0x1c, 0x10, 0x65, 0x87, 0xc0, 0x25, 0x1b, 0x6d, 0x1e, 0xcf,
    0xf9, 0x8d, 0xf1, 0x2f, 0xec, 0xf8, 0xc2, 0x07, 0xfe, 0x02, 0x27,
    0xec, 0xcb, 0x74, 0x75, 0x59, 0xd5, 0x6e, 0xc0, 0x01, 0x4b, 0xb2,
    0xe7, 0x68, 0xfe, 0xef, 0xaf, 0xb6, 0x76, 0xc6, 0xc5};

TEST(H264BitstreamParserTest, ReportsNoQpWithoutParsedSlices) {
  H264BitstreamParser h264_parser;
  EXPECT_FALSE(h264_parser.GetLastSliceQp().has_value());
}

TEST(H264BitstreamParserTest, ReportsNoQpWithOnlyParsedPpsAndSpsSlices) {
  H264BitstreamParser h264_parser;
  h264_parser.ParseBitstream(kH264SpsPps);
  EXPECT_FALSE(h264_parser.GetLastSliceQp().has_value());
}

TEST(H264BitstreamParserTest, ReportsLastSliceQpForImageSlices) {
  H264BitstreamParser h264_parser;
  h264_parser.ParseBitstream(kH264BitstreamChunk);
  std::optional<int> qp = h264_parser.GetLastSliceQp();
  ASSERT_TRUE(qp.has_value());
  EXPECT_EQ(35, *qp);

  // Parse an additional image slice.
  h264_parser.ParseBitstream(kH264BitstreamNextImageSliceChunk);
  qp = h264_parser.GetLastSliceQp();
  ASSERT_TRUE(qp.has_value());
  EXPECT_EQ(37, *qp);
}

TEST(H264BitstreamParserTest, ReportsLastSliceQpForCABACImageSlices) {
  H264BitstreamParser h264_parser;
  h264_parser.ParseBitstream(kH264BitstreamChunkCabac);
  EXPECT_FALSE(h264_parser.GetLastSliceQp().has_value());

  // Parse an additional image slice.
  h264_parser.ParseBitstream(kH264BitstreamNextImageSliceChunkCabac);
  std::optional<int> qp = h264_parser.GetLastSliceQp();
  ASSERT_TRUE(qp.has_value());
  EXPECT_EQ(24, *qp);
}

TEST(H264BitstreamParserTest, ReportsLastSliceQpForWeightedPredSlices) {
  H264BitstreamParser h264_parser;
  h264_parser.ParseBitstream(kH264BitstreamWeightedPred);

  std::optional<int> qp = h264_parser.GetLastSliceQp();
  ASSERT_TRUE(qp.has_value());
  EXPECT_EQ(11, *qp);
}

TEST(H264BitstreamParserTest, ReportsLastSliceQpForWeightedPredSlicesL0Active) {
  H264BitstreamParser h264_parser;
  std::optional<int> qp;
  h264_parser.ParseBitstream(H264BitstreamCVWP1SPS);

  h264_parser.ParseBitstream(H264BitstreamCVWP1PFrame1);
  qp = h264_parser.GetLastSliceQp();
  ASSERT_TRUE(qp.has_value());
  EXPECT_EQ(25, *qp);

  h264_parser.ParseBitstream(H264BitstreamCVWP1PFrame2);
  qp = h264_parser.GetLastSliceQp();
  ASSERT_TRUE(qp.has_value());
  EXPECT_EQ(25, *qp);

  h264_parser.ParseBitstream(H264BitstreamCVWP1PFrame3);
  qp = h264_parser.GetLastSliceQp();
  ASSERT_TRUE(qp.has_value());
  EXPECT_EQ(25, *qp);

  h264_parser.ParseBitstream(H264BitstreamCVWP1PFrame4);
  qp = h264_parser.GetLastSliceQp();
  ASSERT_TRUE(qp.has_value());
  EXPECT_EQ(25, *qp);
}

TEST(H264BitstreamParserTest, ReportsLastSliceQpForWeightedPredSlicesL1Active) {
  H264BitstreamParser h264_parser;
  std::optional<int> qp;
  h264_parser.ParseBitstream(H264BitstreamCVWP2SPS);

  h264_parser.ParseBitstream(H264BitstreamCVWP2BFrame1);
  qp = h264_parser.GetLastSliceQp();
  ASSERT_TRUE(qp.has_value());
  EXPECT_EQ(25, *qp);

  h264_parser.ParseBitstream(H264BitstreamCVWP2BFrame1);
  qp = h264_parser.GetLastSliceQp();
  ASSERT_TRUE(qp.has_value());
  EXPECT_EQ(25, *qp);
}

}  // namespace webrtc
