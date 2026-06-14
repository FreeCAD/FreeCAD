<p align="center">
  <img alt="parashellunderconstructionbanner" src="https://raw.githubusercontent.com/TryParashell/Parashell/refs/heads/main/files/Under%20Construction%20Banner%20GitHub.png">
</p>

<p align="center">This repo is under construction and in early dev stage!</p>

<p align="center">
  <a href='https://www.parashell.cloud/open-source'><img alt="GitHub contributors" src="https://img.shields.io/github/contributors/TryParashell/Parashell"/></a>
  <a href='http://makeapullrequest.com'><img alt='PRs Welcome' src='https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=shields'/></a>
  <a href="https://github.com/TryParashell/Parashell/commits/main"><img alt="GitHub commit activity" src="https://img.shields.io/github/commit-activity/m/Parashell/Parashell"/></a>
  <a href="https://github.com/TryParashell/Parashell/issues?q=is%3Aissue+state%3Aclosed"><img alt="GitHub closed issues" src="https://img.shields.io/github/issues-closed/TryParashell/Parashell"/></a>
  <a href="https://github.com/TryParashell/Parashell/stargazers"><img alt="GitHub stars" src="https://img.shields.io/github/stars/TryParashell/Parashell"/></a>
</p>

<p align="center">
  <a href="https://www.parashell.cloud">Website</a> -
  <a href="https://www.parashell.cloud/open-source">Open Source</a> -
  <a href="https://github.com/TryParashell/Parashell/issues/new?labels=bug">Bug reports</a> -
  <a href="https://github.com/TryParashell/Parashell/issues/new?labels=enhancement">Feature requests</a>
</p>

## Parashell is an AI CAD platform for creating and editing large assemblies with parametric history

Parashell brings AI-assisted modeling to professional CAD. It is designed to generate, edit, and reason about large parametric assemblies, keeping a full feature history so every change stays editable and traceable, just like a traditional CAD workflow.

- **AI-driven modeling** — describe what you want and let Parashell build and modify geometry for you.
- **Parametric history** — every operation is recorded as an editable step, so assemblies remain fully reconfigurable.
- **Large assemblies** — built to handle complex, multi-part designs rather than single bodies.
- **Open source core** — the modeling foundation is fully open and free to build on.

## Table of Contents

- [About Parashell](#parashell-is-an-ai-cad-platform-for-creating-and-editing-large-assemblies-with-parametric-history)
- [Built on open source](#built-on-open-source)
- [Project status](#project-status)
- [Contributing](#contributing)
- [License](#license)

## Built on open source

Parashell stands on top of established open source engineering software:

- **CAD core** — the core components are forked from [FreeCAD](https://github.com/FreeCAD/FreeCAD) and remain fully open source.
- **Geometry kernel** — geometry is powered by [OpenCASCADE (OCCT)](https://github.com/Open-Cascade-SAS/OCCT).

## Project status

Parashell is in an early development stage and under active construction. Interfaces, file formats, and APIs may change without notice. Expect rough edges, and avoid relying on it for production work just yet.

## Contributing

Contributions of every size are welcome:

- Open a [pull request](http://makeapullrequest.com) with a fix or improvement.
- File a [bug report](https://github.com/TryParashell/Parashell/issues/new?labels=bug) when something breaks.
- Submit a [feature request](https://github.com/TryParashell/Parashell/issues/new?labels=enhancement) to shape the roadmap.

Learn more about the project and how to get involved at [parashell.cloud/open-source](https://www.parashell.cloud/open-source).

## License

Parashell's open source core is derived from FreeCAD, which is licensed under the [LGPL](https://github.com/FreeCAD/FreeCAD/blob/main/LICENSE). See the repository's license files for the terms that apply to each component.

### Proprietary features

Parashell's proprietary features are compiled to [Cython](https://cython.org/) in our own repositories and dropped into the `Mod` directory as built modules. These are our own tools and are **not** covered under the LGPL license that applies to the open source core. They remain the proprietary property of Parashell and are distributed under separate, proprietary terms — see [PS_LICENSE](PS_LICENSE).
