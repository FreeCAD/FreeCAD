/********************************************************************************
** Form generated from reading UI file 'DlgIconFolder.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DLGICONFOLDER_H
#define UI_DLGICONFOLDER_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_IconFolders
{
public:
    QVBoxLayout *mainLayout;
    QGroupBox *infoGroup;
    QVBoxLayout *infoLayout;
    QLabel *infoLabel;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QVBoxLayout *tableLayout;
    QTableWidget *tableWidget;
    QHBoxLayout *bottomRowLayout;
    QPushButton *addButton;
    QSpacerItem *horizontalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *IconFolders)
    {
        if (IconFolders->objectName().isEmpty())
            IconFolders->setObjectName("IconFolders");
        IconFolders->resize(600, 600);
        mainLayout = new QVBoxLayout(IconFolders);
        mainLayout->setObjectName("mainLayout");
        infoGroup = new QGroupBox(IconFolders);
        infoGroup->setObjectName("infoGroup");
        infoLayout = new QVBoxLayout(infoGroup);
        infoLayout->setObjectName("infoLayout");
        infoLabel = new QLabel(infoGroup);
        infoLabel->setObjectName("infoLabel");
        infoLabel->setWordWrap(true);

        infoLayout->addWidget(infoLabel);


        mainLayout->addWidget(infoGroup);

        scrollArea = new QScrollArea(IconFolders);
        scrollArea->setObjectName("scrollArea");
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName("scrollAreaWidgetContents");
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 580, 467));
        tableLayout = new QVBoxLayout(scrollAreaWidgetContents);
        tableLayout->setObjectName("tableLayout");
        tableWidget = new QTableWidget(scrollAreaWidgetContents);
        if (tableWidget->columnCount() < 4)
            tableWidget->setColumnCount(4);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        tableWidget->setObjectName("tableWidget");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(tableWidget->sizePolicy().hasHeightForWidth());
        tableWidget->setSizePolicy(sizePolicy);
        tableWidget->setRowCount(0);
        tableWidget->setColumnCount(4);

        tableLayout->addWidget(tableWidget);

        scrollArea->setWidget(scrollAreaWidgetContents);

        mainLayout->addWidget(scrollArea);

        bottomRowLayout = new QHBoxLayout();
        bottomRowLayout->setObjectName("bottomRowLayout");
        addButton = new QPushButton(IconFolders);
        addButton->setObjectName("addButton");

        bottomRowLayout->addWidget(addButton);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        bottomRowLayout->addItem(horizontalSpacer);

        buttonBox = new QDialogButtonBox(IconFolders);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Cancel|QDialogButtonBox::StandardButton::Ok);

        bottomRowLayout->addWidget(buttonBox);


        mainLayout->addLayout(bottomRowLayout);


        retranslateUi(IconFolders);

        QMetaObject::connectSlotsByName(IconFolders);
    } // setupUi

    void retranslateUi(QDialog *IconFolders)
    {
        IconFolders->setWindowTitle(QCoreApplication::translate("IconFolders", "Icon Folders", nullptr));
        infoGroup->setTitle(QString());
        infoLabel->setText(QCoreApplication::translate("IconFolders", "These folders will be searched for icons used in the FreeCAD interface, including custom icons for macros. Use the 'Add new' button to add a folder (for example, from your Mod directory). If the same icon file exists in multiple folders, the last folder in the list takes precedence. Changes take effect after restart.", nullptr));
        QTableWidgetItem *___qtablewidgetitem = tableWidget->horizontalHeaderItem(1);
        ___qtablewidgetitem->setText(QCoreApplication::translate("IconFolders", "Path", nullptr));
        addButton->setText(QCoreApplication::translate("IconFolders", "Add new", nullptr));
    } // retranslateUi

};

namespace Ui {
    class IconFolders: public Ui_IconFolders {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DLGICONFOLDER_H
