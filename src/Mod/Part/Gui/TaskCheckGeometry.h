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
#include <TopTools_MapOfShape.hxx>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <QAbstractItemModel>

class SoSeparator;
class SoSwitch;

namespace PartGui {

class ResultEntry
{
public:
    ResultEntry();
    ~ResultEntry();

    TopoDS_Shape shape;//invisible
    QString name;
    QString type;
    QString error;
    Gui::ViewProvider *viewProvider;
    SoSeparator *boxSep;
    SoSwitch *boxSwitch;
    ResultEntry *parent;
    QList<ResultEntry *> children;
    QStringList selectionStrings;
};

class SetupResultBase
{
protected:
    SetupResultBase(){}
public:
    virtual void go(ResultEntry *entry) = 0;
protected:
    QString selectionName(ResultEntry *entry, const TopoDS_Shape &shape);
    void addTypedSelection(ResultEntry *entry, const TopoDS_Shape &shape, TopAbs_ShapeEnum type);
};

class SetupResultBoundingBox : public SetupResultBase
{
private:
    SetupResultBoundingBox(){}
public:
    virtual void go(ResultEntry *entry);
    friend SetupResultBoundingBox& getSetupResultBoundingBoxObject();
};
SetupResultBoundingBox& getSetupResultBoundingBoxObject();

class SetupResultShellNotClosed : public SetupResultBase
{
private:
    SetupResultShellNotClosed(){}
public:
    virtual void go(ResultEntry *entry);
    friend SetupResultShellNotClosed& getSetupResultShellNotClosedObject();
};
SetupResultShellNotClosed& getSetupResultShellNotClosedObject();

class SetupResultWireNotClosed : public SetupResultBase
{
private:
    SetupResultWireNotClosed(){}
public:
    virtual void go(ResultEntry *entry);
    friend SetupResultWireNotClosed& getSetupResultWireNotClosedObject();
};
SetupResultWireNotClosed& getSetupResultWireNotClosedObject();

class SetupResultInvalidPointCurve : public SetupResultBase
{
private:
    SetupResultInvalidPointCurve(){}
public:
    virtual void go(ResultEntry *entry);
    friend SetupResultInvalidPointCurve& getSetupResultInvalidPointCurveObject();
};
SetupResultInvalidPointCurve& getSetupResultInvalidPointCurveObject();

class SetupResultIntersectingWires : public SetupResultBase
{
private:
    SetupResultIntersectingWires(){}
public:
    virtual void go(ResultEntry *entry);
    friend SetupResultIntersectingWires& getSetupResultIntersectingWiresObject();
};
SetupResultIntersectingWires& getSetupResultIntersectingWiresObject();

class SetupResultInvalidCurveSurface : public SetupResultBase
{
private:
    SetupResultInvalidCurveSurface(){}
public:
    virtual void go(ResultEntry *entry);
    friend SetupResultInvalidCurveSurface& getSetupResultInvalidCurveSurfaceObject();
};
SetupResultInvalidCurveSurface& getSetupResultInvalidCurveSurfaceObject();

class SetupResultInvalidSameParameterFlag : public SetupResultBase
{
private:
    SetupResultInvalidSameParameterFlag(){}
public:
    virtual void go(ResultEntry *entry);
    friend SetupResultInvalidSameParameterFlag& getSetupResultInvalidSameParameterFlagObject();
};
SetupResultInvalidSameParameterFlag& getSetupResultInvalidSameParameterFlagObject();

class SetupResultUnorientableShapeFace : public SetupResultBase
{
private:
    SetupResultUnorientableShapeFace(){}
public:
    virtual void go(ResultEntry *entry);
    friend SetupResultUnorientableShapeFace& getSetupResultUnorientableShapeFaceObject();
};
SetupResultUnorientableShapeFace& getSetupResultUnorientableShapeFaceObject();

typedef boost::tuple<TopAbs_ShapeEnum, BRepCheck_Status, SetupResultBase*> FunctionMapType;

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

private slots:
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
    ResultModel *model;
    QTreeView *treeView;
    QLabel *message;
    TopTools_MapOfShape checkedMap;
    Gui::ViewProvider *currentProvider;
    std::vector<FunctionMapType> functionMap;
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

private:
    TaskCheckGeometryResults* widget;
    Gui::TaskView::TaskBox* taskbox;
};

}

#endif // TASKCHECKGEOMETRY_H
