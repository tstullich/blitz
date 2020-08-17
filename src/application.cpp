#include "application.h"

Application::Application() {
    initWindow();
    initVulkan();
    initUI();
}

Application::~Application() {
    std::cout << "Cleaning up" << std::endl;
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    // Cleanup ImGui
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup Vulkan
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

    for (auto framebuffer : swapchainFramebuffers) {
        vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
    }

    vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(logicalDevice, graphicsPipelineLayout, nullptr);
    vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

    for (auto imageView : swapchainImageViews) {
        vkDestroyImageView(logicalDevice, imageView, nullptr);
    }

    vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(logicalDevice, nullptr);
    vkDestroyInstance(instance, nullptr);

    // Cleanup GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Application::checkDeviceExtensions(VkPhysicalDevice device) {
    uint32_t extensionsCount = 0;
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr) != VK_SUCCESS) {
        std::cout << "Unable to enumerate device extensions!" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, availableExtensions.data());

    std::set<std::string> requiredDeviceExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        requiredDeviceExtensions.erase(extension.extensionName);
    }

    return requiredDeviceExtensions.empty();
}

bool Application::checkValidationLayerSupport() {
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

void Application::createCommandBuffers() {
    commandBuffers.resize(swapchainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        std::cout << "Failed to allocate command buffers!" << std::endl;
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < commandBuffers.size(); ++i) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            std::cout << "Failed to begin recording command buffers!" << std::endl;
            exit(EXIT_FAILURE);
        }

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapchainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapchainExtent;

        // Set the clear color value to black
        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        // Begin recording commands into the command buffer
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            std::cout << "Failed to record command buffers!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

void Application::createCommandPool() {
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = queueIndices.graphicsFamilyIndex;

    if (vkCreateCommandPool(logicalDevice, &createInfo, nullptr, &commandPool) != VK_SUCCESS) {
        std::cout << "Unable to create command pool" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void Application::createFramebuffers() {
    swapchainFramebuffers.resize(swapchainImageViews.size());
    for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
        // We need to attach an image view to the frame buffer for presentation purposes
        VkImageView attachments[] = {swapchainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr,
                                &swapchainFramebuffers[i]) != VK_SUCCESS) {
            std::cout << "Unable to create framebuffer!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

void Application::createGraphicsPipeline() {
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

    // Create information struct for vertex input
    // TODO: Adjust this so that we actually take input from a vertex buffer
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    // Create information struct about input assembly
    // We'll be organizing our vertices in triangles and the GPU should treat the data accordingly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    // Create a viewport. The extent of the viewport will always be the size of the images in the swapchain
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
    scissor.offset = { 0, 0};

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
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
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

    if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr,
                               &graphicsPipelineLayout) != VK_SUCCESS) {
        std::cout << "Unable to create graphics pipeline layout!" << std::endl;
        exit(EXIT_FAILURE);
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
        std::cout << "Unable to create graphics pipeline!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Cleanup our shader modules after pipeline creation
    vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

void Application::createImageViews() {
    swapchainImageViews.resize(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); ++i) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // We do not enable mip-mapping
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(logicalDevice, &createInfo, nullptr,
                              &swapchainImageViews[i]) != VK_SUCCESS) {
            std::cout << "Unable to create image views!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

void Application::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        std::cout << "Unable to establish validation layer support!" << std::endl;
        exit(EXIT_FAILURE);
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
        std::cout << "Unable to create a Vulkan instance!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void Application::createLogicalDevice() {
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
    deviceInfo.pEnabledFeatures = &physicalDeviceFeatures;

    if (enableValidationLayers) {
        deviceInfo.enabledLayerCount = validationLayers.size();
        deviceInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        deviceInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
        std::cout << "Unable to create logical device!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Grab the device queue handles for the graphics and present queues after logical device creation
    // queueIndex = 0 because we are only going to use one graphics queue per queue family
    vkGetDeviceQueue(logicalDevice, queueIndices.graphicsFamilyIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, queueIndices.presentFamilyIndex, 0, &presentQueue);
}

void Application::createRenderPass() {
    // Configure a color attachment that will determine how the framebuffer is used
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchainImageFormat;
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
        std::cout << "Could not create render pass!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

VkShaderModule Application::createShaderModule(const std::vector<char> &shaderCode) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cout << "Unable to create shader module!" << std::endl;
        exit(EXIT_FAILURE);
    }

    return shaderModule;
}

// For cross-platform compatibility we let GLFW take care of the surface creation
void Application::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        std::cout << "Unable to create window surface!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void Application::createSwapchain() {
    SwapchainConfiguration configuration = querySwapchainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = pickSwapchainSurfaceFormat(configuration.surfaceFormats);
    VkExtent2D extent = pickSwapchainExtent(configuration.capabilities);
    VkPresentModeKHR presentMode = pickSwapchainPresentMode(configuration.presentModes);

    uint32_t imageCount = configuration.capabilities.minImageCount + 1;
    if (configuration.capabilities.maxImageCount > 0 && imageCount > configuration.capabilities.maxImageCount) {
        // In case we are exceeding the maximum capacity for swap chain images we reset the value
        imageCount = configuration.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // TODO Change this later to setup for compute

    // Check if the graphics and present queues are the same and setup
    // sharing of the swap chain accordingly
    uint32_t indices[] = { queueIndices.graphicsFamilyIndex, queueIndices.presentFamilyIndex };
    if (queueIndices.presentFamilyIndex == queueIndices.graphicsFamilyIndex) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    } else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = indices;
    }

    swapchainCreateInfo.preTransform = configuration.capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(logicalDevice, &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS) {
        std::cout << "Unable to create swap chain!" << std::endl;
        exit(EXIT_FAILURE);
    }

    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = extent;

    // Store the handles to the swap chain images for later use
    uint32_t swapchainCount;
    vkGetSwapchainImagesKHR(logicalDevice, swapchain, &swapchainCount, nullptr);
    swapchainImages.resize(swapchainCount);
    vkGetSwapchainImagesKHR(logicalDevice, swapchain, &swapchainCount, swapchainImages.data());
}

void Application::createSyncObjects() {
    // Create our semaphores and fences for synchronizing the GPU and CPU
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapchainImages.size(), VK_NULL_HANDLE);

    // Create semaphores for the number of frames that can be submitted to the GPU at a time
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Set this flag to be initialized to be on
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr,
                              &imageAvailableSemaphores[i]) != VK_SUCCESS) {
            std::cout << "Unable to create semaphore!" << std::endl;
            exit(EXIT_FAILURE);
        }

        if (vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr,
                              &renderFinishedSemaphores[i]) != VK_SUCCESS) {
            std::cout << "Unable to create semaphore!" << std::endl;
            exit(EXIT_FAILURE);
        }

        if (vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr,
                          &inFlightFences[i]) != VK_SUCCESS) {
            std::cout << "Unable to create fence!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}


void Application::drawFrame() {
    // Sync for next frame. Fences also need to be manually reset unlike semaphores, which is done here
    vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(logicalDevice, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
                          VK_NULL_HANDLE, &imageIndex);

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(logicalDevice, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    // Mark the image as now being in use by this frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Reset the in-flight fences so we do not get blocked waiting on in-flight images
    vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        std::cout << "Failed to submit draw command buffer!" << std::endl;
        exit(EXIT_FAILURE);
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    // Submit the present queue
    vkQueuePresentKHR(presentQueue, &presentInfo);

    // Simple way for syncing queue submissions and GPU work
    // TODO: Improve this
    vkDeviceWaitIdle(logicalDevice);

    // Advance the current frame to get the semaphore data for the next frame
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::getDeviceQueueIndices() {
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

std::vector<const char *> Application::getRequiredExtensions() const {
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

void Application::initUI() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
}

void Application::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    physicalDevice = pickPhysicalDevice();
    getDeviceQueueIndices();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

void Application::initWindow() {
    if (!glfwInit()) {
        std::cout << "Unable to initialize GLFW!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Do not load OpenGL and make window not resizable
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Renderer", nullptr, nullptr);
    glfwSetKeyCallback(window, keyCallback);

    if (!window) {
        std::cout << "Unable to create window!" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
}

bool Application::isDeviceSuitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);
    bool extensionsSupported = checkDeviceExtensions(device);

    // Check if Swap Chain support is adequate
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapchainConfiguration swapchainConfig = querySwapchainSupport(device);
        swapChainAdequate = !swapchainConfig.presentModes.empty() && !swapchainConfig.surfaceFormats.empty();
    }

    return swapChainAdequate && extensionsSupported && properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

// TODO Later add logic to check if compute capability is available
void Application::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

VkPhysicalDevice Application::pickPhysicalDevice() {
    uint32_t physicalDeviceCount = 0;
    if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) != VK_SUCCESS) {
        std::cout << "Unable to enumerate physical devices!" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

    if (physicalDevices.empty()) {
        std::cout << "No physical devices are available that support Vulkan!";
        exit(EXIT_FAILURE);
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
        std::cout << "Did not find a physical GPU on this system!" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Using fallback physical device: " << properties.deviceName << std::endl;
    return physicalDevices[0];
}

VkExtent2D Application::pickSwapchainExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
    if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
        return surfaceCapabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = { WINDOW_WIDTH, WINDOW_HEIGHT };
        actualExtent.width = std::max(surfaceCapabilities.minImageExtent.width,
                                      std::min(surfaceCapabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(surfaceCapabilities.minImageExtent.height,
                                       std::min(surfaceCapabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

VkPresentModeKHR Application::pickSwapchainPresentMode(const std::vector<VkPresentModeKHR> &presentModes) {
    // Look for triple-buffering present mode if available
    for (VkPresentModeKHR availableMode : presentModes) {
        if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availableMode;
        }
    }

    // Use FIFO mode as our fallback, since it's the only guaranteed mode
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR Application::pickSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats) {
    for (VkSurfaceFormatKHR availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    // As a fallback choose the first format
    // TODO Could establish a ranking and pick best one
    return formats[0];
}

Application::SwapchainConfiguration Application::querySwapchainSupport(const VkPhysicalDevice &device) {
    SwapchainConfiguration config = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &config.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount > 0) {
        config.surfaceFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device,
                                             surface,
                                             &formatCount,
                                             config.surfaceFormats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount > 0) {
        config.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device,
                                                  surface,
                                                  &presentModeCount,
                                                  config.presentModes.data());
    }

    return config;
}

void Application::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }

    // Wait for unfinished work on GPU before ending application
    vkDeviceWaitIdle(logicalDevice);
}

void Application::setupDebugMessenger() {
    if (!enableValidationLayers) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    PopulateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        std::cout << "Failed to setup debug messenger!" << std::endl;
        exit(EXIT_FAILURE);
    }
}