# Security Policy

The FreeCAD project is a FOSS (Free and Open-Source Software) project that has a community of thousands of users and
hundreds of developers worldwide. We encourage responsible reporting of security vulnerabilities that may affect users
of this software, and will endeavor to address these vulnerabilities when they are discovered.

## Bounties

FreeCAD does not have a program to pay bounties for security bugs. If you discover a vulnerability that affects a part
of the FreeCAD project (either directly in FreeCAD, in a library it depends on, or in any of the various other
subprojects such as our website, forums, etc.) we ask you to join the large community of volunteer contributors and
file a report about the issue.

Note that funds may be available from the [FreeCAD Project Association (FPA)](https://fpa.freecad.org) to pursue
security research and/or the development of fixes to any vulnerabilities discovered. However, vulnerabilities held as
hostage in demands for "bounties" will not be entertained. Contact the FPA at fpa@freecad.org for more information.

## Supported Versions

FreeCAD implements security fixes to the current release series, and to the current development on the main branch.

| Version | Supported          |
|---------| ------------------ |
| 26.3dev  | :white_check_mark: |
| 1.1     | :white_check_mark: |
| < 1.1   | :x:                |

## Reporting a Vulnerability

To report a vulnerability use GitHub's security reporting tool:
https://github.com/FreeCAD/FreeCAD/security/advisories/new

Please note that if your report concerns a crash in FreeCAD without a demonstrated exploit, we ask that you report it
as a normal [Issue](https://github.com/FreeCAD/FreeCAD/issues/new/choose) instead. We do not request CVEs for crashes
that don't lead to a real security impact, such as arbitrary code execution, disclosure of personal information, or
unauthorized access to files or network resources. Security reports are deliberately visible to only a small number of
maintainers, so a crash filed here reaches far fewer developers, and will generally be fixed more slowly, than one filed
as a public Issue.

