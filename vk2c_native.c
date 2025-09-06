#include "vk2c_native.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback
        ( __attribute__((unused)) VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity
        , __attribute__((unused)) VkDebugUtilsMessageTypeFlagsEXT messageType
        , const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
        , __attribute__((unused)) void* pUserData) {

    vk2cLog(VK2C_LOG_VALIDATION, "%s", (char*)pCallbackData->pMessage);
    return VK_FALSE;
}

void vk2cMakeContext(Vk2cContex_t* ctx)
{
    *ctx = (Vk2cContex_t) {
        .window = NULL,
        .debugMessenger = VK_NULL_HANDLE,
        .instance = VK_NULL_HANDLE,
        .physicalDevice = VK_NULL_HANDLE,
        .device = VK_NULL_HANDLE,
        .surface = VK_NULL_HANDLE,
        .queueFamilyIndex = UINT32_MAX,
        .queue = VK_NULL_HANDLE,
        .swapchain = VK_NULL_HANDLE,
        .swapchainImageCount = UINT32_MAX,
        .swapchainImages = NULL,
        .swapchainImageViewCount = UINT32_MAX,
        .swapchainImageViews = NULL,
        .renderPass = VK_NULL_HANDLE,
        .swapchainExtent = {},
        .swapchainFormat = {},
        .framebuffers =  NULL,
        .framebufferCount = UINT32_MAX,
    };
}

VK2CResult_t vk2cCreateContext(Vk2cContex_t* ctx)
{
    assert(ctx);
#ifdef VK2C_DEBUG
    vk2cLog(VK2C_LOG_INFO, "This Application is mode of DEBUG");
    const char* layer[] = {
        "VK_LAYER_KHRONOS_validation",
    };
#endif
    const char* instanceExtensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#ifdef __APPLE__
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        "VK_EXT_metal_surface",
#endif
    };
    const char* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset"
    };

    VK2CResult_t result = VK2C_OK;
    result = vk2cCreateGLFWWindow(ctx, 600, 450, "Vulkan Example");
    if (result != VK2C_OK)
    {
        return result;
    }
#ifdef VK2C_DEBUG
    result = vk2cQueryInstanceLayer(NULL, 0);
    if (result != VK2C_OK)
    {
        return result;
    }
    result = vk2cQueryInstanceExtension(NULL, 0);
    if (result != VK2C_OK)
    {
        return result;
    }

    result = vk2cQueryInstanceLayer(layer, sizeof(layer) / sizeof(*layer));
    if (result != VK2C_OK)
    {
        return result;
    }
#endif
    result = vk2cQueryInstanceExtension(instanceExtensions, VK2C_ARRAY_SIZE(instanceExtensions));
    if (result != VK2C_OK)
    {
        return result;
    }
#ifdef VK2C_DEBUG
    result = vk2cCreateInstance(ctx, instanceExtensions, VK2C_ARRAY_SIZE(instanceExtensions), layer, VK2C_ARRAY_SIZE(layer));
#else
    result = vk2cCreateInstance(ctx, instanceExtensions, VK2C_ARRAY_SIZE(instanceExtensions), NULL, 0);
#endif
    if (result != VK2C_OK)
    {
        return result;
    }
#ifdef VK2C_DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    vk2cMakeDebugMessengerCreateInfoEXT(&debugCreateInfo);
    vk2cCreateDebugMessenger(ctx, &debugCreateInfo);
#endif
    result = vk2cCreateSurface(ctx);
    if (result != VK2C_OK)
    {
        return result;
    }

    result = vk2cPickPhysicalDevice(ctx);
    if (result != VK2C_OK)
    {
        return result;
    }
#ifdef VK2C_DEBUG
    vk2cQueryQueueFamily(ctx->physicalDevice, 0);
#endif

    vk2cQueryDeviceExtension(ctx->physicalDevice);
    result = vk2cCreateDevice(ctx, deviceExtensions, VK2C_ARRAY_SIZE(deviceExtensions));
    if (result != VK2C_OK) {
        return result;
    }

    result = vk2cCreateSwapchain(ctx, 600, 480);
    if (result != VK2C_OK)
    {
        return result;
    }

    result = vk2cCreateRenderpass(ctx);
    if (result != VK2C_OK)
    {
        return result;
    }

    result = vk2cCreateFramebuffer(ctx);
    if (result != VK2C_OK)
    {
        return result;
    }

    return VK2C_OK;
}

void vk2cDestroyContext(Vk2cContex_t* ctx)
{
    assert(ctx != NULL);

    vk2cDestroyFramebuffer(ctx);
    vk2cDestroyRenderpass(ctx);
    vk2cDestroySwapchain(ctx);
    vk2cDestroyDevice(ctx);
    vk2cDestroySurface(ctx);
#ifdef VK2C_DEBUG
    vk2cDestroyDebugMessenger(ctx);
#endif
    vk2cDestroyInstance(ctx);
    vk2cDestroyGLFWWindow(ctx);
}

