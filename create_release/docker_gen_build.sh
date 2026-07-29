#!/bin/sh
# Produces a release: a combined bootloader+application Intel HEX and a header-prefixed application
# .bin. See README.md in this folder for prerequisites, outputs and troubleshooting.
#
# Log in to the org container registry first:
#   docker login ghcr.io -u <github-username> -p <personal-access-token-with-read:packages>
#
# The repository root -- the parent of this folder -- is what gets mounted at /project, not this
# folder: the build needs the application, the bootloader submodule, and .git, because the firmware
# version comes from the git tag via cmake/gen_version.cmake. It is resolved from this script's own
# location, so the script works from whatever directory it is invoked in.
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)

# Built and pushed by docker_build_image.sh from the Dockerfile in this folder.
IMAGE=ghcr.io/embedded-design-solutions/stm32c5-release-builder:1.0

docker run --workdir=/project --rm -v "$REPO_ROOT:/project" "$IMAGE" /bin/bash -c ". create_release/gen_build.bash"
