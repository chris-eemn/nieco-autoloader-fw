# Runs INSIDE the build container, with the git superproject bind-mounted at /project as the
# working directory. Builds the bootloader and the application from scratch, combines them into a
# single flashable Intel HEX, and separately packages the application as an update file carrying
# the 16-byte ede_update_file_header_t the USB loader expects.
#
# Both artifacts are named from the version the firmware actually reports: the numeric fields are
# read back out of the generated version header rather than re-derived here, so a filename cannot
# disagree with the image it names. That header comes from `git describe --tags` inside the
# application's CMake build (cmake/gen_version.cmake), which is why the whole repository -- .git
# included -- has to be mounted rather than just the application directory.
#
# Sourced by docker_gen_build.bat / .sh via `. create_release/gen_build.bash`, so `exit` here ends
# the container's shell and becomes the container's exit status. Note this file lives in
# create_release/ but runs with the working directory set to /project (the repository root), which is
# why every path below is relative to the repository root rather than to this folder.

set -e

APP_DIR=autoloader_fw_cmake
BOOT_DIR=autoloader_fw_cmake/stm32c5-bootloader
PRESET=debug_GCC_NUCLEO-C5A3ZG
RELEASE_DIR=release

# The packaging tools are create_release's own copies, so a release depends on the two projects only
# for their source -- never on the tools folders inside them, which are working scratch space.
TOOLS_DIR=create_release

APP_NAME=autoloader_fw
BOOT_NAME=ede-stm32-bootloader

# The bootloader jumps here, and driver_w25q_port/w25q_config.h derives the staged-image slot size
# from the same offset. An application linked anywhere else builds and packs perfectly happily but
# will not run once flashed, so it is refused outright rather than warned about.
EXPECTED_ROM_ORIGIN=0x8020000
LINKER_SCRIPT=$APP_DIR/user_modifiable/Device/STM32C5A3ZGT6/stm32c5a3xg_flash.ld

echo "=== EDE STM32 release build ==="

# git refuses to operate on a repository owned by a different uid, which is exactly what a
# bind-mounted host checkout looks like from in here. Both the version lookup below and the
# application's own CMake version step need this; the submodule is a separate repository.
git config --global --add safe.directory /project
git config --global --add safe.directory "/project/$BOOT_DIR"

# ---------------------------------------------------------------------------
# Submodules must already be on disk
# ---------------------------------------------------------------------------
# The container has no credentials for git@github.com, so it cannot fetch these itself -- they have
# to be checked out on the host before the build runs. Checked explicitly because an uninitialised
# submodule otherwise surfaces as a confusing "cannot find source file" error from CMake, several
# minutes into the run. Note stm32c5-bootloader has a nested driver_w25q of its own.
for submodule in "$APP_DIR/driver_w25q" "$APP_DIR/modbus" "$BOOT_DIR" "$BOOT_DIR/driver_w25q"; do
  if [ -z "$(ls -A "$submodule" 2>/dev/null)" ]; then
    echo "error: $submodule is empty -- submodules are not checked out." >&2
    echo "       Run this on the host, then re-run the build:" >&2
    echo "           git submodule update --init --recursive" >&2
    exit 1
  fi
done

# ---------------------------------------------------------------------------
# Version, from the tag that is already on this commit
# ---------------------------------------------------------------------------
DESCRIBE=$(git describe --tags --dirty --always)
echo "git describe: $DESCRIBE"

if ! git describe --exact-match --tags HEAD >/dev/null 2>&1; then
  echo
  echo "WARNING: HEAD is not itself tagged. The version will come from the most recent reachable"
  echo "         tag ($DESCRIBE), so this build will be numbered as that older release."
  echo "         Tag the commit first if this is a real release."
  echo
fi

if [ -n "$(git status --porcelain)" ]; then
  echo
  echo "WARNING: the working tree has uncommitted changes, so this build cannot be reproduced from"
  echo "         the tag alone. The numeric version is unaffected."
  echo
fi

# ---------------------------------------------------------------------------
# Refuse to build an application linked outside the bootloader's application slot
# ---------------------------------------------------------------------------
if [ ! -f "$LINKER_SCRIPT" ]; then
  echo "error: $LINKER_SCRIPT not found" >&2
  exit 1
fi

