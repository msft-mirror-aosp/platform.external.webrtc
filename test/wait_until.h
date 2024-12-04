/*
 *  Copyright 2024 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TEST_WAIT_UNTIL_H_
#define TEST_WAIT_UNTIL_H_

#include <string>

#include "absl/base/nullability.h"
#include "api/rtc_error.h"
#include "api/units/time_delta.h"
#include "api/units/timestamp.h"
#include "rtc_base/checks.h"
#include "rtc_base/thread.h"
#include "system_wrappers/include/clock.h"
#include "test/gmock.h"
#include "test/wait_until_internal.h"  // IWYU pragma: private

namespace webrtc {

struct WaitUntilSettings {
  // The maximum time to wait for the condition to be met.
  TimeDelta timeout = TimeDelta::Seconds(5);
  // The interval between polling the condition.
  TimeDelta polling_interval = TimeDelta::Millis(1);
  // The clock to use for timing.
  absl::Nullable<SimulatedClock*> clock = nullptr;

  // Name of the result to be used in the error message.
  std::string result_name = "result";
};

// Runs a function `fn`, which returns a result, until `matcher` matches the
// result.
//
// The function is called repeatedly until the result matches the matcher or the
// timeout is reached. If the matcher matches the result, the result is
// returned. Otherwise, an error is returned.
//
// Example:
//
//   int counter = 0;
//   RTCErrorOr<int> result = Waituntil([&] { return ++counter; }, Eq(3))
//   EXPECT_THAT(result, IsOkAndHolds(3));
template <typename Fn, typename Matcher>
auto WaitUntil(const Fn& fn, Matcher matcher, WaitUntilSettings settings = {})
    -> RTCErrorOr<decltype(fn())> {
  if (!settings.clock) {
    RTC_CHECK(rtc::Thread::Current()) << "A current thread is required. An "
                                         "rtc::AutoThread can work for tests.";
  }

  absl::Nonnull<Clock*> clock =
      settings.clock ? settings.clock : Clock::GetRealTimeClock();

  Timestamp start = clock->CurrentTime();

  do {
    auto result = fn();
    if (testing::Value(result, matcher)) {
      return result;
    }
    if (settings.clock) {
      settings.clock->AdvanceTime(settings.polling_interval);
    } else {
      rtc::Thread::Current()->ProcessMessages(0);
      rtc::Thread::Current()->SleepMs(settings.polling_interval.ms());
    }
  } while (clock->CurrentTime() < start + settings.timeout);

  // One more try after the last sleep. This failure will contain the error
  // message.
  auto result = fn();
  testing::StringMatchResultListener listener;
  if (wait_until_internal::ExplainMatchResult(matcher, result, &listener,
                                              settings.result_name)) {
    return result;
  }

  return RTCError(RTCErrorType::INTERNAL_ERROR, listener.str());
}

}  // namespace webrtc

#endif  // TEST_WAIT_UNTIL_H_
