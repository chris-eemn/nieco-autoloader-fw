@echo off
rem Builds the STM32C5 build-environment image from the Dockerfile in this folder and pushes it to
rem the org registry. This is a one-time (or toolchain-upgrade) step -- producing a release only
rem needs docker_gen_build.bat. Requires Docker Desktop to be running.
rem
rem Log in first (a personal access token with write:packages is required to push):
rem   docker login ghcr.io -u <github-username> -p <personal-access-token>
rem
rem Pass "local" as the first argument to build without pushing:
rem   docker_build_image.bat local

rem The tag is an image revision, not the toolchain version -- bump it whenever the Dockerfile
rem changes, and update the same string in the three docker_gen_build scripts.
set IMAGE=ghcr.io/embedded-design-solutions/stm32c5-release-builder:1.0

rem The build context is this folder, resolved from the script's own location so it does not matter
rem where the script is invoked from. .dockerignore empties the context; the Dockerfile downloads
rem its own pinned tarballs and COPYs nothing.
echo Building %IMAGE%
pushd "%~dp0"
docker build -t %IMAGE% .
set BUILD_RESULT=%ERRORLEVEL%
popd

if not "%BUILD_RESULT%"=="0" (
  echo error: image build failed; nothing was pushed.
  exit /b 1
)

if "%1"=="local" (
  echo Built %IMAGE% locally. Skipping push ^("local" was passed^).
  exit /b 0
)

echo Pushing %IMAGE%
docker push %IMAGE%
if errorlevel 1 (
  echo error: push failed. Check that you are logged in to ghcr.io with a token that has
  echo        write:packages, and that you have access to the Embedded-Design-Solutions org.
  exit /b 1
)

echo Done. %IMAGE% is available to docker_gen_build.bat.
