//
// Created by tim on 7/13/20.
//

#include "application.h"

Application::Application() {
    initWindow();
    initVulkan();
}

Application::~Application() {
    std::cout << "Cleaning up" << std::endl;
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    // Cleanup Vulkan
    vkDestroySurfaceKHR(instance, surface, nullptr);
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

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
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

    // Grab the needed Vulkan extensions
    auto extensions = getRequiredExtensions();
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

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
    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    //uint32_t                           queueCreateInfoCount;
    //const VkDeviceQueueCreateInfo*     pQueueCreateInfos;
    //uint32_t                           enabledLayerCount;
    //const char* const*                 ppEnabledLayerNames;
    //uint32_t                           enabledExtensionCount;
    //const char* const*                 ppEnabledExtensionNames;
    //const VkPhysicalDeviceFeatures*    pEnabledFeatures;

    //if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
    //    std::cout << "Unable to create logical device!" << std::endl;
    //    exit(EXIT_FAILURE);
    //}
}

void Application::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        std::cout << "Unable to create window surface!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void Application::getDeviceQueueIndices() {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueProperties.data());
    for (int i = 0; i < queueProperties.size(); ++i) {
        if (queueProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            std::cout << "Found graphics queue index" << std::endl;
            queueIndices.graphicsFamily = i;
        }
    }
}

std::vector<const char *> Application::getRequiredExtensions() const {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    for (const char* extension : extensions) {
        std::cout << extension << std::endl;
    }

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

void Application::initVulkan() {
    createInstance();
    setupDebugMessenger();
    physicalDevice = pickPhysicalDevice();

    getDeviceQueueIndices();

    createLogicalDevice();
    createSurface();

    const float priorities[] = { 1.0f };
    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueCount = 1; // TODO Figure out if we need to make this dynamic
    queueInfo.queueFamilyIndex = queueIndices.graphicsFamily;
    queueInfo.pQueuePriorities = priorities;
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
    return extensionsSupported && properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
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
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
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

void Application::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
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
