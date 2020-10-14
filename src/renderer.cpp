#include "renderer.h"

Renderer::Renderer() {
    loadScene();
    initWindow();
    initVulkan();
    initUI();
}

Renderer::~Renderer() {
    std::cout << "Cleaning up" << std::endl;
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    // Cleanup DearImGui
    ui.cleanup();

    // Cleanup Vulkan
    vertexBuffer.cleanup(logicalDevice);
    vertIndexBuffer.cleanup(logicalDevice);

    for (auto& uniformBuffer : uniformBuffers) {
        uniformBuffer.cleanup(logicalDevice);
    }

    vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

    vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(logicalDevice, graphicsPipelineLayout, nullptr);
    vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

    swapchain.cleanup();

    vkDestroySampler(logicalDevice, textureSampler, nullptr);
    vkDestroyImageView(logicalDevice, textureView, nullptr);
    texture.cleanup(logicalDevice);

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(logicalDevice, nullptr);
    vkDestroyInstance(instance, nullptr);

    // Cleanup GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
}

VkCommandBuffer Renderer::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

bool Renderer::checkDeviceExtensions(VkPhysicalDevice device) {
    uint32_t extensionsCount = 0;
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Unable to enumerate device extensions!");
    }

    std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, availableExtensions.data());

    std::set<std::string> requiredDeviceExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        requiredDeviceExtensions.erase(extension.extensionName);
    }

    return requiredDeviceExtensions.empty();
}

bool Renderer::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }
    return true;
}

// Destroys all the resources associated with swapchain recreation
void Renderer::cleanupSwapchain() {
    for (size_t i = 0; i < swapchain.getImagesSize(); ++i) {
        uniformBuffers[i].cleanup(logicalDevice);
    }

    vkFreeCommandBuffers(logicalDevice, commandPool, static_cast<uint32_t>(commandBuffers.size()),
                     commandBuffers.data());

    vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(logicalDevice, graphicsPipelineLayout, nullptr);
    vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
    swapchain.cleanup();
}

void Renderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

Buffer::BufferContext Renderer::createBufferContext() {
    Buffer::BufferContext ctx = {};
    ctx.physicalDevice = physicalDevice;
    ctx.logicalDevice = logicalDevice;
    ctx.queue = graphicsQueue;
    ctx.commandPool = commandPool;

    return ctx;
}

void Renderer::createCommandBuffers() {
    commandBuffers.resize(swapchain.getFramebufferSize());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }

    for (size_t i = 0; i < commandBuffers.size(); ++i) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffers!");
        }

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapchain.getFramebuffer(i);
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapchain.getExtent();

        // Set the clear color value to black
        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        // Begin recording commands into the command buffer
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // We only have one vertex buffer right now but could easily expand this
        VkBuffer vertexBuffers[] = { vertexBuffer.getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffers[i], vertIndexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

        vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(vertIndices.size()), 1, 0, 0, 0);
        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffers!");
        }
    }
}

