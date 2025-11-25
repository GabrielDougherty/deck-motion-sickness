#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && __GNUC__ >= 4
    #define VK_LAYER_EXPORT __attribute__((visibility("default")))
#elif defined(_WIN32)
    #define VK_LAYER_EXPORT __declspec(dllexport)
#else
    #define VK_LAYER_EXPORT
#endif

// Layer entry points
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL motionsafe_CreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance);

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL motionsafe_DestroyInstance(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator);

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL motionsafe_CreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice);

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL motionsafe_DestroyDevice(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator);

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL motionsafe_GetDeviceQueue(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue* pQueue);

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL motionsafe_QueuePresentKHR(
    VkQueue queue,
    const VkPresentInfoKHR* pPresentInfo);

// Standard layer interface functions
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkNegotiateLoaderLayerInterfaceVersion(
    VkNegotiateLayerInterface* pVersionStruct);

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(
    VkInstance instance,
    const char* pName);

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(
    VkDevice device,
    const char* pName);

#ifdef __cplusplus
}
#endif
