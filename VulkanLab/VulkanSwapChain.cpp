#include "stdafx.h"
#include "VulkanSwapChain.h"

#include "stl_helpers.h"

struct SwapChainSupport
{
   VkSurfaceCapabilitiesKHR capabilities;
   std::vector<VkSurfaceFormatKHR> formats;
   std::vector<VkPresentModeKHR> presentModes;
};

bool operator ==(const VkSurfaceFormatKHR& lhs, const VkSurfaceFormatKHR& rhs)
{
   return lhs.colorSpace == rhs.colorSpace && lhs.format == lhs.format;
}

SwapChainSupport getSwapChainSupport(VkPhysicalDevice vk_phys_device, VkSurfaceKHR vk_surface)
{
   SwapChainSupport details;

   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_phys_device, vk_surface, &details.capabilities);

   uint32_t format_count;
   vkGetPhysicalDeviceSurfaceFormatsKHR(vk_phys_device, vk_surface, &format_count, nullptr);
   if (format_count != 0)
   {
      details.formats.resize(format_count);
      vkGetPhysicalDeviceSurfaceFormatsKHR(vk_phys_device, vk_surface, &format_count, details.formats.data());
   }

   uint32_t present_mode_count;
   vkGetPhysicalDeviceSurfacePresentModesKHR(vk_phys_device, vk_surface, &present_mode_count, nullptr);
   if (present_mode_count != 0)
   {
      details.presentModes.resize(present_mode_count);
      vkGetPhysicalDeviceSurfacePresentModesKHR(vk_phys_device, vk_surface, &present_mode_count, details.presentModes.data());
   }

   return details;
}


VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
   VkSurfaceFormatKHR desired_format = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

   if (available_formats.size() == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED)
   {
      return desired_format;
   }

   if (std::find(RANGE(available_formats), desired_format) != available_formats.end())
      return desired_format;

   return available_formats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> available_prensent_modes)
{
   VkPresentModeKHR desired_mode = VK_PRESENT_MODE_MAILBOX_KHR;

   if (std::find(RANGE(available_prensent_modes), desired_mode) != available_prensent_modes.end())
      return desired_mode;

   return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height)
{
   if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
   {
      return capabilities.currentExtent;
   }
   else
   {
      VkExtent2D actualExtent = { uint32_t(width), uint32_t(height) };

      actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
      actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

      return actualExtent;
   }
}

uint32_t chooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities)
{
   uint32_t imageCount = capabilities.minImageCount + 1; //todo check if +1
   if (capabilities.maxImageCount)
   {
      imageCount = std::min(imageCount, capabilities.maxImageCount);
   }
   return imageCount;
}

VulkanSwapChain::VulkanSwapChain(VkPhysicalDevice vk_phys_device, VkDevice vk_device, VkSurfaceKHR vk_surface, int surface_width, int surface_height)
{
   _createSwapChain(vk_phys_device, vk_device, vk_surface, surface_width, surface_height);
   _createSwapChainImages(vk_device);
   surfaceWidth = surface_width;
   surfaceHeight = surface_height;


   VkSemaphoreCreateInfo semaphoreInfo = {};
   semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

   VkSemaphore semaphore;
   VK_CHECK(vkCreateSemaphore(vk_device, &semaphoreInfo, nullptr, &semaphore));
   imageAvailableSemaphore = VkHandle<VkSemaphore>(semaphore, vk_device, vkDestroySemaphore);
   VK_CHECK(vkCreateSemaphore(vk_device, &semaphoreInfo, nullptr, &semaphore));
   renderFinishedSemaphore = VkHandle<VkSemaphore>(semaphore, vk_device, vkDestroySemaphore);
}

VulkanSwapChain::~VulkanSwapChain()
{

}

void VulkanSwapChain::_createSwapChain(VkPhysicalDevice vk_phys_device, VkDevice vk_device, VkSurfaceKHR vk_surface, int surface_width, int surface_height)
{
   SwapChainSupport swap_chain_support = getSwapChainSupport(vk_phys_device, vk_surface);

   VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats);
   VkPresentModeKHR present_mode = chooseSwapPresentMode(swap_chain_support.presentModes);
   VkExtent2D extent = chooseSwapExtent(swap_chain_support.capabilities, surface_width, surface_height);
   uint32_t imageCount = chooseImageCount(swap_chain_support.capabilities);

   VkSwapchainCreateInfoKHR createInfo = {};
   createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
   createInfo.surface = vk_surface;
   createInfo.minImageCount = imageCount;
   createInfo.imageFormat = surface_format.format;
   createInfo.imageColorSpace = surface_format.colorSpace;
   createInfo.imageExtent = extent;
   createInfo.imageArrayLayers = 1;
   createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; //todo check if dst necessary

   createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
   createInfo.queueFamilyIndexCount = 0;
   createInfo.pQueueFamilyIndices = nullptr;

   /*QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
   uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

   if (indices.graphicsFamily != indices.presentFamily)
   {
   createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
   createInfo.queueFamilyIndexCount = 2;
   createInfo.pQueueFamilyIndices = queueFamilyIndices;
   }
   else
   {
   createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
   createInfo.queueFamilyIndexCount = 0; // Optional
   createInfo.pQueueFamilyIndices = nullptr; // Optional
   }*/

   createInfo.preTransform = swap_chain_support.capabilities.currentTransform;
   createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
   createInfo.presentMode = present_mode;
   createInfo.clipped = VK_TRUE;
   createInfo.oldSwapchain = VK_NULL_HANDLE;

   image_format = surface_format.format;
   VkSwapchainKHR swap_chain;
   VK_CHECK(vkCreateSwapchainKHR(vk_device, &createInfo, nullptr, &swap_chain));
   vk_swap_chain = VkHandle<VkSwapchainKHR>(swap_chain, vk_device, vkDestroySwapchainKHR);
}


void VulkanSwapChain::_createSwapChainImages(VkDevice vk_device)
{
   uint32_t image_count;
   vkGetSwapchainImagesKHR(vk_device, vk_swap_chain, &image_count, nullptr);
   swap_chain_images.resize(image_count);
   vkGetSwapchainImagesKHR(vk_device, vk_swap_chain, &image_count, swap_chain_images.data());

   for (uint32_t i = 0; i < swap_chain_images.size(); i++)
   {
      VkImageViewCreateInfo createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      createInfo.image = swap_chain_images[i];
      createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      createInfo.format = image_format;
      createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      createInfo.subresourceRange.baseMipLevel = 0;
      createInfo.subresourceRange.levelCount = 1;
      createInfo.subresourceRange.baseArrayLayer = 0;
      createInfo.subresourceRange.layerCount = 1;

      VkImageView image_view;
      VK_CHECK(vkCreateImageView(vk_device, &createInfo, nullptr, &image_view));
      swap_chain_image_views.push_back(VkHandle<VkImageView>(image_view, vk_device, vkDestroyImageView));
   }
}
