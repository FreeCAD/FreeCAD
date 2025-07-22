/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/
#ifndef TECHDRAWGUI_SYMBOLCHOOSER_H
#define TECHDRAWGUI_SYMBOLCHOOSER_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QDialog>

class QListWidgetItem;

namespace TechDrawGui {

class Ui_SymbolChooser;
class TechDrawGuiExport SymbolChooser : public QDialog
{
    Q_OBJECT

public:
    SymbolChooser(QWidget *parent = nullptr,
                  QString startDir = QString(),
                  QString source = QString());
    ~SymbolChooser() override;

public Q_SLOTS:
    void onOKClicked();
    void onCancelClicked();
    void onItemClicked(QListWidgetItem* item);
    void onDirectoryChanged(const QString& newDir);

Q_SIGNALS:
    void symbolSelected(QString symbolPath,
                        QString source);

protected:
    void setUiPrimary(void);
    void loadSymbolNames(QString pathToSymbols);

private:
    std::unique_ptr<Ui_SymbolChooser> ui;
    QString m_symbolDir;
    QString m_symbolPath;
    QString m_source;
};

}
#endif // #ifndef TECHDRAWGUI_SYMBOLCHOOSER_H

