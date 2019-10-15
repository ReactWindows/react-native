// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "ByteArrayBuffer.h"
#include "ChakraObjectRef.h"
#include "ChakraRuntimeArgs.h"

#include "jsi/jsi.h"

#ifdef CHAKRACORE
#include "ChakraCore.h"
#include "ChakraCoreDebugger.h"
#else
#ifndef USE_EDGEMODE_JSRT
#define USE_EDGEMODE_JSRT
#endif
#include "jsrt.h"
#endif

#include <memory>
#include <mutex>
#include <sstream>

#if !defined(CHAKRACORE)
class DebugProtocolHandler {};
class DebugService {};
#endif

namespace Microsoft::JSI {

class ChakraRuntime : public facebook::jsi::Runtime {
 public:
  ChakraRuntime(ChakraRuntimeArgs &&args) noexcept;
  ~ChakraRuntime() noexcept;

#pragma region Functions_inherited_from_Runtime

  facebook::jsi::Value evaluateJavaScript(
      const std::shared_ptr<const facebook::jsi::Buffer> &buffer,
      const std::string &sourceURL) override;

  std::shared_ptr<const facebook::jsi::PreparedJavaScript> prepareJavaScript(
      const std::shared_ptr<const facebook::jsi::Buffer> &buffer,
      std::string sourceURL) override;

  facebook::jsi::Value evaluatePreparedJavaScript(
      const std::shared_ptr<const facebook::jsi::PreparedJavaScript> &js)
      override;

  facebook::jsi::Object global() override;

  std::string description() override;

  bool isInspectable() override;

  // We use the default instrumentation() implementation that returns an
  // Instrumentation instance which returns no metrics.

 private:
  // Despite the name "clone" suggesting a deep copy, a return value of these
  // functions points to a new heap allocated ChakraPointerValue whose memeber
  // ChakraObjectRef refers to the same JavaScript object as the member
  // ChakraObjectRef of *pointerValue. This behavior is consistent with that of
  // HermesRuntime and JSCRuntime. Also, Like all ChakraPointerValues, the
  // return value must only be used as an argument to the constructor of
  // jsi::Pointer or one of its derived classes.
  PointerValue *cloneSymbol(const PointerValue *pointerValue) override;
  PointerValue *cloneString(const PointerValue *pointerValue) override;
  PointerValue *cloneObject(const PointerValue *pointerValue) override;
  PointerValue *clonePropNameID(const PointerValue *pointerValue) override;

  facebook::jsi::PropNameID createPropNameIDFromAscii(
      const char *str,
      size_t length) override;
  facebook::jsi::PropNameID createPropNameIDFromUtf8(
      const uint8_t *utf8,
      size_t length) override;
  facebook::jsi::PropNameID createPropNameIDFromString(
      const facebook::jsi::String &str) override;
  std::string utf8(const facebook::jsi::PropNameID &id) override;
  bool compare(
      const facebook::jsi::PropNameID &lhs,
      const facebook::jsi::PropNameID &rhs) override;

  std::string symbolToString(const facebook::jsi::Symbol &s) override;

  facebook::jsi::String createStringFromAscii(const char *str, size_t length)
      override;
  facebook::jsi::String createStringFromUtf8(const uint8_t *utf8, size_t length)
      override;
  std::string utf8(const facebook::jsi::String &str) override;

  facebook::jsi::Object createObject() override;
  facebook::jsi::Object createObject(
      std::shared_ptr<facebook::jsi::HostObject> ho) override;
  std::shared_ptr<facebook::jsi::HostObject> getHostObject(
      const facebook::jsi::Object &) override;
  facebook::jsi::HostFunctionType &getHostFunction(
      const facebook::jsi::Function &) override;

  facebook::jsi::Value getProperty(
      const facebook::jsi::Object &obj,
      const facebook::jsi::PropNameID &name) override;
  facebook::jsi::Value getProperty(
      const facebook::jsi::Object &obj,
      const facebook::jsi::String &name) override;
  bool hasProperty(
      const facebook::jsi::Object &obj,
      const facebook::jsi::PropNameID &name) override;
  bool hasProperty(
      const facebook::jsi::Object &obj,
      const facebook::jsi::String &name) override;
  void setPropertyValue(
      facebook::jsi::Object &obj,
      const facebook::jsi::PropNameID &name,
      const facebook::jsi::Value &value) override;
  void setPropertyValue(
      facebook::jsi::Object &obj,
      const facebook::jsi::String &name,
      const facebook::jsi::Value &value) override;

