#pragma once

#include <vulkan/vulkan.hpp>
#include "VkHandle.h"

struct GLFWwindow;

class VulkanContext
{
public:
   VulkanContext(GLFWwindow* window);
   ~VulkanContext();

   VkHandle<VkInstance> vk_instance;
   VkHandle<VkDebugReportCallbackEXT> vk_debug_callback;
   VkHandle<VkSurfaceKHR> vk_surface;
   VkPhysicalDevice vk_phys_device;
   VkHandle<VkDevice> vk_device;
   VkQueue vk_graphics_queue;
   VkQueue vk_compute_queue;
   VkQueue vk_present_queue;
   VkQueue vk_transfer_queue;
};