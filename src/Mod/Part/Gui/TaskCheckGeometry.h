/***************************************************************************
 *   Copyright (c) 2012 Thomas Anderson <blobfish[at]gmx.com>              *
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

#ifndef TASKCHECKGEOMETRY_H
#define TASKCHECKGEOMETRY_H

#include <functional>
#include <tuple>
#include <QAbstractItemModel>
#include <QElapsedTimer>
#include <QProgressDialog>
#include <BRepCheck_Analyzer.hxx>
#include <BRepCheck_Status.hxx>
#include <Message_ProgressIndicator.hxx>
#include <Standard_Version.hxx>
#include <TopTools_MapOfShape.hxx>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>


class SoSeparator;
class SoSwitch;
class QCheckBox;
class QTextEdit;
class QTreeView;

namespace PartGui {

class ResultEntry
{
public:
    ResultEntry();
    ~ResultEntry();
    void buildEntryName();

    TopoDS_Shape shape;//invisible
    QString name;
    QString type;
    QString error;
    SoSeparator *viewProviderRoot;
    SoSeparator *boxSep;
    SoSwitch *boxSwitch;
    ResultEntry *parent;
    QList<ResultEntry *> children;
    QStringList selectionStrings;
};

QString buildSelectionName(const ResultEntry *entry, const TopoDS_Shape &shape);
void goSetupResultTypedSelection(ResultEntry *entry, const TopoDS_Shape &shape, TopAbs_ShapeEnum type);
void goSetupResultBoundingBox(ResultEntry *entry);
void goSetupResultShellNotClosed(ResultEntry *entry);
void goSetupResultWireNotClosed(ResultEntry *entry);
void goSetupResultInvalidPointCurve(ResultEntry *entry);
void goSetupResultIntersectingWires(ResultEntry *entry);
void goSetupResultInvalidCurveSurface(ResultEntry *entry);
void goSetupResultInvalidSameParameterFlag(ResultEntry *entry);
void goSetupResultUnorientableShapeFace(ResultEntry *entry);

using ResultFunction = std::function<void (ResultEntry *entry)>;
using FunctionMapType = std::tuple<TopAbs_ShapeEnum, BRepCheck_Status, ResultFunction>;

class ResultModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ResultModel(QObject *parent = nullptr);
    ~ResultModel() override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
//    virtual Qt::ItemFlags flags (const QModelIndex &index) const;

    void setResults(ResultEntry *resultsIn);
    ResultEntry* getEntry(const QModelIndex &index);
private:
    ResultEntry* nodeFromIndex(const QModelIndex &index) const;
    ResultEntry *root;
};

class TaskCheckGeometryResults : public QWidget
{
    Q_OBJECT
public:
    explicit TaskCheckGeometryResults(QWidget *parent = nullptr);
    ~TaskCheckGeometryResults() override;
    QString getShapeContentString();
    void goCheck();

private Q_SLOTS:
    void currentRowChanged (const QModelIndex &current, const QModelIndex &previous);


private:
    void setupInterface();
    void recursiveCheck(const BRepCheck_Analyzer &shapeCheck, const TopoDS_Shape &shape,
                        ResultEntry *parent);
    void checkSub(const BRepCheck_Analyzer &shapeCheck, const TopoDS_Shape &shape,
                  const TopAbs_ShapeEnum subType, ResultEntry *parent);
    void dispatchError(ResultEntry *entry, const BRepCheck_Status &stat);
    bool split(QString &input, QString &doc, QString &object, QString &sub);
    void setupFunctionMap();
#if OCC_VERSION_HEX < 0x070500
    int goBOPSingleCheck(const TopoDS_Shape &shapeIn, ResultEntry *theRoot, const QString &baseName,
                         const Handle(Message_ProgressIndicator)& theProgress);
#else
    int goBOPSingleCheck(const TopoDS_Shape &shapeIn, ResultEntry *theRoot, const QString &baseName,
                         const Message_ProgressScope& theScope);
#endif
    void buildShapeContent(App::DocumentObject *pObject, const QString &baseName, const TopoDS_Shape &shape);
    ResultModel *model;
    QTreeView *treeView;
    QLabel *message;
    TopTools_MapOfShape checkedMap;
    SoSeparator *currentSeparator;
    std::vector<FunctionMapType> functionMap;
    std::string shapeContentString;

};

class TaskCheckGeometryDialog : public Gui::TaskView::TaskDialog
{
    Q_OBJECT
public:
    TaskCheckGeometryDialog();
    ~TaskCheckGeometryDialog() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
        {return QDialogButtonBox::Ok | QDialogButtonBox::Close;}
    bool isAllowedAlterDocument() const override
        {return false;}
    bool needsFullSpace() const override {return true;}

private:
    void onRunBOPCheckBoxToggled(bool isOn);
    void onRunSingleThreadedCheckBoxToggled(bool isOn);
    void onLogErrorsCheckBoxToggled(bool isOn);
    void onExpandShapeContentCheckBoxToggled(bool isOn);
    void onAdvancedShapeContentCheckBoxToggled(bool isOn);
    void onAutoRunCheckBoxToggled(bool isOn);
    void onArgumentTypeModeCheckBoxToggled(bool isOn);
    void onSelfInterModeCheckBoxToggled(bool isOn);
    void onSmallEdgeModeCheckBoxToggled(bool isOn);
    void onRebuildFaceModeCheckBoxToggled(bool isOn);
    void onContinuityModeCheckBoxToggled(bool isOn);
    void onTangentModeCheckBoxToggled(bool isOn);
    void onMergeVertexModeCheckBoxToggled(bool isOn);
    void onMergeEdgeModeCheckBoxToggled(bool isOn);
    void onCurveOnSurfaceModeCheckBoxToggled(bool isOn);
    void onClicked(QAbstractButton* btn);

private:
    TaskCheckGeometryResults* widget;
    Gui::TaskView::TaskBox* taskbox;
    Gui::TaskView::TaskBox* shapeContentBox;
    Gui::TaskView::TaskBox* settingsBox;
    QTextEdit *contentLabel;
    QCheckBox *autoRunCheckBox;
    QCheckBox *runBOPCheckBox;
    QCheckBox *runSingleThreadedCheckBox;
    QCheckBox *logErrorsCheckBox;
    QCheckBox *expandShapeContentCheckBox;
    QCheckBox *advancedShapeContentCheckBox;
    QCheckBox *argumentTypeModeCheckBox;
    QCheckBox *selfInterModeCheckBox;
    QCheckBox *smallEdgeModeCheckBox;
    QCheckBox *rebuildFaceModeCheckBox;
    QCheckBox *continuityModeCheckBox;
    QCheckBox *tangentModeCheckBox;
    QCheckBox *mergeVertexModeCheckBox;
    QCheckBox *mergeEdgeModeCheckBox;
    QCheckBox *curveOnSurfaceModeCheckBox;
    bool accept() override;
    bool reject() override;
    void modifyStandardButtons(QDialogButtonBox*) override;
    QPushButton *okBtn;
    QPushButton *settingsBtn;
    QPushButton *resultsBtn;
};

class BOPProgressIndicator : public Message_ProgressIndicator
{
public:
    BOPProgressIndicator (const QString &title, QWidget* parent);
    ~BOPProgressIndicator () override;

#if OCC_VERSION_HEX < 0x070500
    Standard_Boolean Show (const Standard_Boolean theForce = Standard_True) override;
#else
    void Show (const Message_ProgressScope& theScope,
                       const Standard_Boolean isForce) override;
    void Reset() override;
#endif
    Standard_Boolean UserBreak() override;

private:
    int steps;
    bool canceled;
    QElapsedTimer time;
    QProgressDialog* myProgress;
};
}

#endif // TASKCHECKGEOMETRY_H
