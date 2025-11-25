module;

#include "layer.h"
#include <vulkan/vulkan.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <unordered_map>

export module motionsafe.layer;

import motionsafe.dispatch_table;
import motionsafe.overlay;

export namespace motionsafe {

// Layer implementation functions
VkResult CreateInstanceImpl(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance);

void DestroyInstanceImpl(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator);

VkResult CreateDeviceImpl(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice);

void DestroyDeviceImpl(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator);

void GetDeviceQueueImpl(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue* pQueue);

VkResult CreateSwapchainImpl(
    VkDevice device,
    const VkSwapchainCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSwapchainKHR* pSwapchain);

void DestroySwapchainImpl(
    VkDevice device,
    VkSwapchainKHR swapchain,
    const VkAllocationCallbacks* pAllocator);

VkResult GetSwapchainImagesImpl(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint32_t* pSwapchainImageCount,
    VkImage* pSwapchainImages);

VkResult QueuePresentImpl(
    VkQueue queue,
    const VkPresentInfoKHR* pPresentInfo);

} // namespace motionsafe

module :private;

namespace motionsafe {

// Track swapchain metadata for overlay registration
struct SwapchainInfo {
    uint32_t width;
    uint32_t height;
    VkFormat format;
    VkDevice device;
};

static std::unordered_map<VkSwapchainKHR, SwapchainInfo> g_swapchainInfo;

VkResult CreateInstanceImpl(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance) {
    
    std::cout << "[MotionSafe] vkCreateInstance called" << std::endl;
    
    VkLayerInstanceCreateInfo* layer_info = 
        const_cast<VkLayerInstanceCreateInfo*>(
            reinterpret_cast<const VkLayerInstanceCreateInfo*>(pCreateInfo->pNext));
    
    PFN_vkGetInstanceProcAddr next_gpa = nullptr;
    
    while (layer_info && 
           (layer_info->sType != VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO ||
            layer_info->function != VK_LAYER_LINK_INFO)) {
        layer_info = const_cast<VkLayerInstanceCreateInfo*>(
            reinterpret_cast<const VkLayerInstanceCreateInfo*>(layer_info->pNext));
    }
    
    if (layer_info) {
        next_gpa = layer_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
        
        PFN_vkCreateInstance create_func = 
            reinterpret_cast<PFN_vkCreateInstance>(
                next_gpa(VK_NULL_HANDLE, "vkCreateInstance"));
        
        layer_info->u.pLayerInfo = layer_info->u.pLayerInfo->pNext;
        
        VkResult result = create_func(pCreateInfo, pAllocator, pInstance);
        
        if (result == VK_SUCCESS) {
            DispatchManager::GetInstance().InitInstanceDispatchTable(*pInstance, next_gpa);
            std::cout << "[MotionSafe] Instance created successfully" << std::endl;
        }
        
        return result;
    }
    
    return VK_ERROR_INITIALIZATION_FAILED;
}

void DestroyInstanceImpl(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator) {
    
    std::cout << "[MotionSafe] vkDestroyInstance called" << std::endl;
    
    auto* dispatch = DispatchManager::GetInstance().GetInstanceDispatch(instance);
    if (dispatch && dispatch->DestroyInstance) {
        dispatch->DestroyInstance(instance, pAllocator);
    }
    
    DispatchManager::GetInstance().RemoveInstanceDispatch(instance);
}

VkResult CreateDeviceImpl(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice) {
    
    std::cout << "[MotionSafe] vkCreateDevice called" << std::endl;
    
    VkLayerDeviceCreateInfo* layer_info = 
        const_cast<VkLayerDeviceCreateInfo*>(
            reinterpret_cast<const VkLayerDeviceCreateInfo*>(pCreateInfo->pNext));
    
    PFN_vkGetInstanceProcAddr next_gipa = nullptr;
    PFN_vkGetDeviceProcAddr next_gdpa = nullptr;
    
    while (layer_info && 
           (layer_info->sType != VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO ||
            layer_info->function != VK_LAYER_LINK_INFO)) {
        layer_info = const_cast<VkLayerDeviceCreateInfo*>(
            reinterpret_cast<const VkLayerDeviceCreateInfo*>(layer_info->pNext));
    }
    
    if (layer_info) {
        next_gipa = layer_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
        next_gdpa = layer_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
        
        PFN_vkCreateDevice create_func = 
            reinterpret_cast<PFN_vkCreateDevice>(
                next_gipa(VK_NULL_HANDLE, "vkCreateDevice"));
        
        layer_info->u.pLayerInfo = layer_info->u.pLayerInfo->pNext;
        
        VkResult result = create_func(physicalDevice, pCreateInfo, pAllocator, pDevice);
        
        if (result == VK_SUCCESS) {
            DispatchManager::GetInstance().InitDeviceDispatchTable(*pDevice, next_gdpa);
            std::cout << "[MotionSafe] Device created successfully" << std::endl;
        }
        
        return result;
    }
    
    return VK_ERROR_INITIALIZATION_FAILED;
}

void DestroyDeviceImpl(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator) {
    
    std::cout << "[MotionSafe] vkDestroyDevice called" << std::endl;
    
    auto* dispatch = DispatchManager::GetInstance().GetDeviceDispatch(device);
    if (dispatch && dispatch->DestroyDevice) {
        dispatch->DestroyDevice(device, pAllocator);
    }
    
    DispatchManager::GetInstance().RemoveDeviceDispatch(device);
}

void GetDeviceQueueImpl(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue* pQueue) {
    
    auto* dispatch = DispatchManager::GetInstance().GetDeviceDispatch(device);
    if (dispatch && dispatch->GetDeviceQueue) {
        dispatch->GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
        
        // Track which device this queue belongs to
        if (pQueue && *pQueue) {
            DispatchManager::GetInstance().SetQueueDevice(*pQueue, device);
        }
    }
}

VkResult CreateSwapchainImpl(
    VkDevice device,
    const VkSwapchainCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSwapchainKHR* pSwapchain) {
    
    auto* dispatch = DispatchManager::GetInstance().GetDeviceDispatch(device);
    if (!dispatch || !dispatch->CreateSwapchainKHR) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    VkResult result = dispatch->CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
    
    if (result == VK_SUCCESS) {
        std::cout << "[MotionSafe] Swapchain created: " << *pSwapchain 
                  << " (" << pCreateInfo->imageExtent.width << "x" << pCreateInfo->imageExtent.height << ")" 
                  << std::endl;
        
        // Store swapchain metadata for later registration
        g_swapchainInfo[*pSwapchain] = SwapchainInfo{
            pCreateInfo->imageExtent.width,
            pCreateInfo->imageExtent.height,
            pCreateInfo->imageFormat,
            device
        };
    }
    
    return result;
}

void DestroySwapchainImpl(
    VkDevice device,
    VkSwapchainKHR swapchain,
    const VkAllocationCallbacks* pAllocator) {
    
    std::cout << "[MotionSafe] Swapchain destroyed: " << swapchain << std::endl;
    
    // Unregister from overlay system
    overlay::UnregisterSwapchain(swapchain);
    
    // Clean up metadata
    g_swapchainInfo.erase(swapchain);
    
    auto* dispatch = DispatchManager::GetInstance().GetDeviceDispatch(device);
    if (dispatch && dispatch->DestroySwapchainKHR) {
        dispatch->DestroySwapchainKHR(device, swapchain, pAllocator);
    }
}

VkResult GetSwapchainImagesImpl(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint32_t* pSwapchainImageCount,
    VkImage* pSwapchainImages) {
    
    auto* dispatch = DispatchManager::GetInstance().GetDeviceDispatch(device);
    if (!dispatch || !dispatch->GetSwapchainImagesKHR) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    VkResult result = dispatch->GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
    
    if (result == VK_SUCCESS && pSwapchainImages) {
        std::cout << "[MotionSafe] Got " << *pSwapchainImageCount << " swapchain images" << std::endl;
        
        // Register swapchain with overlay system using stored metadata
        auto it = g_swapchainInfo.find(swapchain);
        if (it != g_swapchainInfo.end()) {
            std::vector<VkImage> images(pSwapchainImages, pSwapchainImages + *pSwapchainImageCount);
            overlay::RegisterSwapchain(
                device, swapchain, images,
                it->second.width, it->second.height, it->second.format
            );
        }
    }
    
    return result;
}

VkResult QueuePresentImpl(
    VkQueue queue,
    const VkPresentInfoKHR* pPresentInfo) {
    
    std::cout << "[MotionSafe] vkQueuePresentKHR called" << std::endl;
    
    // Get the device that owns this queue
    VkDevice device = DispatchManager::GetInstance().GetQueueDevice(queue);
    if (device == VK_NULL_HANDLE) {
        std::cerr << "[MotionSafe] Error: Queue not found in device map!" << std::endl;
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    // Render overlay on each swapchain being presented
    for (uint32_t i = 0; i < pPresentInfo->swapchainCount; ++i) {
        VkSwapchainKHR swapchain = pPresentInfo->pSwapchains[i];
        uint32_t imageIndex = pPresentInfo->pImageIndices[i];
        
        // Call overlay rendering
        overlay::RenderOverlay(swapchain, imageIndex);
    }
    
    auto* dispatch = DispatchManager::GetInstance().GetDeviceDispatch(device);
    if (dispatch && dispatch->QueuePresentKHR) {
        return dispatch->QueuePresentKHR(queue, pPresentInfo);
    }
    
    return VK_ERROR_INITIALIZATION_FAILED;
}

} // namespace motionsafe
