# Security Policy

Parashell is a fork of [FreeCAD](https://github.com/FreeCAD/FreeCAD), maintained by [Parashell](https://www.parashell.cloud) — an organisation building AI-backed CAD.

We take the security of Parashell and its users seriously. This document explains how to report a vulnerability and what to expect once you do.

## Reporting a Vulnerability

**Do not** open a public GitHub issue, pull request, or forum post for security vulnerabilities. Public disclosure before a fix is available puts every user at risk.

Instead, report all security issues privately by email to:

**ops@parashell.cloud**

If the vulnerability also affects upstream FreeCAD (for example, it lives in code inherited from the fork rather than in Parashell-specific changes), please **also report it to the FreeCAD developers** so the upstream project can coordinate a fix. See the [FreeCAD security policy](https://github.com/FreeCAD/FreeCAD/security/policy) for their preferred reporting channel.

## What to Include

To help us triage and resolve the issue quickly, please include as much of the following as you can:

- A clear description of the vulnerability and its potential impact.
- The affected version, commit, or build of Parashell.
- Step-by-step instructions to reproduce the issue.
- Any proof-of-concept code, screenshots, or logs.
- Whether you believe the issue originates in Parashell-specific code or in inherited FreeCAD code.
- How you would like to be credited if a fix is published.

## Our Commitment

When you report a vulnerability to ops@parashell.cloud, we will:

1. Acknowledge receipt of your report within a reasonable timeframe.
2. Investigate and validate the issue, keeping you informed of our progress.
3. Work on a fix and coordinate a release, reporting upstream to FreeCAD where applicable.
4. Maintain confidentiality regarding your identity unless you ask to be credited.

## Coordinated Disclosure

We ask that you give us a reasonable opportunity to investigate and release a fix before any public disclosure. We will work with you in good faith to agree on a disclosure timeline, and we appreciate the effort that goes into responsible reporting.

## Scope

This policy covers the Parashell application and the code in this repository. Vulnerabilities in third-party dependencies should generally be reported to their respective maintainers; if you are unsure where an issue belongs, email ops@parashell.cloud and we will help route it.
