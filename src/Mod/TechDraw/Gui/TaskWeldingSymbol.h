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

#ifndef TECHDRAWGUI_TASKWELDINGSYMBOL_H
#define TECHDRAWGUI_TASKWELDINGSYMBOL_H

#include <QPushButton>

#include <App/DocumentObject.h>
#include <Base/Vector3D.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

#include <Mod/TechDraw/Gui/ui_TaskWeldingSymbol.h>


class Ui_TaskWeldingSymbol;
class Ui_TaskCL2Lines;

namespace App {
class DocumentObject;
}

namespace TechDraw
{
class DrawPage;
class DrawView;
class DrawLeaderLine;
class DrawWeldSymbol;
}

namespace TechDraw
{
class Face;
}

namespace TechDrawGui
{
class QGVPage;
class QGIView;
class QGILeaderLine;
class QGIWeldSymbol;
class MDIViewPage;
//class ViewProviderWeld;

class Tile2Add
{
public:
    Tile2Add() {};
    ~Tile2Add() = default;
    bool arrowSide;  // or is row enough?
    int row;
    int col;
    std::string leftText;
    std::string centerText;
    std::string rightText;
    std::string symbolPath;
};

class TaskWeldingSymbol : public QWidget
{
    Q_OBJECT

public:
    TaskWeldingSymbol(TechDraw::DrawLeaderLine* baseFeat);
    ~TaskWeldingSymbol();

public Q_SLOTS:
    void onArrow0Clicked(bool b);
    void onArrow1Clicked(bool b);
    void onOther0Clicked(bool b);
    void onOther1Clicked(bool b);
    void onDirectorySelected(const QString& newDir);
    
public:
    virtual bool accept();
    virtual bool reject();
    void updateTask();
    void saveButtons(QPushButton* btnOK,
                     QPushButton* btnCancel);
    void enableTaskButtons(bool b);
    void setFlipped(bool b);

protected Q_SLOTS:

protected:
    void changeEvent(QEvent *e);

    void blockButtons(bool b);
    void setUiPrimary(void);
    void setUiEdit();

    void turnOnArrow();
    void turnOnOther();
    void removePendingTile(int row, int col);


    App::DocumentObject* createWeldingSymbol(void);
    void updateWeldingSymbol(void);
    std::vector<App::DocumentObject*> createTiles(void);

    void loadSymbolNames(QString pathToSymbols);

    std::string prefSymbolDir();
    QString m_currDir;


private:
    Ui_TaskWeldingSymbol * ui;

    TechDraw::DrawLeaderLine* m_leadFeat;
    TechDraw::DrawWeldSymbol* m_weldFeat;

    std::vector<Tile2Add> m_tiles2Add;

    QPushButton* m_btnOK;
    QPushButton* m_btnCancel;

    int m_arrowCount;
    int m_otherCount;
};


class TaskDlgWeldingSymbol : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgWeldingSymbol(TechDraw::DrawLeaderLine* leader);
    ~TaskDlgWeldingSymbol();

public:
    /// is called the TaskView when the dialog is opened
    virtual void open();
    /// is called by the framework if an button is clicked which has no accept or reject role
    virtual void clicked(int);
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user presses the help button
    virtual void helpRequested() { return;}
    virtual bool isAllowedAlterDocument(void) const
                        { return false; }
    void update();

    void modifyStandardButtons(QDialogButtonBox* box);

protected:

private:
    TaskWeldingSymbol* widget;
    Gui::TaskView::TaskBox* taskbox;

};

} //namespace TechDrawGui

#endif // #ifndef TECHDRAWGUI_TASKWELDINGSYMBOL_H
