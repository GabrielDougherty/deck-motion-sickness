#!/usr/bin/env python3
"""
Update Vulkan loader settings to include the MotionSafe layer.
This script adds the layer to ~/.local/share/vulkan/loader_settings.d/vk_loader_settings.json
"""

import json
import os
import sys
from pathlib import Path

def update_loader_settings():
    home = Path.home()
    settings_file = home / ".local/share/vulkan/loader_settings.d/vk_loader_settings.json"
    layer_manifest = home / ".local/share/vulkan/explicit_layer.d/VK_LAYER_MOTIONSAFE_overlay.json"
    
    # Check if layer manifest exists
    if not layer_manifest.exists():
        print(f"Error: Layer manifest not found at {layer_manifest}")
        return False
    
    # Check if settings file exists
    if not settings_file.exists():
        print(f"Loader settings file not found at {settings_file}")
        print("The layer should be automatically discovered by the Vulkan loader.")
        return True
    
    # Read existing settings
    try:
        with open(settings_file, 'r') as f:
            data = json.load(f)
    except json.JSONDecodeError as e:
        print(f"Error reading loader settings: {e}")
        return False
    
    # Check if our layer is already in the settings
    layer_name = "VK_LAYER_MOTIONSAFE_overlay"
    layers = data.get('settings_array', [{}])[0].get('layers', [])
    
    for layer in layers:
        if layer.get('name') == layer_name:
            print(f"Layer '{layer_name}' is already in loader settings.")
            return True
    
    # Add our layer
    new_layer = {
        "control": "auto",
        "name": layer_name,
        "path": str(layer_manifest),
        "treat_as_implicit_manifest": False
    }
    
    layers.append(new_layer)
    
    # Backup original file
    backup_file = settings_file.with_suffix('.json.bak')
    try:
        import shutil
        shutil.copy2(settings_file, backup_file)
        print(f"Created backup: {backup_file}")
    except Exception as e:
        print(f"Warning: Could not create backup: {e}")
    
    # Write updated settings
    try:
        with open(settings_file, 'w') as f:
            json.dump(data, f, indent=4)
        print(f"Successfully added '{layer_name}' to loader settings.")
        print(f"Updated: {settings_file}")
        return True
    except Exception as e:
        print(f"Error writing loader settings: {e}")
        return False

if __name__ == "__main__":
    success = update_loader_settings()
    sys.exit(0 if success else 1)
