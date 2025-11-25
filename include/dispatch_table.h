#pragma once

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <mutex>

namespace motionsafe {

struct InstanceDispatchTable {
    PFN_vkGetInstanceProcAddr GetInstanceProcAddr;
    PFN_vkDestroyInstance DestroyInstance;
    PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceProperties GetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceFeatures GetPhysicalDeviceFeatures;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties GetPhysicalDeviceQueueFamilyProperties;
    PFN_vkCreateDevice CreateDevice;
    PFN_vkEnumerateDeviceExtensionProperties EnumerateDeviceExtensionProperties;
};

struct DeviceDispatchTable {
    PFN_vkGetDeviceProcAddr GetDeviceProcAddr;
    PFN_vkDestroyDevice DestroyDevice;
    PFN_vkGetDeviceQueue GetDeviceQueue;
    PFN_vkQueueSubmit QueueSubmit;
    PFN_vkQueuePresentKHR QueuePresentKHR;
    PFN_vkCreateCommandPool CreateCommandPool;
    PFN_vkDestroyCommandPool DestroyCommandPool;
    PFN_vkAllocateCommandBuffers AllocateCommandBuffers;
    PFN_vkFreeCommandBuffers FreeCommandBuffers;
    PFN_vkBeginCommandBuffer BeginCommandBuffer;
    PFN_vkEndCommandBuffer EndCommandBuffer;
    PFN_vkQueueWaitIdle QueueWaitIdle;
    PFN_vkDeviceWaitIdle DeviceWaitIdle;
};

class DispatchManager {
public:
    static DispatchManager& GetInstance();

    void SetInstanceDispatch(VkInstance instance, const InstanceDispatchTable& table);
    InstanceDispatchTable* GetInstanceDispatch(VkInstance instance);
    void RemoveInstanceDispatch(VkInstance instance);

    void SetDeviceDispatch(VkDevice device, const DeviceDispatchTable& table);
    DeviceDispatchTable* GetDeviceDispatch(VkDevice device);
    void RemoveDeviceDispatch(VkDevice device);

    void InitInstanceDispatchTable(VkInstance instance, PFN_vkGetInstanceProcAddr gpa);
    void InitDeviceDispatchTable(VkDevice device, PFN_vkGetDeviceProcAddr gpa);

private:
    DispatchManager() = default;
    ~DispatchManager() = default;
    DispatchManager(const DispatchManager&) = delete;
    DispatchManager& operator=(const DispatchManager&) = delete;

    std::unordered_map<VkInstance, InstanceDispatchTable> instance_dispatch_map_;
    std::unordered_map<VkDevice, DeviceDispatchTable> device_dispatch_map_;
    std::mutex instance_mutex_;
    std::mutex device_mutex_;
};

} // namespace motionsafe
