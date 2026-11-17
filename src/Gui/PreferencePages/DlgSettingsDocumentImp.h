// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2002 Jürgen Riegel
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#pragma once

#include <Gui/PropertyPage.h>
#include <memory>

namespace Gui
{
namespace Dialog
{
class Ui_DlgSettingsDocument;

/**
 * The DlgSettingsDocumentImp class implements a preference page to change settings
 * for the document.
 * \author Jürgen Riegel
 */
class DlgSettingsDocumentImp: public PreferencePage
{
    Q_OBJECT

public:
    explicit DlgSettingsDocumentImp(QWidget* parent = nullptr);
    ~DlgSettingsDocumentImp() override;

    void saveSettings() override;
    void loadSettings() override;

protected Q_SLOTS:
    void onLicenseTypeChanged(int index);
    void onDateFormatChanged(const QString& text);

protected:
    void changeEvent(QEvent* e) override;
    void addLicenseTypes();

private:
    std::unique_ptr<Ui_DlgSettingsDocument> ui;
};

}  // namespace Dialog
}  // namespace Gui
