// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"

#include "SwitchViewManager.h"
#include "ShadowNodeBase.h"

#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>

namespace winrt {
  using namespace Windows::UI::Xaml;
  using namespace Windows::UI::Xaml::Controls;
}

namespace react { namespace uwp {

class SwitchShadowNode : public ShadowNodeBase
{
  using Super = ShadowNodeBase;
public:
  SwitchShadowNode() = default;
  void createView() override;
  void updateProperties(const folly::dynamic&& props) override;

private:
  static void OnToggled(IReactInstance& instance, int64_t tag, bool newValue);
  bool m_updating = false;
};

void SwitchShadowNode::createView()
{
  Super::createView();

  auto toggleSwitch = GetView().as<winrt::ToggleSwitch>();
  auto wkinstance = static_cast<SwitchViewManager*>(GetViewManager())->m_wkReactInstance;
  toggleSwitch.Toggled([=](auto&&, auto&&)
  {
    auto instance = wkinstance.lock();
    if (!m_updating && instance != nullptr)
      OnToggled(*instance, m_tag, toggleSwitch.IsOn());
  });
}

void SwitchShadowNode::updateProperties(const folly::dynamic&& props)
{
  m_updating = true;
  Super::updateProperties(std::move(props));
  m_updating = false;
}

/*static*/ void SwitchShadowNode::OnToggled(IReactInstance& instance, int64_t tag, bool newValue)
{
  folly::dynamic eventData = folly::dynamic::object("target", tag)("value", newValue);
  instance.DispatchEvent(tag, "topChange", std::move(eventData));
}

SwitchViewManager::SwitchViewManager(const std::shared_ptr<IReactInstance>& reactInstance)
  : FrameworkElementViewManager(reactInstance)
{
}

const char* SwitchViewManager::GetName() const
{
  return "RCTSwitch";
}

folly::dynamic SwitchViewManager::GetNativeProps() const
{
  auto props = Super::GetNativeProps();

  props.update(folly::dynamic::object
    ("value", "boolean")
    ("disabled", "boolean")
  );

  return props;
}

facebook::react::ShadowNode* SwitchViewManager::createShadow() const
{
  return new SwitchShadowNode();
}

XamlView SwitchViewManager::CreateViewCore(int64_t tag)
{
  auto toggleSwitch = winrt::ToggleSwitch();

  return toggleSwitch;
}

void SwitchViewManager::UpdateProperties(ShadowNodeBase* nodeToUpdate, XamlView viewToUpdate, folly::dynamic reactDiffMap)
{
  auto toggleSwitch = viewToUpdate.as<winrt::ToggleSwitch>();
  if (toggleSwitch == nullptr)
    return;

  for (auto& pair : reactDiffMap.items())
  {
    const folly::dynamic& propertyName = pair.first;
    const folly::dynamic& propertyValue = pair.second;

   if (propertyName.asString() == "disabled")
   {
      if (propertyValue.isBool())
        toggleSwitch.IsEnabled(!propertyValue.asBool());
   }
   else if (propertyName.asString() == "value")
   {
     if (propertyValue.isBool())
       toggleSwitch.IsOn(propertyValue.asBool());
   }
  }

  Super::UpdateProperties(nodeToUpdate, viewToUpdate, reactDiffMap);
}

}}