ROM_ORIGIN=$(sed -n \
  's/^[[:space:]]*ROM[[:space:]]*(rx)[[:space:]]*:[[:space:]]*org[[:space:]]*=[[:space:]]*\(0[xX][0-9A-Fa-f]\+\).*/\1/p' \
  "$LINKER_SCRIPT")

if [ -z "$ROM_ORIGIN" ]; then
  echo "error: could not read a ROM origin out of $LINKER_SCRIPT" >&2
  exit 1
fi

if [ "$((ROM_ORIGIN))" -ne "$((EXPECTED_ROM_ORIGIN))" ]; then
  echo "error: $LINKER_SCRIPT has ROM org = $ROM_ORIGIN, but the bootloader jumps to" >&2
  echo "       $EXPECTED_ROM_ORIGIN. An application linked there will not run as a bootloaded" >&2
  echo "       image, so no release is produced. Fix the linker script and re-run." >&2
  exit 1
fi
echo "ROM origin: $ROM_ORIGIN"

# ---------------------------------------------------------------------------
# Build both projects from scratch
# ---------------------------------------------------------------------------
# --fresh discards the CMake cache and reconfigures from scratch, which also forces a full
# recompile -- a clean release build without `rm -rf build/`. That matters here because /project is
# a bind mount onto the developer's working tree: deleting the build directory outright would also
# destroy anything else living in it that this repository cannot regenerate.
echo
echo "--- building bootloader ---"
(cd "$BOOT_DIR" && cmake --preset "$PRESET" --fresh && cmake --build "build/$PRESET")

echo
echo "--- building application ---"
(cd "$APP_DIR" && cmake --preset "$PRESET" --fresh && cmake --build "build/$PRESET")

APP_BIN=$APP_DIR/build/$PRESET/$APP_NAME.bin
BOOT_BIN=$BOOT_DIR/build/$PRESET/$BOOT_NAME.bin

for artifact in "$APP_BIN" "$BOOT_BIN"; do
  if [ ! -f "$artifact" ]; then
    echo "error: the build did not produce $artifact" >&2
    exit 1
  fi
done

# ---------------------------------------------------------------------------
# Take the version from what was actually compiled in, not from the tag again
# ---------------------------------------------------------------------------
VERSION_HEADER=$APP_DIR/build/$PRESET/version/app_version_git.h

if [ ! -f "$VERSION_HEADER" ]; then
  echo "error: $VERSION_HEADER was not generated by the build" >&2
  exit 1
fi

read_define() {
  sed -n "s/^#define[[:space:]]\+$1[[:space:]]\+\([0-9]\+\).*/\1/p" "$VERSION_HEADER"
}

MAJOR=$(read_define APP_VERSION_MAJOR)
MINOR=$(read_define APP_VERSION_MINOR)
BUILD=$(read_define APP_VERSION_BUILD)
NUMERIC_VALID=$(read_define APP_VERSION_NUMERIC_VALID)

if [ "$NUMERIC_VALID" != "1" ]; then
  echo "error: the build resolved no numeric version (APP_VERSION_NUMERIC_VALID=$NUMERIC_VALID)." >&2
  echo "       There is no vX.Y.Z tag reachable from HEAD, so the release cannot be named." >&2
  exit 1
fi

BASE=v${MAJOR}_${MINOR}_${BUILD}_${APP_NAME}
echo
echo "version compiled in: $MAJOR.$MINOR.$BUILD  ($DESCRIBE)"

# ---------------------------------------------------------------------------
# Combine, and package the application on its own
# ---------------------------------------------------------------------------
mkdir -p "$RELEASE_DIR"

echo
echo "--- combining bootloader + application ---"
# combine_hex.py places the bootloader at 0x08000000 and the application at 0x08020000, leaving
# the gap between them unprogrammed. The raw application binary is passed deliberately: the
# bootloader expects a bare vector table at the application base, not an update header.
python3 "$TOOLS_DIR/combine_hex.py" "$APP_BIN" \
  --bootloader "$BOOT_BIN" \
  --output "$RELEASE_DIR/$BASE.hex"

echo
echo "--- packaging update file ---"
python3 "$TOOLS_DIR/packer.py" "$APP_BIN" "$RELEASE_DIR/$BASE.bin" \
  --major "$MAJOR" --minor "$MINOR" --build "$BUILD"

echo
echo "=== release artifacts ==="
ls -l "$RELEASE_DIR/$BASE.hex" "$RELEASE_DIR/$BASE.bin"