VK2CResult_t vk2cCreateGLFWWindow(Vk2cContex_t* ctx, int width, int height, const char* title)
{
    if (glfwInit() == GLFW_FALSE)
    {
        vk2cLog(VK2C_LOG_ERROR, "vk2cCreateWindow() Error");
        return VK2C_ERROR_GLFW_FAIlED;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    ctx->window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (ctx->window == NULL) {
        vk2cLog(VK2C_LOG_ERROR, "glfwCreateWindow() Error");
        return VK2C_ERROR_GLFW_FAIlED;
    }

    return VK2C_OK;
}

void vk2cDestroyGLFWWindow(const Vk2cContex_t* ctx)
{
    assert(ctx != NULL);
    glfwDestroyWindow(ctx->window);
    glfwTerminate();
}

#ifdef VK2C_DEBUG
VK2CResult_t vk2cQueryInstanceLayer(const char** allowLayers, uint32_t numLayers)
{
    VkResult result;
    uint32_t layerCount;
    result = vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        vk2cLog(VK2C_LOG_ERROR, "vk2cQueryLayerProperties() Error. VkResult %d", result);
        return VK2C_ERROR_QUERY_INSTANCE_LAYER_PROPERTIES_FAILED;
    }

    VkLayerProperties layerProperties[layerCount];
    result = vkEnumerateInstanceLayerProperties(&layerCount, layerProperties);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        vk2cLog(VK2C_LOG_ERROR, "vk2cQueryLayerProperties() Error. VkResult %d", result);
        return VK2C_ERROR_QUERY_INSTANCE_LAYER_PROPERTIES_FAILED;
    }

    if (allowLayers == NULL && numLayers == 0)
    {
        vk2cLog(VK2C_LOG_VERBOSE, "InstanceLayerProperties");
        for (uint32_t i = 0; i < layerCount; i++)
        {
            vk2cLog(VK2C_LOG_VERBOSE, "\t layerName: %s", layerProperties[i].layerName);
            vk2cLog(VK2C_LOG_VERBOSE, "\t description: %s", layerProperties[i].description);
            vk2cLog(VK2C_LOG_VERBOSE, "\t implementationVersion: %d", layerProperties[i].implementationVersion);
            vk2cLog(VK2C_LOG_VERBOSE, "\t specVersion: %d", layerProperties[i].specVersion);
            vk2cLog(VK2C_LOG_VERBOSE, "");
        }
    }
    else
    {
        assert(allowLayers != NULL);
        assert(numLayers <= layerCount);
        for (uint32_t ali = 0; ali < numLayers; ali++)
        {
            bool isSuitable = false;
            for (uint32_t li = 0; li < layerCount; li++)
            {
                if (strcmp(allowLayers[ali], layerProperties[li].layerName) == 0) {
                    isSuitable = true;
                }
            }
            if (!isSuitable)
            {
                vk2cLog(VK2C_LOG_ERROR, "Not Found Suitable Layer");
                return VK2C_ERROR_SUITABLE_LAYER_NOT_FOUND;
            }
        }
    }

    return VK2C_OK;
}

VK2CResult_t vk2cCreateDebugMessenger(Vk2cContex_t* ctx, const VkDebugUtilsMessengerCreateInfoEXT* debugInfoCreateInfo)
{
    assert(ctx != NULL);
    assert(ctx->instance != NULL);

    VkResult result;
    const PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilMessengerEXT
        = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx->instance, "vkCreateDebugUtilsMessengerEXT");
    if (createDebugUtilMessengerEXT == NULL)
    {
        vk2cLog(VK2C_LOG_ERROR, "vkGetInstanceProcAddr Error");
        return VK2C_ERROR_GET_INSTANCE_PROC_ADDRESS;
    }
    result = createDebugUtilMessengerEXT(ctx->instance, debugInfoCreateInfo, NULL, &ctx->debugMessenger);
    if (result != VK_SUCCESS)
    {
        vk2cLog(VK2C_LOG_ERROR, "PFN_vkCreateDebugUtilsMessengerEXT Error. VkResult %d", result);
        return VK2C_ERROR_CREATE_DEBUG_MESSENGER_FAILED;
    }

    return VK2C_OK;
}

void vk2cDestroyDebugMessenger(Vk2cContex_t* ctx)
{
    if (ctx == NULL) return;
    if (ctx->instance == VK_NULL_HANDLE) return;
    if (ctx->debugMessenger == VK_NULL_HANDLE) return;

    const PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilMessengerEXT
        = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx->instance, "vkDestroyDebugUtilsMessengerEXT");
    if (vkDestroyDebugUtilMessengerEXT == NULL)
    {
        vk2cLog(VK2C_LOG_ERROR, "PFN_vkDestroyDebugUtilsMessengerEXT Error.");
        return;
    }

    vkDestroyDebugUtilMessengerEXT(ctx->instance, ctx->debugMessenger, NULL);
    ctx->debugMessenger = VK_NULL_HANDLE;
}

void vk2cMakeDebugMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT* debugCreateInfo)
{
    debugCreateInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo->messageSeverity
        = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo->messageType
        = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo->pfnUserCallback = debugCallback;
    debugCreateInfo->pUserData = NULL;
    debugCreateInfo->flags = 0;
    debugCreateInfo->pNext = NULL;
}

#endif

VkApplicationInfo vk2cMakeAppInfo()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_API_VERSION_1_4;
    appInfo.pApplicationName = VK2C_APP_NAME;
    appInfo.applicationVersion = VK2C_APP_VERSION;
    appInfo.pEngineName = VK2C_ENGINE_NAME;
    appInfo.engineVersion = VK2C_ENGINE_VERSION;

    return appInfo;
}

