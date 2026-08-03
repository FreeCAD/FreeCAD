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

#pragma once

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


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
    void init() {
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
    TaskWeldingSymbol(TechDraw::DrawLeaderLine* leadFeat);
    TaskWeldingSymbol(TechDraw::DrawWeldSymbol* weldFeat);
    ~TaskWeldingSymbol() override;

    virtual bool accept();
    virtual bool reject();
    void updateTask();
    void saveButtons(QPushButton* btnOK,
                     QPushButton* btnCancel);
    void enableTaskButtons(bool enable);

public Q_SLOTS:
    void symbolDialog(const char* source);
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

protected:
    void changeEvent(QEvent *event) override;
    void setUiPrimary();
    void setUiEdit();

    TechDraw::DrawWeldSymbol* createWeldingSymbol();
    void updateWeldingSymbol();

    void getTileFeats();
    void updateTiles();

    void collectArrowData();
    void collectOtherData();

    std::string prefSymbolDir();

    QString m_currDir;

private:
    std::unique_ptr<Ui_TaskWeldingSymbol> ui;

    TechDraw::DrawLeaderLine* m_leadFeat;
    TechDraw::DrawWeldSymbol* m_weldFeat;
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
    explicit TaskDlgWeldingSymbol(TechDraw::DrawLeaderLine* leader);
    explicit TaskDlgWeldingSymbol(TechDraw::DrawWeldSymbol* weld);
    ~TaskDlgWeldingSymbol() override;

    /// is called the TaskView when the dialog is opened
    void open() override;
    /// is called by the framework if an button is clicked which has no accept or reject role
    void clicked(int) override;
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    /// is called by the framework if the user presses the help button
    bool isAllowedAlterDocument() const override
                        { return false; }
    void update();

    void modifyStandardButtons(QDialogButtonBox* box) override;

private:
    TaskWeldingSymbol* widget;
    Gui::TaskView::TaskBox* taskbox;

};

} //namespace TechDrawGui