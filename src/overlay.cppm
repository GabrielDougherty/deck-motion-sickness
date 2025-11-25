module;

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <unordered_map>

export module motionsafe.overlay;

export namespace motionsafe {
namespace overlay {

// Forward declarations
void Initialize();
void Shutdown();

void RegisterSwapchain(VkDevice device, VkSwapchainKHR swapchain, 
                      const std::vector<VkImage>& images,
                      uint32_t width, uint32_t height, VkFormat format);

void UnregisterSwapchain(VkSwapchainKHR swapchain);

void RenderOverlay(VkSwapchainKHR swapchain, uint32_t imageIndex);

} // namespace overlay
} // namespace motionsafe

module :private;

namespace motionsafe {
namespace overlay {

// Overlay state per swapchain
struct SwapchainOverlay {
    VkDevice device = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;
    
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    
    uint32_t width = 0;
    uint32_t height = 0;
    VkFormat format = VK_FORMAT_UNDEFINED;
};

// Global overlay state
static std::unordered_map<VkSwapchainKHR, SwapchainOverlay> g_swapchainOverlays;

void Initialize() {
    std::cout << "[MotionSafe] Overlay system initialized" << std::endl;
}

void Shutdown() {
    std::cout << "[MotionSafe] Overlay system shutdown" << std::endl;
    // TODO: Clean up all overlay resources
    g_swapchainOverlays.clear();
}

void RegisterSwapchain(VkDevice device, VkSwapchainKHR swapchain, 
                      const std::vector<VkImage>& images,
                      uint32_t width, uint32_t height, VkFormat format) {
    SwapchainOverlay overlay{};
    overlay.device = device;
    overlay.swapchain = swapchain;
    overlay.images = images;
    overlay.width = width;
    overlay.height = height;
    overlay.format = format;
    
    g_swapchainOverlays[swapchain] = overlay;
    
    std::cout << "[MotionSafe] Registered swapchain: " << swapchain 
              << " (" << width << "x" << height << ", " << images.size() << " images)" << std::endl;
}

void UnregisterSwapchain(VkSwapchainKHR swapchain) {
    auto it = g_swapchainOverlays.find(swapchain);
    if (it != g_swapchainOverlays.end()) {
        // TODO: Clean up Vulkan resources for this swapchain
        g_swapchainOverlays.erase(it);
        std::cout << "[MotionSafe] Unregistered swapchain: " << swapchain << std::endl;
    }
}

void RenderOverlay(VkSwapchainKHR swapchain, uint32_t imageIndex) {
    auto it = g_swapchainOverlays.find(swapchain);
    if (it == g_swapchainOverlays.end()) {
        return;  // Swapchain not registered yet
    }
    
    // TODO: Render the actual overlay
    // For now, just log that we would render
    std::cout << "[MotionSafe] Would render overlay on image " << imageIndex << std::endl;
}

} // namespace overlay
} // namespace motionsafe
