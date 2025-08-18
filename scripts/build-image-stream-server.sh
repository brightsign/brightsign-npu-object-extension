#!/bin/bash

# Download script for image-stream-server pre-built binary
set -e

# Note: We only need the OUTPUT_BINARY parameter now
OUTPUT_BINARY="$3"

echo "Downloading image-stream-server-player from release..."
echo "  Output: $OUTPUT_BINARY"

# Download URL for the pre-built binary
DOWNLOAD_URL="https://github.com/brightsign/bs-image-stream-server/releases/download/v0.1/image-stream-server-player"

# Check if binary already exists
if [ -f "$OUTPUT_BINARY" ]; then
    echo "Binary already exists at $OUTPUT_BINARY, skipping download"
else
    echo "Downloading image-stream-server-player binary..."
    echo "  From: $DOWNLOAD_URL"
    echo "  To: $OUTPUT_BINARY"
    
    # Download the binary
    if curl -L -o "$OUTPUT_BINARY" "$DOWNLOAD_URL"; then
        echo "Download completed successfully"
        
        # Make it executable
        chmod +x "$OUTPUT_BINARY"
        echo "Made binary executable"
    else
        echo "Download failed"
        exit 1
    fi
fi

# Verify the binary exists and is executable
if [ -f "$OUTPUT_BINARY" ] && [ -x "$OUTPUT_BINARY" ]; then
    echo "Image stream server player binary ready at $OUTPUT_BINARY"
else
    echo "Binary verification failed"
    exit 1
fi
