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

/**
 * @brief Layer implementation of vkCreateInstance
 * 
 * Intercepts instance creation to initialize the layer's dispatch table.
 * Extracts the next layer's function pointers from the pNext chain, calls
 * the next vkCreateInstance, and sets up instance-level dispatch.
 * 
 * @param pCreateInfo Pointer to VkInstanceCreateInfo structure
 * @param pAllocator Pointer to allocation callbacks (optional)
 * @param pInstance Pointer to store the created instance handle
 * @return VK_SUCCESS on success, or appropriate error code
 */
VkResult CreateInstanceImpl(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance);

/**
 * @brief Layer implementation of vkDestroyInstance
 * 
 * Intercepts instance destruction to clean up the layer's dispatch table.
 * Calls the next layer's vkDestroyInstance and removes the instance from
 * the dispatch manager.
 * 
 * @param instance The instance to destroy
 * @param pAllocator Pointer to allocation callbacks (optional)
 */
void DestroyInstanceImpl(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator);

/**
 * @brief Layer implementation of vkCreateDevice
 * 
 * Intercepts device creation to initialize the device-level dispatch table.
 * Extracts the next layer's function pointers, calls the next vkCreateDevice,
 * and initializes device dispatch with all intercepted device functions.
 * 
 * @param physicalDevice Physical device to create logical device from
 * @param pCreateInfo Pointer to VkDeviceCreateInfo structure
 * @param pAllocator Pointer to allocation callbacks (optional)
 * @param pDevice Pointer to store the created device handle
 * @return VK_SUCCESS on success, or appropriate error code
 */
VkResult CreateDeviceImpl(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice);

/**
 * @brief Layer implementation of vkDestroyDevice
 * 
 * Intercepts device destruction to clean up device dispatch table.
 * Calls the next layer's vkDestroyDevice and removes the device from
 * the dispatch manager.
 * 
 * @param device The device to destroy
 * @param pAllocator Pointer to allocation callbacks (optional)
 */
void DestroyDeviceImpl(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator);

/**
 * @brief Layer implementation of vkGetDeviceQueue
 * 
 * Intercepts queue retrieval to track queue-to-device mappings.
 * This mapping is essential for later determining which device owns
 * a queue during presentation.
 * 
 * @param device The device that owns the queue
 * @param queueFamilyIndex Index of the queue family
 * @param queueIndex Index within the queue family
 * @param pQueue Pointer to store the retrieved queue handle
 */
void GetDeviceQueueImpl(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue* pQueue);

/**
 * @brief Layer implementation of vkCreateSwapchainKHR
 * 
 * Intercepts swapchain creation to store metadata for overlay registration.
 * Records swapchain dimensions, format, and owning device for later use
 * when registering with the overlay system.
 * 
 * @param device The device creating the swapchain
 * @param pCreateInfo Pointer to VkSwapchainCreateInfoKHR structure
 * @param pAllocator Pointer to allocation callbacks (optional)
 * @param pSwapchain Pointer to store the created swapchain handle
 * @return VK_SUCCESS on success, or appropriate error code
 */
VkResult CreateSwapchainImpl(
    VkDevice device,
    const VkSwapchainCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSwapchainKHR* pSwapchain);

/**
 * @brief Layer implementation of vkDestroySwapchainKHR
 * 
 * Intercepts swapchain destruction to clean up overlay resources.
 * Unregisters the swapchain from the overlay system, which destroys
 * all associated rendering resources, then calls the next layer's destroy.
 * 
 * @param device The device that owns the swapchain
 * @param swapchain The swapchain to destroy
 * @param pAllocator Pointer to allocation callbacks (optional)
 */
void DestroySwapchainImpl(
    VkDevice device,
    VkSwapchainKHR swapchain,
    const VkAllocationCallbacks* pAllocator);

/**
 * @brief Layer implementation of vkGetSwapchainImagesKHR
 * 
 * Intercepts swapchain image retrieval to register with overlay system.
 * On the second call (when pSwapchainImages != nullptr), registers the
 * swapchain and its images with the overlay system for rendering.
 * 
 * @param device The device that owns the swapchain
 * @param swapchain The swapchain to query images from
 * @param pSwapchainImageCount Pointer to image count (in/out)
 * @param pSwapchainImages Pointer to array of image handles (optional)
 * @return VK_SUCCESS on success, or appropriate error code
 */
VkResult GetSwapchainImagesImpl(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint32_t* pSwapchainImageCount,
    VkImage* pSwapchainImages);

/**
 * @brief Layer implementation of vkQueuePresentKHR
 * 
 * Intercepts frame presentation to render the motion-sickness overlay.
 * For each swapchain being presented, renders the overlay on the specified
 * image before calling the next layer's present function.
 * 
 * @param queue The queue to present on
 * @param pPresentInfo Pointer to VkPresentInfoKHR structure
 * @return Result from the next layer's vkQueuePresentKHR
 */
VkResult QueuePresentImpl(
    VkQueue queue,
    const VkPresentInfoKHR* pPresentInfo);

} // namespace motionsafe

module :private;

namespace motionsafe {

/**
 * @brief Metadata about a swapchain for delayed overlay registration
 * 
 * Stores information captured during vkCreateSwapchainKHR that will be
 * needed later when registering with the overlay system during
 * vkGetSwapchainImagesKHR.
 */
struct SwapchainInfo {
    uint32_t width;    ///< Swapchain width in pixels
    uint32_t height;   ///< Swapchain height in pixels
    VkFormat format;   ///< Image format
    VkDevice device;   ///< Owning device
};

/// Global map of swapchain metadata, indexed by swapchain handle
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
        overlay::RenderOverlay(queue, swapchain, imageIndex);
    }
    
    auto* dispatch = DispatchManager::GetInstance().GetDeviceDispatch(device);
    if (dispatch && dispatch->QueuePresentKHR) {
        return dispatch->QueuePresentKHR(queue, pPresentInfo);
    }
    
    return VK_ERROR_INITIALIZATION_FAILED;
}

} // namespace motionsafe
