#include "stdafx.h"

//#define VK_NO_PROTOTYPES
#include <cstdint>

using uint8=std::uint8_t;

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <assert.h>

#include <iostream>
#include <fstream>


#include "VulkanSwapChain.h"
#include "VulkanContext.h"
#include "tools.h"

VkHandle<VkShaderModule> createShaderModule(VkDevice vk_device, const char* spirv_path)
{
   std::ifstream input_file(spirv_path, std::ios::binary);
   std::vector<char> code((std::istreambuf_iterator<char>(input_file)),(std::istreambuf_iterator<char>()));
   
   VkShaderModuleCreateInfo createInfo = {};
   createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   createInfo.codeSize =  code.size();
   createInfo.pCode = (uint32_t*)code.data();

   VkShaderModule vk_shader;
   VK_CHECK(vkCreateShaderModule(vk_device, &createInfo, nullptr, &vk_shader));
   return VkHandle<VkShaderModule>(vk_shader, vk_device, vkDestroyShaderModule);
}


bool endsWith(std::string const &fullString, std::string const &ending)
{
   if (fullString.length() >= ending.length()) 
   {
      return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
   }
   else 
   {
      return false;
   }
}

VkShaderStageFlagBits getShaderStage(const std::string& filename)
{
   if (endsWith(filename, "vert.spv"))
   {
      return VK_SHADER_STAGE_VERTEX_BIT;
   }
   else if (endsWith(filename, "frag.spv"))
   {
      return VK_SHADER_STAGE_FRAGMENT_BIT;
   }
   else if (endsWith(filename, "tesc.spv"))
   {
      return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
   }
   else if (endsWith(filename, "tese.spv"))
   {
      return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
   }
   else if (endsWith(filename, "geom.spv"))
   {
      return VK_SHADER_STAGE_GEOMETRY_BIT;
   }
   else if (endsWith(filename, "comp.spv"))
   {
      return VK_SHADER_STAGE_COMPUTE_BIT;
   }
   else
      return (VkShaderStageFlagBits)0;
}

VkHandle<VkPipeline> createGraphicsPipeline(VkDevice vk_device, VkRenderPass vk_render_pass, const std::vector<std::string> shaders, int width, int height)
{
   VkPipelineShaderStageCreateInfo stage_create_info[4];
   VkHandle<VkShaderModule> shader_modules[4];

   int i = 0;
   for (const auto& stage : shaders)
   {
      shader_modules[i] = createShaderModule(vk_device, stage.c_str());
      
      VkPipelineShaderStageCreateInfo shaderStageInfo = {};
      shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shaderStageInfo.stage = getShaderStage(stage);
      shaderStageInfo.module = shader_modules[i];
      shaderStageInfo.pName = "main";
      stage_create_info[i] = shaderStageInfo;

      i++;
   }  

   VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
   vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   vertexInputInfo.vertexBindingDescriptionCount = 0;
   vertexInputInfo.vertexAttributeDescriptionCount = 0;

   VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
   inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   inputAssembly.primitiveRestartEnable = VK_FALSE;

   VkViewport viewport = {};
   viewport.x = 0.0f;
   viewport.y = 0.0f;
   viewport.width = (float)width;
   viewport.height = (float)height;
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;

   VkRect2D scissor = {};
   scissor.offset = { 0, 0 };
   scissor.extent = { uint32_t(width), uint32_t(height) };

   VkPipelineViewportStateCreateInfo viewportState = {};
   viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   viewportState.viewportCount = 1;
   viewportState.pViewports = &viewport;
   viewportState.scissorCount = 1;
   viewportState.pScissors = &scissor;

   VkPipelineRasterizationStateCreateInfo rasterizer = {};
   rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   rasterizer.depthClampEnable = VK_FALSE;
   rasterizer.rasterizerDiscardEnable = VK_FALSE;
   rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
   rasterizer.lineWidth = 1.0f;
   rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
   rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
   rasterizer.depthBiasEnable = VK_FALSE;

   VkPipelineMultisampleStateCreateInfo multisampling = {};
   multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   multisampling.sampleShadingEnable = VK_FALSE;
   multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

   VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
   colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
   colorBlendAttachment.blendEnable = VK_FALSE;

   VkPipelineColorBlendStateCreateInfo colorBlending = {};
   colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   colorBlending.logicOpEnable = VK_FALSE;
   colorBlending.logicOp = VK_LOGIC_OP_COPY;
   colorBlending.attachmentCount = 1;
   colorBlending.pAttachments = &colorBlendAttachment;
   colorBlending.blendConstants[0] = 0.0f;
   colorBlending.blendConstants[1] = 0.0f;
   colorBlending.blendConstants[2] = 0.0f;
   colorBlending.blendConstants[3] = 0.0f;

   VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
   pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   pipelineLayoutInfo.setLayoutCount = 0;
   pipelineLayoutInfo.pushConstantRangeCount = 0;

   VkPipelineLayout pipelineLayout;
   VK_CHECK(vkCreatePipelineLayout(vk_device, &pipelineLayoutInfo, nullptr, &pipelineLayout));//todo handle for pipeline layout

   VkGraphicsPipelineCreateInfo pipelineInfo = {};
   pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipelineInfo.stageCount = shaders.size();
   pipelineInfo.pStages = stage_create_info;
   pipelineInfo.pVertexInputState = &vertexInputInfo;
   pipelineInfo.pInputAssemblyState = &inputAssembly;
   pipelineInfo.pViewportState = &viewportState;
   pipelineInfo.pRasterizationState = &rasterizer;
   pipelineInfo.pMultisampleState = &multisampling;
   pipelineInfo.pColorBlendState = &colorBlending;
   pipelineInfo.layout = pipelineLayout;
   pipelineInfo.renderPass = vk_render_pass;
   pipelineInfo.subpass = 0;
   pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

   VkPipeline graphicsPipeline;
   VK_CHECK(vkCreateGraphicsPipelines(vk_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));
   return VkHandle<VkPipeline>(graphicsPipeline, vk_device, vkDestroyPipeline);
}

