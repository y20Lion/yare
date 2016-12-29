#include "stdafx.h"
#include "VulkanContext.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "tools.h"
#include <iostream>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags,
                                                    VkDebugReportObjectTypeEXT objType,
                                                    uint64_t obj,
                                                    size_t location,
                                                    int32_t code,
                                                    const char* layerPrefix,
                                                    const char* msg,
                                                    void* userData)
{

   std::cerr << "validation layer: " << msg << std::endl;

   return VK_FALSE;
}
namespace vkExt
{
   PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallbackEXT;
   PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallbackEXT;
}

void loadFunctionPointers(VkInstance vk_instance)
{
   vkExt::CreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vk_instance, "vkCreateDebugReportCallbackEXT");
   vkExt::DestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(vk_instance, "vkDestroyDebugReportCallbackEXT");
}


VkResult createDebugReportCallbackEXT(VkInstance vk_instance, VkDebugReportCallbackEXT* pCallback)
{
   VkDebugReportCallbackCreateInfoEXT debug_callback_info = {};
   debug_callback_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
   debug_callback_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
   debug_callback_info.pfnCallback = debugCallback;

   return vkExt::CreateDebugReportCallbackEXT(vk_instance, &debug_callback_info, nullptr, pCallback);
}

#define DEBUG_LAYERS

std::vector<const char*> getRequiredExtensions()
{
   unsigned int glfwExtensionCount = 0;
   const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

   std::vector<const char*> extensions;
   for (unsigned int i = 0; i < glfwExtensionCount; i++)
   {
      extensions.push_back(glfwExtensions[i]);
   }

#ifdef DEBUG_LAYERS
   extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

   return extensions;
}


std::vector<const char*> getDebugLayers()
{
   std::vector<const char*> validation_layers = { "VK_LAYER_LUNARG_standard_validation" };
   return validation_layers;
}

VkInstance createVkInstance()
{
   VkApplicationInfo appInfo = {};
   appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   appInfo.pApplicationName = "VulkanLab";
   appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.pEngineName = "yare";
   appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

   auto extension_names = getRequiredExtensions();

   VkInstanceCreateInfo createInfo = {};
   createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   createInfo.pApplicationInfo = &appInfo;
   createInfo.enabledExtensionCount = uint32_t(extension_names.size());
   createInfo.ppEnabledExtensionNames = extension_names.data();

#ifdef DEBUG_LAYERS
   auto validation_layers = getDebugLayers();
   createInfo.enabledLayerCount = uint32_t(validation_layers.size());
   createInfo.ppEnabledLayerNames = validation_layers.data();
#endif

   VkInstance vk_instance;
   VK_CHECK(vkCreateInstance(&createInfo, nullptr, &vk_instance));

   loadFunctionPointers(vk_instance);

   return vk_instance;
}

VkPhysicalDevice getPhysicalDevice(VkInstance vk_instance)
{
   VkPhysicalDevice physical_device = VK_NULL_HANDLE;

   uint32_t device_count = 0;
   vkEnumeratePhysicalDevices(vk_instance, &device_count, nullptr);
   assert(device_count != 0);

   std::vector<VkPhysicalDevice> devices(device_count);
   vkEnumeratePhysicalDevices(vk_instance, &device_count, devices.data());

   VkPhysicalDeviceProperties device_properties;
   vkGetPhysicalDeviceProperties(devices[0], &device_properties);

   return devices[0];
}

std::vector<VkQueueFamilyProperties> getQueueFamilies(VkPhysicalDevice vk_device)
{
   uint32_t queue_family_count = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(vk_device, &queue_family_count, nullptr);

   std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
   vkGetPhysicalDeviceQueueFamilyProperties(vk_device, &queue_family_count, queue_families.data());

   return queue_families;
}

struct VkDeviceAndQueues
{
   VkDevice vk_device;
   VkQueue vk_graphics_queue;
   VkQueue vk_compute_queue;
   VkQueue vk_present_queue;
   VkQueue vk_transfer_queue;
};

