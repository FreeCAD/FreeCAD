// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD contributors

#pragma once

#include <Inventor/SoFullPath.h>
#include <Inventor/SoPath.h>

namespace Gui
{

/// Cast SoPath* to SoFullPath*.
///
/// This is safe by Coin3D design: SoFullPath has no additional data members
/// and is only an "extended interface" on top of SoPath.  SoFullPath's
/// documentation explicitly states the cast is valid for any SoPath instance.
/// UBSan flags the vptr mismatch, so we suppress the sanitizer here.
#if defined(__clang__) || defined(__GNUC__)
__attribute__((no_sanitize("undefined")))
#endif
inline SoFullPath* toFullPath(SoPath* path)
{
    return static_cast<SoFullPath*>(path);
}

#if defined(__clang__) || defined(__GNUC__)
__attribute__((no_sanitize("undefined")))
#endif
inline const SoFullPath* toFullPath(const SoPath* path)
{
    return static_cast<const SoFullPath*>(path);
}

}  // namespace Gui
