# create_release

Produces a release of the STM32C5A3ZG stepper firmware inside a Docker container, so the output does
not depend on what happens to be installed on the machine that builds it.

One command builds the bootloader and the application from scratch and emits two artifacts: a
combined Intel HEX for programming a blank board in one shot, and a header-prefixed application
binary for the field-update path.

## What you get

Both land in `../release/`, named from the git tag the build was made at:

| File | What it is | Used for |
|---|---|---|
| `v<major>_<minor>_<build>_stepper_144pin.hex` | Bootloader at `0x08000000` + application at `0x08020000` | Programming a board over ST-LINK / STM32CubeProgrammer |
| `v<major>_<minor>_<build>_stepper_144pin.bin` | 16-byte `ede_update_file_header_t` + application | Field update over USB thumb drive or USART3 |

For example, a build at tag `v2.4.5` produces `v2_4_5_stepper_144pin.hex` and
`v2_4_5_stepper_144pin.bin`.

## Prerequisites

1. **Docker Desktop running.** If it is not, every script fails with
   `failed to connect to the docker API at npipe:...`.

2. **Logged in to the org registry**, so the build image can be pulled:

       docker login ghcr.io -u <github-username> -p <personal-access-token>

   A token with `read:packages` is enough to produce releases. `write:packages` is only needed to
   push a new build image.

3. **Submodules checked out.** The container has no GitHub credentials and cannot fetch them itself.
   Run this on the host — note `--recursive`, because the bootloader has a `driver_w25q` submodule of
   its own:

       git submodule update --init --recursive

   The submodules are needed for their **source**, so the bootloader can be compiled. Nothing here
   uses the `tools/` folders inside them; this folder carries its own copies of everything it runs.

4. **The commit is tagged.** The firmware version comes from `git describe`, and the artifacts are
   named from it. Tag before building:

       git tag v2.4.5

   The build still runs on an untagged commit, but it will warn and number the release from the most
   recent older tag, which is almost never what you want.

## Creating a release

From this folder:

    docker_gen_build.bat

Use `docker_gen_build.sh` from a POSIX shell, or `docker_gen_build_gitbash.sh` under Git Bash /
MSYS2 (it sets the MSYS path-conversion guards that a bind mount needs).

The scripts locate the repository from their own path, so they work from any working directory. The
**repository root** is mounted at `/project` inside the container, not this folder — the build needs
the application, the bootloader submodule, and `.git` all together.

Expect a few minutes: both projects are compiled from scratch, roughly 460 translation units each.

## What the build does

`gen_build.bash` runs inside the container and, in order:

1. Marks `/project` as a safe git directory — a bind-mounted host checkout looks foreign to git.
2. Checks the submodules are populated, and stops immediately if not.
3. Reads the version from `git describe --tags`, warning if the commit is untagged or the tree dirty.
4. **Refuses to continue unless the application's linker script has `ROM org = 0x8020000`.** That is
   the address the bootloader jumps to; an application linked anywhere else builds and packages
   perfectly happily but will not run. See "ROM origin" below.
5. Builds the bootloader, then the application, each with `cmake --preset ... --fresh` for a clean
   configure and a full recompile.
6. Reads `APP_VERSION_MAJOR/MINOR/BUILD` back out of the generated `app_version_git.h` and names the
   artifacts from those — so a filename can never disagree with the version compiled into the image.
7. Combines the two binaries into the HEX, and separately packages the application with its update
   header.

Because step 6 takes the version from what was actually compiled rather than re-deriving it, the
version in the filename, the version in the update header, and the version the firmware prints at
boot are guaranteed to agree.

## Using the outputs

**The HEX** programs a blank or fully-erased board in one operation. The bootloader occupies
`0x08000000`, the application starts at `0x08020000`, and the gap between them is deliberately left
out of the file so those pages stay erased instead of being filled. Its entry point is the
bootloader's reset vector.

**The BIN** is the field-update file. Copy it to the **root directory** of a FAT-formatted USB
thumb drive and insert the drive after the board has booted; the firmware scans the root, picks the
highest version it finds, validates the header, and reports whether an update is needed. Keep the
generated filename exactly as it is — the firmware matches on the `v<major>_<minor>_<build>` prefix
and the `_stepper_144pin.bin` suffix, and cross-checks the version in the name against the version in
the header. The same file can be streamed over USART3 with
`stepper_144pin_cmake/tools/uart_sender.py`.

Note the HEX deliberately does **not** contain the update header. The bootloader expects a bare
vector table at the application base, so `combine_hex.py` strips the header if one is present.

## What's in this folder

