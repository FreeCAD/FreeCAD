// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#include <QCoreApplication>
#include <QGridLayout>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>
#endif

#include "StartView.h"
#include "FileCardDelegate.h"
#include "FileCardView.h"
#include "Gui/Workbench.h"
#include <Gui/Document.h>
#include <App/DocumentObject.h>
#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <3rdParty/GSL/include/gsl/pointers>

using namespace StartGui;

TYPESYSTEM_SOURCE_ABSTRACT(StartGui::StartView, Gui::MDIView)  // NOLINT

namespace
{

struct NewButton
{
    QString heading;
    QString description;
    QString iconPath;
};

gsl::owner<QPushButton*> createNewButton(const NewButton& newButton)
{
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Start");
    const auto cardSpacing = static_cast<int>(hGrp->GetInt("FileCardSpacing", 20));      // NOLINT
    const auto newFileIconSize = static_cast<int>(hGrp->GetInt("NewFileIconSize", 48));  // NOLINT

    auto button = gsl::owner<QPushButton*>(new QPushButton());
    auto mainLayout = gsl::owner<QHBoxLayout*>(new QHBoxLayout(button));
    auto iconLabel = gsl::owner<QLabel*>(new QLabel(button));
    mainLayout->addWidget(iconLabel);
    QIcon baseIcon(newButton.iconPath);
    iconLabel->setPixmap(baseIcon.pixmap(newFileIconSize, newFileIconSize));

    auto textLayout = gsl::owner<QVBoxLayout*>(new QVBoxLayout);
    auto textLabelLine1 = gsl::owner<QLabel*>(new QLabel(button));
    textLabelLine1->setText(QLatin1String("<b>") + newButton.heading + QLatin1String("</b>"));
    auto textLabelLine2 = gsl::owner<QLabel*>(new QLabel(button));
    textLabelLine2->setText(newButton.description);
    textLabelLine2->setWordWrap(true);
    textLayout->addWidget(textLabelLine1);
    textLayout->addWidget(textLabelLine2);
    textLayout->setSpacing(0);
    mainLayout->addItem(textLayout);

    mainLayout->addStretch();

    button->setMinimumHeight(newFileIconSize + cardSpacing);
    return button;
}

}  // namespace

StartView::StartView(Gui::Document* pcDocument, QWidget* parent)
    : Gui::MDIView(pcDocument, parent)
    , _contents(new QScrollArea(parent))
{
    setObjectName(QLatin1String("StartView"));
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Start");
    auto cardSpacing = hGrp->GetInt("FileCardSpacing", 20);  // NOLINT

    auto scrolledWidget = gsl::owner<QWidget*>(new QWidget(this));
    _contents->setWidget(scrolledWidget);
    _contents->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    _contents->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
    _contents->setWidgetResizable(true);
    auto layout = gsl::owner<QVBoxLayout*>(new QVBoxLayout(scrolledWidget));
    layout->setSizeConstraint(QLayout::SizeConstraint::SetMinAndMaxSize);

    // New WB notice: temporary to explain why all your setting disappeared
    auto newStartWBNotice = gsl::owner<QLabel*>(
        new QLabel(tr("NOTE: The Start Workbench has been completely rewritten to remove all "
                      "network access, and to remove "
                      "its dependency on Chromium. This is still a work-in-progress, and not all "
                      "settings from the previous "
                      "version of Start have been migrated yet.")));
    newStartWBNotice->setWordWrap(true);
    layout->addWidget(newStartWBNotice);

    const QLatin1String h1Start("<h1>");
    const QLatin1String h1End("</h1>");

    auto newFileLabel = gsl::owner<QLabel*>(new QLabel(h1Start + tr("New File") + h1End));
    layout->addWidget(newFileLabel);
    auto gridLayout = gsl::owner<QGridLayout*>(new QGridLayout);
    layout->addLayout(gridLayout);
    configureNewFileButtons(gridLayout);

    auto recentFilesLabel = gsl::owner<QLabel*>(new QLabel(h1Start + tr("Recent Files") + h1End));
    layout->addWidget(recentFilesLabel);
    auto recentFilesListWidget = gsl::owner<FileCardView*>(new FileCardView(_contents));
    connect(recentFilesListWidget, &QListView::clicked, this, &StartView::fileCardSelected);
    layout->addWidget(recentFilesListWidget);

    auto examplesLabel = gsl::owner<QLabel*>(new QLabel(h1Start + tr("Examples") + h1End));
    layout->addWidget(examplesLabel);
    auto examplesListWidget = gsl::owner<FileCardView*>(new FileCardView(_contents));
    connect(examplesListWidget, &QListView::clicked, this, &StartView::fileCardSelected);
    layout->addWidget(examplesListWidget);

    layout->setSpacing(static_cast<int>(cardSpacing));
    layout->addStretch();

    setCentralWidget(_contents);

    QString title = QCoreApplication::translate("Workbench", "Start");
    setWindowTitle(title);

    configureExamplesListWidget(examplesListWidget);
    configureRecentFilesListWidget(recentFilesListWidget);
}


