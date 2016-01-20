/*
 * libjingle
 * Copyright 2004 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TALK_MEDIA_BASE_VIDEORENDERER_H_
#define TALK_MEDIA_BASE_VIDEORENDERER_H_

#if !defined(NDEBUG)
#include <string>
#endif

#include "webrtc/base/sigslot.h"

namespace cricket {

class VideoFrame;

// Abstract interface for rendering VideoFrames.
class VideoRenderer {
 public:
  virtual ~VideoRenderer() {}
  // Called when the video has changed size.
  // TODO(nisse): This method is not really used, and should be
  // deleted. Provide a default do-nothing implementation, to easy the
  // transition as the method is deleted in subclasses, in particular,
  // chrome's MockVideoRenderer class.
  virtual bool SetSize(int width, int height, int reserved) { return true; };
  // Called when a new frame is available for display.
  virtual bool RenderFrame(const VideoFrame *frame) = 0;

#if !defined(NDEBUG)
  // Allow renderer dumping out rendered frames.
  virtual bool SetDumpPath(const std::string &path) { return true; }
#endif
};

}  // namespace cricket

#endif  // TALK_MEDIA_BASE_VIDEORENDERER_H_
