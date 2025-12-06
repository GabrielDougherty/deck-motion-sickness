module;

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <array>
#include <chrono>
#include <cmath>

export module motionsafe.overlay;

import motionsafe.dispatch_table;
import motionsafe.motion_sensor;

export namespace motionsafe {
namespace overlay {

// Forward declarations

/**
 * @brief Initialize the overlay system
 * 
 * Sets up global state for the overlay rendering system. Should be called
 * once during layer initialization.
 */
void Initialize();

/**
 * @brief Shutdown the overlay system
 * 
 * Cleans up all overlay resources and global state. Should be called
 * during layer cleanup.
 */
void Shutdown();

/**
 * @brief Register a swapchain with the overlay system
 * 
 * Registers a swapchain and its images for overlay rendering. This must be
 * called after swapchain creation and image retrieval.
 * 
 * @param device The Vulkan device that owns the swapchain
 * @param swapchain The swapchain handle to register
 * @param images Vector of swapchain images to render overlays on
 * @param width Width of the swapchain images in pixels
 * @param height Height of the swapchain images in pixels
 * @param format Vulkan format of the swapchain images
 */
void RegisterSwapchain(VkDevice device, VkSwapchainKHR swapchain, 
                      const std::vector<VkImage>& images,
                      uint32_t width, uint32_t height, VkFormat format);

/**
 * @brief Unregister a swapchain from the overlay system
 * 
 * Removes a swapchain from the overlay system and cleans up all associated
 * rendering resources (pipelines, command buffers, framebuffers, etc.).
 * 
 * @param swapchain The swapchain handle to unregister
 */
void UnregisterSwapchain(VkSwapchainKHR swapchain);

/**
 * @brief Render the overlay on a swapchain image
 * 
 * Records and submits commands to render the motion-sickness overlay on the
 * specified swapchain image. Called before each frame presentation.
 * 
 * @param queue The queue to submit rendering commands to
 * @param swapchain The swapchain being presented
 * @param imageIndex Index of the swapchain image to render on
 */
void RenderOverlay(VkQueue queue, VkSwapchainKHR swapchain, uint32_t imageIndex);

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
    VkShaderModule vertShader = VK_NULL_HANDLE;
    VkShaderModule fragShader = VK_NULL_HANDLE;
    
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    
    uint32_t width = 0;
    uint32_t height = 0;
    VkFormat format = VK_FORMAT_UNDEFINED;
    bool initialized = false;
    
    // Animation timing
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastFrameTime;
    
    // Integrated position offset for smooth motion
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    
    // Smoothed velocity for momentum effect
    float smoothedVelX = 0.0f;
    float smoothedVelY = 0.0f;
};

// Global overlay state
static std::unordered_map<VkSwapchainKHR, SwapchainOverlay> g_swapchainOverlays;

/**
 * @brief Read a compiled SPIR-V shader file from disk
 * 
 * @param filename Path to the shader file
 * @return Vector containing the shader bytecode, or empty vector on failure
 */
static std::vector<char> ReadShaderFile(const char* filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        // Don't spam logs - we try multiple paths
        return {};
    }
    
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    
    return buffer;
}

/**
 * @brief Create a Vulkan shader module from SPIR-V bytecode
 * 
 * @param device The Vulkan device to create the shader module on
 * @param code Vector containing SPIR-V bytecode
 * @return Shader module handle, or VK_NULL_HANDLE on failure
 */
static VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code) {
    if (code.empty()) {
        return VK_NULL_HANDLE;
    }
    
    auto* dispatch = DispatchManager::GetInstance().GetDeviceDispatch(device);
    if (!dispatch || !dispatch->CreateShaderModule) {
        return VK_NULL_HANDLE;
    }
    
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    VkShaderModule shaderModule;
    if (dispatch->CreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cerr << "[MotionSafe] Failed to create shader module" << std::endl;
        return VK_NULL_HANDLE;
    }
    
    return shaderModule;
}

