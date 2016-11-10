/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <Inventor/nodes/SoGroup.h>
# include <Inventor/nodes/SoTexture2.h>
# include <QImage>
# include <QMessageBox>
# include <QImageReader>
#endif

#include <Inventor/nodes/SoTextureCoordinateEnvironment.h>
#include <QDialogButtonBox>

#include <App/Application.h>

#include "TextureMapping.h"
#include "BitmapFactory.h"
#include "ui_TextureMapping.h"
#include "Application.h"
#include "Document.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::TextureMapping */

TextureMapping::TextureMapping(QWidget* parent, Qt::WindowFlags fl)
  : QDialog(parent, fl), grp(0), tex(0), env(0)
{
    ui = new Ui_TextureMapping();
    ui->setupUi(this);
    ui->checkGlobal->hide();

    // add all supported QImage formats
    QStringList formats;
    QList<QByteArray> qtformats = QImageReader::supportedImageFormats();
    for (QList<QByteArray>::Iterator it = qtformats.begin(); it != qtformats.end(); ++it) {
        formats << QString::fromLatin1("*.%1").arg(QLatin1String(*it));
    }
    
    ui->fileChooser->setFilter(tr("Image files (%1)").arg(formats.join(QLatin1String(" "))));

    this->tex = new SoTexture2();
    this->tex->ref();
    this->env = new SoTextureCoordinateEnvironment();
    this->env->ref();

    std::string path = App::GetApplication().Config()["TextureImage"];
    if (!path.empty()) {
        QString file = QString::fromUtf8(path.c_str());
        ui->fileChooser->setFileName(file);
        on_fileChooser_fileNameSelected(file);
    }
}

TextureMapping::~TextureMapping()
{
    this->tex->unref();
    this->env->unref();
    delete ui;
}

void TextureMapping::accept()
{
    QDialog::accept();
}

void TextureMapping::reject()
{
    if (this->grp) {
        this->grp->removeChild(this->tex);
        if (this->grp->findChild(this->env) > -1)
            this->grp->removeChild(this->env);
        this->grp->unref();
    }

    QDialog::reject();
}

void TextureMapping::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QDialog::changeEvent(e);
    }
}

void TextureMapping::on_fileChooser_fileNameSelected(const QString& s)
{
    QImage image;
    if (!image.load(s)) {
        QMessageBox::warning(this, tr("No image"), tr("The specified file is not a valid image file."));
        return;
    }

    if (!this->grp) {
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc) {
            Gui::MDIView* mdi = doc->getActiveView();
            if (mdi && mdi->isDerivedFrom(View3DInventor::getClassTypeId())) {
                Gui::View3DInventorViewer* view = static_cast<View3DInventor*>(mdi)->getViewer();
                this->grp = static_cast<SoGroup *>(view->getSceneGraph());
                this->grp->ref();
                this->grp->insertChild(this->tex,1);
                if (ui->checkEnv->isChecked())
                    this->grp->insertChild(this->env,2);
            }
        }
    }

    if (!this->grp) {
        QMessageBox::warning(this, tr("No 3d view"), tr("No active 3d view found."));
        return;
    }

    SoSFImage texture;
    Gui::BitmapFactory().convert(image, texture);
    this->tex->image = texture;
    //this->tex->filename = (const char*)s.toUtf8();
    App::GetApplication().Config()["TextureImage"] = (const char*)s.toUtf8();
}

void TextureMapping::on_checkEnv_toggled(bool b)
{
    if (!this->grp)
        return;
    if (b) {
        this->grp->insertChild(this->env,2);
    }
    else {
        this->grp->removeChild(this->env);
    }
}

// ---------------------------------------

TaskTextureMapping::TaskTextureMapping()
{
    dialog = new TextureMapping();
    taskbox = new Gui::TaskView::TaskBox(QPixmap(), dialog->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(dialog);
    Content.push_back(taskbox);
}

TaskTextureMapping::~TaskTextureMapping()
{
    // automatically deleted in the sub-class
}

bool TaskTextureMapping::accept()
{
    dialog->accept();
    return (dialog->result() == QDialog::Accepted);
}

bool TaskTextureMapping::reject()
{
    dialog->reject();
    return (dialog->result() == QDialog::Rejected);
}

#include "moc_TextureMapping.cpp"
