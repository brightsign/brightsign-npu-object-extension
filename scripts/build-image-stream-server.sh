#!/bin/bash

# Build script for image-stream-server
set -e

IMAGE_STREAM_SERVER_DIR="$1"
IMAGE_STREAM_SERVER_BINARY="$2"
OUTPUT_BINARY="$3"

echo "Building bs-image-stream-server..."
echo "  Source dir: $IMAGE_STREAM_SERVER_DIR"
echo "  Expected binary: $IMAGE_STREAM_SERVER_BINARY"
echo "  Output: $OUTPUT_BINARY"

# Clone or update repository
if [ ! -d "$IMAGE_STREAM_SERVER_DIR" ]; then
    echo "Cloning bs-image-stream-server..."
    git clone git@github.com:brightsign/bs-image-stream-server.git "$IMAGE_STREAM_SERVER_DIR"
else
    echo "Repository already exists, updating..."
    cd "$IMAGE_STREAM_SERVER_DIR"
    git pull origin main || true
fi

# Build the image stream server
echo "Building image stream server..."
cd "$IMAGE_STREAM_SERVER_DIR"
echo "Current directory: $(pwd)"
make build-arm64

# Check if binary was created and copy it
if [ -f "$IMAGE_STREAM_SERVER_BINARY" ]; then
    echo "Build completed successfully, copying binary..."
    cp "$IMAGE_STREAM_SERVER_BINARY" "$OUTPUT_BINARY"
    echo "Image stream server binary copied to $OUTPUT_BINARY"
else
    echo "Build failed - binary not found at $IMAGE_STREAM_SERVER_BINARY"
    exit 1
fi
