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
http://localhost:8010/vnc.html
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
http://localhost:8010/vnc.html
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