VK2CResult_t vk2cQueryInstanceExtension(const char** allowExtensions, uint32_t numExtensions)
{
    VkResult result;
    uint32_t extensionCount;
    result = vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        vk2cLog(VK2C_LOG_ERROR, "vk2cQueryExtensionInfo() Error. VkResult %d", result);
        return VK2C_ERROR_QUERY_INSTANCE_EXTENSION_PROPERTIES_FAILED;
    }

    VkExtensionProperties extensions[extensionCount];
    result = vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
        vk2cLog(VK2C_LOG_ERROR, "vk2cQueryExtensionInfo() Error. VkResult %d", result);
        return VK2C_ERROR_QUERY_INSTANCE_EXTENSION_PROPERTIES_FAILED;
    }

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    if (allowExtensions == NULL && numExtensions == 0)
    {
        vk2cLog(VK2C_LOG_VERBOSE, "InstanceExtensionProperties");
        for (uint32_t i = 0; i < extensionCount; i++)
        {
            vk2cLog(VK2C_LOG_VERBOSE, "\t Extension Name: %s", extensions[i].extensionName);
            vk2cLog(VK2C_LOG_VERBOSE, "\t Extension Spec Version %d", extensions[i].specVersion);
            vk2cLog(VK2C_LOG_VERBOSE, "");
        }
        vk2cLog(VK2C_LOG_VERBOSE, "glfwGetRequiredInstanceExtensions");
        for (uint32_t i = 0; i < glfwExtensionCount; i++)
        {
            vk2cLog(VK2C_LOG_VERBOSE, "\t Extension Name: %s", glfwExtensions[i]);
        }
    }
    else
    {
        assert(allowExtensions != NULL);
        assert(numExtensions <= extensionCount);

        for (uint32_t aei = 0; aei < numExtensions; aei++)
        {
            bool isSuitable = false;
            for (uint32_t ei = 0; ei < extensionCount; ei++)
            {
                if (strcmp(allowExtensions[aei], extensions[ei].extensionName) == 0) {
                    isSuitable = true;
                }
            }
            if (!isSuitable) {
                vk2cLog(VK2C_LOG_ERROR, "Not Found Suitable Extension");
                return VK2C_ERROR_SUITABLE_EXTENSION_NOT_FOUND;
            }
        }

        if (glfwExtensions) {
            for (uint32_t gei = 0; gei < glfwExtensionCount; gei++)
            {
                bool found = false;
                for (uint32_t aei = 0; aei < numExtensions; aei++)
                {
                    if (strcmp(glfwExtensions[gei], allowExtensions[aei]) == 0) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    vk2cLog(VK2C_LOG_ERROR, "Missing GLFW required extension: %s", glfwExtensions[gei]);
                    return VK2C_ERROR_SUITABLE_EXTENSION_NOT_FOUND;
                }
            }
        }
    }

    return VK2C_OK;
}

VK2CResult_t vk2cCreateInstance(Vk2cContex_t* ctx, const char** extensionNames, uint32_t numExtensions, const char** layerNames, uint32_t numLayers)
{
    assert(ctx != NULL);

    VkResult result;
    const VkApplicationInfo appInfo = vk2cMakeAppInfo();

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = numExtensions;
    instanceCreateInfo.ppEnabledExtensionNames = extensionNames;
#ifdef VK2C_DEBUG
    instanceCreateInfo.enabledLayerCount = numLayers;
    instanceCreateInfo.ppEnabledLayerNames = layerNames;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    vk2cMakeDebugMessengerCreateInfoEXT(&debugCreateInfo);
    instanceCreateInfo.pNext = &debugCreateInfo;
#else
    instanceCreateInfo.enabledLayerCount = 0;
    instanceCreateInfo.ppEnabledLayerNames = NULL;
#endif

#ifdef __APPLE__
    instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    result = vkCreateInstance(&instanceCreateInfo, NULL, &ctx->instance);
    if (result != VK_SUCCESS)
    {
        vk2cLog(VK2C_LOG_ERROR, "vkCreateInstance Error. VkResult %d", result);
        return VK2C_ERROR_CREATE_INSTANCE_FAILED;
    }

    return VK2C_OK;
}

void vk2cDestroyInstance(Vk2cContex_t* ctx)
{
    assert(ctx != NULL);
    assert(ctx->instance != VK_NULL_HANDLE);

    vkDestroyInstance(ctx->instance, NULL);
    ctx->instance = VK_NULL_HANDLE;
}

VK2CResult_t vk2cCreateSurface(Vk2cContex_t* ctx)
{
    assert(ctx != NULL);
    assert(ctx->window != NULL);
    assert(ctx->instance != VK_NULL_HANDLE);

    VkResult result = glfwCreateWindowSurface(ctx->instance, ctx->window, NULL, &ctx->surface);
    if (result != VK_SUCCESS)
    {
        vk2cLog(VK2C_LOG_ERROR, "glfwCreateWindowSurface Failed: VkResult %d", result);
        return VK2C_ERROR_CREATE_GLFW_SURFACE_FAILED;
    }

    return VK2C_OK;
}