void Renderer::createCommandPool() {
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = queueIndices.graphicsFamilyIndex;

    if (vkCreateCommandPool(logicalDevice, &createInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create command pool!");
    }
}

// TODO Check if this is used by Dear IMGUI and how to move it
void Renderer::createDescriptorPool() {
    VkDescriptorPoolSize uboPoolSize = {};
    uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboPoolSize.descriptorCount = static_cast<uint32_t>(swapchain.getImagesSize());

    VkDescriptorPoolSize texturePoolSize = {};
    texturePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texturePoolSize.descriptorCount = static_cast<uint32_t>(swapchain.getImagesSize());

    std::array<VkDescriptorPoolSize, 2> poolSizes = { uboPoolSize, texturePoolSize };

    VkDescriptorPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.maxSets = static_cast<uint32_t>(swapchain.getImagesSize());
    createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    createInfo.pPoolSizes = poolSizes.data();

    if (vkCreateDescriptorPool(logicalDevice, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create descriptor pool!");
    }
}

void Renderer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboBinding = {};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding textureBinding = {};
    textureBinding.binding = 1;
    textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.descriptorCount = 1;
    textureBinding.pImmutableSamplers = nullptr;
    textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> descriptorLayouts = {uboBinding, textureBinding};

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(descriptorLayouts.size());
    layoutCreateInfo.pBindings = descriptorLayouts.data();

    if (vkCreateDescriptorSetLayout(logicalDevice, &layoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create descriptor set layouts!");
    }
}

void Renderer::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(swapchain.getImagesSize(), descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchain.getImagesSize());
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(swapchain.getImagesSize());
    if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Unable to allocate descriptor sets!");
    }

    for (size_t i = 0; i < swapchain.getImagesSize(); ++i) {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = uniformBuffers[i].getBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(Camera);

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureView;
        imageInfo.sampler = textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void Renderer::createGraphicsPipeline() {
    // Load our shader modules in from disk
    auto vertShaderCode = ShaderLoader::load("shaders/vert.spv");
    auto fragShaderCode = ShaderLoader::load("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Get an input binding
    auto vertexDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    // Create information struct for vertex input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Create information struct about input assembly
    // We'll be organizing our vertices in triangles and the GPU should treat the data accordingly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    // Create a viewport. The extent of the viewport will always be the size of the images in the swapchain
    VkExtent2D swapchainExtent = swapchain.getExtent();
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainExtent.width);
    viewport.height = static_cast<float>(swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Create a scissor. Any pixels outside the scissor region will be discarded
    VkRect2D scissor = {};
    scissor.extent = swapchainExtent;
    scissor.offset = {0, 0};

    VkPipelineViewportStateCreateInfo viewPortCreateInfo = {};
    viewPortCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewPortCreateInfo.viewportCount = 1;
    viewPortCreateInfo.pViewports = &viewport;
    viewPortCreateInfo.scissorCount = 1;
    viewPortCreateInfo.pScissors = &scissor;

    // Configure the rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizerCreateInfo.lineWidth = 1.0f;
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizerCreateInfo.depthBiasClamp = 0.0f;
    rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

    // Configure multisampling
    VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
    multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
    multisamplingCreateInfo.minSampleShading = 1.0f;
    multisamplingCreateInfo.pSampleMask = nullptr;
    multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisamplingCreateInfo.alphaToOneEnable = VK_FALSE;

    // Configure the color blending stage
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    // Create color blending info
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

    // Create a pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr,
                               &graphicsPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create graphics pipeline layout!");
    }

    // Create the info for our graphics pipeline. We'll add a programmable vertex and fragment shader stage
    VkGraphicsPipelineCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.stageCount = 2;
    createInfo.pStages = shaderStages;
    createInfo.pVertexInputState = &vertexInputInfo;
    createInfo.pInputAssemblyState = &inputAssemblyInfo;
    createInfo.pViewportState = &viewPortCreateInfo;
    createInfo.pRasterizationState = &rasterizerCreateInfo;
    createInfo.pMultisampleState = &multisamplingCreateInfo;
    createInfo.pDepthStencilState = nullptr;
    createInfo.pColorBlendState = &colorBlending;
    createInfo.pDynamicState = nullptr;
    createInfo.layout = graphicsPipelineLayout;
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;
    createInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr,
                                  &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create graphics pipeline!");
    }

    // Cleanup our shader modules after pipeline creation
    vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

VkImageView Renderer::createImageView(VkImage image, VkFormat format) {
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create a texture image view!");
    }

    return imageView;
}

void Renderer::createIndexBuffer() {
    auto ctx = createBufferContext();
    vertIndexBuffer.create(ctx, vertIndices);
}

void Renderer::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Unable to establish validation layer support!");
    }

    VkApplicationInfo applicationInfo;
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = "Renderer";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "No Engine";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0 , 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_2;

    // Grab the needed Vulkan extensions. This also initializes the list of required extensions
    requiredExtensions = getRequiredExtensions();
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

    // Enable validation layers and debug messenger if needed
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        debugCreateInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
    } else {
        instanceCreateInfo.enabledLayerCount = 0;
        debugCreateInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create a Vulkan instance!");
    }
}

