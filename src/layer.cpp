#include "layer.h"
import motionsafe.dispatch_table;
#include <iostream>
#include <cstring>

using namespace motionsafe;

VKAPI_ATTR VkResult VKAPI_CALL motionsafe_CreateInstance(
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

VKAPI_ATTR void VKAPI_CALL motionsafe_DestroyInstance(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator) {
    
    std::cout << "[MotionSafe] vkDestroyInstance called" << std::endl;
    
    auto* dispatch = DispatchManager::GetInstance().GetInstanceDispatch(instance);
    if (dispatch && dispatch->DestroyInstance) {
        dispatch->DestroyInstance(instance, pAllocator);
    }
    
    DispatchManager::GetInstance().RemoveInstanceDispatch(instance);
}

VKAPI_ATTR VkResult VKAPI_CALL motionsafe_CreateDevice(
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

VKAPI_ATTR void VKAPI_CALL motionsafe_DestroyDevice(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator) {
    
    std::cout << "[MotionSafe] vkDestroyDevice called" << std::endl;
    
    auto* dispatch = DispatchManager::GetInstance().GetDeviceDispatch(device);
    if (dispatch && dispatch->DestroyDevice) {
        dispatch->DestroyDevice(device, pAllocator);
    }
    
    DispatchManager::GetInstance().RemoveDeviceDispatch(device);
}

VKAPI_ATTR VkResult VKAPI_CALL motionsafe_QueuePresentKHR(
    VkQueue queue,
    const VkPresentInfoKHR* pPresentInfo) {
    
    std::cout << "[MotionSafe] vkQueuePresentKHR called" << std::endl;
    
    VkDevice device = *reinterpret_cast<VkDevice*>(queue);
    
    auto* dispatch = DispatchManager::GetInstance().GetDeviceDispatch(device);
    if (dispatch && dispatch->QueuePresentKHR) {
        return dispatch->QueuePresentKHR(queue, pPresentInfo);
    }
    
    return VK_ERROR_INITIALIZATION_FAILED;
}
