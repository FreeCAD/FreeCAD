# Running FreeCAD With Docker

This project includes a Docker setup for building FreeCAD and opening it through a browser-based noVNC desktop.

## Prerequisites

- Docker Desktop installed and running
- PowerShell or another terminal
- This repository checked out locally

## Start FreeCAD

Run these commands from PowerShell:

```powershell
cd "E:\Scratch Pad\MetaCad\FreeCAD"
docker compose build
docker compose run --rm configure
docker compose run --rm build
docker compose --profile desktop up desktop
```

Then open this URL in your browser:

```text
http://localhost:8010/vnc.html?resize=scale&autoconnect=true
```

## What Each Command Does

```powershell
cd "E:\Scratch Pad\MetaCad\FreeCAD"
```

Moves the terminal into the FreeCAD repository folder. Docker Compose must be run from the folder that contains `docker-compose.yml`.

```powershell
docker compose build
```

Builds the local Docker image from the `Dockerfile`. This installs Ubuntu packages, Pixi, and the FreeCAD dependency environment needed to configure and build the project.

```powershell
docker compose run --rm configure
```

Runs the FreeCAD CMake configure step inside a temporary container.

Internally, this runs:

```bash
pixi run configure-debug
```

It creates the debug build configuration under:

```text
build/debug
```

The `--rm` flag removes the temporary container after the command finishes.

```powershell
docker compose run --rm build
```

Compiles FreeCAD inside a temporary container.

Internally, this runs:

```bash
pixi run build-debug
```

This can take a long time because FreeCAD is a large C++/Qt application. If interrupted, running the same command again usually resumes from the existing build files.

```powershell
docker compose --profile desktop up desktop
```

Starts the browser-accessible desktop service. This runs Xvfb, x11vnc, noVNC, and FreeCAD inside the container.

The service exposes noVNC on host port `8010`, so FreeCAD can be opened at:

```text
http://localhost:8010/vnc.html?resize=scale&autoconnect=true
```

Keep this command running while using FreeCAD in the browser. Stop it with `Ctrl + C`.

## When To Use `docker compose down -v`

Use this command only when you want to reset Docker state for this project:

```powershell
docker compose down -v
```

It stops Compose services and removes the named Docker volumes used by this setup, including:

- the FreeCAD build volume
- the Pixi environment cache volume
- the ccache compiler cache volume

This is useful when:

- the build cache is corrupted
- CMake or Ninja keeps failing due to stale generated files
- you want a clean rebuild from scratch
- you see timestamp-related errors such as `manifest 'build.ninja' still dirty`

It does not delete your source code, but the next build will take longer because Docker must recreate the removed caches.

After running it, start again with:

```powershell
docker compose build
docker compose run --rm configure
docker compose run --rm build
docker compose --profile desktop up desktop
```

## Faster Development And Testing

You do not need to run every Docker command after every code change.

The full startup sequence is:

```powershell
docker compose build
docker compose run --rm configure
docker compose run --rm build
docker compose --profile desktop up desktop
```

Use the full sequence when setting up the project for the first time, after deleting Docker volumes, or after changing major build/dependency configuration.

For day-to-day development, use the smallest command needed for the type of change you made.

### If You Changed Python Code

For most Python-only changes, especially in:

```text
src/Mod/copilot
```

you usually do not need to rebuild the Docker image.

Use:

```powershell
docker compose --profile desktop up desktop
```

If FreeCAD is already running, stop the desktop service with `Ctrl + C`, then start it again:

```powershell
docker compose --profile desktop up desktop
```

This is usually enough because the repository is mounted into the container.

If the changed Python files are copied into FreeCAD's build output by CMake and the change does not appear inside FreeCAD, run:

```powershell
docker compose run --rm build
docker compose --profile desktop up desktop
```

### If You Added Or Removed Python Files

If you add or remove files that are listed in a `CMakeLists.txt`, run configure and build again:

```powershell
docker compose run --rm configure
docker compose run --rm build
docker compose --profile desktop up desktop
```

Use this for changes such as:

- adding a new file under `src/Mod/copilot`
- removing a Copilot file
- updating `src/Mod/copilot/CMakeLists.txt`
- updating `src/Mod/CMakeLists.txt`

### If You Changed C++ Code

Run only the build step:

```powershell
docker compose run --rm build
docker compose --profile desktop up desktop
```

CMake/Ninja should rebuild only the files affected by your change. It should not rebuild all of FreeCAD unless many shared files changed.

### If You Changed CMake Configuration

Run configure, then build:

```powershell
docker compose run --rm configure
docker compose run --rm build
docker compose --profile desktop up desktop
```

Use this when you change files such as:

- `CMakeLists.txt`
- `CMakePresets.json`
- CMake helper files
- build options
- module registration

### If You Changed Docker Configuration

Rebuild the Docker image:

```powershell
docker compose build
docker compose --profile desktop up desktop
```

Use this when you change:

- `Dockerfile`
- `docker-compose.yml`
- `.dockerignore`
- files under `docker/`
- OS-level packages
- container startup behavior

If the Docker change also affects the FreeCAD build environment, run:

```powershell
docker compose build
docker compose run --rm configure
docker compose run --rm build
docker compose --profile desktop up desktop
```

### If You Changed Pixi Dependencies

Run the full build setup:

```powershell
docker compose build
docker compose run --rm configure
docker compose run --rm build
docker compose --profile desktop up desktop
```

Use this when you change:

- `pixi.toml`
- `pixi.lock`
- compiler or dependency versions

### Quick Command Guide

Use this table as the normal development guide:

| Change Type | Commands |
| --- | --- |
| Start existing built FreeCAD | `docker compose --profile desktop up desktop` |
| Python-only change | restart desktop service |
| Python change not appearing | `docker compose run --rm build` then start desktop |
| Added/removed CMake-listed files | `configure`, then `build`, then start desktop |
| C++ change | `build`, then start desktop |
| CMake change | `configure`, then `build`, then start desktop |
| Dockerfile or startup script change | `docker compose build`, then start desktop |
| Dependency change | full sequence |
| Corrupted build/cache | `docker compose down -v`, then full sequence |

### Recommended Copilot Development Loop

For Copilot work, use this loop most of the time:

```powershell
docker compose --profile desktop up desktop
```

Make a Python change under:

```text
src/Mod/copilot
```

Then restart the desktop service:

```powershell
Ctrl + C
docker compose --profile desktop up desktop
```

If FreeCAD does not pick up the change:

```powershell
docker compose run --rm build
docker compose --profile desktop up desktop
```