void StartView::configureNewFileButtons(QGridLayout* layout) const
{
    auto newEmptyFile = createNewButton({tr("Empty file"),
                                         tr("Create a new empty FreeCAD file"),
                                         QLatin1String(":/icons/document-new.svg")});
    auto openFile = createNewButton({tr("Open File"),
                                     tr("Open an existing CAD file or 3D model"),
                                     QLatin1String(":/icons/document-open.svg")});
    auto partDesign = createNewButton({tr("Parametric Part"),
                                       tr("Create a part with the Part Design workbench"),
                                       QLatin1String(":/icons/PartDesignWorkbench.svg")});
    auto assembly = createNewButton({tr("Assembly"),
                                     tr("Create an assembly project"),
                                     QLatin1String(":/icons/AssemblyWorkbench.svg")});
    auto draft = createNewButton({tr("2D Draft"),
                                  tr("Create a 2D Draft with the Draft workbench"),
                                  QLatin1String(":/icons/DraftWorkbench.svg")});
    auto arch = createNewButton({tr("BIM/Architecture"),
                                 tr("Create an architectural project"),
                                 QLatin1String(":/icons/ArchWorkbench.svg")});

    layout->addWidget(partDesign, 0, 0);
    layout->addWidget(assembly, 0, 1);
    layout->addWidget(draft, 0, 2);
    layout->addWidget(arch, 1, 0);
    layout->addWidget(newEmptyFile, 1, 1);
    layout->addWidget(openFile, 1, 2);

    connect(newEmptyFile, &QPushButton::clicked, this, &StartView::newEmptyFile);
    connect(openFile, &QPushButton::clicked, this, &StartView::openExistingFile);
    connect(partDesign, &QPushButton::clicked, this, &StartView::newPartDesignFile);
    connect(assembly, &QPushButton::clicked, this, &StartView::newAssemblyFile);
    connect(draft, &QPushButton::clicked, this, &StartView::newDraftFile);
    connect(arch, &QPushButton::clicked, this, &StartView::newArchFile);
}

void StartView::configureFileCardWidget(QListView* fileCardWidget)
{
    auto delegate = gsl::owner<FileCardDelegate*>(new FileCardDelegate);
    fileCardWidget->setItemDelegate(delegate);
    fileCardWidget->setMinimumWidth(fileCardWidget->parentWidget()->width());
    fileCardWidget->setGridSize(
        fileCardWidget->itemDelegate()->sizeHint(QStyleOptionViewItem(),
                                                 fileCardWidget->model()->index(0, 0)));
}


void StartView::configureRecentFilesListWidget(QListView* recentFilesListWidget)
{
    _recentFilesModel.loadRecentFiles();
    recentFilesListWidget->setModel(&_recentFilesModel);
    configureFileCardWidget(recentFilesListWidget);
}


void StartView::configureExamplesListWidget(QListView* examplesListWidget)
{
    _examplesModel.loadExamples();
    examplesListWidget->setModel(&_examplesModel);
    configureFileCardWidget(examplesListWidget);
}


void StartView::newEmptyFile() const
{
    Gui::Application::Instance->commandManager().runCommandByName("Std_New");
    postStart(PostStartBehavior::switchWorkbench);
}

void StartView::newPartDesignFile() const
{
    Gui::Application::Instance->commandManager().runCommandByName("Std_New");
    Gui::Application::Instance->activateWorkbench("PartDesignWorkbench");
    Gui::Application::Instance->commandManager().runCommandByName("PartDesign_Body");
    postStart(PostStartBehavior::doNotSwitchWorkbench);
}

void StartView::openExistingFile() const
{
    auto originalDocument = Gui::Application::Instance->activeDocument();
    Gui::Application::Instance->commandManager().runCommandByName("Std_Open");
    if (Gui::Application::Instance->activeDocument() != originalDocument) {
        // Only run this if the user chose a new document to open (that is, they didn't cancel the
        // open file dialog)
        postStart(PostStartBehavior::switchWorkbench);
    }
}

void StartView::newAssemblyFile() const
{
    Gui::Application::Instance->commandManager().runCommandByName("Std_New");
    Gui::Application::Instance->activateWorkbench("AssemblyWorkbench");
    Gui::Application::Instance->commandManager().runCommandByName("Assembly_CreateAssembly");
    Gui::Application::Instance->commandManager().runCommandByName("Std_Refresh");
    postStart(PostStartBehavior::doNotSwitchWorkbench);
}

void StartView::newDraftFile() const
{
    Gui::Application::Instance->commandManager().runCommandByName("Std_New");
    Gui::Application::Instance->activateWorkbench("DraftWorkbench");
    Gui::Application::Instance->commandManager().runCommandByName("Std_ViewTop");
    postStart(PostStartBehavior::doNotSwitchWorkbench);
}

void StartView::newArchFile() const
{
    Gui::Application::Instance->commandManager().runCommandByName("Std_New");
    try {
        Gui::Application::Instance->activateWorkbench("BIMWorkbench");
    }
    catch (...) {
        Gui::Application::Instance->activateWorkbench("ArchWorkbench");
    }
    postStart(PostStartBehavior::doNotSwitchWorkbench);
}

void StartView::postStart(PostStartBehavior behavior) const
{
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Start");

    if (behavior == PostStartBehavior::switchWorkbench) {
        auto wb = hGrp->GetASCII("AutoloadModule", "");
        if (wb == "$LastModule") {
            wb = App::GetApplication()
                     .GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")
                     ->GetASCII("LastModule", "");
        }
        if (!wb.empty()) {
            Gui::Application::Instance->activateWorkbench(wb.c_str());
        }
    }
    auto closeStart = hGrp->GetBool("closeStart", false);
    if (closeStart) {
        this->window()->close();
    }
}


void StartView::fileCardSelected(const QModelIndex& index)
{
    auto file = index.data(static_cast<int>(Start::DisplayedFilesModelRoles::path)).toString();
    auto command = std::string("FreeCAD.loadFile('") + file.toStdString() + "')";
    try {
        Base::Interpreter().runString(command.c_str());
        postStart(PostStartBehavior::doNotSwitchWorkbench);
    }
    catch (Base::PyException& e) {
        Base::Console().Error(e.getMessage().c_str());
    }
    catch (Base::Exception& e) {
        Base::Console().Error(e.getMessage().c_str());
    }
    catch (...) {
        Base::Console().Error("An unknown error occurred");
    }
}