void vk2cDestroySurface(Vk2cContex_t* ctx)
{
    assert(ctx != NULL);
    assert(ctx->instance != VK_NULL_HANDLE);
    assert(ctx->surface != VK_NULL_HANDLE);

    vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
    ctx->surface = VK_NULL_HANDLE;
}

VK2CResult_t vk2cPickPhysicalDevice(Vk2cContex_t* ctx)
{
    assert(ctx != NULL);
    assert(ctx->instance != VK_NULL_HANDLE);

    VkResult result;
    uint32_t deviceCount = 0;
    result = vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, NULL);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        vk2cLog(VK2C_LOG_ERROR, "vkEnumeratePhysicalDevices() Error. VkResult %d", result);
        return VK2C_ERROR_QUERY_PHYSICAL_DEVICES_FAILED;
    }
    VkPhysicalDevice physicalDevices[deviceCount];
    result = vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, physicalDevices);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        vk2cLog(VK2C_LOG_ERROR, "vkEnumeratePhysicalDevices() Error. VkResult %d", result);
        return VK2C_ERROR_QUERY_PHYSICAL_DEVICES_FAILED;
    }

#ifdef VK2C_DEBUG
    vk2cLog(VK2C_LOG_VERBOSE, "Found %d Physical Devices", deviceCount);
#endif
    for (uint32_t i = 0; i < deviceCount; i++)
    {
#ifdef VK2C_DEBUG
        vk2cLog(VK2C_LOG_VERBOSE, "\t Physical Device 0x%x", physicalDevices[i]);
#endif
        if (vk2cIsSuitablePhysicalDevice(physicalDevices[i]))
        {
            ctx->physicalDevice = physicalDevices[i];
            break;
        }
    }
    vk2cLog(VK2C_LOG_INFO, "Pick 0x%x Physical Devices", ctx->physicalDevice);

    return VK2C_OK;
}

bool vk2cIsSuitablePhysicalDevice(VkPhysicalDevice physicalDevice)
{
    return vk2cQueryQueueFamily(physicalDevice, VK_QUEUE_GRAPHICS_BIT) != UINT32_MAX;
}