  bool isArray(const facebook::jsi::Object &obj) const override;
  bool isArrayBuffer(const facebook::jsi::Object &obj) const override;
  bool isFunction(const facebook::jsi::Object &obj) const override;
  bool isHostObject(const facebook::jsi::Object &obj) const override;
  bool isHostFunction(const facebook::jsi::Function &func) const override;
  // Returns the names of all enumerable properties of an object. This
  // corresponds the properties iterated through by the JavaScript for..in loop.
  facebook::jsi::Array getPropertyNames(
      const facebook::jsi::Object &obj) override;

  facebook::jsi::WeakObject createWeakObject(
      const facebook::jsi::Object &obj) override;
  facebook::jsi::Value lockWeakObject(
      const facebook::jsi::WeakObject &weakObj) override;

  facebook::jsi::Array createArray(size_t length) override;
  size_t size(const facebook::jsi::Array &arr) override;
  size_t size(const facebook::jsi::ArrayBuffer &arrBuf) override;
  // The lifetime of the buffer returned is the same as the lifetime of the
  // ArrayBuffer. The returned buffer pointer does not count as a reference to
  // the ArrayBuffer for the purpose of garbage collection.
  uint8_t *data(const facebook::jsi::ArrayBuffer &arrBuf) override;
  facebook::jsi::Value getValueAtIndex(
      const facebook::jsi::Array &arr,
      size_t index) override;
  void setValueAtIndexImpl(
      facebook::jsi::Array &arr,
      size_t index,
      const facebook::jsi::Value &value) override;

  facebook::jsi::Function createFunctionFromHostFunction(
      const facebook::jsi::PropNameID &name,
      unsigned int paramCount,
      facebook::jsi::HostFunctionType func) override;
  facebook::jsi::Value call(
      const facebook::jsi::Function &func,
      const facebook::jsi::Value &jsThis,
      const facebook::jsi::Value *args,
      size_t count) override;
  facebook::jsi::Value callAsConstructor(
      const facebook::jsi::Function &func,
      const facebook::jsi::Value *args,
      size_t count) override;

  // For now, pushing a scope does nothing, and popping a scope forces the
  // JavaScript garbage collector to run.
  ScopeState *pushScope() override;
  void popScope(ScopeState *) override;

  bool strictEquals(
      const facebook::jsi::Symbol &a,
      const facebook::jsi::Symbol &b) const override;
  bool strictEquals(
      const facebook::jsi::String &a,
      const facebook::jsi::String &b) const override;
  bool strictEquals(
      const facebook::jsi::Object &a,
      const facebook::jsi::Object &b) const override;

  bool instanceOf(
      const facebook::jsi::Object &obj,
      const facebook::jsi::Function &func) override;

#pragma endregion Functions_inherited_from_Runtime

 public:
  // These three functions only performs shallow copies.
  ChakraObjectRef ToChakraObjectRef(const facebook::jsi::Value &value);
  std::vector<ChakraObjectRef> ToChakraObjectRefs(
      const facebook::jsi::Value *value,
      size_t count);
  facebook::jsi::Value ToJsiValue(ChakraObjectRef &&ref);

  // TODO (yicyao):
  // JsValueRef->JSValue (needs make.*Value so it must be member function)
  facebook::jsi::Value createValue(JsValueRef value) const;
  // Value->JsValueRef (similar to above)
  JsValueRef valueRef(const facebook::jsi::Value &value);

 protected:
  ChakraRuntimeArgs &runtimeArgs() {
    return m_args;
  }

 private:
  // ChakraPointerValue is needed for working with Facebook's jsi::Pointer class
  // and must only be used for this purpose. Every instance of
  // ChakraPointerValue should be allocated on the heap and be used as an
  // argument to the constructor of jsi::Pointer or one of its derived classes.
  // Pointer makes sure that invalidate(), which frees the heap allocated
  // ChakraPointerValue, is called upon destruction. Since the constructor of
  // jsi::Pointer is protected, we usually have to invoke it through
  // jsi::Runtime::make. The code should look something like:
  //
  //     make<Pointer>(new ChakraPointerValue(...));
  //
  // or you can use the helper function MakePointer(), as defined below.
  template <typename T>
  struct ChakraPointerValueTemplate : PointerValue {
   public:
    ChakraPointerValueTemplate(const T &ref) noexcept : m_ref{ref} {
      static_assert(
          std::is_same<T, ChakraObjectRef>::value ||
              // Since only ChakraCore offers the JsWeakRef type alias, we
              // cannot use it here; so void* is the best alternative we can use
              // here.
              std::is_same<T, void *>::value,
          "ChakraPointerValueTemplate should only be instantiated for "
          "ChakraObjectRef and JsWeakRef.");
    }