VkHandle<VkRenderPass> createRenderPass(VkDevice vk_device, VkFormat color_attachment_format)
{
   VkAttachmentDescription colorAttachment = {};
   colorAttachment.format = color_attachment_format;
   colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
   colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

   VkAttachmentReference colorAttachmentRef = {};
   colorAttachmentRef.attachment = 0;
   colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

   VkSubpassDescription subpass = {};
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   subpass.colorAttachmentCount = 1;
   subpass.pColorAttachments = &colorAttachmentRef;

   VkRenderPassCreateInfo renderPassInfo = {};
   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   renderPassInfo.attachmentCount = 1;
   renderPassInfo.pAttachments = &colorAttachment;
   renderPassInfo.subpassCount = 1;
   renderPassInfo.pSubpasses = &subpass;

   VkRenderPass render_pass;
   VK_CHECK(vkCreateRenderPass(vk_device, &renderPassInfo, nullptr, &render_pass));
   return VkHandle<VkRenderPass>(render_pass, vk_device, vkDestroyRenderPass);
}

/*std::vector<VkHandle<VkFramebuffer>> createFramebuffers(VkDevice vk_device, VkRenderPass render_pass, const VulkanSwapChain& swapchain)
{
   //swapChainFramebuffers.resize(swapChainImageViews.size(), VDeleter<VkFramebuffer>{device, vkDestroyFramebuffer});
   std::vector<VkHandle<VkFramebuffer>> framebuffers;
   for (size_t i = 0; i < swapChainImageViews.size(); i++) {
      VkImageView attachments[] = { swapchain.[i] };

      VkFramebufferCreateInfo framebufferInfo = {};
      framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass = renderPass;
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.pAttachments = attachments;
      framebufferInfo.width = swapChainExtent.width;
      framebufferInfo.height = swapChainExtent.height;
      framebufferInfo.layers = 1;

      if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, swapChainFramebuffers[i].replace()) != VK_SUCCESS) {
         throw std::runtime_error("failed to create framebuffer!");
   }

   return framebuffers;
}*/

int main()
{
   glfwInit();

   glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   int width = 800;
   int height = 600;
   
   GLFWwindow* window = glfwCreateWindow(width, height, "Vulkan window", nullptr, nullptr);

   VulkanContext context(window);
   VulkanSwapChain swapchain(context.vk_phys_device, context.vk_device, context.vk_surface, width, height);

   auto vertex_shader = createShaderModule(context.vk_device, "shader.vert.spv");
   VkHandle<VkShaderModule> a = std::move(vertex_shader);
   /*auto fragment_shader = createShaderModule(context.vk_device, "shader.frag.spv");*/
   
   auto vk_render_pass = createRenderPass(context.vk_device, swapchain.image_format);
   auto vk_pipeline = createGraphicsPipeline(context.vk_device, vk_render_pass, { "shader.vert.spv" , "shader.frag.spv" }, width, height);
   /*auto vk_framebuffer = createFramebuffer();*/


   uint32_t extensionCount = 0;
   vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
   std::cout << extensionCount << " extensions supported" << std::endl;


   glm::mat4 matrix;
   glm::vec4 vec;
   auto test = matrix * vec;

   while (!glfwWindowShouldClose(window))
   {
      glfwPollEvents();
   } 

   glfwDestroyWindow(window);
   glfwTerminate();

   return 0;
}

