#include "ui.h"

void blitz::UserInterface::cleanupResources() {
    for (auto framebuffer : framebuffers) {
        vkDestroyFramebuffer(context.logicalDevice, framebuffer, nullptr);
    }

    vkFreeCommandBuffers(context.logicalDevice, commandPool, static_cast<uint32_t>(commandBuffers.size()),
                         commandBuffers.data());
}

void blitz::UserInterface::cleanup() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    cleanupResources();

    // TODO Investigate issue where resizing window leaks descriptor pool resources
    vkDestroyDescriptorPool(context.logicalDevice, descriptorPool, nullptr);
    vkDestroyCommandPool(context.logicalDevice, commandPool, nullptr);
    vkDestroyRenderPass(context.logicalDevice, renderPass, nullptr);
}

void blitz::UserInterface::draw() {
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (ImGui::CollapsingHeader("Camera")) {
        ImGui::Text("Position (X: %.4f Y: %.4f Z: %.4f)", options.camPosition[0], options.camPosition[1], options.camPosition[2]);

        ImGui::InputFloat("X", &options.camPosition[0], 0.1f, 2.0f, "%.4f");
        ImGui::InputFloat("Y", &options.camPosition[1], 0.1f, 2.0f, "%.4f");
        ImGui::InputFloat("Z", &options.camPosition[2], 0.1f, 2.0f, "%.4f");

        ImGui::Text("Look At (X: %.4f Y: %.4f Z: %.4f)", options.camLookAt[0], options.camLookAt[1], options.camLookAt[2]);
    }

    ImGui::Checkbox("Rotate Object", &options.rotate);

    //ImGui::ShowDemoWindow();

    ImGui::Render();
}

void blitz::UserInterface::init(const UIContext &ctx) {
    context = ctx;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // Initialize some DearImgui specific resources
    createDescriptorPool();
    createRenderPass();
    createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    createCommandBuffers();
    createFramebuffers();

    // Provide bind points from Vulkan API
    ImGui_ImplGlfw_InitForVulkan(context.window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = context.instance;
    init_info.PhysicalDevice = context.physicalDevice;
    init_info.Device = context.logicalDevice;
    init_info.QueueFamily = context.graphicsFamilyIndex;
    init_info.Queue = context.graphicsQueue;
    init_info.DescriptorPool = descriptorPool;
    init_info.MinImageCount = context.imageCount;
    init_info.ImageCount = context.imageCount;
    ImGui_ImplVulkan_Init(&init_info, renderPass);

    // Upload the fonts for DearImgui
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);
    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    endSingleTimeCommands(commandBuffer, commandPool);
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

VkCommandBuffer blitz::UserInterface::beginSingleTimeCommands(VkCommandPool cmdPool) const {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = cmdPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = {};
    vkAllocateCommandBuffers(context.logicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Could not create one-time command buffer!");
    }

    return commandBuffer;
}

void blitz::UserInterface::createCommandBuffers() {
    commandBuffers.resize(context.imageCount);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(context.logicalDevice, &commandBufferAllocateInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Unable to allocate UI command buffers!");
    }
}

void blitz::UserInterface::createCommandPool(VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = context.graphicsFamilyIndex;
    commandPoolCreateInfo.flags = flags;

    if (vkCreateCommandPool(context.logicalDevice, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Could not create graphics command pool!");
    }
}

// Copied this code from DearImgui's setup:
// https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/main.cpp
void blitz::UserInterface::createDescriptorPool() {
    VkDescriptorPoolSize pool_sizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;
    if (vkCreateDescriptorPool(context.logicalDevice, &pool_info, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Cannot allocate UI descriptor pool!");
    }
}

void blitz::UserInterface::createFramebuffers() {
    // Create some UI framebuffers. These will be used in the render pass for the UI
    framebuffers.resize(context.imageCount);
    VkImageView attachment[1];
    VkExtent2D extent = context.swapchain.getExtent();
    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = renderPass;
    info.attachmentCount = 1;
    info.pAttachments = attachment;
    info.width = extent.width;
    info.height = extent.height;
    info.layers = 1;
    for (uint32_t i = 0; i < context.swapchain.getImagesSize(); ++i) {
        attachment[0] = context.swapchain.getImageView(i);
        if (vkCreateFramebuffer(context.logicalDevice, &info, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Unable to create UI framebuffers!");
        }
    }
}

void blitz::UserInterface::createRenderPass() {
    // Create an attachment description for the render pass
    VkAttachmentDescription attachmentDescription = {};
    attachmentDescription.format = context.swapchain.getImageFormat();
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Need UI to be drawn on top of main
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Last pass so we want to present after
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // Create a color attachment reference
    VkAttachmentReference attachmentReference = {};
    attachmentReference.attachment = 0;
    attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Create a subpass
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachmentReference;

    // Create a subpass dependency to synchronize our main and UI render passes
    // We want to render the UI after the geometry has been written to the framebuffer
    // so we need to configure a subpass dependency as such
    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Create external dependency
    subpassDependency.dstSubpass = 0; // The geometry subpass comes first
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Wait on writes
    subpassDependency.dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Finally create the UI render pass
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &attachmentDescription;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &subpassDependency;

    if (vkCreateRenderPass(context.logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create UI render pass!");
    }
}

void blitz::UserInterface::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool cmdPool) const {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.graphicsQueue);

    vkFreeCommandBuffers(context.logicalDevice, cmdPool, 1, &commandBuffer);
}

VkCommandBuffer blitz::UserInterface::recordCommands(uint32_t bufferIdx, VkExtent2D swapchainExtent) {
    VkCommandBufferBeginInfo cmdBufferBegin = {};
    cmdBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBegin.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffers[bufferIdx], &cmdBufferBegin) != VK_SUCCESS) {
        throw std::runtime_error("Unable to start recording UI command buffer!");
    }

    VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffers[bufferIdx];
    renderPassBeginInfo.renderArea.extent.width = swapchainExtent.width;
    renderPassBeginInfo.renderArea.extent.height = swapchainExtent.height;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffers[bufferIdx], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Grab and record the draw data for Dear Imgui
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[bufferIdx]);

    // End and submit render pass
    vkCmdEndRenderPass(commandBuffers[bufferIdx]);

    if (vkEndCommandBuffer(commandBuffers[bufferIdx]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffers!");
    }

    return commandBuffers[bufferIdx];
}

void blitz::UserInterface::recreate(const UIContext &ctx) {
    cleanupResources();

    context = ctx;

    ImGui_ImplVulkan_SetMinImageCount(context.imageCount);
    createCommandBuffers();
    createFramebuffers();
}