    ChakraPointerValueTemplate(T &&ref) noexcept : m_ref{std::move(ref)} {}

    // Declaring ~ChakraPointerValueTemplate() private prevents the compiler
    // from implicitly generating the following functions, so we have to tell
    // the compiler to do so.
    ChakraPointerValueTemplate(
        const ChakraPointerValueTemplate &other) noexcept = default;
    ChakraPointerValueTemplate(ChakraPointerValueTemplate &&other) noexcept =
        default;
    ChakraPointerValueTemplate &operator=(
        const ChakraPointerValueTemplate &rhs) noexcept = default;
    ChakraPointerValueTemplate &operator=(
        ChakraPointerValueTemplate &&rhs) noexcept = default;

    inline void invalidate() noexcept override {
      delete this;
    }

    inline const T &GetRef() const noexcept {
      return m_ref;
    }

   private:
    // ~ChakraPointerValueTemplate() should only be invoked by invalidate().
    // Hence we make it private.
    ~ChakraPointerValueTemplate() noexcept = default;

    T m_ref;
  };

  using ChakraPointerValue = ChakraPointerValueTemplate<ChakraObjectRef>;

  template <typename T>
  inline T MakePointer(ChakraObjectRef &&ref) {
    static_assert(
        std::is_base_of<facebook::jsi::Pointer, T>::value,
        "MakePointer should only be instantiated for classes derived from "
        "facebook::jsi::Pointer.");
    return make<T>(new ChakraPointerValue(std::move(ref)));
  }

  // The pointer passed to this function must point to a ChakraPointerValue.
  inline static ChakraPointerValue *CloneChakraPointerValue(
      const PointerValue *pointerValue) {
    return new ChakraPointerValue(
        *(static_cast<const ChakraPointerValue *>(pointerValue)));
  }

  // The jsi::Pointer passed to this function must hold a ChakraPointerValue.
  inline static const ChakraObjectRef &GetChakraObjectRef(
      const facebook::jsi::Pointer &p) {
    return static_cast<const ChakraPointerValue *>(getPointerValue(p))
        ->GetRef();
  }

  class HostObjectProxy {
   public:
    facebook::jsi::Value Get(const facebook::jsi::PropNameID &propNameId) {
      return hostObject_->get(runtime_, propNameId);
    }

    void Set(
        const facebook::jsi::PropNameID &propNameId,
        const facebook::jsi::Value &value) {
      hostObject_->set(runtime_, propNameId, value);
    }

    std::vector<facebook::jsi::PropNameID> Enumerator() {
      return hostObject_->getPropertyNames(runtime_);
    }

    HostObjectProxy(
        ChakraRuntime &rt,
        const std::shared_ptr<facebook::jsi::HostObject> &hostObject)
        : runtime_(rt), hostObject_(hostObject) {}
    std::shared_ptr<facebook::jsi::HostObject> getHostObject() {
      return hostObject_;
    }

   private:
    ChakraRuntime &runtime_;
    std::shared_ptr<facebook::jsi::HostObject> hostObject_;
  };

  template <class T>
  class ObjectWithExternalData : public facebook::jsi::Object {
   public:
    static facebook::jsi::Object create(ChakraRuntime &rt, T *externalData);
    static ObjectWithExternalData<T> fromExisting(
        ChakraRuntime &rt,
        facebook::jsi::Object &&obj);

   public:
    T *getExternalData();
    ObjectWithExternalData(const Runtime::PointerValue *value)
        : Object(const_cast<Runtime::PointerValue *>(value)) {
    } // TODO :: const_cast

    ObjectWithExternalData(ObjectWithExternalData &&) = default;
    ObjectWithExternalData &operator=(ObjectWithExternalData &&) = default;
  };

  template <class T>
  friend class ObjectWithExternalData;

  // TODO (yicyao)
  inline void checkException(JsErrorCode res);
  inline void checkException(JsErrorCode res, const char *msg);
  void ThrowUponJsError(JsErrorCode error, const char *const chakraApiName);

  // TODO (yicyao)
  // static JsWeakRef newWeakObjectRef(const facebook::jsi::Object &obj);
  // static JsValueRef strongObjectRef(const facebook::jsi::WeakObject &obj);

  // TODO (yicyao)
  // Factory methods for creating String/Object
  // facebook::jsi::String createString(JsValueRef stringRef) const;
  // facebook::jsi::PropNameID createPropNameID(JsValueRef stringRef);

  template <class T>
  facebook::jsi::Object createObject(JsValueRef objectRef, T *externalData)
      const;
  facebook::jsi::Object createObject(JsValueRef objectRef) const;

