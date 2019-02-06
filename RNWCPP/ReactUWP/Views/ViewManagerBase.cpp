// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"

#include "ViewManagerBase.h"

#include "ShadowNodeBase.h"

#include <Modules/NativeUIManager.h>

#include <IXamlRootView.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>

using namespace folly;
namespace winrt {
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
}

namespace react { namespace uwp {

float GetConstrainedResult(float constrainTo, float measuredSize, YGMeasureMode measureMode)
{
  if (measureMode == YGMeasureMode::YGMeasureModeExactly)
    return constrainTo;
  if (measureMode == YGMeasureMode::YGMeasureModeAtMost)
    return std::min(constrainTo, measuredSize);
  return measuredSize;
}

YGSize DefaultYogaSelfMeasureFunc(YGNodeRef node, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode)
{
  YogaContext* context = reinterpret_cast<YogaContext*>(YGNodeGetContext(node));

  // TODO: VEC context != nullptr, DefaultYogaSelfMeasureFunc expects a context.

  XamlView view = context->view;
  auto element = view.as<winrt::UIElement>();

  float constrainToWidth = widthMode == YGMeasureMode::YGMeasureModeUndefined ? std::numeric_limits<float>::max() : width;
  float constrainToHeight = heightMode == YGMeasureMode::YGMeasureModeUndefined ? std::numeric_limits<float>::max() : height;

  try
  {
    winrt::Windows::Foundation::Size availableSpace(constrainToWidth, constrainToHeight);

    // Clear out current size so it doesn't constrain the measurement
    element.ClearValue(winrt::FrameworkElement::WidthProperty());
    element.ClearValue(winrt::FrameworkElement::HeightProperty());

    element.Measure(availableSpace);
  }
  catch (winrt::hresult_error const& )
  {
    // TODO add more error handling here
    assert(false);
  }

  YGSize desiredSize = {
    GetConstrainedResult(constrainToWidth, element.DesiredSize().Width, widthMode),
    GetConstrainedResult(constrainToHeight, element.DesiredSize().Height, heightMode) };
  return desiredSize;
}

std::string NormalizeInputEventName(const std::string eventName)
{
  std::string result = eventName;
  if (eventName.compare(0, 2, "on") == 0)
  { // replace on with top
    result = result.replace(0, 2, "top");
  }
  else if (eventName.compare(0, 3, "top") != 0)
  { // doesn't start with top, insert top
    result[0] = std::toupper(result[0]);
    result.insert(0, "top");
  }

  return result;
}

ViewManagerBase::ViewManagerBase(const std::shared_ptr<IReactInstance>& reactInstance)
  : m_wkReactInstance(reactInstance)
{
}

dynamic ViewManagerBase::GetExportedViewConstants() const
{
  return dynamic::object();
}

dynamic ViewManagerBase::GetCommands() const
{
  return dynamic::object();
}

dynamic ViewManagerBase::GetNativeProps() const
{
  folly::dynamic props = folly::dynamic::object();
  props.update(folly::dynamic::object
    ("onLayout", "function")
  );
  return props;
}

dynamic ViewManagerBase::GetConstants() const
{
  folly::dynamic constants = folly::dynamic::object
    ("Constants", GetExportedViewConstants())
    ("Commands", GetCommands())
    ("NativeProps", GetNativeProps());

  const auto bubblingEventTypesConstants = GetExportedCustomBubblingEventTypeConstants();
  if (!bubblingEventTypesConstants.empty())
    constants["bubblingEventTypes"] = bubblingEventTypesConstants;
  const auto directEventTypesConstants = GetExportedCustomDirectEventTypeConstants();
  if (!directEventTypesConstants.empty())
    constants["directEventTypes"] = directEventTypesConstants;

  return constants;
}

facebook::react::ShadowNode* ViewManagerBase::createShadow() const
{
  // This class is the default ShadowNode that most view managers can use. If they need special functionality
  //  they should override this function and create their own ShadowNodeBase sub-class.
  return new ShadowNodeBase();
}

void ViewManagerBase::destroyShadow(facebook::react::ShadowNode* node) const
{
  delete node;
}

dynamic ViewManagerBase::GetExportedCustomBubblingEventTypeConstants() const
{
  const std::string standardEvents[] = {
    // Generic events
    "press",
    "change",
    "focus",
    "blur",
    "submitEditing",
    "endEditing",
    "keyPress",

    // Touch events
    "touchStart",
    "touchMove",
    "touchCancel",
    "touchEnd",
  };

  folly::dynamic bubblingEvents = folly::dynamic::object();
  for (auto& e : standardEvents)
  {
    std::string eventName = NormalizeInputEventName(e);
    std::string bubbleName = eventName;
    bubbleName.replace(0, 3, "on");

    folly::dynamic registration = folly::dynamic::object();
    registration["bubbled"] = bubbleName;
    registration["captured"] = bubbleName + "Capture";

    bubblingEvents[eventName] = folly::dynamic::object("phasedRegistrationNames", registration);
  }

  return bubblingEvents;
}

dynamic ViewManagerBase::GetExportedCustomDirectEventTypeConstants() const
{
  folly::dynamic eventTypes = folly::dynamic::object();
  eventTypes.update(folly::dynamic::object
    ("topLayout", folly::dynamic::object("registrationName", "onLayout"))
    ("topMouseEnter", folly::dynamic::object("registrationName", "onMouseEnter"))
    ("topMouseLeave", folly::dynamic::object("registrationName", "onMouseLeave"))
//    ("topMouseMove", folly::dynamic::object("registrationName", "onMouseMove"))
  );
  return eventTypes;
}

XamlView ViewManagerBase::CreateView(int64_t tag)
{
  XamlView view = CreateViewCore(tag);

  // Set the tag if the element type supports it
  auto obj = view.try_as<winrt::DependencyObject>();
  if (obj != nullptr)
    obj.SetValue(winrt::FrameworkElement::TagProperty(), winrt::PropertyValue::CreateInt64(tag));

  return view;
}

void ViewManagerBase::AddView(XamlView parent, XamlView child, int64_t index)
{
  // ASSERT: Child must either implement or not allow children.
  assert(false);
}

XamlView ViewManagerBase::GetChildAt(XamlView parent, int64_t index)
{
  // ASSERT: Child must either implement or not allow children.
  assert(false);
  return nullptr;
}

void ViewManagerBase::RemoveAllChildren(XamlView parent)
{
}

void ViewManagerBase::RemoveChildAt(XamlView parent, int64_t index)
{
  // ASSERT: Child must either implement or not allow children.
  assert(false);
}

int64_t ViewManagerBase::GetChildCount(XamlView parent)
{
  return 0;
}

void ViewManagerBase::UpdateProperties(ShadowNodeBase* nodeToUpdate, XamlView viewToUpdate, dynamic reactDiffMap)
{
  // Directly dirty this node since non-layout changes like the text property do not trigger relayout
  //  There isn't actually a yoga node for RawText views, but it will invalidate the ancestors which
  //  will include the containing Text element. And that's what matters.
  int64_t tag = GetTag(viewToUpdate);
  auto instance = m_wkReactInstance.lock();
  if (instance != nullptr)
    static_cast<NativeUIManager*>(instance->NativeUIManager())->DirtyYogaNode(tag);

  for (auto& pair : reactDiffMap.items())
  {
    const folly::dynamic& propertyName = pair.first;
    const folly::dynamic& propertyValue = pair.second;

    if (propertyName == "onLayout")
    {
      nodeToUpdate->m_onLayout = !propertyValue.isNull() && propertyValue.asBool();
    }
  }
}

void ViewManagerBase::DispatchCommand(XamlView viewToUpdate, int64_t commandId, const folly::dynamic& commandArgs)
{
  assert(false); // View did not handle its command
}

void ViewManagerBase::SetLayoutProps(ShadowNodeBase& nodeToUpdate, XamlView viewToUpdate, float left, float top, float width, float height)
{
  auto element = viewToUpdate.as<winrt::UIElement>();
  if (element == nullptr) {
    // TODO: Assert
    return;
  }

  // TODO: Check if parent is indeed a Canvas
  winrt::Canvas::SetLeft(element, left);
  winrt::Canvas::SetTop(element, top);

  auto fe = element.as<winrt::FrameworkElement>();
  fe.Width(width);
  fe.Height(height);

  if (nodeToUpdate.m_onLayout)
  {
    int64_t tag = GetTag(viewToUpdate);
    folly::dynamic layout = folly::dynamic::object
      ("x", left)
      ("y", top)
      ("height", height)
      ("width", width);

    folly::dynamic eventData = folly::dynamic::object
      ("target", tag)
      ("layout", std::move(layout));

    auto instance = m_wkReactInstance.lock();
    if (instance != nullptr)
      instance->DispatchEvent(tag, "topLayout", std::move(eventData));
  }
}

YGMeasureFunc ViewManagerBase::GetYogaCustomMeasureFunc() const
{
  return nullptr;
}

bool ViewManagerBase::RequiresYogaNode() const
{
  return true;
}

bool ViewManagerBase::IsNativeControlWithSelfLayout() const
{
  return GetYogaCustomMeasureFunc() != nullptr;
}

} }
