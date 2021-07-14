// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <CppWinRTIncludes.h>
#include <cxxreact/CxxModule.h>
#include <cxxreact/MessageQueueThread.h>

#include <folly/dynamic.h>
#include <memory>
#include <vector>

#include <winrt/Windows.Foundation.h>
namespace Microsoft::ReactNative {

typedef winrt::Windows::Foundation::DateTime TDateTime;
typedef winrt::Windows::Foundation::TimeSpan TTimeSpan;

class TimingModule;

struct Timer {
  Timer(int64_t id, TDateTime targetTime, TTimeSpan period, bool repeat) {
    Id = id;
    TargetTime = targetTime;
    Period = period;
    Repeat = repeat;
  }

  bool operator<(const Timer &timer) const {
    return timer.TargetTime < TargetTime;
  }

  bool operator==(int64_t id) const {
    return id == Id;
  }

  int64_t Id;
  TDateTime TargetTime;
  TTimeSpan Period;
  bool Repeat;
};

class TimerQueue {
 public:
  TimerQueue();

  void Push(int64_t id, TDateTime targetTime, TTimeSpan period, bool repeat);
  void Pop();
  Timer &Front();
  void Remove(int64_t id);

  bool IsEmpty();

 private:
  std::vector<Timer> m_timerVector;
};

class Timing : public std::enable_shared_from_this<Timing> {
 public:
  Timing(TimingModule *parent);
  void Disconnect();

  void createTimer(int64_t id, double duration, double jsSchedulingTime, bool repeat);
  void deleteTimer(int64_t id);
  void setSendIdleEvents(bool sendIdleEvents);

 private:
  std::weak_ptr<facebook::react::Instance> getInstance() noexcept;
  void OnTick();
  winrt::system::DispatcherQueueTimer EnsureDispatcherTimer();
  void StartRendering();
  void StartDispatcherTimer();
  void StopTicks();

 private:
  TimingModule *m_parent;
  TimerQueue m_timerQueue;
  xaml::Media::CompositionTarget::Rendering_revoker m_rendering;
  winrt::system::DispatcherQueueTimer m_dispatcherQueueTimer{nullptr};
  bool m_usingRendering{false};
};

class TimingModule : public facebook::xplat::module::CxxModule {
 public:
  TimingModule();
  ~TimingModule();
  std::string getName();
  virtual auto getConstants() -> std::map<std::string, folly::dynamic>;
  virtual auto getMethods() -> std::vector<Method>;

  static const char *name;

 private:
  std::shared_ptr<Timing> m_timing;
};

} // namespace Microsoft::ReactNative
