#include "dispatch_table.h"
#include <cstring>

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
