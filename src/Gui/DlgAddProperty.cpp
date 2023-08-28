/****************************************************************************
 *   Copyright (c) 2019 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QMessageBox>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Tools.h>

#include "DlgAddProperty.h"
#include "ui_DlgAddProperty.h"
#include "MainWindow.h"
#include "ViewProviderDocumentObject.h"


using namespace Gui;
using namespace Gui::Dialog;

DlgAddProperty::DlgAddProperty(QWidget* parent,
        std::unordered_set<App::PropertyContainer *> &&c)
  : QDialog( parent )
  , containers(std::move(c))
  , ui(new Ui_DlgAddProperty)
{
    ui->setupUi(this);

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/PropertyView");
    auto defType = Base::Type::fromName(
            hGrp->GetASCII("NewPropertyType","App::PropertyString").c_str());
    if(defType.isBad())
        defType = App::PropertyString::getClassTypeId();

    std::vector<Base::Type> types;
    Base::Type::getAllDerivedFrom(Base::Type::fromName("App::Property"),types);
    std::sort(types.begin(), types.end(), [](Base::Type a, Base::Type b) { return strcmp(a.getName(), b.getName()) < 0; });

    for(const auto& type : types) {
        ui->comboType->addItem(QString::fromLatin1(type.getName()));
        if(type == defType)
            ui->comboType->setCurrentIndex(ui->comboType->count()-1);
    }

    ui->edtGroup->setText(QString::fromLatin1(
                hGrp->GetASCII("NewPropertyGroup","Base").c_str()));
    ui->chkAppend->setChecked(hGrp->GetBool("NewPropertyAppend",true));
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgAddProperty::~DlgAddProperty() = default;

static std::string containerName(const App::PropertyContainer *c) {
    auto doc = Base::freecad_dynamic_cast<App::Document>(c);
    if(doc)
        return doc->getName();
    auto obj = Base::freecad_dynamic_cast<App::DocumentObject>(c);
    if(obj)
        return obj->getFullName();
    auto vpd = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(c);
    if(vpd)
        return vpd->getObject()->getFullName();
    return "?";
}

void DlgAddProperty::accept()
{
    std::string name = ui->edtName->text().toUtf8().constData();
    std::string group = ui->edtGroup->text().toUtf8().constData();
    if(name.empty() || group.empty()
            || name != Base::Tools::getIdentifier(name)
            || group != Base::Tools::getIdentifier(group))
    {
        QMessageBox::critical(getMainWindow(),
            QObject::tr("Invalid name"),
            QObject::tr("The property name or group name must only contain alpha numericals,\n"
                        "underscore, and must not start with a digit."));
        return;
    }

    if(ui->chkAppend->isChecked())
        name = group + "_" + name;

    for(auto c : containers) {
        auto prop = c->getPropertyByName(name.c_str());
        if(prop && prop->getContainer() == c) {
            QMessageBox::critical(getMainWindow(),
                QObject::tr("Invalid name"),
                QObject::tr("The property '%1' already exists in '%2'").arg(
                    QString::fromLatin1(name.c_str()),
                    QString::fromLatin1(containerName(c).c_str())));
            return;
        }
    }

    std::string type = ui->comboType->currentText().toLatin1().constData();

    for(auto it=containers.begin();it!=containers.end();++it) {
        try {
            (*it)->addDynamicProperty(type.c_str(),name.c_str(),
                    group.c_str(),ui->edtDoc->toPlainText().toUtf8().constData());
        } catch(Base::Exception &e) {
            e.ReportException();
            for(auto it2=containers.begin();it2!=it;++it2) {
                try {
                    (*it2)->removeDynamicProperty(name.c_str());
                } catch(Base::Exception &e) {
                    e.ReportException();
                }
            }
            QMessageBox::critical(getMainWindow(),
                QObject::tr("Add property"),
                QObject::tr("Failed to add property to '%1': %2").arg(
                    QString::fromLatin1(containerName(*it).c_str()),
                    QString::fromUtf8(e.what())));
            return;
        }
    }
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/PropertyView");
    hGrp->SetASCII("NewPropertyType",type.c_str());
    hGrp->SetASCII("NewPropertyGroup",group.c_str());
    hGrp->SetBool("NewPropertyAppend",ui->chkAppend->isChecked());
    QDialog::accept();
}

#include "moc_DlgAddProperty.cpp"
