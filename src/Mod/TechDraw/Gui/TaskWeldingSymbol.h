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

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

class QPushButton;
class Ui_TaskWeldingSymbol;

namespace App {
class DocumentObject;
}

namespace TechDraw
{
class DrawPage;
class DrawView;
class DrawLeaderLine;
class DrawWeldSymbol;
class DrawTileWeld;
class DrawTile;
}

namespace TechDraw
{
class Face;
}

namespace TechDrawGui
{
class QGSPage;
class QGVPage;
class QGIView;
class QGILeaderLine;
class QGIWeldSymbol;
class MDIViewPage;
//class ViewProviderWeld;

class TileImage
{
public:
    TileImage() { init(); }
    ~TileImage() = default;
    bool toBeSaved;
    bool arrowSide;
    int row;
    int col;
    std::string leftText;
    std::string centerText;
    std::string rightText;
    std::string symbolPath;
    std::string symbolString;
    std::string tileName;
    void init(void) {
        toBeSaved = false;
        arrowSide = true;
        row = 0;
        col = 0;
        leftText.clear();
        centerText.clear();
        rightText.clear();
        symbolPath.clear();
        symbolString.clear();
        tileName.clear();
    }

};

class TechDrawGuiExport TaskWeldingSymbol : public QWidget
{
    Q_OBJECT

public:
    TaskWeldingSymbol(TechDraw::DrawLeaderLine* baseFeat);
    TaskWeldingSymbol(TechDraw::DrawWeldSymbol* weldFeat);
    ~TaskWeldingSymbol();

public Q_SLOTS:
    void onArrowSymbolCreateClicked();
    void onArrowSymbolClicked();
    void onOtherSymbolCreateClicked();
    void onOtherSymbolClicked();
    void onOtherEraseCreateClicked();
    void onOtherEraseClicked();
    void onFlipSidesCreateClicked();
    void onFlipSidesClicked();
    void onArrowTextChanged();
    void onOtherTextChanged();
    void onWeldingChanged();
    void onDirectorySelected(const QString& newDir);
    void onSymbolSelected(QString symbolPath, QString source);

public:
    virtual bool accept();
    virtual bool reject();
    void updateTask();
    void saveButtons(QPushButton* btnOK,
                     QPushButton* btnCancel);
    void enableTaskButtons(bool b);

protected Q_SLOTS:

protected:
    void changeEvent(QEvent *e);
    void setUiPrimary(void);
    void setUiEdit();

    TechDraw::DrawWeldSymbol* createWeldingSymbol(void);
    void updateWeldingSymbol(void);

/*    std::vector<App::DocumentObject*> createTiles(void);*/
    void getTileFeats(void);
    void updateTiles(void);

    void collectArrowData(void);
    void collectOtherData(void);

    std::string prefSymbolDir();

    QString m_currDir;

private:
    std::unique_ptr<Ui_TaskWeldingSymbol> ui;

    TechDraw::DrawLeaderLine* m_leadFeat;
    TechDraw::DrawWeldSymbol* m_weldFeat;
/*    TechDraw::DrawTileWeld*   m_arrowIn;    //save starting values*/
/*    TechDraw::DrawTileWeld*   m_otherIn;*/
    TechDraw::DrawTileWeld*   m_arrowFeat;
    TechDraw::DrawTileWeld*   m_otherFeat;

    TileImage m_arrowOut;
    TileImage m_otherOut;

    QString m_arrowPath;
    QString m_otherPath;
    QString m_arrowSymbol;
    QString m_otherSymbol;

    QPushButton* m_btnOK;
    QPushButton* m_btnCancel;

    bool m_createMode;
    bool m_otherDirty;
};


class TaskDlgWeldingSymbol : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgWeldingSymbol(TechDraw::DrawLeaderLine* leader);
    TaskDlgWeldingSymbol(TechDraw::DrawWeldSymbol* weld);
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
