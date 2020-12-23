/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QHeaderView>
#endif

#include "DlgCheckableMessageBox.h"
#include "ui_DlgCheckableMessageBox.h"

#include <QPushButton>
#include <QtCore/QDebug>

#include "MainWindow.h" 

#include <App/Application.h>
//#include <App/Parameter.h>


namespace Gui {
namespace Dialog {
QByteArray toParamEntry(QString name)
{
    name.replace(QString::fromLatin1(" "), QString::fromLatin1("_"));
    return name.toLatin1();
}

void DlgCheckableMessageBox::showMessage(const QString& header, const QString& message, bool check, const QString& checkText)
{
    bool checked = App::GetApplication().GetParameterGroupByPath( QByteArray("User parameter:BaseApp/CheckMessages"))->GetBool(toParamEntry(header));

    if (!checked) {
        DlgCheckableMessageBox *mb = new DlgCheckableMessageBox(Gui::getMainWindow());
        mb->setWindowTitle(header);
        mb->setIconPixmap(QMessageBox::standardIcon(QMessageBox::Warning));
        mb->setText(message);
        mb->setPrefEntry(header);
        mb->setCheckBoxText(checkText);
        mb->setChecked(check);
        mb->setStandardButtons(QDialogButtonBox::Ok);
        mb->setDefaultButton(QDialogButtonBox::Ok);
        mb->show();
    }
}


struct DlgCheckableMessageBoxPrivate {
    DlgCheckableMessageBoxPrivate() : clickedButton(0) {}

    Ui::DlgCheckableMessageBox ui;
    QAbstractButton *clickedButton;
};

DlgCheckableMessageBox::DlgCheckableMessageBox(QWidget *parent) :
    QDialog(parent),
    m_d(new DlgCheckableMessageBoxPrivate)
{
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_d->ui.setupUi(this);
    m_d->ui.pixmapLabel->setVisible(false);
    connect(m_d->ui.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_d->ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(m_d->ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(slotClicked(QAbstractButton*)));
}

DlgCheckableMessageBox::~DlgCheckableMessageBox()
{
    delete m_d;
}

void DlgCheckableMessageBox::setPrefEntry(const QString& entry)
{
    paramEntry = toParamEntry(entry);
    bool checked = App::GetApplication().GetParameterGroupByPath(QByteArray("User parameter:BaseApp/CheckMessages"))->GetBool(paramEntry);
    setChecked(checked);
}


void DlgCheckableMessageBox::slotClicked(QAbstractButton *b)
{
    m_d->clickedButton = b;
}

QAbstractButton *DlgCheckableMessageBox::clickedButton() const
{
    return m_d->clickedButton;
}

QDialogButtonBox::StandardButton DlgCheckableMessageBox::clickedStandardButton() const
{
    if (m_d->clickedButton)
        return m_d->ui.buttonBox->standardButton(m_d->clickedButton);
    return QDialogButtonBox::NoButton;
}

QString DlgCheckableMessageBox::text() const
{
    return m_d->ui.messageLabel->text();
}

void DlgCheckableMessageBox::setText(const QString &t)
{
    m_d->ui.messageLabel->setText(t);
}

QPixmap DlgCheckableMessageBox::iconPixmap() const
{
    if (const QPixmap *p = m_d->ui.pixmapLabel->pixmap())
        return QPixmap(*p);
    return QPixmap();
}

void DlgCheckableMessageBox::setIconPixmap(const QPixmap &p)
{
    m_d->ui.pixmapLabel->setPixmap(p);
    m_d->ui.pixmapLabel->setVisible(!p.isNull());
}

bool DlgCheckableMessageBox::isChecked() const
{
    return m_d->ui.checkBox->isChecked();
}

void DlgCheckableMessageBox::setChecked(bool s)
{
    m_d->ui.checkBox->setChecked(s);
}

QString DlgCheckableMessageBox::checkBoxText() const
{
    return m_d->ui.checkBox->text();
}

void DlgCheckableMessageBox::setCheckBoxText(const QString &t)
{
    m_d->ui.checkBox->setText(t);
}

QDialogButtonBox::StandardButtons DlgCheckableMessageBox::standardButtons() const
{
    return m_d->ui.buttonBox->standardButtons();
}

void DlgCheckableMessageBox::setStandardButtons(QDialogButtonBox::StandardButtons s)
{
    m_d->ui.buttonBox->setStandardButtons(s);
}

QDialogButtonBox::StandardButton DlgCheckableMessageBox::defaultButton() const
{
    Q_FOREACH (QAbstractButton *b, m_d->ui.buttonBox->buttons())
        if (QPushButton *pb = qobject_cast<QPushButton *>(b))
            if (pb->isDefault())
               return m_d->ui.buttonBox->standardButton(pb);
    return QDialogButtonBox::NoButton;
}

void DlgCheckableMessageBox::setDefaultButton(QDialogButtonBox::StandardButton s)
{
    if (QPushButton *b = m_d->ui.buttonBox->button(s)) {
        b->setDefault(true);
        b->setFocus();
    }
}

void DlgCheckableMessageBox::accept()
{
    if(!paramEntry.isEmpty())
        App::GetApplication().GetParameterGroupByPath( QByteArray("User parameter:BaseApp/CheckMessages"))->SetBool(paramEntry,isChecked());
    QDialog::accept();
}

void DlgCheckableMessageBox::reject()
{
    if(!paramEntry.isEmpty())
        App::GetApplication().GetParameterGroupByPath( QByteArray("User parameter:BaseApp/CheckMessages"))->SetBool(paramEntry,isChecked());
    QDialog::reject();
}

QDialogButtonBox::StandardButton
        DlgCheckableMessageBox::question(QWidget *parent,
                                      const QString &title,
                                      const QString &question,
                                      const QString &checkBoxText,
                                      bool *checkBoxSetting,
                                      QDialogButtonBox::StandardButtons buttons,
                                      QDialogButtonBox::StandardButton defaultButton)
{
    DlgCheckableMessageBox mb(parent);
    mb.setWindowTitle(title);
    mb.setIconPixmap(QMessageBox::standardIcon(QMessageBox::Question));
    mb.setText(question);
    mb.setCheckBoxText(checkBoxText);
    mb.setChecked(*checkBoxSetting);
    mb.setStandardButtons(buttons);
    mb.setDefaultButton(defaultButton);
    mb.exec();
    *checkBoxSetting = mb.isChecked();
    return mb.clickedStandardButton();
}

QMessageBox::StandardButton DlgCheckableMessageBox::dialogButtonBoxToMessageBoxButton(QDialogButtonBox::StandardButton db)
{
    return static_cast<QMessageBox::StandardButton>(int(db));
}

} // namespace Dialog
} // namespace Gui

#include "moc_DlgCheckableMessageBox.cpp"
