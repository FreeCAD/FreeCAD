/***************************************************************************
 *   Copyright (c) 2020 Uwe Stöhr                                          *
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

#ifndef PARTGUI_CUTTING_H
#define PARTGUI_CUTTING_H

#include <QDialog>
#include <App/DocumentObject.h>

namespace Gui { class View3DInventor; }

namespace PartGui {

class Ui_Cutting;

/**
 * @author Uwe Stöhr
 */
class Cutting : public QDialog
{
    Q_OBJECT

public:
    static Cutting* makeDockWidget(Gui::View3DInventor*);
    Cutting(Gui::View3DInventor* view, QWidget* parent = nullptr);
    ~Cutting();

protected Q_SLOTS:
    void startCutting();
    void onGroupBoxXtoggled(bool);
    void onGroupBoxYtoggled(bool);
    void onGroupBoxZtoggled(bool);
    void onCutXvalueChanged(double);
    void onCutYvalueChanged(double);
    void onCutZvalueChanged(double);
    void onCutXHSsliderMoved(int);
    void onCutYHSsliderMoved(int);
    void onCutZHSsliderMoved(int);
    void onFlipXclicked();
    void onFlipYclicked();
    void onFlipZclicked();
    void onRefreshCutPBclicked();
    void onGroupBoxCustomtoggled(bool);
    void onCutCustomvalueChanged(double);
    void onDirXvalueChanged(double);
    void onDirYvalueChanged(double);
    void onDirZvalueChanged(double);

public:
    void reject();

private:
    Ui_Cutting* ui;
    std::vector<App::DocumentObject*> ObjectsListVisible;
    App::Document* doc; //pointer to active document
    bool hasBoxX = false;
    bool hasBoxY = false;
    bool hasBoxZ = false;
    bool hasBoxCustom = false;
    void noDocumentActions();
    SbBox3f getViewBoundingBox();
    void refreshCutRanges(SbBox3f, bool forXValue = true, bool forYValue = true, bool forZValue = true,
        bool forXRange = true, bool forYRange = true, bool forZRange = true, bool forCustom = true);
    const char* CompoundName = "CutViewCompound";
    const char* BoxXName = "CutViewBoxX";
    const char* BoxYName = "CutViewBoxY";
    const char* BoxZName = "CutViewBoxZ";
    const char* CutXName = "CutViewCutX";
    const char* CutYName = "CutViewCutY";
    const char* CutZName = "CutViewCutZ";
};

} // namespace PartGui

#endif // PARTGUI_CUTTING_H
