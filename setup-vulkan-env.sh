#!/usr/bin/env bash
# Source this file to set up the Vulkan environment for testing the layer
# Usage: source setup-vulkan-env.sh

# Set the VulkanSDK ICD path
export VK_ICD_FILENAMES=~/VulkanSDK/1.4.328.1/macOS/share/vulkan/icd.d/MoltenVK_icd.json

# Optionally enable the MotionSafe layer
# Uncomment the next line to automatically enable the layer for all Vulkan applications
# export VK_INSTANCE_LAYERS="VK_LAYER_MOTIONSAFE_overlay"

echo "Vulkan environment configured:"
echo "  VK_ICD_FILENAMES=$VK_ICD_FILENAMES"
if [ -n "$VK_INSTANCE_LAYERS" ]; then
    echo "  VK_INSTANCE_LAYERS=$VK_INSTANCE_LAYERS"
fi