  facebook::jsi::Object createProxy(
      facebook::jsi::Object &&target,
      facebook::jsi::Object &&handler) noexcept;
  facebook::jsi::Function createProxyConstructor() noexcept;
  facebook::jsi::Object createHostObjectProxyHandler() noexcept;

  // TODO (yicyao)
  // Used by factory methods and clone methods
  facebook::jsi::Runtime::PointerValue *makeStringValue(JsValueRef str) const;
  template <class T>
  facebook::jsi::Runtime::PointerValue *makeObjectValue(
      JsValueRef obj,
      T *externaldata) const;
  facebook::jsi::Runtime::PointerValue *makeObjectValue(JsValueRef obj) const;
  facebook::jsi::Runtime::PointerValue *makePropertyIdValue(
      JsPropertyIdRef propId) const;
  facebook::jsi::Runtime::PointerValue *makeWeakRefValue(JsWeakRef obj) const;

  // Promise Helpers
  static void CALLBACK
  PromiseContinuationCallback(JsValueRef funcRef, void *callbackState) noexcept;
  static void CALLBACK PromiseRejectionTrackerCallback(
      JsValueRef promise,
      JsValueRef reason,
      bool handled,
      void *callbackState);

  void PromiseContinuation(JsValueRef value) noexcept;
  void
  PromiseRejectionTracker(JsValueRef promise, JsValueRef reason, bool handled);

  void setupNativePromiseContinuation() noexcept;

  // Memory tracker helpers
  void setupMemoryTracker() noexcept;

  // In-proc debugging helpers
  void startDebuggingIfNeeded();
  void stopDebuggingIfNeeded();

  JsErrorCode enableDebugging(
      JsRuntimeHandle runtime,
      std::string const &runtimeName,
      bool breakOnNextLine,
      uint16_t port,
      std::unique_ptr<DebugProtocolHandler> &debugProtocolHandler,
      std::unique_ptr<DebugService> &debugService);
  void ProcessDebuggerCommandQueue();

  static void CALLBACK ProcessDebuggerCommandQueueCallback(void *callbackState);

  // Version related helpers
  static void initRuntimeVersion() noexcept;
  static uint64_t getRuntimeVersion() {
    return s_runtimeVersion;
  }

  // Miscellaneous
  std::unique_ptr<const facebook::jsi::Buffer> generatePreparedScript(
      const std::string &sourceURL,
      const facebook::jsi::Buffer &sourceBuffer) noexcept;
  facebook::jsi::Value evaluateJavaScriptSimple(
      const facebook::jsi::Buffer &buffer,
      const std::string &sourceURL);
  bool evaluateSerializedScript(
      const facebook::jsi::Buffer &scriptBuffer,
      const facebook::jsi::Buffer &serializedScriptBuffer,
      const std::string &sourceURL);
  static JsValueRef CALLBACK HostFunctionCall(
      JsValueRef callee,
      bool isConstructCall,
      JsValueRef *argumentsIncThis,
      unsigned short argumentCountIncThis,
      void *callbackState);

  static std::once_flag s_runtimeVersionInitFlag;
  static uint64_t s_runtimeVersion;

  // Arguments shared by the specializations
  ChakraRuntimeArgs m_args;

  JsRuntimeHandle m_runtime;
  ChakraObjectRef m_context;

  // Note: For simplicity, We are pinning the script and serialized script
  // buffers in the facebook::jsi::Runtime instance assuming as these buffers
  // are needed to stay alive for the lifetime of the facebook::jsi::Runtime
  // implementation. This approach doesn't make sense for other external buffers
  // which may get created during the execution as that will stop the backing
  // buffer from getting released when the JSValue gets collected.

  // These buffers are kept to serve the source callbacks when evaluating
  // serialized scripts.
  std::vector<std::shared_ptr<const facebook::jsi::Buffer>> m_pinnedScripts;

  // These buffers back the external array buffers that we handover to
  // ChakraCore.
  std::vector<std::shared_ptr<const facebook::jsi::Buffer>>
      m_pinnedPreparedScripts;

  static constexpr const char *const s_proxyGetHostObjectTargetPropName =
      "$$ProxyGetHostObjectTarget$$";
  static constexpr const char *const s_proxyIsHostObjectPropName =
      "$$ProxyIsHostObject$$";

  std::string m_debugRuntimeName;
  int m_debugPort{0};
  std::unique_ptr<DebugProtocolHandler> m_debugProtocolHandler;
  std::unique_ptr<DebugService> m_debugService;
  constexpr static char DebuggerDefaultRuntimeName[] = "runtime1";
  constexpr static int DebuggerDefaultPort = 9229;
};

} // namespace Microsoft::JSI
