#pragma once

#include <functional>
#include <vulkan/vulkan.h>

template <typename T>
class VkHandle {
public:
   VkHandle() : VkHandle(VK_NULL_HANDLE, [](T, VkAllocationCallbacks*) {}) {}

   VkHandle(T object, std::function<void(T, VkAllocationCallbacks*)> deletef): object(object) {
      this->deleter = [=](T obj) { deletef(obj, nullptr); };
   }

   VkHandle(T object, VkInstance instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) : object(object ){
      this->deleter = [&instance, deletef](T obj) { deletef(instance, obj, nullptr); };
   }

   VkHandle(T object, VkDevice device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef): object(object) {
      this->deleter = [&device, deletef](T obj) { deletef(device, obj, nullptr); };
   }

   ~VkHandle() {
      cleanup();
   }

   VkHandle(VkHandle<T> && other) noexcept
   {
      cleanup();      
      deleter = other.deleter;
      object = other.object;
      other.object = VK_NULL_HANDLE;
   }

   VkHandle& operator =(VkHandle<T> && other)
   {
      cleanup();      
      deleter = other.deleter;
      object = other.object;
      other.object = VK_NULL_HANDLE;
      return *this;
   }

   const T* operator &() const {
      return &object;
   }

   T* replace() {
      cleanup();
      return &object;
   }

   operator T() const {
      return object;
   }

   void operator=(T rhs) {
      if (rhs != object) {
         cleanup();
         object = rhs;
      }
   }

   template<typename V>
   bool operator==(V rhs) {
      return object == T(rhs);
   }

private:
   T object{ VK_NULL_HANDLE };
   std::function<void(T)> deleter;

   void cleanup() {
      if (object != VK_NULL_HANDLE) {
         deleter(object);
      }
      object = VK_NULL_HANDLE;
   }
};