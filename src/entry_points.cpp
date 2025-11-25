#include "layer.h"
#include "dispatch_table.h"
#include <cstring>

VKAPI_ATTR VkResult VKAPI_CALL vkNegotiateLoaderLayerInterfaceVersion(
    VkNegotiateLayerInterface* pVersionStruct) {
    
    if (pVersionStruct->sType != LAYER_NEGOTIATE_INTERFACE_STRUCT) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    pVersionStruct->pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
    pVersionStruct->pfnGetDeviceProcAddr = vkGetDeviceProcAddr;
    pVersionStruct->pfnGetPhysicalDeviceProcAddr = nullptr;
    
    if (pVersionStruct->loaderLayerInterfaceVersion > CURRENT_LOADER_LAYER_INTERFACE_VERSION) {
        pVersionStruct->loaderLayerInterfaceVersion = CURRENT_LOADER_LAYER_INTERFACE_VERSION;
    }
    
    return VK_SUCCESS;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(
    VkInstance instance,
    const char* pName) {
    
    if (!pName) {
        return nullptr;
    }
    
    if (strcmp(pName, "vkGetInstanceProcAddr") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetInstanceProcAddr);
    }
    
    if (strcmp(pName, "vkCreateInstance") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(motionsafe_CreateInstance);
    }
    
    if (strcmp(pName, "vkDestroyInstance") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(motionsafe_DestroyInstance);
    }
    
    if (strcmp(pName, "vkCreateDevice") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(motionsafe_CreateDevice);
    }
    
    if (strcmp(pName, "vkNegotiateLoaderLayerInterfaceVersion") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(vkNegotiateLoaderLayerInterfaceVersion);
    }
    
    if (instance) {
        auto* dispatch = motionsafe::DispatchManager::GetInstance().GetInstanceDispatch(instance);
        if (dispatch && dispatch->GetInstanceProcAddr) {
            return dispatch->GetInstanceProcAddr(instance, pName);
        }
    }
    
    return nullptr;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(
    VkDevice device,
    const char* pName) {
    
    if (!pName) {
        return nullptr;
    }
    
    if (strcmp(pName, "vkGetDeviceProcAddr") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetDeviceProcAddr);
    }
    
    if (strcmp(pName, "vkDestroyDevice") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(motionsafe_DestroyDevice);
    }
    
    if (strcmp(pName, "vkQueuePresentKHR") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(motionsafe_QueuePresentKHR);
    }
    
    if (device) {
        auto* dispatch = motionsafe::DispatchManager::GetInstance().GetDeviceDispatch(device);
        if (dispatch && dispatch->GetDeviceProcAddr) {
            return dispatch->GetDeviceProcAddr(device, pName);
        }
    }
    
    return nullptr;
}