VkDeviceQueueCreateInfo queueCreateInfo(uint32_t queue_family_index, uint32_t queue_count, const float* queue_priority)
{
   VkDeviceQueueCreateInfo queue_create_info = {};
   queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
   queue_create_info.queueFamilyIndex = queue_family_index;
   queue_create_info.queueCount = queue_count;
   queue_create_info.pQueuePriorities = queue_priority;

   return queue_create_info;
}

VkDeviceAndQueues createDeviceAndQueues(VkPhysicalDevice vk_phys_device, VkSurfaceKHR vk_surface)
{
   std::vector<VkQueueFamilyProperties> families = getQueueFamilies(vk_phys_device);

   VkBool32 presentSupport = false;
   vkGetPhysicalDeviceSurfaceSupportKHR(vk_phys_device, 0, vk_surface, &presentSupport);

   const uint32_t general_family_index = 0;
   const uint32_t transfer_family_index = 1;

   float queue_priority[10] = { 1.0f };
   VkDeviceQueueCreateInfo queue_create_info[2];
   queue_create_info[0] = queueCreateInfo(general_family_index, 3, queue_priority);
   queue_create_info[1] = queueCreateInfo(transfer_family_index, 1, queue_priority);

   VkPhysicalDeviceFeatures device_features = {};

   VkDeviceCreateInfo device_info = {};
   device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   device_info.pQueueCreateInfos = queue_create_info;
   device_info.queueCreateInfoCount = 2;
   device_info.pEnabledFeatures = &device_features;

#ifdef DEBUG_LAYERS
   auto validation_layers = getDebugLayers();
   device_info.enabledLayerCount = uint32_t(validation_layers.size());
   device_info.ppEnabledLayerNames = validation_layers.data();
#endif

   std::vector<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
   device_info.enabledExtensionCount = uint32_t(extensions.size());
   device_info.ppEnabledExtensionNames = extensions.data();

   VkDeviceAndQueues result;
   VK_CHECK(vkCreateDevice(vk_phys_device, &device_info, nullptr, &result.vk_device));

   vkGetDeviceQueue(result.vk_device, general_family_index, 0, &result.vk_graphics_queue);
   vkGetDeviceQueue(result.vk_device, general_family_index, 1, &result.vk_compute_queue);
   vkGetDeviceQueue(result.vk_device, general_family_index, 2, &result.vk_present_queue);
   vkGetDeviceQueue(result.vk_device, transfer_family_index, 0, &result.vk_transfer_queue);

   return result;
}

VulkanContext::VulkanContext(GLFWwindow* window)
{
   vk_instance = VkHandle<VkInstance>(createVkInstance(), vkDestroyInstance);

   VkDebugReportCallbackEXT debug_callback;
   VK_CHECK(createDebugReportCallbackEXT(vk_instance, &debug_callback));
   vk_debug_callback = VkHandle<VkDebugReportCallbackEXT>(debug_callback, vk_instance, vkExt::DestroyDebugReportCallbackEXT);

   VkSurfaceKHR surface;
   VK_CHECK(glfwCreateWindowSurface(vk_instance, window, nullptr, &surface));
   vk_surface = VkHandle<VkSurfaceKHR>(surface, vk_instance, vkDestroySurfaceKHR);

   vk_phys_device = getPhysicalDevice(vk_instance);
   VkDeviceAndQueues queues = createDeviceAndQueues(vk_phys_device, vk_surface);   
   vk_device = VkHandle<VkDevice>(queues.vk_device, vkDestroyDevice);
   vk_graphics_queue = queues.vk_graphics_queue;
   vk_compute_queue = queues.vk_compute_queue;
   vk_present_queue = queues.vk_present_queue;
   vk_transfer_queue = queues.vk_transfer_queue;
}

VulkanContext::~VulkanContext()
{

}