/**
 * @brief Create a render pass configured for alpha blending overlay rendering
 * 
 * Creates a render pass that loads existing framebuffer content and blends
 * the overlay on top using alpha compositing. Uses PRESENT_SRC_KHR layout
 * for both initial and final layouts.
 * 
 * @param overlay Swapchain overlay structure to store the render pass in
 * @param dispatch Device dispatch table for Vulkan function calls
 * @return true if render pass creation succeeded, false otherwise
 */
static bool CreateRenderPass(SwapchainOverlay& overlay, DeviceDispatchTable* dispatch) {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = overlay.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;  // Load existing content
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    
    if (dispatch->CreateRenderPass(overlay.device, &renderPassInfo, nullptr, &overlay.renderPass) != VK_SUCCESS) {
        std::cerr << "[MotionSafe] Failed to create render pass" << std::endl;
        return false;
    }
    
    return true;
}

void Initialize() {
    std::cout << "[MotionSafe] Overlay system initialized" << std::endl;
    motion_sensor::Initialize();
}

void Shutdown() {
    std::cout << "[MotionSafe] Overlay system shutdown" << std::endl;
    motion_sensor::Shutdown();
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
    overlay.initialized = false;
    
    g_swapchainOverlays[swapchain] = overlay;
    
    std::cout << "[MotionSafe] Registered swapchain: " << swapchain 
              << " (" << width << "x" << height << ", " << images.size() << " images)" << std::endl;
}

void UnregisterSwapchain(VkSwapchainKHR swapchain) {
    auto it = g_swapchainOverlays.find(swapchain);
    if (it != g_swapchainOverlays.end()) {
        SwapchainOverlay& overlay = it->second;
        auto* dispatch = DispatchManager::GetInstance().GetDeviceDispatch(overlay.device);
        
        if (dispatch) {
            if (overlay.pipeline) dispatch->DestroyPipeline(overlay.device, overlay.pipeline, nullptr);
            if (overlay.pipelineLayout) dispatch->DestroyPipelineLayout(overlay.device, overlay.pipelineLayout, nullptr);
            if (overlay.renderPass) dispatch->DestroyRenderPass(overlay.device, overlay.renderPass, nullptr);
            if (overlay.vertShader) dispatch->DestroyShaderModule(overlay.device, overlay.vertShader, nullptr);
            if (overlay.fragShader) dispatch->DestroyShaderModule(overlay.device, overlay.fragShader, nullptr);
            
            for (auto fb : overlay.framebuffers) {
                if (fb) dispatch->DestroyFramebuffer(overlay.device, fb, nullptr);
            }
            for (auto iv : overlay.imageViews) {
                if (iv) dispatch->DestroyImageView(overlay.device, iv, nullptr);
            }
            if (overlay.commandPool) {
                dispatch->DestroyCommandPool(overlay.device, overlay.commandPool, nullptr);
            }
        }
        
        g_swapchainOverlays.erase(it);
        std::cout << "[MotionSafe] Unregistered swapchain: " << swapchain << std::endl;
    }
}

/**
 * @brief Load shader files and create shader modules
 * 
 * Reads vertex and fragment shader SPIR-V files from disk and creates
 * Vulkan shader modules for the overlay rendering pipeline.
 * 
 * @param overlay Swapchain overlay structure to store shader modules in
 * @return true if both shader modules were created successfully, false otherwise
 */
