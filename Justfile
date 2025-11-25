run-vkcube:
    #!/usr/bin/env zsh
    source ./setup-vulkan-env.sh
    VK_INSTANCE_LAYERS="VK_LAYER_MOTIONSAFE_overlay" /opt/homebrew/Cellar/vulkan-tools/1.4.328.1/cube/vkcube.app/Contents/MacOS/vkcube

run-vkcube-without-layer:
    #!/usr/bin/env zsh
    source ./setup-vulkan-env.sh
    /opt/homebrew/Cellar/vulkan-tools/1.4.328.1/cube/vkcube.app/Contents/MacOS/vkcube