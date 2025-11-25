#include "layer.h"
import motionsafe.layer;

// C wrapper functions - these are exported with C linkage for the Vulkan loader

VKAPI_ATTR VkResult VKAPI_CALL motionsafe_CreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance) {
    return motionsafe::CreateInstanceImpl(pCreateInfo, pAllocator, pInstance);
}

VKAPI_ATTR void VKAPI_CALL motionsafe_DestroyInstance(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator) {
    motionsafe::DestroyInstanceImpl(instance, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL motionsafe_CreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice) {
    return motionsafe::CreateDeviceImpl(physicalDevice, pCreateInfo, pAllocator, pDevice);
}

VKAPI_ATTR void VKAPI_CALL motionsafe_DestroyDevice(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator) {
    motionsafe::DestroyDeviceImpl(device, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL motionsafe_GetDeviceQueue(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue* pQueue) {
    motionsafe::GetDeviceQueueImpl(device, queueFamilyIndex, queueIndex, pQueue);
}

VKAPI_ATTR VkResult VKAPI_CALL motionsafe_QueuePresentKHR(
    VkQueue queue,
    const VkPresentInfoKHR* pPresentInfo) {
    return motionsafe::QueuePresentImpl(queue, pPresentInfo);
}