uint32_t vk2cQueryQueueFamily(VkPhysicalDevice physicalDevice, VkQueueFlags flags)
{
    assert(physicalDevice != VK_NULL_HANDLE);

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties queueFamily[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamily);

    if (flags == 0)
    {
        vk2cLog(VK2C_LOG_VERBOSE, "PhysicalDeviceQueueFamilyProperties. queueFamilyCount: %d", queueFamilyCount);
        for (uint32_t i = 0; i < queueFamilyCount; i++)
        {
            vk2cLog(VK2C_LOG_VERBOSE, "\t queueCount: %d", queueFamily[i].queueCount);
            if (queueFamily[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                vk2cLog(VK2C_LOG_VERBOSE, "\t queueFlags: VK_QUEUE_GRAPHICS_BIT");
            }
            else if (queueFamily[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                vk2cLog(VK2C_LOG_VERBOSE, "\t queueFlags: VK_QUEUE_COMPUTE_BIT");
            }
            else if (queueFamily[i].queueFlags & VK_QUEUE_PROTECTED_BIT)
            {
                vk2cLog(VK2C_LOG_VERBOSE, "\t queueFlags: VK_QUEUE_PROTECTED_BIT");
            }
            else if (queueFamily[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                vk2cLog(VK2C_LOG_VERBOSE, "\t queueFlags: VK_QUEUE_TRANSFER_BIT");
            }
            else if (queueFamily[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
            {
                vk2cLog(VK2C_LOG_VERBOSE, "\t queueFlags: VK_QUEUE_SPARSE_BINDING_BIT");
            }
            else
            {
                vk2cLog(VK2C_LOG_VERBOSE, "\t queueFlags: INVALID. %d", queueFamily[i].queueFlags);
            }
            vk2cLog(VK2C_LOG_VERBOSE, "\t timestampValidBits: %u", queueFamily[i].timestampValidBits);
            vk2cLog(VK2C_LOG_VERBOSE
                , "\t minImageTransferGranularity: %u x %u x %u"
                , queueFamily[i].minImageTransferGranularity.width
                , queueFamily[i].minImageTransferGranularity.height
                , queueFamily[i].minImageTransferGranularity.depth);
            vk2cLog(VK2C_LOG_VERBOSE, "");
        }
        return UINT32_MAX;
    }

    uint32_t foundQueueIndex = -1;
    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if (queueFamily[i].queueCount == 0)
        {
            continue;
        }
        if ((queueFamily[i].queueFlags & flags) == flags)
        {
            foundQueueIndex = i;
            break;
        }
    }

    return foundQueueIndex;
}

uint32_t vk2cGetPresentFamilyIndex(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties props[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, props);

    uint32_t presentIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        if (presentSupport)
        {
            presentIndex = i;
            break;
        }
    }

    return presentIndex;
}

void vk2cMakeQueueFamily(VkDeviceQueueCreateInfo* queueCreateInfo, uint32_t findQueueFamilyIndex, const float* queuePriority, int flags)
{
    queueCreateInfo->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo->queueFamilyIndex = findQueueFamilyIndex;
    queueCreateInfo->queueCount = 1;
    queueCreateInfo->flags = flags;
    queueCreateInfo->pQueuePriorities = queuePriority;
    queueCreateInfo->pNext = NULL;
}

VK2CResult_t vk2cQueryDeviceExtension(VkPhysicalDevice physicalDevice)
{
    uint32_t deviceExtensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &deviceExtensionCount, NULL);
    VkExtensionProperties deviceExtensions[deviceExtensionCount];
    vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &deviceExtensionCount, deviceExtensions);

    vk2cLog(VK2C_LOG_VERBOSE, "DeviceExtensionProperties");
    for (uint32_t i = 0; i < deviceExtensionCount; i++)
    {
        vk2cLog(VK2C_LOG_VERBOSE, "\t extensionName: %s", deviceExtensions[i].extensionName);
        vk2cLog(VK2C_LOG_VERBOSE, "\t specVersion: %d", deviceExtensions[i].specVersion);
        vk2cLog(VK2C_LOG_VERBOSE, "");
    }

    return VK2C_OK;
}

VK2CResult_t vk2cCreateDevice(Vk2cContex_t* ctx, const char** extensionNames, uint32_t numExtensions)
{
    assert(ctx != NULL);
    assert(ctx->physicalDevice != VK_NULL_HANDLE);

    VkResult result;
    uint32_t findQueueFamilyIndex = vk2cQueryQueueFamily(ctx->physicalDevice, VK_QUEUE_GRAPHICS_BIT);
    if (findQueueFamilyIndex == UINT32_MAX)
    {
        vk2cLog(VK2C_LOG_ERROR, "Device does not support queue families");
        return VK2C_ERROR_DEVICE_QUEUE_NOT_FOUND;
    }
    ctx->queueFamilyIndex = findQueueFamilyIndex;

    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    vk2cMakeQueueFamily(&queueCreateInfo, findQueueFamilyIndex, &priority, 0);

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.ppEnabledExtensionNames = extensionNames;
    deviceCreateInfo.enabledExtensionCount = numExtensions;
    deviceCreateInfo.pNext = NULL;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

    result = vkCreateDevice(ctx->physicalDevice, &deviceCreateInfo, NULL, &ctx->device);
    if (result != VK_SUCCESS)
    {
        vk2cLog(VK2C_LOG_ERROR, "vkCreateDevice(): failed to create device. VkResult: %d", result);
        return VK2C_ERROR_CREATE_DEVICES_FAILED;
    }

    vkGetDeviceQueue(ctx->device, findQueueFamilyIndex, 0, &ctx->queue);
    if (ctx->queue == VK_NULL_HANDLE)
    {
        vk2cLog(VK2C_LOG_ERROR, "queueFamily: VK_NULL_HANDLE");
        return VK2C_ERROR_DEVICE_QUEUE_NOT_FOUND;
    }

    return VK2C_OK;
}

void vk2cDestroyDevice(Vk2cContex_t* ctx)
{
    assert(ctx != NULL);
    assert(ctx->device != VK_NULL_HANDLE);

    vkDeviceWaitIdle(ctx->device);
    vkDestroyDevice(ctx->device, NULL);
    ctx->device = VK_NULL_HANDLE;
}

VK2CResult_t vk2cCreateSwapchain(Vk2cContex_t* ctx, int windowWidth, int windowHeight)
{
    assert(ctx != NULL);
    assert(ctx->device != VK_NULL_HANDLE);

    VkResult result;
    VkSurfaceCapabilitiesKHR caps;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physicalDevice, ctx->surface, &caps);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        vk2cLog(VK2C_LOG_ERROR, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR(). Result %d", result);
        return VK2C_ERROR_QUERY_PHYSICAL_DEVICE_SURFACE_CAPABILITIES_FAILED;
    }

    uint32_t formatCount = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physicalDevice, ctx->surface, &formatCount, NULL);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        vk2cLog(VK2C_LOG_ERROR, "vkGetPhysicalDeviceSurfaceFormatsKHR(). Result %d", result);
        return VK2C_ERROR_QUERY_PHYSICAL_DEVICE_SURFACE_FORMAT_FAILED;
    }
    VkSurfaceFormatKHR surfaceFormats[formatCount];
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physicalDevice, ctx->surface, &formatCount, surfaceFormats);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        vk2cLog(VK2C_LOG_ERROR, "vkGetPhysicalDeviceSurfaceFormatsKHR(). Result %d", result);
        return VK2C_ERROR_QUERY_PHYSICAL_DEVICE_SURFACE_FORMAT_FAILED;
    }

    uint32_t presentModeCount = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physicalDevice, ctx->surface, &presentModeCount, NULL);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        vk2cLog(VK2C_LOG_ERROR, "vkGetPhysicalDeviceSurfacePresentModesKHR(). Result %d", result);
        return VK2C_ERROR_QUERY_PHYSICAL_DEVICE_SURFACE_PRESENT_MODE_FAILED;
    }
    VkPresentModeKHR presentModes[presentModeCount];
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physicalDevice, ctx->surface, &presentModeCount, presentModes);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        vk2cLog(VK2C_LOG_ERROR, "vkGetPhysicalDeviceSurfacePresentModesKHR(). Result %d", result);
        return VK2C_ERROR_QUERY_PHYSICAL_DEVICE_SURFACE_PRESENT_MODE_FAILED;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = ctx->surface;
    swapchainCreateInfo.pNext = NULL;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain= VK_NULL_HANDLE;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.preTransform = caps.currentTransform;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
        imageCount = caps.maxImageCount;
    }
    swapchainCreateInfo.minImageCount = imageCount;

    swapchainCreateInfo.presentMode = vk2cChoosePresentMode(presentModes, presentModeCount);
    swapchainCreateInfo.imageExtent = vk2cChooseSwapExtent(&caps, windowWidth, windowHeight);;

    const VkSurfaceFormatKHR surfaceFormatKHR = vk2cChooseSurfaceFormat(surfaceFormats, formatCount);
    swapchainCreateInfo.imageFormat = surfaceFormatKHR.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormatKHR.colorSpace;

    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.pQueueFamilyIndices = NULL;
    swapchainCreateInfo.queueFamilyIndexCount = 0;

    uint32_t presentFamily = vk2cGetPresentFamilyIndex(ctx->physicalDevice, ctx->surface);
    if (presentFamily == UINT32_MAX) {
        vk2cLog(VK2C_LOG_ERROR, "No present queue family found");
        return VK2C_ERROR_DEVICE_QUEUE_NOT_FOUND;
    }
    if (ctx->queueFamilyIndex != presentFamily) {
        uint32_t queueFamilyIndices[2] = { ctx->queueFamilyIndex, presentFamily };
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = NULL;
    }

    result = vkCreateSwapchainKHR(ctx->device, &swapchainCreateInfo, NULL, &ctx->swapchain);
    if (result != VK_SUCCESS)
    {
        vk2cLog(VK2C_LOG_VERBOSE, "vkCreateSwapchainKHR Error. Result %d", result);
        return VK2C_ERROR_CREATE_SWAPCHAIN_FAILED;
    }

    uint32_t swapchainImageCount = 0;
    result = vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &swapchainImageCount, NULL);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        vk2cLog(VK2C_LOG_ERROR, "vkGetSwapchainImagesKHR Error. Result %d", result);
        return VK2C_ERROR_QUERY_SWAPCHAIN_IMAGES_FAILED;
    }
    VkImage* images = malloc(sizeof(VkImage) * swapchainImageCount);
    if (images == NULL)
    {
        vk2cLog(VK2C_LOG_ERROR, "Error of Allocating %d Size", swapchainImageCount * sizeof(VkImage));
        free(images);
        return VK2C_ERROR_OOM;
    }
    result = vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &swapchainImageCount, images);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        vk2cLog(VK2C_LOG_ERROR, "vkGetSwapchainImagesKHR Error. Result %d", result);
        return VK2C_ERROR_QUERY_SWAPCHAIN_IMAGES_FAILED;
    }

    VkImageView* imageViews = malloc(sizeof(VkImageView) * swapchainImageCount);
    if (imageViews == NULL)
    {
        vk2cLog(VK2C_LOG_ERROR, "Error of Allocating %d Size", swapchainImageCount * sizeof(VkImageView));
        free(images);
        free(imageViews);
        return VK2C_ERROR_CREATE_SWAPCHAIN_IMAGES_FAILED;
    }

    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        VkImageViewCreateInfo imageView = {};
        imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageView.image = images[i];
        imageView.format = surfaceFormatKHR.format;
        imageView.pNext = NULL;
        imageView.components.r = VK_COMPONENT_SWIZZLE_R;
        imageView.components.g = VK_COMPONENT_SWIZZLE_G;
        imageView.components.b = VK_COMPONENT_SWIZZLE_B;
        imageView.components.a = VK_COMPONENT_SWIZZLE_A;
        imageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageView.subresourceRange.baseMipLevel = 0;
        imageView.subresourceRange.levelCount = 1;
        imageView.subresourceRange.baseArrayLayer = 0;
        imageView.subresourceRange.layerCount = 1;

        result = vkCreateImageView(ctx->device, &imageView, NULL, &imageViews[i]);
        if (result != VK_SUCCESS)
        {
            vk2cLog(VK2C_LOG_ERROR, "vkCreateImageView Error. Result %d", result);
            free(images);
            free(imageViews);
            return VK2C_ERROR_CREATE_SWAPCHAIN_IMAGES_FAILED;
        }
    }

    ctx->swapchainImages = images;
    ctx->swapchainImageCount = swapchainImageCount;
    ctx->swapchainImageViews = imageViews;
    ctx->swapchainImageViewCount = swapchainImageCount;
    ctx->swapchainExtent = caps.currentExtent;
    ctx->swapchainFormat = surfaceFormatKHR.format;

    return VK2C_OK;
}