This folder is self-contained: everything a release runs lives here, so the process never depends on
the working `tools/` folders inside `stepper_144pin_cmake` or the bootloader submodule. Those are
scratch space and are free to change or disappear.

| File | Role |
|---|---|
| `docker_gen_build.bat` / `.sh` / `_gitbash.sh` | Entry points — start the container. Use the `_gitbash` one under Git Bash/MSYS2 |
| `gen_build.bash` | Runs inside the container; the release procedure itself |
| `combine_hex.py` | Merges the bootloader and application binaries into one Intel HEX |
| `packer.py` | Prepends the 16-byte `ede_update_file_header_t` to the application binary |
| `crc16_ccitt.py` | CRC used by both of the above; must match the firmware's `crc16_ccitt.c` |
| `requirements.txt` | Documents that the tooling needs no third-party packages |
| `Dockerfile`, `.dockerignore` | The build image |
| `docker_build_image.bat` / `.sh` | Build and publish that image |

The three Python files are copies of tools that also exist elsewhere in the repository. They were
verified byte-identical in output at the time they were copied. If you fix a bug in one, the other
copy does not get the fix — check both.

## The build image

    ghcr.io/embedded-design-solutions/stm32c5-release-builder:1.0

This is already published, so producing a release just pulls it — the first run downloads it, later
runs use the local copy. You only need the rest of this section if you are changing the build
environment itself.

Rebuild and publish it with:

    docker_build_image.bat          # build and push
    docker_build_image.bat local    # build only, no push

The tag is an image revision, not the toolchain version. Bump it when you change the `Dockerfile`,
and update the same string in the three `docker_gen_build` scripts — pushing new contents over an
existing tag leaves everyone who already pulled `1.0` silently on the old image.

Contents, all pinned by URL in the `Dockerfile` here:

| Component | Version | Why pinned by URL |
|---|---|---|
| Debian | bookworm-slim | base |
| arm-gnu-toolchain | 14.3.rel1 | bookworm ships arm-none-eabi-gcc 12; the project builds with 14.3 |
| CMake | 4.0.0 | bookworm ships 3.25; the project requires ≥ 3.30 |
| Ninja | 1.12.1 | reproducibility |
| python3, git, make | distro | packaging tools are standard-library only, so no pip |

The image ends with a layer that runs `--version` on all five tools, so a bad download fails at
image build time rather than in someone's release. `.dockerignore` empties the build context — the
Dockerfile downloads its own tarballs and copies nothing from the repository.

## Troubleshooting

| Message | Cause | Fix |
|---|---|---|
| `failed to connect to the docker API at npipe:...` | Docker Desktop is not running | Start it |
| `... is empty -- submodules are not checked out` | A submodule was never initialised | `git submodule update --init --recursive` on the host |
| `has ROM org = 0x..., but the bootloader jumps to 0x8020000` | The application is linked for the wrong address | See below |
| `the build resolved no numeric version` | No `vX.Y.Z` tag is reachable from HEAD | Tag the commit |
| `WARNING: HEAD is not itself tagged` | Building a commit past the last tag | Tag it, or accept the older number |
| `denied` / `unauthorized` on pull | Not logged in, or no org access | `docker login ghcr.io` with a `read:packages` token |
| `denied` on push | Token lacks `write:packages` | Reissue the token |

### ROM origin

`stepper_144pin_cmake/user_modifiable/Device/STM32C5A3ZGT6/stm32c5a3xg_flash.ld` must read:

    ROM (rx) : org = 0x8020000, len = 0xE0000

That address is load-bearing in three places: the bootloader jumps to it, `w25q_config.h` derives the
staged-image slot size from it, and this build refuses to package an image linked anywhere else.
It is sometimes temporarily changed to `0x8000000` to run the application standalone without the
bootloader — that change must be reverted before cutting a release.

## Limitations

- **The container's compiler is not the one a local STM32CubeIDE build uses.** This image carries
  ARM's upstream `arm-gnu-toolchain 14.3.rel1`; a local build uses ST's patched "GNU Tools for
  STM32" fork of the same GCC base (14.3.1 20250623). Both produce working firmware, but do not
  expect the container's output to be byte-identical to a local build.
- **x86_64 only.** The toolchain tarball is the x86_64-host build. On Apple Silicon the image needs
  `--platform linux/amd64` and will run under emulation.
- **The download URLs are version-pinned but not checksum-pinned.** Worth adding SHA256 verification
  to the `Dockerfile` for a toolchain used to cut releases.

## Related

`stepper_144pin_cmake/tools/fake_create_release.py` does something deliberately different: it
rebuilds locally with an *arbitrary* injected version, for testing the update path without cutting a
real release. It is not a substitute for this folder — it does not build the bootloader and produces
no combined HEX.
