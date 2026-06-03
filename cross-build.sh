#!/bin/sh
set -e

IMAGE_NAME="smartpiano-cross-builder:latest"

if ! command -v podman >/dev/null 2>&1; then
  echo "Error: podman is required but not installed." >&2
  exit 1
fi

echo "=== 1/2 Building the cross-compilation image ==="
podman build -t "$IMAGE_NAME" -f Containerfile.cross .

echo "=== 2/2 Compiling Engine and UI ==="
# Mount the workspace and the build script, then execute it
podman run --rm -it \
  -v "$PWD:/workspace" \
  -v "$PWD/build-in-container.sh:/build-in-container.sh:ro" \
  "$IMAGE_NAME" /build-in-container.sh

echo "=== Success! Binaries are ready in the 'deploy' directory. ==="
