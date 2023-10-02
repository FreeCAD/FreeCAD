// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>      *
 *   Copyright (c) 2023 FreeCAD Project Association                         *
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
# include <QVBoxLayout>
#endif

#include <Base/Console.h>

#include "PropertyPage.h"
#include "PrefWidgets.h"
#include "UiLoader.h"


using namespace Gui::Dialog;

/** Construction */
PropertyPage::PropertyPage(QWidget* parent)
    : QWidget(parent)
    , bChanged{false}
{
}

/** Applies all changes. Reimplement this in your subclasses. */
void PropertyPage::apply()
{
}

/** Discards all changes. Reimplement this in your subclasses. */
void PropertyPage::cancel()
{
}

/** Resets to the default values. Reimplement this in your subclasses. */
void PropertyPage::reset()
{
}

/** Returns whether the page was modified or not. */
bool PropertyPage::isModified() const
{
    return bChanged;
}

/** Sets the page to be modified. */
void PropertyPage::setModified(bool value)
{
    bChanged = value;
}

/** Applies all changes calling @ref apply() and resets the modified state. */
void PropertyPage::onApply()
{
    if (isModified()) {
        apply();
    }

    setModified(false);
}

/** Discards all changes calling @ref cancel() and resets the modified state. */
void PropertyPage::onCancel()
{
    if (isModified()) {
        cancel();
        setModified(false);
    }
}

/** Resets to the default values. */
void PropertyPage::onReset()
{
    reset();
}

// ----------------------------------------------------------------

/** Construction */
PreferencePage::PreferencePage(QWidget* parent) : QWidget(parent), restartRequired(false)
{
}

void PreferencePage::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
}

bool PreferencePage::isRestartRequired() const
{
    return restartRequired;
}

void PreferencePage::requireRestart()
{
    restartRequired = true;
}



// ----------------------------------------------------------------

PreferenceUiForm::PreferenceUiForm(const QString& fn, QWidget* parent)
  : PreferencePage(parent)
  , form(nullptr)
{
    auto loader = UiLoader::newInstance();
    loader->setWorkingDirectory(QFileInfo(fn).absolutePath());
    QFile file(fn);
    if (file.open(QFile::ReadOnly)) {
        form = loader->load(&file, this);
    }
    file.close();
    if (form) {
        this->setWindowTitle(form->windowTitle());
        auto layout = new QVBoxLayout;
        layout->addWidget(form);
        setLayout(layout);
    }
    else {
        Base::Console().Error("Failed to load UI file from '%s'\n",
            (const char*)fn.toUtf8());
    }
}

PreferenceUiForm::~PreferenceUiForm() = default;

void PreferenceUiForm::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
}

template <typename PW>
void PreferenceUiForm::loadPrefWidgets()
{
    QList<PW> pw = form->findChildren<PW>();
    for (typename QList<PW>::iterator it = pw.begin(); it != pw.end(); ++it)
        (*it)->onRestore();
}

template <typename PW>
void PreferenceUiForm::savePrefWidgets()
{
    QList<PW> pw = form->findChildren<PW>();
    for (typename QList<PW>::iterator it = pw.begin(); it != pw.end(); ++it)
        (*it)->onSave();
}

void PreferenceUiForm::loadSettings()
{
    if (!form)
        return;

    // search for all pref widgets to restore their settings
    loadPrefWidgets<Gui::PrefSpinBox        *>();
    loadPrefWidgets<Gui::PrefDoubleSpinBox  *>();
    loadPrefWidgets<Gui::PrefLineEdit       *>();
    loadPrefWidgets<Gui::PrefTextEdit       *>();
    loadPrefWidgets<Gui::PrefFileChooser    *>();
    loadPrefWidgets<Gui::PrefComboBox       *>();
    loadPrefWidgets<Gui::PrefFontBox        *>();
    loadPrefWidgets<Gui::PrefCheckBox       *>();
    loadPrefWidgets<Gui::PrefRadioButton    *>();
    loadPrefWidgets<Gui::PrefSlider         *>();
    loadPrefWidgets<Gui::PrefColorButton    *>();
    loadPrefWidgets<Gui::PrefUnitSpinBox    *>();
    loadPrefWidgets<Gui::PrefQuantitySpinBox*>();
}

void PreferenceUiForm::saveSettings()
{
    if (!form)
        return;

    // search for all pref widgets to save their settings
    savePrefWidgets<Gui::PrefSpinBox        *>();
    savePrefWidgets<Gui::PrefDoubleSpinBox  *>();
    savePrefWidgets<Gui::PrefLineEdit       *>();
    savePrefWidgets<Gui::PrefTextEdit       *>();
    savePrefWidgets<Gui::PrefFileChooser    *>();
    savePrefWidgets<Gui::PrefComboBox       *>();
    savePrefWidgets<Gui::PrefFontBox        *>();
    savePrefWidgets<Gui::PrefCheckBox       *>();
    savePrefWidgets<Gui::PrefRadioButton    *>();
    savePrefWidgets<Gui::PrefSlider         *>();
    savePrefWidgets<Gui::PrefColorButton    *>();
    savePrefWidgets<Gui::PrefUnitSpinBox    *>();
    savePrefWidgets<Gui::PrefQuantitySpinBox*>();
}

// ----------------------------------------------------------------

/** Construction */
CustomizeActionPage::CustomizeActionPage(QWidget* parent) : QWidget(parent)
{
}

/** Destruction */
CustomizeActionPage::~CustomizeActionPage() = default;

bool CustomizeActionPage::event(QEvent* e)
{
    bool ok = QWidget::event(e);

    if (e->type() == QEvent::ParentChange || e->type() == QEvent::ParentAboutToChange) {
        QWidget* topLevel = this->parentWidget();
        while (topLevel && !topLevel->inherits("QDialog"))
            topLevel = topLevel->parentWidget();
        if (topLevel) {
            int index = topLevel->metaObject()->indexOfSignal( QMetaObject::normalizedSignature("addMacroAction(const QByteArray&)") );
            if (index >= 0) {
                if (e->type() == QEvent::ParentChange) {
                    connect(topLevel, SIGNAL(addMacroAction( const QByteArray& )),
                            this, SLOT(onAddMacroAction( const QByteArray& )));
                    connect(topLevel, SIGNAL(removeMacroAction( const QByteArray& )),
                            this, SLOT(onRemoveMacroAction( const QByteArray& )));
                    connect(topLevel, SIGNAL(modifyMacroAction( const QByteArray& )),
                            this, SLOT(onModifyMacroAction( const QByteArray& )));
                }
                else {
                    disconnect(topLevel, SIGNAL(addMacroAction( const QByteArray& )),
                               this, SLOT(onAddMacroAction( const QByteArray& )));
                    disconnect(topLevel, SIGNAL(removeMacroAction( const QByteArray& )),
                               this, SLOT(onRemoveMacroAction( const QByteArray& )));
                    disconnect(topLevel, SIGNAL(modifyMacroAction( const QByteArray& )),
                               this, SLOT(onModifyMacroAction( const QByteArray& )));
                }
            }
        }
    }

    return ok;
}

void CustomizeActionPage::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
}

#include "moc_PropertyPage.cpp"