void vk2cDestroySwapchain(Vk2cContex_t* ctx)
{
    assert(ctx != NULL);
    assert(ctx->swapchain != NULL);
    assert(ctx->swapchainImages != NULL && ctx->swapchainImageCount != UINT32_MAX);
    assert(ctx->swapchainImageViews != NULL && ctx->swapchainImageViewCount != UINT32_MAX);

    for (uint32_t i = 0; i < ctx->swapchainImageViewCount; i++)
    {
        vkDestroyImageView(ctx->device, ctx->swapchainImageViews[i], NULL);
    }
    free(ctx->swapchainImageViews);
    ctx->swapchainImageViewCount = UINT32_MAX;
    ctx->swapchainImageViews = NULL;

    vkDestroySwapchainKHR(ctx->device, ctx->swapchain, NULL);

    free(ctx->swapchainImages);
    ctx->swapchainImageCount = UINT32_MAX;
    ctx->swapchainImages = NULL;
}

VkSurfaceFormatKHR vk2cChooseSurfaceFormat(const VkSurfaceFormatKHR* availableFormats, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        if (availableFormats[i].format == VK_FORMAT_R8G8B8A8_SRGB
            && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormats[i];
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR vk2cChoosePresentMode(const VkPresentModeKHR* availableModes, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        if (availableModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
        if (availableModes[i] == VK_PRESENT_MODE_FIFO_KHR)
        {
            return VK_PRESENT_MODE_FIFO_KHR;
        }
    }
    return VK_PRESENT_MODE_IMMEDIATE_KHR;
}
VkExtent2D vk2cChooseSwapExtent(const VkSurfaceCapabilitiesKHR* caps, int windowWidth, int windowHeight)
{
    if (caps->currentExtent.width != UINT32_MAX)
    {
        return caps->currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = { .width = windowWidth, .height = windowHeight };
        actualExtent.width = VK2C_CLAMP(actualExtent.width, caps->minImageExtent.width, caps->maxImageExtent.width);
        actualExtent.height = VK2C_CLAMP(actualExtent.height, caps->minImageExtent.height, caps->maxImageExtent.height);
        return actualExtent;
    }
}

VK2CResult_t vk2cCreateRenderpass(Vk2cContex_t* ctx)
{
    assert(ctx != NULL);

    VkResult result;

    VkRenderPassCreateInfo renderpassCreateInfo = {};
    renderpassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderpassCreateInfo.flags = 0;

    VkAttachmentDescription attachDesc;
    attachDesc.format = ctx->swapchainFormat;
    attachDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    attachDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    renderpassCreateInfo.attachmentCount = 1;
    renderpassCreateInfo.pAttachments = &attachDesc;

    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderpassCreateInfo.dependencyCount = 1;
    renderpassCreateInfo.pDependencies = &subpassDependency;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc = {};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &colorAttachmentRef;

    renderpassCreateInfo.subpassCount = 1;
    renderpassCreateInfo.pSubpasses = &subpassDesc;

    result = vkCreateRenderPass(ctx->device, &renderpassCreateInfo, NULL, &ctx->renderPass);
    if (result != VK_SUCCESS)
    {
        vk2cLog(VK2C_LOG_ERROR, "vkCreateRenderPass() Error. Result %d", result);
        return VK2C_ERROR_CREATE_RENDERPASS_FAILED;
    }

    return VK2C_OK;
}

void vk2cDestroyRenderpass(Vk2cContex_t* ctx)
{
    assert(ctx != NULL);
    assert(ctx->renderPass != NULL);

    vkDestroyRenderPass(ctx->device, ctx->renderPass, NULL);
    ctx->renderPass = VK_NULL_HANDLE;
}

VK2CResult_t vk2cCreateFramebuffer(Vk2cContex_t* ctx)
{
    assert(ctx != NULL);
    assert(ctx->swapchainImageViews != NULL && ctx->swapchainImageViewCount != UINT32_MAX);
    assert(ctx->renderPass != NULL);

    VkResult result;
    VkFramebuffer* framebuffers = calloc(ctx->swapchainImageViewCount, sizeof(VkFramebuffer));
    if (framebuffers == NULL)
    {
        vk2cLog(VK2C_LOG_ERROR, "Error of Allocating. Size: %d", ctx->swapchainImageViewCount * sizeof(VkFramebuffer));
        return VK2C_ERROR_OOM;
    }

    for (uint32_t i = 0; i < ctx->swapchainImageViewCount; i++)
    {
        VkImageView attachments[] = { ctx->swapchainImageViews[i] };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = ctx->renderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = ctx->swapchainExtent.width;
        framebufferCreateInfo.height = ctx->swapchainExtent.height;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.layers = 1;

        result = vkCreateFramebuffer(ctx->device, &framebufferCreateInfo, NULL, &framebuffers[i]);
        if (result != VK_SUCCESS)
        {
            vk2cLog(VK2C_LOG_ERROR, "vkCreateFramebuffer() Error. Result %d", result);
            for (uint32_t j = 0; j < ctx->swapchainImageViewCount; j++)
            {
                free(framebuffers[i]);
            }
            free(framebuffers);
            return VK2C_ERROR_CREATE_FRAMEBUFFER_FAILED;
        }
    }
    ctx->framebuffers = framebuffers;
    ctx->framebufferCount = ctx->swapchainImageViewCount;

    return VK2C_OK;
}

void vk2cDestroyFramebuffer(Vk2cContex_t* ctx)
{
    assert(ctx != NULL);

    for (uint32_t i = 0; i < ctx->framebufferCount; i++)
    {
        vkDestroyFramebuffer(ctx->device, ctx->framebuffers[i], NULL);
    }
    free(ctx->framebuffers);
    ctx->framebuffers = NULL;
}

uint32_t* vk2cReadFileOrNull(const char* path, size_t* outSize)
{
    assert(outSize != NULL);

    FILE* file = fopen(path, "r");
    fseek(file, 0, SEEK_END);
    *outSize = ftell(file);
    rewind(file);
    uint32_t* buffer = calloc(*outSize, sizeof(uint32_t));
    if (buffer == NULL) {
        return NULL;
    }
    fread(buffer, *outSize, 1, file);
    fclose(file);
    return buffer;
}

VkShaderModule vk2cCreateShaderModuleOrNull(const Vk2cContex_t* ctx, const char* path)
{
    assert(ctx != NULL);
    assert(ctx->device != NULL);

    size_t codeSize = -1;
    const uint32_t* code = vk2cReadFileOrNull(path, &codeSize);
    if (code == NULL)
    {
        vk2cLog(VK2C_LOG_ERROR, "Error reading shader code file %s", path);
        return VK_NULL_HANDLE;
    }
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.codeSize = codeSize;
    createInfo.pCode = code;
    createInfo.flags = 0;

    VkShaderModule shaderModule = VK_NULL_HANDLE;

    const VkResult result = vkCreateShaderModule(ctx->device, &createInfo, NULL, &shaderModule);
    if (result != VK_SUCCESS)
    {
        vk2cLog(VK2C_LOG_ERROR, "Error creating shader module file %s. Result %d", path, result);
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

VK2CResult_t vk2cCreatePipeline(Vk2cContex_t* ctx)
{
    assert(ctx != NULL);
    assert(ctx->renderPass != NULL);

    VkShaderModule fragmentShader = vk2cCreateShaderModuleOrNull(ctx, "frag.spv");
    if (fragmentShader == NULL)
    {
        vk2cLog(VK2C_LOG_VERBOSE, "Error of creating shader module.");
        return VK2C_ERROR_SHADER_MODULE_NOT_FOUND;
    }

    VkShaderModule vertexShader = vk2cCreateShaderModuleOrNull(ctx, "vert.spv");
    if (vertexShader == NULL)
    {
        vk2cLog(VK2C_LOG_VERBOSE, "Error of creating shader module.");
        return VK2C_ERROR_SHADER_MODULE_NOT_FOUND;
    }

    VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {};
    fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    fragmentShaderStageCreateInfo.module = fragmentShader;
    fragmentShaderStageCreateInfo.pName = "main";
    fragmentShaderStageCreateInfo.flags = 0;
    fragmentShaderStageCreateInfo.pSpecializationInfo = NULL;

    VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {};
    vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    vertexShaderStageCreateInfo.module = fragmentShader;
    vertexShaderStageCreateInfo.pName = "main";
    vertexShaderStageCreateInfo.flags = 0;
    vertexShaderStageCreateInfo.pSpecializationInfo = NULL;

    const VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo[] = { fragmentShaderStageCreateInfo, vertexShaderStageCreateInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(uint32_t);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = NULL;

    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = NULL;
    vertexInputInfo.pNext = NULL;
    vertexInputInfo.flags = 0;

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.renderPass = ctx->renderPass;
    graphicsPipelineCreateInfo.flags = 0;
    graphicsPipelineCreateInfo.pNext = NULL;

    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.pStages = pipelineShaderStageCreateInfo;

    VkResult result;
    result = vkCreateGraphicsPipelines(ctx->device
        , VK_NULL_HANDLE
        , 0
        , &graphicsPipelineCreateInfo
        , NULL
        , &ctx->graphicsPipeline);
    if (result != VK_SUCCESS)
    {
        vk2cLog(VK2C_LOG_VERBOSE, "Error creating graphics pipeline. Result %d", result);
        return VK2C_ERROR_CREATE_GRAPHICS_PIPELINES_FAILED;
    }

    return VK2C_OK;
}

void vk2cDestroyPipeline(Vk2cContex_t* ctx)
{
    assert(ctx != NULL);
}


void vk2cPanic(const char* errmsg)
{
    fprintf(stderr, "%s\n", errmsg);
    exit(EXIT_FAILURE);
    assert(0);
}

void vk2cLog(Vk2cLogLevel logLevel, const char* msg, ...)
{
    va_list ap;
    va_start(ap, msg);

    va_list ap2;
    va_copy(ap2, ap);
    int needed = vsnprintf(NULL, 0, msg, ap2);
    va_end(ap2);

    if (needed < 0) {
        va_end(ap);
        return;
    }

    char* buf = malloc((size_t)needed + 1);
    if (!buf) {
        va_end(ap);
        return;
    }
    vsnprintf(buf, (size_t)needed + 1, msg, ap);
    va_end(ap);

    if (logLevel & VK2C_LOG_INFO)
    {
        fprintf(stdout, "[INFO] : %s\n", buf);
    }
    if (logLevel & VK2C_LOG_VERBOSE)
    {
        fprintf(stdout, "[VERBOSE]: %s\n", buf);
    }
    if (logLevel & VK2C_LOG_WARNING)
    {
        fprintf(stdout, "[WARNING] : %s\n", buf);
    }
    if (logLevel & VK2C_LOG_ERROR)
    {
        fprintf(stderr, "[ERROR] : %s\n", buf);
    }
    if (logLevel & VK2C_LOG_VALIDATION)
    {
        fprintf(stdout, "[VALIDATION]: %s\n", buf);
    }

    free(buf);
}