void Renderer::createLogicalDevice() {
    // Setup our Command Queues
    std::set<uint32_t> uniqueQueueIndices = {queueIndices.graphicsFamilyIndex, queueIndices.presentFamilyIndex};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    const float priority = 1.0f;
    for (uint32_t queueIndex : uniqueQueueIndices) {
        VkDeviceQueueCreateInfo queueInfo = {};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueCount = 1;
        queueInfo.queueFamilyIndex = queueIndex;
        queueInfo.pQueuePriorities = &priority;
        queueCreateInfos.push_back(queueInfo);
    }

    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkPhysicalDeviceFeatures physicalDeviceFeatures = {}; // TODO Specify features
    physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

    deviceInfo.pEnabledFeatures = &physicalDeviceFeatures;

    if (enableValidationLayers) {
        deviceInfo.enabledLayerCount = validationLayers.size();
        deviceInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        deviceInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create logical device!");
    }

    // Grab the device queue handles for the graphics and present queues after logical device creation
    // queueIndex = 0 because we are only going to use one graphics queue per queue family
    vkGetDeviceQueue(logicalDevice, queueIndices.graphicsFamilyIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, queueIndices.presentFamilyIndex, 0, &presentQueue);
}

void Renderer::createRenderPass() {
    // Configure a color attachment that will determine how the framebuffer is used
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchain.getImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Comes before UI rendering now

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Define a subpass that is attached to the graphics pipeline
    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Create the info for the render pass
    VkRenderPassCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &colorAttachment;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpassDescription;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(logicalDevice, &createInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Could not create render pass!");
    }
}

VkShaderModule Renderer::createShaderModule(const std::vector<char> &shaderCode) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create shader module!");
    }

    return shaderModule;
}

// For cross-platform compatibility we let GLFW take care of the surface creation
void Renderer::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create window surface!");
    }
}

void Renderer::createSwapchain() {
    swapchain.init(createSwapchainContext());
}

Swapchain::SwapchainContext Renderer::createSwapchainContext() {
    Swapchain::SwapchainContext ctx = {};
    ctx.physicalDevice = physicalDevice;
    ctx.logicalDevice = logicalDevice;
    ctx.commandPool = commandPool;
    ctx.surface = surface;
    ctx.graphicsQueueIndex = queueIndices.graphicsFamilyIndex;
    ctx.presentQueueIndex = queueIndices.presentFamilyIndex;
    ctx.extentWidth = windowWidth;
    ctx.extentHeight = windowHeight;
    return ctx;
}

void Renderer::createSyncObjects() {
    // Create our semaphores and fences for synchronizing the GPU and CPU
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapchain.getImagesSize(), VK_NULL_HANDLE);

    // Create semaphores for the number of frames that can be submitted to the GPU at a time
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Set this flag to be initialized to be on
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr,
                              &imageAvailableSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("Unable to create semaphore!");
        }

        if (vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr,
                              &renderFinishedSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("Unable to create semaphore!");
        }

        if (vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr,
                          &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Unable to create fence!");
        }
    }
}

