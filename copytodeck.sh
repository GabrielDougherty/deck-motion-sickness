#!/usr/bin/env bash
set -e

echo "Syncing files to Steam Deck..."
rsync -avz --exclude=build --exclude=.git --exclude='*.o' --exclude='*.so' --exclude='*.dylib' \
    /Users/gabriel/ws/deck-motion-sickness/ deck@steamdeck:~/deck-motion-sickness/

echo "✓ Files synced to Steam Deck"
echo "Run ./buildondeck.sh next to build and install"
