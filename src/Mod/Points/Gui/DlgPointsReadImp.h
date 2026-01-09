// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2002 Jürgen Riegel <juergen.riegel@web.de>                             *
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
 

#ifndef POINTSGUI_DLGREADPOINTS_H
#define POINTSGUI_DLGREADPOINTS_H

#include <FCGlobal.h>

#include <QDialog>
#include <memory>


namespace PointsGui
{
class Ui_DlgPointsRead;

/** The points read dialog
 */
class DlgPointsReadImp: public QDialog
{
    Q_OBJECT

public:
    explicit DlgPointsReadImp(
        const char* FileName,
        QWidget* parent = nullptr,
        Qt::WindowFlags fl = Qt::WindowFlags()
    );
    ~DlgPointsReadImp() override;

private:
    std::unique_ptr<Ui_DlgPointsRead> ui;
    std::string _FileName;

    Q_DISABLE_COPY_MOVE(DlgPointsReadImp)
};

}  // namespace PointsGui

#endif  // POINTSGUI_DLGREADPOINTS_H