// TODO Look into recording a buffer that uploads all texture asynchronously
void Renderer::createTextureImage() {
    // Allocate a staging buffer
    auto ctx = createBufferContext();
    auto stagingBuffer = TextureBuffer();
    stagingBuffer.create(ctx, textureImage);

    // Allocate GPU texture memory
    texture = Texture(physicalDevice, logicalDevice, textureImage);

    // Record and execute commands to copy the image texture into memory
    transitionImageLayout(texture.getImage(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer.getBuffer(), texture.getImage(), static_cast<uint32_t>(textureImage.width),
                      static_cast<uint32_t>(textureImage.height));
    transitionImageLayout(texture.getImage(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Cleanup staging buffer
    stagingBuffer.cleanup(logicalDevice);
}

void Renderer::createTextureImageView() {
    textureView = createImageView(texture.getImage(), VK_FORMAT_R8G8B8A8_SRGB);
}

void Renderer::createTextureSampler() {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR; // TODO Grab this from glTF
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create texture sampler!");
    }
}

UserInterface::UIContext Renderer::createUIContext() {
    UserInterface::UIContext context = {};
    context.window = window;
    context.instance = instance;
    context.physicalDevice = physicalDevice;
    context.logicalDevice = logicalDevice;
    context.graphicsQueue = graphicsQueue;
    context.imageCount = swapchain.getImageCount();
    context.graphicsFamilyIndex = queueIndices.graphicsFamilyIndex;
    context.swapchain = swapchain;
    return context;
}

void Renderer::createUniformBuffers() {
    Camera cam = {};
    uniformBuffers.resize(swapchain.getImagesSize());
    for (size_t i = 0; i < swapchain.getImagesSize(); ++i) {
        uniformBuffers[i].create(createBufferContext(), cam);
    }
}

void Renderer::createVertexBuffer() {
    auto ctx = createBufferContext();
    vertexBuffer.create(ctx, vertices);
}

void Renderer::drawFrame() {
    // Sync for next frame. Fences also need to be manually reset unlike semaphores, which is done here
    vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(logicalDevice, swapchain.getSwapchain(), UINT64_MAX,
                                            imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    // If the window has been resized or another event causes the swap chain to become invalid,
    // we need to recreate it
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Unable to acquire swap chain!");
    }

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(logicalDevice, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    // Mark the image as now being in use by this frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    // Record UI draw data
    auto uiCommandBuffer = ui.recordCommands(imageIndex, swapchain.getExtent());

    updateUniformBuffer(imageIndex);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    std::array<VkCommandBuffer, 2> cmdBuffers = {commandBuffers[imageIndex], uiCommandBuffer};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
    submitInfo.pCommandBuffers = cmdBuffers.data();

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Reset the in-flight fences so we do not get blocked waiting on in-flight images
    vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {swapchain.getSwapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    // Submit the present queue
    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        // Recreate the swap chain if the window size has changed
        framebufferResized = false;
        recreateSwapchain();
    } else if (result != VK_SUCCESS ) {
        throw std::runtime_error("Unable to present the swap chain image!");
    }

    // Advance the current frame to get the semaphore data for the next frame
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}

void Renderer::getDeviceQueueIndices() {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueProperties.data());
    for (size_t i = 0; i < queueProperties.size(); ++i) {
        if (queueProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueIndices.graphicsFamilyIndex = i;
        }

        // Check if a queue supports presentation
        // If this returns false, make sure to enable DRI3 if using X11
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        if (presentSupport) {
            queueIndices.presentFamilyIndex = i;
        }

        if (queueProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queueIndices.computeFamilyIndex = i;
        }
    }
}

std::vector<const char *> Renderer::getRequiredExtensions() const {
    uint32_t glfwExtensionCount = 0;
    const char** glfwRequiredExtensions;
    glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwRequiredExtensions, glfwRequiredExtensions + glfwExtensionCount);
    for (const char* extension : extensions) {
        std::cout << extension << std::endl;
    }

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

void Renderer::initUI() {
    ui.init(createUIContext());
}

void Renderer::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    physicalDevice = pickPhysicalDevice();
    getDeviceQueueIndices();
    createLogicalDevice();
    createSwapchain();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    swapchain.initFramebuffers(renderPass);
    createCommandPool();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void Renderer::initWindow() {
    if (!glfwInit()) {
        throw std::runtime_error("Unable to initialize GLFW!");
    }

    // Do not load OpenGL and make window not resizable
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(windowWidth, windowHeight, "Blitz", nullptr, nullptr);
    glfwSetKeyCallback(window, keyCallback);

    // Add a pointer that allows GLFW to reference our instance
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Unable to create window!");
    }
}

bool Renderer::isDeviceSuitable(VkPhysicalDevice device) {
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);
    bool extensionsSupported = checkDeviceExtensions(device);

    // Check if Swap Chain support is adequate
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        Swapchain::SwapchainConfiguration swapchainConfig = Swapchain::querySwapchainSupport(device, surface);
        swapChainAdequate = !swapchainConfig.presentModes.empty() && !swapchainConfig.surfaceFormats.empty();
    }

    return swapChainAdequate && extensionsSupported && properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        supportedFeatures.samplerAnisotropy;
}

void Renderer::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void Renderer::loadMesh(tinygltf::Model &model, tinygltf::Mesh &mesh) {
    for (auto primitive : mesh.primitives) {
        if (primitive.indices >= 0) {
            // Mesh supports indexing. Load the indices
            loadMeshIndices(model, primitive);
        }

        loadVertexAttributes(model, mesh, primitive);

        if (primitive.material >= 0) {
            loadMeshMaterial(model, primitive);
        }
    }
}

