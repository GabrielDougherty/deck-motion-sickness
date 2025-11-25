module;

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <mutex>
#include <cstring>

export module motionsafe.dispatch_table;

export namespace motionsafe {

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

module :private;

namespace motionsafe {

DispatchManager& DispatchManager::GetInstance() {
    static DispatchManager instance;
    return instance;
}

void DispatchManager::SetInstanceDispatch(VkInstance instance, const InstanceDispatchTable& table) {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    instance_dispatch_map_[instance] = table;
}

InstanceDispatchTable* DispatchManager::GetInstanceDispatch(VkInstance instance) {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    auto it = instance_dispatch_map_.find(instance);
    if (it != instance_dispatch_map_.end()) {
        return &it->second;
    }
    return nullptr;
}

void DispatchManager::RemoveInstanceDispatch(VkInstance instance) {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    instance_dispatch_map_.erase(instance);
}

void DispatchManager::SetDeviceDispatch(VkDevice device, const DeviceDispatchTable& table) {
    std::lock_guard<std::mutex> lock(device_mutex_);
    device_dispatch_map_[device] = table;
}

DeviceDispatchTable* DispatchManager::GetDeviceDispatch(VkDevice device) {
    std::lock_guard<std::mutex> lock(device_mutex_);
    auto it = device_dispatch_map_.find(device);
    if (it != device_dispatch_map_.end()) {
        return &it->second;
    }
    return nullptr;
}

void DispatchManager::RemoveDeviceDispatch(VkDevice device) {
    std::lock_guard<std::mutex> lock(device_mutex_);
    device_dispatch_map_.erase(device);
}

void DispatchManager::InitInstanceDispatchTable(VkInstance instance, PFN_vkGetInstanceProcAddr gpa) {
    InstanceDispatchTable table{};
    
    table.GetInstanceProcAddr = gpa;
    
    #define LOAD_INSTANCE_FUNC(name) \
        table.name = reinterpret_cast<PFN_vk##name>(gpa(instance, "vk" #name))
    
    LOAD_INSTANCE_FUNC(DestroyInstance);
    LOAD_INSTANCE_FUNC(EnumeratePhysicalDevices);
    LOAD_INSTANCE_FUNC(GetPhysicalDeviceProperties);
    LOAD_INSTANCE_FUNC(GetPhysicalDeviceFeatures);
    LOAD_INSTANCE_FUNC(GetPhysicalDeviceQueueFamilyProperties);
    LOAD_INSTANCE_FUNC(CreateDevice);
    LOAD_INSTANCE_FUNC(EnumerateDeviceExtensionProperties);
    
    #undef LOAD_INSTANCE_FUNC
    
    SetInstanceDispatch(instance, table);
}

void DispatchManager::InitDeviceDispatchTable(VkDevice device, PFN_vkGetDeviceProcAddr gpa) {
    DeviceDispatchTable table{};
    
    table.GetDeviceProcAddr = gpa;
    
    #define LOAD_DEVICE_FUNC(name) \
        table.name = reinterpret_cast<PFN_vk##name>(gpa(device, "vk" #name))
    
    LOAD_DEVICE_FUNC(DestroyDevice);
    LOAD_DEVICE_FUNC(GetDeviceQueue);
    LOAD_DEVICE_FUNC(QueueSubmit);
    LOAD_DEVICE_FUNC(QueuePresentKHR);
    LOAD_DEVICE_FUNC(CreateCommandPool);
    LOAD_DEVICE_FUNC(DestroyCommandPool);
    LOAD_DEVICE_FUNC(AllocateCommandBuffers);
    LOAD_DEVICE_FUNC(FreeCommandBuffers);
    LOAD_DEVICE_FUNC(BeginCommandBuffer);
    LOAD_DEVICE_FUNC(EndCommandBuffer);
    LOAD_DEVICE_FUNC(QueueWaitIdle);
    LOAD_DEVICE_FUNC(DeviceWaitIdle);
    
    #undef LOAD_DEVICE_FUNC
    
    SetDeviceDispatch(device, table);
}

} // namespace motionsafe
