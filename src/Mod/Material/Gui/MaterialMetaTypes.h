// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Ladislav Michl
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

#include <memory>

#include <QMetaType>

#include <Gui/MetaTypes.h>

#include <Mod/Material/App/MaterialFilter.h>
#include <Mod/Material/App/MaterialLibrary.h>
#include <Mod/Material/App/MaterialValue.h>
#include <Mod/Material/App/Materials.h>
#include <Mod/Material/App/ModelLibrary.h>

// The material objects the Qt models and views carry in their item data

// NOLINTBEGIN
Q_DECLARE_METATYPE(Materials::MaterialFilter)
Q_DECLARE_METATYPE(Materials::Material*)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::Material>)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::MaterialLibrary>)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::MaterialLibraryLocal>)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::ModelLibrary>)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::ModelLibraryLocal>)
Q_DECLARE_METATYPE(Materials::MaterialValue)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::Array2D>)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::Array3D>)
// NOLINTEND
