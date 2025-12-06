#!/usr/bin/env bash
set -e

echo "Building on Steam Deck..."
ssh -t deck@steamdeck 'cd ~/deck-motion-sickness && \
    rm -rf build && \
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ && \
    ninja -C build && \
    sudo ninja -C build install'

echo ""
echo "✓ Built and installed on Steam Deck"
echo "Layer installed to:"
echo "  - ~/.local/share/vulkan/implicit_layer.d/ (user)"
echo "  - /usr/share/vulkan/implicit_layer.d/ (system)"
