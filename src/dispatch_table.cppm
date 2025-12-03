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
    
    // Swapchain functions
    PFN_vkCreateSwapchainKHR CreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR DestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR GetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR AcquireNextImageKHR;
    
    // Command buffer functions
    PFN_vkCreateCommandPool CreateCommandPool;
    PFN_vkDestroyCommandPool DestroyCommandPool;
    PFN_vkAllocateCommandBuffers AllocateCommandBuffers;
    PFN_vkFreeCommandBuffers FreeCommandBuffers;
    PFN_vkBeginCommandBuffer BeginCommandBuffer;
    PFN_vkEndCommandBuffer EndCommandBuffer;
    
    // Synchronization
    PFN_vkQueueWaitIdle QueueWaitIdle;
    PFN_vkDeviceWaitIdle DeviceWaitIdle;
    PFN_vkCreateFence CreateFence;
    PFN_vkDestroyFence DestroyFence;
    PFN_vkWaitForFences WaitForFences;
    PFN_vkResetFences ResetFences;
    
    // Pipeline and rendering (we'll need these for overlay)
    PFN_vkCreateRenderPass CreateRenderPass;
    PFN_vkDestroyRenderPass DestroyRenderPass;
    PFN_vkCreateFramebuffer CreateFramebuffer;
    PFN_vkDestroyFramebuffer DestroyFramebuffer;
    PFN_vkCreateImageView CreateImageView;
    PFN_vkDestroyImageView DestroyImageView;
    PFN_vkCreateShaderModule CreateShaderModule;
    PFN_vkDestroyShaderModule DestroyShaderModule;
    PFN_vkCreateGraphicsPipelines CreateGraphicsPipelines;
    PFN_vkDestroyPipeline DestroyPipeline;
    PFN_vkCreatePipelineLayout CreatePipelineLayout;
    PFN_vkDestroyPipelineLayout DestroyPipelineLayout;
    
    // Command buffer recording
    PFN_vkCmdBeginRenderPass CmdBeginRenderPass;
    PFN_vkCmdEndRenderPass CmdEndRenderPass;
    PFN_vkCmdBindPipeline CmdBindPipeline;
    PFN_vkCmdDraw CmdDraw;
    PFN_vkCmdPipelineBarrier CmdPipelineBarrier;
    PFN_vkCmdPushConstants CmdPushConstants;
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

    // Queue to device mapping
    void SetQueueDevice(VkQueue queue, VkDevice device);
    VkDevice GetQueueDevice(VkQueue queue);
    void RemoveQueue(VkQueue queue);

private:
    DispatchManager() = default;
    ~DispatchManager() = default;
    DispatchManager(const DispatchManager&) = delete;
    DispatchManager& operator=(const DispatchManager&) = delete;

    std::unordered_map<VkInstance, InstanceDispatchTable> instance_dispatch_map_;
    std::unordered_map<VkDevice, DeviceDispatchTable> device_dispatch_map_;
    std::unordered_map<VkQueue, VkDevice> queue_to_device_map_;
    std::mutex instance_mutex_;
    std::mutex device_mutex_;
    std::mutex queue_mutex_;
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
    
    // Swapchain
    LOAD_DEVICE_FUNC(CreateSwapchainKHR);
    LOAD_DEVICE_FUNC(DestroySwapchainKHR);
    LOAD_DEVICE_FUNC(GetSwapchainImagesKHR);
    LOAD_DEVICE_FUNC(AcquireNextImageKHR);
    
    // Command buffers
    LOAD_DEVICE_FUNC(CreateCommandPool);
    LOAD_DEVICE_FUNC(DestroyCommandPool);
    LOAD_DEVICE_FUNC(AllocateCommandBuffers);
    LOAD_DEVICE_FUNC(FreeCommandBuffers);
    LOAD_DEVICE_FUNC(BeginCommandBuffer);
    LOAD_DEVICE_FUNC(EndCommandBuffer);
    
    // Synchronization
    LOAD_DEVICE_FUNC(QueueWaitIdle);
    LOAD_DEVICE_FUNC(DeviceWaitIdle);
    LOAD_DEVICE_FUNC(CreateFence);
    LOAD_DEVICE_FUNC(DestroyFence);
    LOAD_DEVICE_FUNC(WaitForFences);
    LOAD_DEVICE_FUNC(ResetFences);
    
    // Pipeline and rendering
    LOAD_DEVICE_FUNC(CreateRenderPass);
    LOAD_DEVICE_FUNC(DestroyRenderPass);
    LOAD_DEVICE_FUNC(CreateFramebuffer);
    LOAD_DEVICE_FUNC(DestroyFramebuffer);
    LOAD_DEVICE_FUNC(CreateImageView);
    LOAD_DEVICE_FUNC(DestroyImageView);
    LOAD_DEVICE_FUNC(CreateShaderModule);
    LOAD_DEVICE_FUNC(DestroyShaderModule);
    LOAD_DEVICE_FUNC(CreateGraphicsPipelines);
    LOAD_DEVICE_FUNC(DestroyPipeline);
    LOAD_DEVICE_FUNC(CreatePipelineLayout);
    LOAD_DEVICE_FUNC(DestroyPipelineLayout);
    
    // Command buffer recording
    LOAD_DEVICE_FUNC(CmdBeginRenderPass);
    LOAD_DEVICE_FUNC(CmdEndRenderPass);
    LOAD_DEVICE_FUNC(CmdBindPipeline);
    LOAD_DEVICE_FUNC(CmdDraw);
    LOAD_DEVICE_FUNC(CmdPipelineBarrier);
    LOAD_DEVICE_FUNC(CmdPushConstants);
    
    #undef LOAD_DEVICE_FUNC
    
    SetDeviceDispatch(device, table);
}

void DispatchManager::SetQueueDevice(VkQueue queue, VkDevice device) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    queue_to_device_map_[queue] = device;
}

VkDevice DispatchManager::GetQueueDevice(VkQueue queue) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    auto it = queue_to_device_map_.find(queue);
    if (it != queue_to_device_map_.end()) {
        return it->second;
    }
    return VK_NULL_HANDLE;
}

void DispatchManager::RemoveQueue(VkQueue queue) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    queue_to_device_map_.erase(queue);
}

} // namespace motionsafe
