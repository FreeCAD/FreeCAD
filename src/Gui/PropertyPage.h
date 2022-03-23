/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DIALOG_PROPERTYPAGE_H
#define GUI_DIALOG_PROPERTYPAGE_H

#include <QWidget>
#include <FCGlobal.h>

namespace Gui {
namespace Dialog {

/** Base class for property pages.
 * \author Werner Mayer
 */
class GuiExport PropertyPage : public QWidget
{
    Q_OBJECT

public:
    PropertyPage(QWidget* parent = nullptr);
    virtual ~PropertyPage();

    bool isModified();
    void setModified(bool b);
    void onApply();
    void onCancel();
    void onReset();

protected:
    virtual void apply();
    virtual void cancel();
    virtual void reset();

private:
    bool bChanged; /**< for internal use only */

protected Q_SLOTS:
    virtual void loadSettings()=0;
    virtual void saveSettings()=0;
};

/** Base class for preferences pages.
 * \author Werner Mayer
 */
class GuiExport PreferencePage : public QWidget
{
    Q_OBJECT

public:
    PreferencePage(QWidget* parent = nullptr);
    virtual ~PreferencePage();

public Q_SLOTS:
    virtual void loadSettings()=0;
    virtual void saveSettings()=0;

protected:
    virtual void changeEvent(QEvent *e) = 0;
};

/** Subclass that embeds a form from a UI file.
 * \author Werner Mayer
 */
class GuiExport PreferenceUiForm : public PreferencePage
{
    Q_OBJECT

public:
    PreferenceUiForm(const QString& fn, QWidget* parent = nullptr);
    virtual ~PreferenceUiForm();

    void loadSettings();
    void saveSettings();

protected:
    void changeEvent(QEvent *e);

private:
    template <typename PW>
    void loadPrefWidgets();
    template <typename PW>
    void savePrefWidgets();

private:
    QWidget* form;
};

/** Base class for custom pages.
 * \author Werner Mayer
 */
class GuiExport CustomizeActionPage : public QWidget
{
    Q_OBJECT

public:
    CustomizeActionPage(QWidget* parent = nullptr);
    virtual ~CustomizeActionPage();

protected:
    bool event(QEvent* e);
    virtual void changeEvent(QEvent *e) = 0;

protected Q_SLOTS:
    virtual void onAddMacroAction(const QByteArray&)=0;
    virtual void onRemoveMacroAction(const QByteArray&)=0;
    virtual void onModifyMacroAction(const QByteArray&)=0;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_PROPERTYPAGE_H
