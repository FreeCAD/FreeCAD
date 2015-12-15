/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QPixmap>
# include <QDialog>
# include <QListIterator>
#endif

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/MainWindow.h>
#include <App/Document.h>
#include <Base/Tools.h>

#include "ui_FeaturePickDialog.h"
#include "FeaturePickDialog.h"

using namespace PartDesignGui;

FeaturePickDialog::FeaturePickDialog(std::vector<App::DocumentObject*>& objects)
  : QDialog(Gui::getMainWindow()), ui(new Ui_FeaturePickDialog)
{
    ui->setupUi(this);
    for (std::vector<App::DocumentObject*>::const_iterator o = objects.begin(); o != objects.end(); ++o)
        ui->listWidget->addItem(QString::fromLatin1((*o)->getNameInDocument()));
}

FeaturePickDialog::~FeaturePickDialog()
{

}

std::vector<App::DocumentObject*> FeaturePickDialog::getFeatures() {
    std::vector<App::DocumentObject*> result;

    for (std::vector<QString>::const_iterator s = features.begin(); s != features.end(); ++s)
        result.push_back(App::GetApplication().getActiveDocument()->getObject(s->toLatin1().data()));

    return result;
}



void FeaturePickDialog::accept()
{
    features.clear();
    QListIterator<QListWidgetItem*> i(ui->listWidget->selectedItems());
    while (i.hasNext())
        features.push_back(i.next()->text());

    QDialog::accept();
}
#include "moc_FeaturePickDialog.cpp"
