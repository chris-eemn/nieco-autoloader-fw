# Build environment for the STM32C5A3ZG stepper-motor firmware and its bootloader -- the ARM
# equivalent of the sam_build_env image used by the ATSAM projects.
#
# Contains only what a release build needs: the ARM bare-metal toolchain, CMake, Ninja, Python 3
# and git. No flashing or debug tooling, since releases are produced here and programmed elsewhere.
#
# Build and push with docker_build_image.bat (or .sh). Consumed by docker_gen_build.bat, which
# mounts the repository at /project and runs gen_build.bash inside this image.
#
# Everything fetched here is version-pinned by URL rather than taken from the distribution, because
# Debian bookworm ships CMake 3.25 (the project needs >= 3.30) and arm-none-eabi-gcc 12 (the project
# is built with 14.3). Note the toolchain is ARM's upstream release, not ST's patched "GNU Tools for
# STM32" fork that a local STM32CubeIDE build uses; the GCC base is the same (14.3.1 20250623), but
# do not expect binaries to be byte-identical to a local build.

FROM debian:bookworm-slim

LABEL org.opencontainers.image.source="https://github.com/Embedded-Design-Solutions/NUCLEO-C5A3ZG-stepper-motor"
LABEL org.opencontainers.image.description="ARM bare-metal build environment for the STM32C5A3ZG stepper firmware and bootloader"

ARG ARM_TOOLCHAIN_VERSION=14.3.rel1
ARG CMAKE_VERSION=4.0.0
ARG NINJA_VERSION=1.12.1

ENV DEBIAN_FRONTEND=noninteractive

# git is needed because the firmware version comes from `git describe` during the CMake build;
# python3 runs the packaging and hex-combining tools (both standard library only, so no pip).
RUN apt-get update \
 && apt-get install --no-install-recommends -y \
      ca-certificates \
      curl \
      xz-utils \
      unzip \
      git \
      python3 \
      make \
 && rm -rf /var/lib/apt/lists/*

# ARM bare-metal toolchain. The tarball's top-level directory carries the full version in its name,
# so it is extracted with the component stripped into a stable path.
RUN mkdir -p /opt/arm-none-eabi \
 && curl -fL --retry 3 -o /tmp/arm-toolchain.tar.xz \
      "https://developer.arm.com/-/media/Files/downloads/gnu/${ARM_TOOLCHAIN_VERSION}/binrel/arm-gnu-toolchain-${ARM_TOOLCHAIN_VERSION}-x86_64-arm-none-eabi.tar.xz" \
 && tar -xJf /tmp/arm-toolchain.tar.xz -C /opt/arm-none-eabi --strip-components=1 \
 && rm /tmp/arm-toolchain.tar.xz

RUN curl -fL --retry 3 -o /tmp/cmake.tar.gz \
      "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" \
 && mkdir -p /opt/cmake \
 && tar -xzf /tmp/cmake.tar.gz -C /opt/cmake --strip-components=1 \
 && rm /tmp/cmake.tar.gz

RUN curl -fL --retry 3 -o /tmp/ninja.zip \
      "https://github.com/ninja-build/ninja/releases/download/v${NINJA_VERSION}/ninja-linux.zip" \
 && unzip -q /tmp/ninja.zip -d /usr/local/bin \
 && chmod +x /usr/local/bin/ninja \
 && rm /tmp/ninja.zip

ENV PATH="/opt/arm-none-eabi/bin:/opt/cmake/bin:${PATH}"

# Fail image creation rather than the first build if any of this is missing or mismatched. The
# toolchain check also catches a tarball whose layout changed, since the binaries are what matter,
# not the extracted paths.
RUN arm-none-eabi-gcc --version \
 && arm-none-eabi-objcopy --version \
 && cmake --version \
 && ninja --version \
 && python3 --version \
 && git --version

WORKDIR /project
