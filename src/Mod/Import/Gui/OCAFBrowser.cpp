// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPointer>
#include <QStyle>
#include <QTextStream>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <TDataStd.hxx>
#include <TDataStd_Integer.hxx>
#include <TDataStd_Name.hxx>
#include <TDataStd_TreeNode.hxx>
#include <TDF_AttributeIterator.hxx>
#include <TDF_ChildIterator.hxx>
#include <TDF_ChildIDIterator.hxx>
#include <TDF_IDList.hxx>
#include <TDF_Label.hxx>
#include <TDF_ListIteratorOfIDList.hxx>
#include <TDF_TagSource.hxx>
#include <TDocStd_Document.hxx>
#include <TDocStd_Owner.hxx>
#include <TNaming_NamedShape.hxx>
#include <TNaming_UsedShapes.hxx>
#include <XCAFDoc_Color.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_LayerTool.hxx>
#include <XCAFDoc_Location.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ShapeMapTool.hxx>
#endif

#include "OCAFBrowser.h"
#include <Gui/MainWindow.h>

using namespace ImportGui;

OCAFBrowser::OCAFBrowser(Handle(TDocStd_Document) hDoc)
    : pDoc(hDoc)
{
    myGroupIcon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);

    TDataStd::IDList(myList);
    myList.Append(TDataStd_TreeNode::GetDefaultTreeID());
    myList.Append(TDataStd_Integer::GetID());
    myList.Append(TDocStd_Owner::GetID());
    myList.Append(TNaming_NamedShape::GetID());
    myList.Append(TNaming_UsedShapes::GetID());
    myList.Append(XCAFDoc_Color::GetID());
    myList.Append(XCAFDoc_ColorTool::GetID());
    myList.Append(XCAFDoc_LayerTool::GetID());
    myList.Append(XCAFDoc_ShapeTool::GetID());
    myList.Append(XCAFDoc_ShapeMapTool::GetID());
    myList.Append(XCAFDoc_Location::GetID());
}

std::string OCAFBrowser::toString(const TCollection_ExtendedString& extstr) const
{
    char* str = new char[extstr.LengthOfCString() + 1];
    extstr.ToUTF8CString(str);
    std::string text(str);
    delete[] str;
    return text;
}

void OCAFBrowser::load(QTreeWidget* theTree)
{
    theTree->clear();

    QTreeWidgetItem* root = new QTreeWidgetItem();
    root->setText(0, QLatin1String("0"));
    root->setIcon(0, myGroupIcon);
    theTree->addTopLevelItem(root);

    load(pDoc->GetData()->Root(), root, QString::fromLatin1("0"));
}

void OCAFBrowser::load(const TDF_Label& label, QTreeWidgetItem* item, const QString& s)
{
    label.Dump(std::cout);

    Handle(TDataStd_Name) name;
    if (label.FindAttribute(TDataStd_Name::GetID(), name)) {
        QString text =
            QString::fromLatin1("%1 %2").arg(s, QString::fromUtf8(toString(name->Get()).c_str()));
        item->setText(0, text);
    }


    TDF_IDList localList;
    TDF_AttributeIterator itr(label);
    for (; itr.More(); itr.Next()) {
        localList.Append(itr.Value()->ID());
    }

    for (TDF_ListIteratorOfIDList it(localList); it.More(); it.Next()) {
        Handle(TDF_Attribute) attr;
        if (label.FindAttribute(it.Value(), attr)) {
            QTreeWidgetItem* child = new QTreeWidgetItem();
            item->addChild(child);
            if (it.Value() == TDataStd_Name::GetID()) {
                QString text;
                QTextStream str(&text);
                str << attr->DynamicType()->Name();
                str << " = " << toString(Handle(TDataStd_Name)::DownCast(attr)->Get()).c_str();
                child->setText(0, text);
            }
            else if (it.Value() == TDF_TagSource::GetID()) {
                QString text;
                QTextStream str(&text);
                str << attr->DynamicType()->Name();
                str << " = " << Handle(TDF_TagSource)::DownCast(attr)->Get();
                child->setText(0, text);
            }
            else if (it.Value() == TDataStd_Integer::GetID()) {
                QString text;
                QTextStream str(&text);
                str << attr->DynamicType()->Name();
                str << " = " << Handle(TDataStd_Integer)::DownCast(attr)->Get();
                child->setText(0, text);
            }
            else if (it.Value() == TNaming_NamedShape::GetID()) {
                TopoDS_Shape shape = Handle(TNaming_NamedShape)::DownCast(attr)->Get();
                QString text;
                QTextStream str(&text);
                str << attr->DynamicType()->Name() << " = ";
                if (!shape.IsNull()) {
                    switch (shape.ShapeType()) {
                        case TopAbs_COMPOUND:
                            str << "COMPOUND PRIMITIVE";
                            break;
                        case TopAbs_COMPSOLID:
                            str << "COMPSOLID PRIMITIVE";
                            break;
                        case TopAbs_SOLID:
                            str << "SOLID PRIMITIVE";
                            break;
                        case TopAbs_SHELL:
                            str << "SHELL PRIMITIVE";
                            break;
                        case TopAbs_FACE:
                            str << "FACE PRIMITIVE";
                            break;
                        case TopAbs_WIRE:
                            str << "WIRE PRIMITIVE";
                            break;
                        case TopAbs_EDGE:
                            str << "EDGE PRIMITIVE";
                            break;
                        case TopAbs_VERTEX:
                            str << "VERTEX PRIMITIVE";
                            break;
                        case TopAbs_SHAPE:
                            str << "SHAPE PRIMITIVE";
                            break;
                    }
                }
                child->setText(0, text);
            }
            else {
                child->setText(0, QLatin1String(attr->DynamicType()->Name()));
            }
        }
    }


    int i = 1;
    for (TDF_ChildIterator it(label); it.More(); it.Next(), i++) {
        QString text = QString::fromLatin1("%1:%2").arg(s).arg(i);
        QTreeWidgetItem* child = new QTreeWidgetItem();
        child->setText(0, text);
        child->setIcon(0, myGroupIcon);
        item->addChild(child);
        load(it.Value(), child, text);
    }
}

void OCAFBrowser::showDialog(const QString& title, Handle(TDocStd_Document) hDoc)
{
    static QPointer<QDialog> dlg = nullptr;
    if (!dlg) {
        dlg = new QDialog(Gui::getMainWindow());
        QTreeWidget* tree = new QTreeWidget();
        tree->setHeaderLabel(QString::fromLatin1("OCAF Browser"));

        QVBoxLayout* layout = new QVBoxLayout;
        layout->addWidget(tree);
        dlg->setLayout(layout);

        QDialogButtonBox* btn = new QDialogButtonBox(dlg);
        btn->setStandardButtons(QDialogButtonBox::Close);
        QObject::connect(btn, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
        QHBoxLayout* boxlayout = new QHBoxLayout;
        boxlayout->addWidget(btn);
        layout->addLayout(boxlayout);
    }

    dlg->setWindowTitle(title);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();

    OCAFBrowser browse(hDoc);
    browse.load(dlg->findChild<QTreeWidget*>());
}
