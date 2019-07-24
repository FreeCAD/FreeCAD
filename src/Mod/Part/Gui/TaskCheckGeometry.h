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

#include <boost/tuple/tuple.hpp>
#include <BRepCheck_Analyzer.hxx>
#include <BRepCheck_Status.hxx>
#include <Message_ProgressIndicator.hxx>
#include <TopTools_MapOfShape.hxx>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <QAbstractItemModel>
#include <QProgressDialog>
#include <QTime>

class SoSeparator;
class SoSwitch;
class QCheckBox;
class QTextEdit;

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

typedef boost::function<void (ResultEntry *entry)> ResultFunction;
typedef boost::tuple<TopAbs_ShapeEnum, BRepCheck_Status, ResultFunction> FunctionMapType;

class ResultModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    ResultModel(QObject *parent = 0);
    ~ResultModel();
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent) const;
    virtual int columnCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
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
    TaskCheckGeometryResults(QWidget *parent = 0);
    ~TaskCheckGeometryResults();
    QString getShapeContentString();

private Q_SLOTS:
    void currentRowChanged (const QModelIndex &current, const QModelIndex &previous);


private:
    void setupInterface();
    void goCheck();
    void recursiveCheck(const BRepCheck_Analyzer &shapeCheck, const TopoDS_Shape &shape,
                        ResultEntry *parent);
    void checkSub(const BRepCheck_Analyzer &shapeCheck, const TopoDS_Shape &shape,
                  const TopAbs_ShapeEnum subType, ResultEntry *parent);
    void dispatchError(ResultEntry *entry, const BRepCheck_Status &stat);
    bool split(QString &input, QString &doc, QString &object, QString &sub);
    void setupFunctionMap();
    int goBOPSingleCheck(const TopoDS_Shape &shapeIn, ResultEntry *theRoot, const QString &baseName,
                         const Handle(Message_ProgressIndicator)& theProgress);
    void buildShapeContent(const QString &baseName, const TopoDS_Shape &shape);
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
    ~TaskCheckGeometryDialog();

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
        {return QDialogButtonBox::Close;}
    virtual bool isAllowedAlterDocument(void) const
        {return false;}
    virtual bool needsFullSpace() const {return true;}

private Q_SLOTS:
    void on_runBOPCheckBox_toggled(bool isOn);

private:
    TaskCheckGeometryResults* widget;
    Gui::TaskView::TaskBox* taskbox;
    Gui::TaskView::TaskBox* shapeContentBox;
    Gui::TaskView::TaskBox* settingsBox;
    QTextEdit *contentLabel;
    QCheckBox *runBOPCheckBox;
};

class BOPProgressIndicator : public Message_ProgressIndicator
{
public:
    BOPProgressIndicator (const QString &title, QWidget* parent);
    virtual ~BOPProgressIndicator ();

    virtual Standard_Boolean Show (const Standard_Boolean theForce = Standard_True);
    virtual Standard_Boolean UserBreak();

private:
    int steps;
    bool canceled;
    QTime time;
    QProgressDialog* myProgress;
};

}

#endif // TASKCHECKGEOMETRY_H
