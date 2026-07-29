@echo off
rem Produces a release: a combined bootloader+application Intel HEX and a header-prefixed
rem application .bin. Run from powershell/cmd on a windows PC with Docker Desktop running.
rem See README.md in this folder for prerequisites, outputs and troubleshooting.
rem
rem Log in to the org container registry first:
rem   docker login ghcr.io -u <github-username> -p <personal-access-token-with-read:packages>
rem
rem The repository root -- the parent of this folder -- is what gets mounted at /project, not this
rem folder: the build needs the application, the bootloader submodule, and .git, because the
rem firmware version comes from the git tag via cmake/gen_version.cmake. It is resolved from this
rem script's own location, so the script works from whatever directory it is invoked in.
pushd "%~dp0.."
set REPO_ROOT=%CD%
popd

rem Built and pushed by docker_build_image.bat from the Dockerfile in this folder.
set IMAGE=ghcr.io/embedded-design-solutions/stm32c5-release-builder:1.0

docker run --workdir=/project --rm -v "%REPO_ROOT%:/project" %IMAGE% /bin/bash -c ". create_release/gen_build.bash"
