#ifndef VK2C_NATIVE_H
#define VK2C_NATIVE_H

#include "vulkan/vulkan_core.h"
#ifdef __cplusplus
extern "C" {
#endif

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <glfw/glfw3.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "vk2c.h"

#ifdef NDEBUG
#undef VK2C_DEBUG
#else
#define VK2C_DEBUG
#endif

#define VK2C_APP_NAME ("vk2c_application")
#define VK2C_ENGINE_NAME ("vk2c_engine")
#define VK2C_APP_VERSION (VK_MAKE_VERSION(0, 0, 1))
#define VK2C_ENGINE_VERSION (VK_MAKE_VERSION(0, 0, 1))

#define VK2C_ARRAY_SIZE(X) (sizeof(X) / sizeof(*(X)))
#define VK2C_CLAMP(x, a, b) (x < a ? x : (x > b ? b: x))

typedef enum {
    VK2C_LOG_ERROR   = 1 << 0,
    VK2C_LOG_WARNING = 1 << 1,
    VK2C_LOG_INFO    = 1 << 2,
    VK2C_LOG_VERBOSE = 1 << 3,
    VK2C_LOG_VALIDATION = 1 << 4,
} Vk2cLogLevel;

struct Vk2cContex
{
    GLFWwindow* window;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;

    uint32_t queueFamilyIndex;
    VkQueue queue;

    VkSwapchainKHR swapchain;
    uint32_t swapchainImageCount;
    VkImage* swapchainImages;
    uint32_t swapchainImageViewCount;
    VkImageView* swapchainImageViews;
    VkExtent2D swapchainExtent;
    VkFormat swapchainFormat;

    VkRenderPass renderPass;

    VkFramebuffer* framebuffers;
    uint32_t framebufferCount;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
};

#ifdef VK2C_DEBUG
const static bool bEnableValidation = true;
#else
const static bool bEnableValidation = false;
#endif

void vk2cMakeContext(Vk2cContex_t* ctx);
VK2CResult_t vk2cCreateContext(Vk2cContex_t* ctx);
void vk2cDestroyContext(Vk2cContex_t* ctx);
#ifdef VK2C_DEBUG
/**
 * @param allowLayers list of allowing layer properties. if value is NULL, it queries all layer properties.
 * @param numLayers a number of list. if {allowLayers} is NULL, This value must define 0.
 */
VK2CResult_t vk2cQueryInstanceLayer(const char** allowLayers, uint32_t numLayers);

void vk2cMakeDebugMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT* debugCreateInfo);
VK2CResult_t vk2cCreateDebugMessenger(Vk2cContex_t* ctx, const VkDebugUtilsMessengerCreateInfoEXT* debugInfoCreateInfo);
void vk2cDestroyDebugMessenger(Vk2cContex_t* ctx);
#endif
VkApplicationInfo vk2cMakeAppInfo(void);

/**
 * @param allowExtensions list of allowing extension properties. if value is NULL, it queries all extension properties.
 * @param numExtensions a number of list. if {allowExtensions} is NULL, This value must define 0.
 */
VK2CResult_t vk2cQueryInstanceExtension(const char** allowExtensions, uint32_t numExtensions);

VK2CResult_t vk2cCreateGLFWWindow(Vk2cContex_t* ctx, int width, int height, const char* title);
void vk2cDestroyGLFWWindow(const Vk2cContex_t* ctx);

VK2CResult_t vk2cCreateInstance(Vk2cContex_t* ctx, const char** extensionNames, uint32_t numExtensions, const char** layerNames, uint32_t numLayers);
void vk2cDestroyInstance(Vk2cContex_t* ctx);

VK2CResult_t vk2cCreateSurface(Vk2cContex_t* ctx);
void vk2cDestroySurface(Vk2cContex_t* ctx);

VK2CResult_t vk2cPickPhysicalDevice(Vk2cContex_t* ctx);
bool vk2cIsSuitablePhysicalDevice(VkPhysicalDevice device);
/**
 * @param queueCreateInfo queueCreateInfo must not be VK_NULL_HANDLE.
 * @param flags if flags is 0, print all queue families.
 */
void vk2cMakeQueueFamily(VkDeviceQueueCreateInfo* queueCreateInfo, uint32_t findQueueFamilyIndex, const float* queuePriority, int flags);
uint32_t vk2cQueryQueueFamily(VkPhysicalDevice physicalDevice, VkQueueFlags flags);
VK2CResult_t vk2cQueryDeviceExtension(VkPhysicalDevice physicalDevice);
uint32_t vk2cGetPresentFamilyIndex(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);


VK2CResult_t vk2cCreateDevice(Vk2cContex_t* ctx, const char** extensionNames, uint32_t numExtensions);
void vk2cDestroyDevice(Vk2cContex_t* ctx);

VK2CResult_t vk2cCreateSwapchain(Vk2cContex_t* ctx, int windowWidth, int windowHeight);
void vk2cDestroySwapchain(Vk2cContex_t* ctx);
VkSurfaceFormatKHR vk2cChooseSurfaceFormat(const VkSurfaceFormatKHR* availableFormats, uint32_t count);
VkPresentModeKHR vk2cChoosePresentMode(const VkPresentModeKHR* availableModes, uint32_t count);
VkExtent2D vk2cChooseSwapExtent(const VkSurfaceCapabilitiesKHR* caps, int windowWidth, int windowHeight);

VK2CResult_t vk2cCreateRenderpass(Vk2cContex_t* ctx);
void vk2cDestroyRenderpass(Vk2cContex_t* ctx);

VK2CResult_t vk2cCreateFramebuffer(Vk2cContex_t* ctx);
void vk2cDestroyFramebuffer(Vk2cContex_t* ctx);

uint32_t* vk2cReadFileOrNull(const char* path, size_t* outSize);
VkShaderModule vk2cCreateShaderModuleOrNull(const Vk2cContex_t* ctx, const char* path);

VK2CResult_t vk2cCreatePipeline(Vk2cContex_t* ctx);
void vk2cDestroyPipeline(Vk2cContex_t* ctx);

void vk2cPanic(const char* errmsg);
void vk2cLog(Vk2cLogLevel logLevel, const char* msg, ...);

#ifdef __cplusplus
}
#endif

#endif
