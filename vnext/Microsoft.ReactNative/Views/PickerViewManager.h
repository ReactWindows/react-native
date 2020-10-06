// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <Views/ControlViewManager.h>

namespace react::uwp {

class PickerViewManager : public ControlViewManager {
  using Super = ControlViewManager;

 public:
  PickerViewManager(const Mso::React::IReactContext &context);

  const char *GetName() const override;
  folly::dynamic GetNativeProps() const override;

  facebook::react::ShadowNode *createShadow() const override;

  YGMeasureFunc GetYogaCustomMeasureFunc() const override;

 protected:
  XamlView CreateViewCore(int64_t tag) override;

  friend class PickerShadowNode;
};

} // namespace react::uwp