void Renderer::loadMeshIndices(tinygltf::Model &model, tinygltf::Primitive &primitive) {
    auto indicesAccessor = model.accessors[primitive.indices];
    auto bufferView = model.bufferViews[indicesAccessor.bufferView];
    auto buffer = model.buffers[bufferView.buffer];
    auto bufferOffset = bufferView.byteOffset;
    auto offset = indicesAccessor.byteOffset / sizeof(float);
    if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        auto indicesBytes = reinterpret_cast<unsigned short*>(buffer.data.data() + bufferOffset);
        auto bufferStart = indicesBytes + offset;
        auto bufferEnd = indicesBytes + bufferView.byteLength / sizeof(unsigned short);
        std::vector<uint32_t> indices(bufferStart, bufferEnd);
        vertIndices.insert(vertIndices.end(), indices.begin(), indices.end());
    } else {
        auto indicesBytes = reinterpret_cast<unsigned int*>(buffer.data.data() + bufferOffset);
        auto bufferStart = indicesBytes + offset;
        auto bufferEnd = indicesBytes + bufferView.byteLength / sizeof(unsigned int);
        std::vector<uint32_t> indices(bufferStart, bufferEnd);
        vertIndices.insert(vertIndices.end(), indices.begin(), indices.end());
    }
}

void Renderer::loadMeshMaterial(tinygltf::Model &model, tinygltf::Primitive &primitive) {
    // TODO Expand the logic here later to properly handle PBR-related things
    auto material = model.materials[primitive.material];
    if (material.pbrMetallicRoughness.baseColorTexture.index >= 0) {
        auto tex = model.textures[material.pbrMetallicRoughness.baseColorTexture.index];
        textureImage = model.images[tex.source];
        sampler = model.samplers[tex.sampler];
    }
}

void Renderer::loadVertexAttributes(tinygltf::Model &model, tinygltf::Mesh &mesh, tinygltf::Primitive &primitive) {
    auto positionIndex = primitive.attributes.find("POSITION")->second;
    auto normalIndex = primitive.attributes.find("NORMAL")->second;
    auto texIndex = primitive.attributes.find("TEXCOORD_0")->second;

    if (positionIndex >= 0) {
        auto positionAccessor = model.accessors[positionIndex];
        auto bufferView = model.bufferViews[positionAccessor.bufferView];
        auto buffer = model.buffers[bufferView.buffer];
        // Kinda hacky way of initializing the vertices buffer since we are assuming
        // the POSITION attribute is always present (although that seems quite reasonable).
        vertices.resize(positionAccessor.count);

        // Calculate the necessary offsets for the position of the vertices out of the buffer
        auto positionBytes = reinterpret_cast<float *>(buffer.data.data() + bufferView.byteOffset);
        auto componentOffset = positionAccessor.byteOffset / sizeof(float);
        auto bufferStart = positionBytes + componentOffset;
        auto bufferEnd = positionBytes + bufferView.byteLength / sizeof(float);
        std::vector<float> positions(bufferStart, bufferEnd);
        for (size_t i = 0; i < positionAccessor.count; ++i) {
            vertices[i].position[0] = positions[i * 3];
            vertices[i].position[1] = positions[i * 3 + 1];
            vertices[i].position[2] = positions[i * 3 + 2];
        }
    }

    if (normalIndex >= 0) {
        auto normalAccessor = model.accessors[normalIndex];
        auto bufferView = model.bufferViews[normalAccessor.bufferView];
        auto buffer = model.buffers[bufferView.buffer];

        // Calculate the necessary offsets for the vertex normals out of the buffer
        auto positionBytes = reinterpret_cast<float *>(buffer.data.data() + bufferView.byteOffset);
        auto componentOffset = normalAccessor.byteOffset / sizeof(float);
        auto bufferStart = positionBytes + componentOffset;
        auto bufferEnd = positionBytes + bufferView.byteLength / sizeof(float);
        std::vector<float> normals(bufferStart, bufferEnd);
        for (size_t i = 0; i < normalAccessor.count; ++i) {
            vertices[i].normal[0] = normals[i * 3];
            vertices[i].normal[1] = normals[i * 3 + 1];
            vertices[i].normal[2] = normals[i * 3 + 2];
        }
    }

    if (texIndex >= 0) {
        auto texAccessor = model.accessors[texIndex];
        auto bufferView = model.bufferViews[texAccessor.bufferView];
        auto buffer = model.buffers[bufferView.buffer];

        // Calculate the necessary offsets for the vertex normals out of the buffer
        auto positionBytes = reinterpret_cast<float *>(buffer.data.data() + bufferView.byteOffset);
        auto componentOffset = texAccessor.byteOffset / sizeof(float);
        auto bufferStart = positionBytes + componentOffset;
        auto bufferEnd = positionBytes + bufferView.byteLength / sizeof(float);
        std::vector<float> texCoords(bufferStart, bufferEnd);
        for (size_t i = 0; i < texAccessor.count; ++i) {
            vertices[i].texCoord[0] = texCoords[i * 2];
            vertices[i].texCoord[1] = texCoords[i * 2 + 1];
        }
    }
}

