run-vkcube-auto:
    #!/usr/bin/env zsh
    source ./setup-vulkan-env.sh
    echo "Running vkcube with motion sickness overlay for 2 seconds..."
    VK_INSTANCE_LAYERS="VK_LAYER_MOTIONSAFE_overlay" /opt/homebrew/Cellar/vulkan-tools/1.4.328.1/cube/vkcube.app/Contents/MacOS/vkcube || true


run-vkcube:
    #!/usr/bin/env zsh
    source ./setup-vulkan-env.sh
    echo "Running vkcube with motion sickness overlay for 2 seconds..."
    VK_INSTANCE_LAYERS="VK_LAYER_MOTIONSAFE_overlay" timeout 10 /opt/homebrew/Cellar/vulkan-tools/1.4.328.1/cube/vkcube.app/Contents/MacOS/vkcube || true
    echo ""
    echo "Did the overlay render correctly? (y/n/q to quit)"
    read -k 1 response
    echo ""
    case $response in
        y|Y) echo "✅ PASS - Overlay rendered correctly" ;;
        n|N) echo "❌ FAIL - Overlay did not render correctly" ;;
        q|Q) echo "⏭️  SKIP - Test skipped" ;;
        *) echo "Invalid response" ;;
    esac

run-vkcube-without-layer:
    #!/usr/bin/env zsh
    source ./setup-vulkan-env.sh
    /opt/homebrew/Cellar/vulkan-tools/1.4.328.1/cube/vkcube.app/Contents/MacOS/vkcube