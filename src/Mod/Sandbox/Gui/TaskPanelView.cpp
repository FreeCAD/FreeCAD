/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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

/// Here the FreeCAD includes sorted by Base,App,Gui......

#include <boost/bind/bind.hpp>
#include <QVariant>
#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QWidget>
#include <QTabWidget>

#include <QButtonGroup>
#include <QPushButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QMessageBox>

#include <Gui/ActionFunction.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/BitmapFactory.h>

#include "TaskPanelView.h"


using namespace SandboxGui;
namespace bp = boost::placeholders;


#if defined(QSINT_ACTIONPANEL)
class Ui_TaskGroup
{
public:
    QAction *actionNew;
    QAction *actionLoad;
    QAction *actionSave;
    QAction *actionPrint;
    QGridLayout *gridLayout;
    QSint::ActionPanel *ActionPanel;
    QSint::ActionGroup *ActionGroup1;
    QVBoxLayout *verticalLayout;
    QRadioButton *rbDefaultScheme;
    QRadioButton *rbXPBlueScheme;
    QRadioButton *rbXPBlue2Scheme;
    QRadioButton *rbVistaScheme;
    QRadioButton *rbMacScheme;
    QRadioButton *rbAndroidScheme;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *MainWindow2)
    {
        if (MainWindow2->objectName().isEmpty())
            MainWindow2->setObjectName(QString::fromUtf8("MainWindow2"));
        MainWindow2->resize(529, 407);
        MainWindow2->setStyleSheet(QString::fromUtf8("\n"
            "QWidget2 {\n"
"    background-color: green;\n"
"}\n"
""));
        actionNew = new QAction(MainWindow2);
        actionNew->setObjectName(QString::fromUtf8("actionNew"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/document-new.svg"), QSize(), QIcon::Normal, QIcon::Off);
        actionNew->setIcon(icon);
        actionLoad = new QAction(MainWindow2);
        actionLoad->setObjectName(QString::fromUtf8("actionLoad"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/document-open.svg"), QSize(), QIcon::Normal, QIcon::Off);
        actionLoad->setIcon(icon1);
        actionSave = new QAction(MainWindow2);
        actionSave->setObjectName(QString::fromUtf8("actionSave"));
        actionSave->setEnabled(false);
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/document-save.svg"), QSize(), QIcon::Normal, QIcon::Off);
        actionSave->setIcon(icon2);
        actionPrint = new QAction(MainWindow2);
        actionPrint->setObjectName(QString::fromUtf8("actionPrint"));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/document-print.svg"), QSize(), QIcon::Normal, QIcon::Off);
        actionPrint->setIcon(icon3);
        gridLayout = new QGridLayout(MainWindow2);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        ActionPanel = new QSint::ActionPanel(MainWindow2);
        ActionPanel->setObjectName(QString::fromUtf8("ActionPanel"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(ActionPanel->sizePolicy().hasHeightForWidth());
        ActionPanel->setSizePolicy(sizePolicy);

        gridLayout->addWidget(ActionPanel, 0, 0, 2, 1);

        ActionGroup1 = new QSint::ActionGroup(MainWindow2);
        ActionGroup1->setObjectName(QString::fromUtf8("ActionGroup1"));
        ActionGroup1->setProperty("expandable", QVariant(true));
        ActionGroup1->setProperty("header", QVariant(true));
        verticalLayout = new QVBoxLayout(ActionGroup1);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        rbDefaultScheme = new QRadioButton(ActionGroup1);
        rbDefaultScheme->setObjectName(QString::fromUtf8("rbDefaultScheme"));
        rbDefaultScheme->setChecked(true);

        verticalLayout->addWidget(rbDefaultScheme);

        rbXPBlueScheme = new QRadioButton(ActionGroup1);
        rbXPBlueScheme->setObjectName(QString::fromUtf8("rbXPBlueScheme"));

        verticalLayout->addWidget(rbXPBlueScheme);

        rbXPBlue2Scheme = new QRadioButton(ActionGroup1);
        rbXPBlue2Scheme->setObjectName(QString::fromUtf8("rbXPBlue2Scheme"));

        verticalLayout->addWidget(rbXPBlue2Scheme);

        rbVistaScheme = new QRadioButton(ActionGroup1);
        rbVistaScheme->setObjectName(QString::fromUtf8("rbVistaScheme"));

        verticalLayout->addWidget(rbVistaScheme);

        rbMacScheme = new QRadioButton(ActionGroup1);
        rbMacScheme->setObjectName(QString::fromUtf8("rbMacScheme"));

        verticalLayout->addWidget(rbMacScheme);

        rbAndroidScheme = new QRadioButton(ActionGroup1);
        rbAndroidScheme->setObjectName(QString::fromUtf8("rbAndroidScheme"));

        verticalLayout->addWidget(rbAndroidScheme);


        gridLayout->addWidget(ActionGroup1, 0, 1, 1, 1);

        verticalSpacer = new QSpacerItem(20, 57, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 1, 1, 1, 1);


        retranslateUi(MainWindow2);

        QMetaObject::connectSlotsByName(MainWindow2);
    } // setupUi

    void retranslateUi(QWidget *MainWindow2)
    {
        MainWindow2->setWindowTitle(QApplication::translate("MainWindow2", "ActionBox Example"));
        actionNew->setText(QApplication::translate("MainWindow2", "Create new file"));
        actionLoad->setText(QApplication::translate("MainWindow2", "Load a file"));
        actionSave->setText(QApplication::translate("MainWindow2", "Save current file"));
        actionPrint->setText(QApplication::translate("MainWindow2", "Print file contents"));
        ActionGroup1->setProperty("headerText", QVariant(QApplication::translate("MainWindow2", "Choose Scheme")));
        rbDefaultScheme->setText(QApplication::translate("MainWindow2", "Default"));
        rbXPBlueScheme->setText(QApplication::translate("MainWindow2", "XP Blue"));
        rbXPBlue2Scheme->setText(QApplication::translate("MainWindow2", "XP Blue 2"));
        rbVistaScheme->setText(QApplication::translate("MainWindow2", "Vista"));
        rbMacScheme->setText(QApplication::translate("MainWindow2", "MacOS"));
        rbAndroidScheme->setText(QApplication::translate("MainWindow2", "Android"));
        Q_UNUSED(MainWindow2);
    } // retranslateUi

};

class Ui_TaskActionBox
{
public:
    QVBoxLayout *verticalLayout;
    QVBoxLayout *verticalLayout_3;
    QLabel *label;
    QFrame *line_2;
    QGridLayout *gridLayout_2;
    QSint::ActionBox *ActionBox1;
    QSint::ActionBox *ActionBox2;
    QSpacerItem *verticalSpacer;
    QSint::ActionBox *ActionBox3;
    QSint::ActionBox *ActionBox4;
    Gui::TaskView::TaskGroup *ActionBox5;
    QVBoxLayout *verticalLayout_4;
    QLabel *label_2;
    QFrame *line;
    QVBoxLayout *verticalLayout_2;
    QSint::ActionLabel *ActionLabel1;
    QSint::ActionLabel *ActionLabel2;
    QSint::ActionLabel *ActionLabel3;

    void setupUi(QWidget *MainWindow)
    {
        MainWindow->resize(642, 850);
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
        MainWindow->setSizePolicy(sizePolicy);
        MainWindow->setStyleSheet(QString::fromUtf8("\n"
"SandboxGui--TaskPanelView {\n"
"	background-color: green;\n"
"}\n"
""));
        verticalLayout = new QVBoxLayout(MainWindow);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        label = new QLabel(MainWindow);
        label->setObjectName(QString::fromUtf8("label"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy1);

        verticalLayout_3->addWidget(label);

        line_2 = new QFrame(MainWindow);
        line_2->setObjectName(QString::fromUtf8("line_2"));
        line_2->setFrameShape(QFrame::HLine);
        line_2->setFrameShadow(QFrame::Sunken);

        verticalLayout_3->addWidget(line_2);


        verticalLayout->addLayout(verticalLayout_3);

        gridLayout_2 = new QGridLayout();
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        ActionBox1 = new QSint::ActionBox(MainWindow);
        ActionBox1->setObjectName(QString::fromUtf8("ActionBox1"));
        ActionBox1->setFrameShape(QFrame::StyledPanel);
        ActionBox1->setFrameShadow(QFrame::Raised);

        gridLayout_2->addWidget(ActionBox1, 0, 0, 1, 1);

        ActionBox2 = new QSint::ActionBox(MainWindow);
        ActionBox2->setObjectName(QString::fromUtf8("ActionBox2"));
        ActionBox2->setFrameShape(QFrame::StyledPanel);
        ActionBox2->setFrameShadow(QFrame::Raised);

        gridLayout_2->addWidget(ActionBox2, 1, 0, 1, 1);

        verticalSpacer = new QSpacerItem(94, 28, QSizePolicy::Minimum, QSizePolicy::Minimum);

        gridLayout_2->addItem(verticalSpacer, 3, 0, 1, 1);

        ActionBox3 = new QSint::ActionBox(MainWindow);
        ActionBox3->setObjectName(QString::fromUtf8("ActionBox3"));
        ActionBox3->setFrameShape(QFrame::StyledPanel);
        ActionBox3->setFrameShadow(QFrame::Raised);

        gridLayout_2->addWidget(ActionBox3, 0, 1, 1, 1);

        ActionBox4 = new QSint::ActionBox(MainWindow);
        ActionBox4->setObjectName(QString::fromUtf8("ActionBox4"));
        ActionBox4->setFrameShape(QFrame::StyledPanel);
        ActionBox4->setFrameShadow(QFrame::Raised);

        gridLayout_2->addWidget(ActionBox4, 1, 1, 1, 1);

        ActionBox5 = new Gui::TaskView::TaskGroup(MainWindow);
        ActionBox5->setObjectName(QString::fromUtf8("ActionBox5"));
        ActionBox5->setFrameShape(QFrame::StyledPanel);
        ActionBox5->setFrameShadow(QFrame::Raised);

        gridLayout_2->addWidget(ActionBox5, 2, 1, 1, 1);

        verticalLayout->addLayout(gridLayout_2);

        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        label_2 = new QLabel(MainWindow);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        sizePolicy1.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
        label_2->setSizePolicy(sizePolicy1);

        verticalLayout_4->addWidget(label_2);

        line = new QFrame(MainWindow);
        line->setObjectName(QString::fromUtf8("line"));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        verticalLayout_4->addWidget(line);


        verticalLayout->addLayout(verticalLayout_4);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        ActionLabel1 = new QSint::ActionLabel(MainWindow);
        ActionLabel1->setObjectName(QString::fromUtf8("ActionLabel1"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(ActionLabel1->sizePolicy().hasHeightForWidth());
        ActionLabel1->setSizePolicy(sizePolicy2);

        verticalLayout_2->addWidget(ActionLabel1);

        ActionLabel2 = new QSint::ActionLabel(MainWindow);
        ActionLabel2->setObjectName(QString::fromUtf8("ActionLabel2"));
        sizePolicy2.setHeightForWidth(ActionLabel2->sizePolicy().hasHeightForWidth());
        ActionLabel2->setSizePolicy(sizePolicy2);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/document-open.svg"), QSize(), QIcon::Normal, QIcon::Off);
        ActionLabel2->setIcon(icon);

        verticalLayout_2->addWidget(ActionLabel2);

        ActionLabel3 = new QSint::ActionLabel(MainWindow);
        ActionLabel3->setObjectName(QString::fromUtf8("ActionLabel3"));
        sizePolicy2.setHeightForWidth(ActionLabel3->sizePolicy().hasHeightForWidth());
        ActionLabel3->setSizePolicy(sizePolicy2);
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/document-print.svg"), QSize(), QIcon::Normal, QIcon::Off);
        ActionLabel3->setIcon(icon1);

        verticalLayout_2->addWidget(ActionLabel3);


        verticalLayout->addLayout(verticalLayout_2);


        retranslateUi(MainWindow);
    } // setupUi

    void retranslateUi(QWidget *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("TestTaskBox", "ActionBox Example"));
        label->setText(QApplication::translate("TestTaskBox", "ActionBoxes"));
        label_2->setText(QApplication::translate("TestTaskBox", "ActionLabels"));
        ActionLabel1->setText(QApplication::translate("TestTaskBox", "Simple clickable action"));
        ActionLabel2->setText(QApplication::translate("TestTaskBox", "Simple clickable action with icon"));
#ifndef QT_NO_TOOLTIP
        ActionLabel3->setToolTip(QApplication::translate("TestTaskBox", "Tooltip of the ActionLabel"));
#endif // QT_NO_TOOLTIP
        ActionLabel3->setText(QApplication::translate("TestTaskBox", "Simple clickable action with icon and tooltip"));
        Q_UNUSED(MainWindow);
    } // retranslateUi
};
#endif

TaskPanelView::TaskPanelView(QWidget *parent)
  : QWidget(parent)
{
    Gui::ActionFunction* func = new Gui::ActionFunction(this);
    QAction* action = new QAction(this);
    func->trigger(action, boost::bind(&TaskPanelView::executeAction, this));

#if defined(QSINT_ACTIONPANEL)

    QGridLayout* customLayout = new QGridLayout(this);
    QTabWidget* tabWidget = new QTabWidget(this);
    customLayout->addWidget(tabWidget, 0, 0);
    this->resize(642, 850);

    {
    Ui_TaskActionBox* ui(new Ui_TaskActionBox);
    QWidget* page1 = new QWidget();
    ui->setupUi(page1);
    tabWidget->addTab(page1, QLatin1String("Action Box"));

    // setup ActionBox 1
    ui->ActionBox1->setIcon(QPixmap(QString::fromLatin1(":/icons/document-open.svg")));
    ui->ActionBox1->header()->setText(QString::fromLatin1("Header of the group"));
    connect(ui->ActionBox1->header(), SIGNAL(clicked()), action, SIGNAL(triggered()));

    QSint::ActionLabel *a1 = ui->ActionBox1->createItem(QString::fromLatin1("This action has no icon"));
    connect(a1, SIGNAL(clicked()), action, SIGNAL(triggered()));
    QSint::ActionLabel *a2 = ui->ActionBox1->createItem(QPixmap(QString::fromLatin1(":/icons/document-print.svg")),
                                                QString::fromLatin1("This action has icon"));
    connect(a2, SIGNAL(clicked()), action, SIGNAL(triggered()));

    QLayout *hbl1 = ui->ActionBox1->createHBoxLayout();
    QSint::ActionLabel *a3 = ui->ActionBox1->createItem(QString::fromLatin1("1st action in row"), hbl1);
    connect(a3, SIGNAL(clicked()), action, SIGNAL(triggered()));
    QSint::ActionLabel *a4 = ui->ActionBox1->createItem(QString::fromLatin1("2nd action in row"), hbl1);
    connect(a4, SIGNAL(clicked()), action, SIGNAL(triggered()));

    // setup ActionBox 2
    ui->ActionBox2->setIcon(QPixmap(QString::fromLatin1(":/icons/document-save.png")));
    ui->ActionBox2->header()->setText(QString::fromLatin1("Checkable actions allowed"));
    connect(ui->ActionBox2->header(), SIGNAL(clicked()), action, SIGNAL(triggered()));

    QSint::ActionLabel *b1 = ui->ActionBox2->createItem(QString::fromLatin1("Action 1 (Exclusive)"));
    b1->setCheckable(true);
    b1->setAutoExclusive(true);
    b1->setChecked(true);
    QSint::ActionLabel *b2 = ui->ActionBox2->createItem(QString::fromLatin1("Action 2 (Exclusive)"));
    b2->setCheckable(true);
    b2->setAutoExclusive(true);
    QSint::ActionLabel *b3 = ui->ActionBox2->createItem(QString::fromLatin1("Action 3 (Exclusive)"));
    b3->setCheckable(true);
    b3->setAutoExclusive(true);

    QSint::ActionLabel *b4 = ui->ActionBox2->createItem(QString::fromLatin1("Non-exclusive but still checkable"));
    b4->setCheckable(true);

    // setup ActionBox 3
    ui->ActionBox3->setIcon(QPixmap(QString::fromLatin1(":/icons/document-print.png")));
    ui->ActionBox3->header()->setText(QString::fromLatin1("Also, widgets allowed as well"));

    ui->ActionBox3->addWidget(new QPushButton(QString::fromLatin1("PushButton"), this));
    ui->ActionBox3->addWidget(new QCheckBox(QString::fromLatin1("CheckBox"), this));
    QLayout *hbl3 = ui->ActionBox3->createHBoxLayout();
    ui->ActionBox3->addWidget(new QRadioButton(QString::fromLatin1("RadioButton 1"), this), hbl3);
    ui->ActionBox3->addWidget(new QRadioButton(QString::fromLatin1("RadioButton 2"), this), hbl3);

    // setup ActionBox 4
    ui->ActionBox4->setIcon(QPixmap(QString::fromLatin1(":/icons/document-open.png")));
    ui->ActionBox4->header()->setText(QString::fromLatin1("ActionBox with different scheme"));

    ui->ActionBox4->createItem(QString::fromLatin1("This action has no icon"));
    ui->ActionBox4->createItem(QPixmap(QString::fromLatin1(":/icons/document-print.png")),
                                                QString::fromLatin1("This action has icon"));
    QLayout *hbl4 = ui->ActionBox4->createHBoxLayout();
    ui->ActionBox4->createItem(QString::fromLatin1("1st action in row"), hbl4);
    ui->ActionBox4->createItem(QString::fromLatin1("2nd action in row"), hbl4);
    ui->ActionBox4->createItem(QString::fromLatin1("3rd action in row"), hbl4);

    const char* ActionBoxNewStyle =
        "QSint--ActionBox {"
            "background-color: #333333;"
            "border: 1px solid #000000;"
            "text-align: left;"
        "}"

        "QSint--ActionBox:hover {"
            "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #666666, stop: 1 #333333);"
            "border: 1px solid #222222;"
        "}"

        "QSint--ActionBox QSint--ActionLabel[class='header'] {"
            "text-align: center;"
            "font: 14px bold;"
            "color: #999999;"
            "background-color: transparent;"
            "border: 1px solid transparent;"
        "}"

        "QSint--ActionBox QSint--ActionLabel[class='header']:hover {"
            "color: #aaaaaa;"
            "text-decoration: underline;"
            "border: 1px dotted #aaaaaa;"
        "}"

        "QSint--ActionBox QSint--ActionLabel[class='action'] {"
            "background-color: transparent;"
            "border: none;"
            "color: #777777;"
            "text-align: left;"
            "font: 11px;"
        "}"

        "QSint--ActionBox QSint--ActionLabel[class='action']:hover {"
            "color: #888888;"
            "text-decoration: underline;"
        "}"

        "QSint--ActionBox QSint--ActionLabel[class='action']:on {"
            "background-color: #ddeeff;"
            "color: #006600;"
        "}"
    ;

    ui->ActionBox4->setStyleSheet(QString::fromLatin1(ActionBoxNewStyle));

    // setup ActionBox 5
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    ui->ActionBox5->setIcon(QPixmap(QString::fromLatin1(":/icons/document-save.png")));
    ui->ActionBox5->header()->setText(QString::fromLatin1("TaskGroup with different scheme"));

    rcCmdMgr.addTo("Std_New", ui->ActionBox5);
    rcCmdMgr.addTo("Std_Open", ui->ActionBox5);
    rcCmdMgr.addTo("Std_Save", ui->ActionBox5);
    ui->ActionBox5->setStyleSheet(QString::fromLatin1(ActionBoxNewStyle));
    }
    {
    Ui_TaskGroup* ui(new Ui_TaskGroup);
    QWidget* page2 = new QWidget();
    ui->setupUi(page2);
    tabWidget->addTab(page2, QLatin1String("Action Group"));

    this->actionGroup = ui->ActionPanel;

    // create ActionGroups on ActionPanel
    QIcon save = QIcon::fromTheme(QString::fromLatin1("document-save"));
    QSint::ActionGroup *group1 = ui->ActionPanel->createGroup(save.pixmap(24,24), QString::fromLatin1("Expandable Group"));
    group1->addAction(ui->actionNew);
    group1->addAction(ui->actionLoad);
    group1->addWidget(new QPushButton(QString::fromLatin1("Just a button"), this));
    group1->addAction(ui->actionSave);
    group1->addAction(ui->actionPrint);
    group1->addWidget(new QPushButton(QString::fromLatin1("Just another button"), this));

    QIcon redo = QIcon::fromTheme(QString::fromLatin1("edit-redo"));
    QSint::ActionGroup *group2 = ui->ActionPanel->createGroup(redo.pixmap(24,24), QString::fromLatin1("Non-Expandable Group"), false);
    group2->addAction(ui->actionNew);
    group2->addAction(ui->actionLoad);
    group2->addAction(ui->actionSave);
    group2->addAction(ui->actionPrint);

    ui->ActionPanel->addWidget(new QLabel(QString::fromLatin1("Action Group without header"), this));

    QSint::ActionGroup *group3 = ui->ActionPanel->createGroup();
    group3->addAction(ui->actionNew);

    QHBoxLayout *hbl = new QHBoxLayout();
    group3->groupLayout()->addLayout(hbl);
    hbl->addWidget(group3->addAction(ui->actionLoad, false));
    hbl->addWidget(group3->addAction(ui->actionSave, false));

    group3->addAction(ui->actionPrint);

    ui->ActionPanel->addStretch();


    // setup standalone ActionGroup

    ui->ActionGroup1->setScheme(QSint::WinXPPanelScheme::defaultScheme());

    ui->ActionGroup1->addWidget(ui->rbDefaultScheme);
    ui->ActionGroup1->addWidget(ui->rbXPBlueScheme);
    ui->ActionGroup1->addWidget(ui->rbXPBlue2Scheme);
    ui->ActionGroup1->addWidget(ui->rbVistaScheme);
    ui->ActionGroup1->addWidget(ui->rbMacScheme);
    ui->ActionGroup1->addWidget(ui->rbAndroidScheme);

    Gui::ActionFunction* func = new Gui::ActionFunction(this);

    QAction* defaultAction = new QAction(this);
    connect(ui->rbDefaultScheme, SIGNAL(toggled(bool)), defaultAction, SIGNAL(toggled(bool)));
    func->toggle(defaultAction, boost::bind(&TaskPanelView::on_rbDefaultScheme_toggled, this, bp::_1));

    QAction* xpBlueAction = new QAction(this);
    connect(ui->rbXPBlueScheme, SIGNAL(toggled(bool)), xpBlueAction, SIGNAL(toggled(bool)));
    func->toggle(xpBlueAction, boost::bind(&TaskPanelView::on_rbXPBlueScheme_toggled, this, bp::_1));

    QAction* xpBlue2Action = new QAction(this);
    connect(ui->rbXPBlue2Scheme, SIGNAL(toggled(bool)), xpBlue2Action, SIGNAL(toggled(bool)));
    func->toggle(xpBlue2Action, boost::bind(&TaskPanelView::on_rbXPBlue2Scheme_toggled, this, bp::_1));

    QAction* vistaAction = new QAction(this);
    connect(ui->rbVistaScheme, SIGNAL(toggled(bool)), vistaAction, SIGNAL(toggled(bool)));
    func->toggle(vistaAction, boost::bind(&TaskPanelView::on_rbVistaScheme_toggled, this, bp::_1));

    QAction* macAction = new QAction(this);
    connect(ui->rbMacScheme, SIGNAL(toggled(bool)), macAction, SIGNAL(toggled(bool)));
    func->toggle(macAction, boost::bind(&TaskPanelView::on_rbMacScheme_toggled, this, bp::_1));

    QAction* androidAction = new QAction(this);
    connect(ui->rbAndroidScheme, SIGNAL(toggled(bool)), androidAction, SIGNAL(toggled(bool)));
    func->toggle(androidAction, boost::bind(&TaskPanelView::on_rbAndroidScheme_toggled, this, bp::_1));
    }
#endif
}

TaskPanelView::~TaskPanelView()
{
}

void TaskPanelView::executeAction()
{
    QMessageBox::about(0, QString::fromLatin1("Action clicked"), QString::fromLatin1("Do something here :)"));
}

void TaskPanelView::on_rbDefaultScheme_toggled(bool b)
{
#if defined(QSINT_ACTIONPANEL)
    if (b)
        static_cast<QSint::ActionPanel *>(actionGroup)->setScheme(QSint::ActionPanelScheme::defaultScheme());
#endif
}

void TaskPanelView::on_rbXPBlueScheme_toggled(bool b)
{
#if defined(QSINT_ACTIONPANEL)
    if (b)
        static_cast<QSint::ActionPanel *>(actionGroup)->setScheme(QSint::WinXPPanelScheme::defaultScheme());
#endif
}

void TaskPanelView::on_rbXPBlue2Scheme_toggled(bool b)
{
#if defined(QSINT_ACTIONPANEL)
    if (b)
        static_cast<QSint::ActionPanel *>(actionGroup)->setScheme(QSint::WinXPPanelScheme2::defaultScheme());
#endif
}

void TaskPanelView::on_rbVistaScheme_toggled(bool b)
{
#if defined(QSINT_ACTIONPANEL)
    if (b)
        static_cast<QSint::ActionPanel *>(actionGroup)->setScheme(QSint::WinVistaPanelScheme::defaultScheme());
#endif
}

void TaskPanelView::on_rbMacScheme_toggled(bool b)
{
#if defined(QSINT_ACTIONPANEL)
    if (b)
        static_cast<QSint::ActionPanel *>(actionGroup)->setScheme(QSint::MacPanelScheme::defaultScheme());
#endif
}

void TaskPanelView::on_rbAndroidScheme_toggled(bool b)
{
#if defined(QSINT_ACTIONPANEL)
    if (b)
        static_cast<QSint::ActionPanel *>(actionGroup)->setScheme(QSint::AndroidPanelScheme::defaultScheme());
#endif
}

//#include "moc_TaskPanelView.cpp"
