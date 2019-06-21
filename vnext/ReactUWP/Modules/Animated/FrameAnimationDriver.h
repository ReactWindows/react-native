// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "AnimatedNode.h"
#include "AnimationDriver.h"
#include <folly/dynamic.h>

namespace react { namespace uwp {
  class FrameAnimationDriver : public AnimationDriver
  {
  public:
    FrameAnimationDriver(int64_t id, const std::shared_ptr<ValueAnimatedNode>& animatedValue, const Callback& endCallback, const folly::dynamic& config);

  private:
    std::vector<double> m_frames {};
    double m_toValue { 0 };
    int64_t m_iterations { 0 };
  };
} }
