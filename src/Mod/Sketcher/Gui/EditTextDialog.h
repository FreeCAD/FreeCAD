// SPDX - License - Identifier: LGPL - 2.1 - or -later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pierre-Louis Boyer                                  *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#ifndef SKETCHERGUI_EDITTEXTDIALOG_H
#define SKETCHERGUI_EDITTEXTDIALOG_H

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QDialog>
# include <QMap>
# include <QString>
#endif

namespace Ui
{
class EditTextDialog;
}

namespace SketcherGui
{

class ViewProviderSketch;

class EditTextDialog: public QDialog
{
    Q_OBJECT

public:
    explicit EditTextDialog(
        ViewProviderSketch* viewProvider,
        int constraintIndex,
        QWidget* parent = nullptr
    );
    ~EditTextDialog() override;

private Q_SLOTS:
    void on_buttonBox_accepted();

private:
    Ui::EditTextDialog* ui;
    ViewProviderSketch* sketchView;
    int constrIndex;
    QMap<QString, QString> fontPathMap;

    void populateFontList();
    QString findFontNameFromPath(const QString& path) const;
};

}  // namespace SketcherGui

#endif  // SKETCHERGUI_EDITTEXTDIALOG_H