void Renderer::loadNode(tinygltf::Model &model, tinygltf::Node &node) {
    if (node.mesh >= 0 && node.mesh < model.meshes.size()) {
        loadMesh(model, model.meshes[node.mesh]);
    }

    // Parse child nodes
    for (int i : node.children) {
        loadNode(model, model.nodes[i]);
    }
}

void Renderer::loadScene() {
    auto model = GLTFLoader::load("models/box-textured/BoxTextured.gltf");
    auto scene = model.scenes[model.defaultScene];
    for (int nodeId : scene.nodes) {
        loadNode(model, model.nodes[nodeId]);
    }
}

VkPhysicalDevice Renderer::pickPhysicalDevice() {
    uint32_t physicalDeviceCount = 0;
    if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Unable to enumerate physical devices!");
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

    if (physicalDevices.empty()) {
        throw std::runtime_error("No physical devices are available that support Vulkan!");
    }

    for (const auto& device : physicalDevices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        if (isDeviceSuitable(device)) {
            std::cout << "Using discrete GPU: " << properties.deviceName << std::endl;
            return device;
        }
    }

    // Did not find a discrete GPU, pick the first device from the list as a fallback
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevices[0], &properties);
    // Need to check if this is at least a physical device with GPU capabilities
    if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
        throw std::runtime_error("Did not find a physical GPU on this system!");
    }

    std::cout << "Using fallback physical device: " << properties.deviceName << std::endl;
    return physicalDevices[0];
}

// In case the swapchain is invalidated, i.e. during window resizing,
// we need to implement a mechanism to recreate it
void Renderer::recreateSwapchain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        // Idle wait in case our application has been minimized
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    windowWidth = width;
    windowHeight = height;

    vkDeviceWaitIdle(logicalDevice);

    cleanupSwapchain();

    // Recreate main application Vulkan resources
    createSwapchain();
    createRenderPass();
    createGraphicsPipeline();
    swapchain.initFramebuffers(renderPass);
    createDescriptorPool();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();

    // We also need to take care of the UI
    ImGui_ImplVulkan_SetMinImageCount(swapchain.getImageCount());
    ui.recreate(createUIContext());
}

void Renderer::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ui.draw();
        drawFrame();
    }

    // Wait for unfinished work on GPU before ending application
    vkDeviceWaitIdle(logicalDevice);
}

void Renderer::setupDebugMessenger() {
    if (!enableValidationLayers) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    PopulateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("Failed to setup debug messenger!");
    }
}

void Renderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    // Choose the correct pipeline transition based on the old and current image layouts
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("Unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(commandBuffer);
}

void Renderer::updateUniformBuffer(size_t bufferIdx) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    auto extent = swapchain.getExtent();
    auto options = ui.getOptions();
    if (options.rotate) {
        cam.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    }
    cam.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    cam.proj = glm::perspective(glm::radians(45.0f), extent.width / (float) extent.height, 0.1f, 10.0f);
    cam.proj[1][1] *= -1;

    void* data;
    vkMapMemory(logicalDevice, uniformBuffers[bufferIdx].getDeviceMemory(), 0, sizeof(cam), 0, &data);
    memcpy(data, &cam, sizeof(cam));
    vkUnmapMemory(logicalDevice, uniformBuffers[bufferIdx].getDeviceMemory());
}