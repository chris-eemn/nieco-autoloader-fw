#!/bin/sh
# Builds the STM32C5 build-environment image from the Dockerfile in this folder and pushes it to the
# org registry. This is a one-time (or toolchain-upgrade) step -- producing a release only needs
# docker_gen_build.sh. Requires the docker daemon to be running.
#
# Log in first (a personal access token with write:packages is required to push):
#   docker login ghcr.io -u <github-username> -p <personal-access-token>
#
# Pass "local" as the first argument to build without pushing:
#   ./docker_build_image.sh local

set -e

# The tag is an image revision, not the toolchain version -- bump it whenever the Dockerfile
# changes, and update the same string in the three docker_gen_build scripts.
IMAGE=ghcr.io/embedded-design-solutions/stm32c5-release-builder:1.0

# The build context is this folder, resolved from the script's own location so it does not matter
# where the script is invoked from. .dockerignore empties the context; the Dockerfile downloads its
# own pinned tarballs and COPYs nothing.
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

echo "Building $IMAGE"
docker build -t "$IMAGE" "$SCRIPT_DIR"

if [ "$1" = "local" ]; then
  echo "Built $IMAGE locally. Skipping push (\"local\" was passed)."
  exit 0
fi

echo "Pushing $IMAGE"
if ! docker push "$IMAGE"; then
  echo "error: push failed. Check that you are logged in to ghcr.io with a token that has" >&2
  echo "       write:packages, and that you have access to the Embedded-Design-Solutions org." >&2
  exit 1
fi

echo "Done. $IMAGE is available to docker_gen_build.sh."
