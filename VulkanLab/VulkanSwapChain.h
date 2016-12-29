#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

#include "VkHandle.h"

class VulkanSwapChain
{
public:
   VulkanSwapChain(VkPhysicalDevice vk_phys_device, VkDevice vk_device, VkSurfaceKHR vk_surface, int surface_width, int surface_height);
   ~VulkanSwapChain();

   VkHandle<VkSwapchainKHR> vk_swap_chain;
   std::vector<VkImage> swap_chain_images;
   std::vector<VkHandle<VkImageView>> swap_chain_image_views;
   VkFormat image_format;

private:
   void _createSwapChain(VkPhysicalDevice vk_phys_device, VkDevice vk_device, VkSurfaceKHR vk_surface, int surface_width, int surface_height);
   void _createSwapChainImages(VkDevice vk_device);
};