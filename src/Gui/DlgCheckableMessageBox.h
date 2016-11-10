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

#ifndef CHECKABLEMESSAGEBOX_H
#define CHECKABLEMESSAGEBOX_H

//#include "utils_global.h"

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QDialog>

namespace Gui {
namespace Dialog {

struct DlgCheckableMessageBoxPrivate;

/* A messagebox suitable for questions with a
 * "Do not ask me again" checkbox. Emulates the QMessageBox API with
 * static conveniences. */

class GuiExport DlgCheckableMessageBox : public QDialog
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QPixmap iconPixmap READ iconPixmap WRITE setIconPixmap)
    Q_PROPERTY(bool isChecked READ isChecked WRITE setChecked)
  //Q_PROPERTY(QString prefEntry WRITE setPrefEntry) // Must have a READ accessor!
    Q_PROPERTY(QString checkBoxText READ checkBoxText WRITE setCheckBoxText)
    Q_PROPERTY(QDialogButtonBox::StandardButtons buttons READ standardButtons WRITE setStandardButtons)
    Q_PROPERTY(QDialogButtonBox::StandardButton defaultButton READ defaultButton WRITE setDefaultButton)
public:
    explicit DlgCheckableMessageBox(QWidget *parent);
    virtual ~DlgCheckableMessageBox();

    static QDialogButtonBox::StandardButton
        question(QWidget *parent,
                 const QString &title,
                 const QString &question,
                 const QString &checkBoxText,
                 bool *checkBoxSetting,
                 QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Yes|QDialogButtonBox::No,
                 QDialogButtonBox::StandardButton defaultButton = QDialogButtonBox::No);

    QString text() const;
    void setText(const QString &);

    void setPrefEntry(const QString& entry);

    virtual void accept();
    virtual void reject();

    bool isChecked() const;
    void setChecked(bool s);

    QString checkBoxText() const;
    void setCheckBoxText(const QString &);

   QDialogButtonBox::StandardButtons standardButtons() const;
   void setStandardButtons(QDialogButtonBox::StandardButtons s);

   QDialogButtonBox::StandardButton defaultButton() const;
   void setDefaultButton(QDialogButtonBox::StandardButton s);

    // see static QMessageBox::standardPixmap()
    QPixmap iconPixmap() const;
    void setIconPixmap (const QPixmap &p);

   // Query the result
   QAbstractButton *clickedButton() const;
   QDialogButtonBox::StandardButton clickedStandardButton() const;

   // Conversion convenience
   static QMessageBox::StandardButton dialogButtonBoxToMessageBoxButton(QDialogButtonBox::StandardButton);

   // convenient show method
   static void showMessage(const QString& haeder, const QString& message, bool check=false, const QString& checkText = QString::fromLatin1("Don't show me again"));

private Q_SLOTS:
   void slotClicked(QAbstractButton *b);

private:
   DlgCheckableMessageBoxPrivate *m_d;
   QByteArray paramEntry;
};

} // namespace Dialog
} // namespace Gui

#endif // CHECKABLEMESSAGEBOX_H
