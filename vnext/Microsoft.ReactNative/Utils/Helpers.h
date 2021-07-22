// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <IReactInstance.h>
#include <JSValue.h>
#include <React.h>
#include <stdint.h>

namespace Microsoft::ReactNative {

using namespace std;

struct ReactId {
  int64_t tag{0};
  bool isValid{false};
};

template <typename T>
inline typename T asEnum(winrt::Microsoft::ReactNative::JSValue const &obj) {
  return static_cast<T>(obj.AsInt64());
}

ReactId getViewId(const Mso::React::IReactContext &context, xaml::FrameworkElement const &fe);
std::int32_t CountOpenPopups();

bool IsRS3OrHigher();
bool IsRS4OrHigher();
bool IsRS5OrHigher();
bool Is19H1OrHigher();
bool Is21H1OrHigher();

bool IsXamlIsland();
bool IsWinUI3Island();

} // namespace Microsoft::ReactNative