static bool CreateShaderModules(SwapchainOverlay& overlay) {
    // Try multiple shader locations: user install first (no permission issues), then system, then relative path
    std::vector<std::string> shader_paths = {
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/.local/lib/shaders/",
        "/usr/lib/shaders/",
        "shaders/"
    };
    
    std::vector<char> vertCode, fragCode;
    
    for (const auto& path : shader_paths) {
        vertCode = ReadShaderFile((path + "motion_overlay.vert.spv").c_str());
        fragCode = ReadShaderFile((path + "motion_overlay.frag.spv").c_str());
        if (!vertCode.empty() && !fragCode.empty()) {
            std::cout << "[MotionSafe] Loaded shaders from: " << path << std::endl;
            break;
        }
    }
    
    if (vertCode.empty() || fragCode.empty()) {
        std::cerr << "[MotionSafe] Failed to read shader files (did you compile them?)" << std::endl;
        return false;
    }
    
    overlay.vertShader = CreateShaderModule(overlay.device, vertCode);
    overlay.fragShader = CreateShaderModule(overlay.device, fragCode);
    
    if (!overlay.vertShader || !overlay.fragShader) {
        std::cerr << "[MotionSafe] Failed to create shader modules" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief Create the graphics pipeline layout
 * 
 * Creates a pipeline layout with push constants for aspect ratio.
 * 
 * @param overlay Swapchain overlay structure to store the pipeline layout in
 * @param dispatch Device dispatch table for Vulkan function calls
 * @return true if pipeline layout creation succeeded, false otherwise
 */
static bool CreatePipelineLayout(SwapchainOverlay& overlay, DeviceDispatchTable* dispatch) {
    // Push constant for aspect ratio, time, and motion velocity
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 4;  // aspect ratio + time + velocityX + velocityY
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    if (dispatch->CreatePipelineLayout(overlay.device, &pipelineLayoutInfo, nullptr, &overlay.pipelineLayout) != VK_SUCCESS) {
        std::cerr << "[MotionSafe] Failed to create pipeline layout" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief Create the graphics pipeline for overlay rendering
 * 
 * Configures and creates a complete graphics pipeline with:
 * - Vertex and fragment shaders
 * - No vertex input (fullscreen triangle generated in shader)
 * - Alpha blending for transparent overlay
 * - Viewport and scissor matching swapchain dimensions
 * 
 * @param overlay Swapchain overlay structure to store the pipeline in
 * @param dispatch Device dispatch table for Vulkan function calls
 * @return true if pipeline creation succeeded, false otherwise
 */
static bool CreateGraphicsPipeline(SwapchainOverlay& overlay, DeviceDispatchTable* dispatch) {
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = overlay.vertShader;
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = overlay.fragShader;
    fragShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(overlay.width);
    viewport.height = static_cast<float>(overlay.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {overlay.width, overlay.height};
    
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = overlay.pipelineLayout;
    pipelineInfo.renderPass = overlay.renderPass;
    pipelineInfo.subpass = 0;
    
    if (dispatch->CreateGraphicsPipelines(overlay.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &overlay.pipeline) != VK_SUCCESS) {
        std::cerr << "[MotionSafe] Failed to create graphics pipeline" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief Create image views and framebuffers for each swapchain image
 * 
 * For each swapchain image, creates:
 * - An image view for accessing the image in shaders
 * - A framebuffer that references the image view for rendering
 * 
 * @param overlay Swapchain overlay structure to store views and framebuffers in
 * @param dispatch Device dispatch table for Vulkan function calls
 * @return true if all views and framebuffers were created successfully, false otherwise
 */
static bool CreateImageViewsAndFramebuffers(SwapchainOverlay& overlay, DeviceDispatchTable* dispatch) {
    overlay.imageViews.resize(overlay.images.size());
    overlay.framebuffers.resize(overlay.images.size());
    
    for (size_t i = 0; i < overlay.images.size(); i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = overlay.images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = overlay.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        
        if (dispatch->CreateImageView(overlay.device, &viewInfo, nullptr, &overlay.imageViews[i]) != VK_SUCCESS) {
            std::cerr << "[MotionSafe] Failed to create image view " << i << std::endl;
            return false;
        }
        
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = overlay.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &overlay.imageViews[i];
        framebufferInfo.width = overlay.width;
        framebufferInfo.height = overlay.height;
        framebufferInfo.layers = 1;
        
        if (dispatch->CreateFramebuffer(overlay.device, &framebufferInfo, nullptr, &overlay.framebuffers[i]) != VK_SUCCESS) {
            std::cerr << "[MotionSafe] Failed to create framebuffer " << i << std::endl;
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Create command pool and allocate command buffers
 * 
 * Creates a command pool with RESET_COMMAND_BUFFER flag and allocates one
 * primary command buffer per swapchain image for recording overlay rendering.
 * 
 * @note Currently uses queue family index 0, should be improved to query
 *       the actual queue family from the presentation queue
 * 
 * @param overlay Swapchain overlay structure to store command resources in
 * @param dispatch Device dispatch table for Vulkan function calls
 * @return true if command pool and buffers were created successfully, false otherwise
 */
static bool CreateCommandResources(SwapchainOverlay& overlay, DeviceDispatchTable* dispatch) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = 0;  // TODO: Get actual queue family
    
    if (dispatch->CreateCommandPool(overlay.device, &poolInfo, nullptr, &overlay.commandPool) != VK_SUCCESS) {
        std::cerr << "[MotionSafe] Failed to create command pool" << std::endl;
        return false;
    }
    
    overlay.commandBuffers.resize(overlay.images.size());
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = overlay.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(overlay.commandBuffers.size());
    
    if (dispatch->AllocateCommandBuffers(overlay.device, &allocInfo, overlay.commandBuffers.data()) != VK_SUCCESS) {
        std::cerr << "[MotionSafe] Failed to allocate command buffers" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief Initialize all overlay rendering resources for a swapchain
 * 
 * Orchestrates the creation of all Vulkan resources needed for overlay rendering:
 * - Render pass with alpha blending
 * - Vertex and fragment shader modules
 * - Pipeline layout
 * - Graphics pipeline with full state configuration
 * - Image views and framebuffers for each swapchain image
 * - Command pool and command buffers
 * 
 * This function is called lazily on the first frame presentation for each swapchain.
 * 
 * @param overlay Swapchain overlay structure to initialize
 * @return true if all resources were initialized successfully, false on any failure
 */
static bool InitializeOverlay(SwapchainOverlay& overlay) {
    auto* dispatch = DispatchManager::GetInstance().GetDeviceDispatch(overlay.device);
    if (!dispatch) {
        std::cerr << "[MotionSafe] No dispatch table for device" << std::endl;
        return false;
    }
    
    if (!CreateRenderPass(overlay, dispatch)) return false;
    if (!CreateShaderModules(overlay)) return false;
    if (!CreatePipelineLayout(overlay, dispatch)) return false;
    if (!CreateGraphicsPipeline(overlay, dispatch)) return false;
    if (!CreateImageViewsAndFramebuffers(overlay, dispatch)) return false;
    if (!CreateCommandResources(overlay, dispatch)) return false;
    
    // Initialize animation start time
    overlay.startTime = std::chrono::steady_clock::now();
    overlay.lastFrameTime = overlay.startTime;
    overlay.offsetX = 0.0f;
    overlay.offsetY = 0.0f;
    overlay.smoothedVelX = 0.0f;
    overlay.smoothedVelY = 0.0f;
    
    std::cout << "[MotionSafe] Overlay resources initialized successfully" << std::endl;
    return true;
}

/**
 * @brief Render the motion-sickness overlay on a swapchain image
 * 
 * Records and executes rendering commands to composite the overlay on top of
 * the application's rendered frame. This function:
 * 1. Lazily initializes overlay resources on first use
 * 2. Records a command buffer with render pass and draw commands
 * 3. Submits the command buffer to the queue
 * 4. Waits for completion (blocking)
 * 
 * @note Currently uses QueueWaitIdle for simplicity, which blocks the CPU.
 *       For better performance, should use fences and semaphores for async operation.
 * 
 * @param queue The Vulkan queue to submit rendering commands to
 * @param swapchain The swapchain handle being presented
 * @param imageIndex Index of the swapchain image to render the overlay on
 */
void RenderOverlay(VkQueue queue, VkSwapchainKHR swapchain, uint32_t imageIndex) {
    auto it = g_swapchainOverlays.find(swapchain);
    if (it == g_swapchainOverlays.end()) {
        return;  // Swapchain not registered yet
    }
    
    SwapchainOverlay& overlay = it->second;
    
    // Initialize on first use
    if (!overlay.initialized) {
        if (!InitializeOverlay(overlay)) {
            std::cerr << "[MotionSafe] Failed to initialize overlay" << std::endl;
            return;
        }
        overlay.initialized = true;
    }
    
    auto* dispatch = DispatchManager::GetInstance().GetDeviceDispatch(overlay.device);
    if (!dispatch) {
        return;
    }
    
    // Update motion sensor data
    motion_sensor::Update();
    const auto& motionData = motion_sensor::GetMotionData();
    
    // Record command buffer for this image
    VkCommandBuffer cmd = overlay.commandBuffers[imageIndex];
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    if (dispatch->BeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        std::cerr << "[MotionSafe] Failed to begin command buffer" << std::endl;
        return;
    }
    
    // Begin render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = overlay.renderPass;
    renderPassInfo.framebuffer = overlay.framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {overlay.width, overlay.height};
    
    dispatch->CmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // Bind pipeline
    dispatch->CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, overlay.pipeline);
    
    // Calculate delta time and update integrated position
    auto now = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(now - overlay.lastFrameTime).count();
    overlay.lastFrameTime = now;
    
    // Clamp delta time to avoid huge jumps (e.g., when paused/resumed)
    deltaTime = std::min(deltaTime, 0.1f);
    
    // Apply momentum/inertia smoothing to velocity
    // Lower smoothing factor = more lag/momentum (0.0 = no change, 1.0 = instant response)
    const float momentumSmoothing = 0.08f;  // Reduced for more momentum/smoothness
    overlay.smoothedVelX = overlay.smoothedVelX + (motionData.smoothedVelX - overlay.smoothedVelX) * momentumSmoothing;
    overlay.smoothedVelY = overlay.smoothedVelY + (motionData.smoothedVelY - overlay.smoothedVelY) * momentumSmoothing;
    
    // Apply velocity ceiling to prevent dots from moving too fast
    const float maxVelocity = 1.0f;  // Increased maximum velocity in units per second
    float velX = overlay.smoothedVelX;
    float velY = overlay.smoothedVelY;
    
    // Clamp velocity magnitude
    float velMagnitude = std::sqrt(velX * velX + velY * velY);
    if (velMagnitude > maxVelocity) {
        float scale = maxVelocity / velMagnitude;
        velX *= scale;
        velY *= scale;
    }
    
    // Integrate velocity to get smooth position offset (dots move opposite to device motion)
    overlay.offsetX += -velX * deltaTime;
    overlay.offsetY += -velY * deltaTime;
    
    float elapsedTime = std::chrono::duration<float>(now - overlay.startTime).count();
    
    // Wrap offsets at spacing boundaries for seamless grid repetition
    const float spacing = 0.12f;  // Match shader spacing
    float wrappedOffsetX = std::fmod(overlay.offsetX, spacing);
    float wrappedOffsetY = std::fmod(overlay.offsetY, spacing);
    if (wrappedOffsetX < 0) wrappedOffsetX += spacing;
    if (wrappedOffsetY < 0) wrappedOffsetY += spacing;
    
    // Push constants: aspect ratio, time, and wrapped offsets
    float pushConstants[4] = {
        static_cast<float>(overlay.width) / static_cast<float>(overlay.height),
        elapsedTime,
        wrappedOffsetX,
        wrappedOffsetY
    };
    dispatch->CmdPushConstants(cmd, overlay.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), pushConstants);
    
    // Draw fullscreen triangle (3 vertices, no vertex buffer needed)
    dispatch->CmdDraw(cmd, 3, 1, 0, 0);
    
    // End render pass
    dispatch->CmdEndRenderPass(cmd);
    
    if (dispatch->EndCommandBuffer(cmd) != VK_SUCCESS) {
        std::cerr << "[MotionSafe] Failed to end command buffer" << std::endl;
        return;
    }
    
    // Submit command buffer to queue
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    
    if (dispatch->QueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        std::cerr << "[MotionSafe] Failed to submit command buffer" << std::endl;
        return;
    }
    
    // Wait for queue to finish (simple approach - not optimal for performance)
    if (dispatch->QueueWaitIdle) {
        dispatch->QueueWaitIdle(queue);
    }
}

} // namespace overlay
} // namespace motionsafe
