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

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QTextEdit>

#include <Base/Parameter.h>

#include "Widgets.h"
#include "FileDialog.h"
#include "SpinBox.h"
#include "QuantitySpinBox.h"
#include "Window.h"


namespace Gui
{
class CommandManager;
class WidgetFactoryInst;

/** The preference widget class.
 * If you want to extend a QWidget class to save/restore its data
 * you just have to derive from this class and implement the methods
 * restorePreferences() and savePreferences().
 *
 * To restore and save the settings of any widgets in own dialogs you have
 * call onRestore() e.g. in the dialog's constructor and call onSave() e.g.
 * in accept() for each widget you want to enable this mechanism.
 *
 * For more information of how to use these widgets in normal container widgets
 * which are again in a dialog refer to the description of Gui::Dialog::DlgPreferencesImp.
 *
 * \author Werner Mayer
 */
class GuiExport PrefWidget: public WindowParameter
{
public:
    void setEntryName(const QByteArray& name);
    QByteArray entryName() const;
    /** Does the same as setEntryName().
     * This function is added for convenience because the ui compiler
     * will use this function if the attribute stdset isn't set to 0 in a .ui file.
     */
    void setPrefEntry(const QByteArray& name);

    void setParamGrpPath(const QByteArray& path);
    QByteArray paramGrpPath() const;
    /** Does the same as setParamGrpPath().
     * This function is added for convenience because the ui compiler
     * will use this function if the attribute stdset isn't set to 0 in a .ui file.
     */
    void setPrefPath(const QByteArray& name);

    void OnChange(Base::Subject<const char*>& rCaller, const char* sReason) override;
    void onSave();
    void onRestore();

protected:
    /** Restores the preferences
     * Must be reimplemented in any subclasses.
     */
    virtual void restorePreferences() = 0;
    /** Save the preferences
     * Must be reimplemented in any subclasses.
     */
    virtual void savePreferences() = 0;
    /** Print warning that saving failed.
     */
    void failedToSave(const QString&) const;
    /** Print warning that restoring failed.
     */
    void failedToRestore(const QString&) const;

    PrefWidget();
    ~PrefWidget() override;

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;

    // friends
    friend class Gui::WidgetFactoryInst;

protected:
    bool m_Restored = false;
};

/** The PrefSpinBox class.
 * \author Werner Mayer
 */
class GuiExport PrefSpinBox: public QSpinBox, public PrefWidget
{
    Q_OBJECT

    Q_PROPERTY(
        QByteArray prefEntry READ entryName WRITE setEntryName
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath
    )  // clazy:exclude=qproperty-without-notify

public:
    explicit PrefSpinBox(QWidget* parent = nullptr);
    ~PrefSpinBox() override;

protected:
    void wheelEvent(QWheelEvent* event) override;
    // restore from/save to parameters
    void restorePreferences() override;
    void savePreferences() override;
};

/** The PrefDoubleSpinBox class.
 * \author Werner Mayer
 */
class GuiExport PrefDoubleSpinBox: public QDoubleSpinBox, public PrefWidget
{
    Q_OBJECT

    Q_PROPERTY(
        QByteArray prefEntry READ entryName WRITE setEntryName
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath
    )  // clazy:exclude=qproperty-without-notify

public:
    explicit PrefDoubleSpinBox(QWidget* parent = nullptr);
    ~PrefDoubleSpinBox() override;

protected:
    void wheelEvent(QWheelEvent* event) override;
    // restore from/save to parameters
    void restorePreferences() override;
    void savePreferences() override;
};

/**
 * The PrefLineEdit class.
 * \author Werner Mayer
 */
class GuiExport PrefLineEdit: public QLineEdit, public PrefWidget
{
    Q_OBJECT

    Q_PROPERTY(
        QByteArray prefEntry READ entryName WRITE setEntryName
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath
    )  // clazy:exclude=qproperty-without-notify

public:
    explicit PrefLineEdit(QWidget* parent = nullptr);
    ~PrefLineEdit() override;

protected:
    // restore from/save to parameters
    void restorePreferences() override;
    void savePreferences() override;
};

/**
 * The PrefLineEdit class.
 * \author Chris Hennes
 */
class GuiExport PrefTextEdit: public QTextEdit, public PrefWidget
{
    Q_OBJECT

    Q_PROPERTY(
        QByteArray prefEntry READ entryName WRITE setEntryName
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath
    )  // clazy:exclude=qproperty-without-notify

public:
    explicit PrefTextEdit(QWidget* parent = nullptr);
    ~PrefTextEdit() override;

protected:
    // restore from/save to parameters
    void restorePreferences() override;
    void savePreferences() override;
};

/**
 * The PrefFileChooser class.
 * \author Werner Mayer
 */
class GuiExport PrefFileChooser: public FileChooser, public PrefWidget
{
    Q_OBJECT

    Q_PROPERTY(
        QByteArray prefEntry READ entryName WRITE setEntryName
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath
    )  // clazy:exclude=qproperty-without-notify

public:
    explicit PrefFileChooser(QWidget* parent = nullptr);
    ~PrefFileChooser() override;

protected:
    // restore from/save to parameters
    void restorePreferences() override;
    void savePreferences() override;
};

/**
 * The PrefComboBox class.
 * \author Werner Mayer
 *
 * The PrefComboBox supports restoring/saving variant type of item data. You
 * can add a property named 'prefType' with the type you want. If no such
 * property is found, the class defaults to restore/save the item index.
 *
 * Note that there is special handling for 'prefType' of QString, which means
 * to restore/save the item text. This allows the combox to be editable, and
 * accepts user entered value. Use QByteArray if you want to restore/save a
 * non translatable string stored as item data.
 */
class GuiExport PrefComboBox: public QComboBox, public PrefWidget
{
    Q_OBJECT

    Q_PROPERTY(
        QByteArray prefEntry READ entryName WRITE setEntryName
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath
    )  // clazy:exclude=qproperty-without-notify

public:
    explicit PrefComboBox(QWidget* parent = nullptr);
    ~PrefComboBox() override;

protected:
    void wheelEvent(QWheelEvent* event) override;
    // restore from/save to parameters
    void restorePreferences() override;
    void savePreferences() override;
    virtual QMetaType::Type getParamType() const;

private:
    QVariant m_Default;
    int m_DefaultIndex;
    QString m_DefaultText;
};

/**
 * The PrefCheckBox class.
 * \author Werner Mayer
 */
class GuiExport PrefCheckBox: public QCheckBox, public PrefWidget
{
    Q_OBJECT

    Q_PROPERTY(
        QByteArray prefEntry READ entryName WRITE setEntryName
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath
    )  // clazy:exclude=qproperty-without-notify

public:
    explicit PrefCheckBox(QWidget* parent = nullptr);
    ~PrefCheckBox() override;

protected:
    // restore from/save to parameters
    void restorePreferences() override;
    void savePreferences() override;
};

/**
 * The PrefRadioButton class.
 * \author Werner Mayer
 */
class GuiExport PrefRadioButton: public QRadioButton, public PrefWidget
{
    Q_OBJECT

    Q_PROPERTY(
        QByteArray prefEntry READ entryName WRITE setEntryName
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath
    )  // clazy:exclude=qproperty-without-notify

public:
    explicit PrefRadioButton(QWidget* parent = nullptr);
    ~PrefRadioButton() override;

protected:
    // restore from/save to parameters
    void restorePreferences() override;
    void savePreferences() override;
};

/**
 * The PrefSlider class.
 * \author Werner Mayer
 */
class GuiExport PrefSlider: public QSlider, public PrefWidget
{
    Q_OBJECT

    Q_PROPERTY(
        QByteArray prefEntry READ entryName WRITE setEntryName
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath
    )  // clazy:exclude=qproperty-without-notify

public:
    explicit PrefSlider(QWidget* parent = nullptr);
    ~PrefSlider() override;

protected:
    // restore from/save to parameters
    void restorePreferences() override;
    void savePreferences() override;
};

/**
 * The PrefColorButton class.
 * \author Werner Mayer
 */
class GuiExport PrefColorButton: public ColorButton, public PrefWidget
{
    Q_OBJECT

    Q_PROPERTY(
        QByteArray prefEntry READ entryName WRITE setEntryName
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath
    )  // clazy:exclude=qproperty-without-notify

public:
    explicit PrefColorButton(QWidget* parent = nullptr);
    ~PrefColorButton() override;

protected:
    // restore from/save to parameters
    void restorePreferences() override;
    void savePreferences() override;

private:
    QColor m_Default;
};

/** The PrefUnitSpinBox class.
 * \author wandererfan
 * a simple Unit aware spin box.
 * See also \ref PrefQuantitySpinBox
 */
class GuiExport PrefUnitSpinBox: public QuantitySpinBox, public PrefWidget
{
    Q_OBJECT

    Q_PROPERTY(
        QByteArray prefEntry READ entryName WRITE setEntryName
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath
    )  // clazy:exclude=qproperty-without-notify

public:
    explicit PrefUnitSpinBox(QWidget* parent = nullptr);
    ~PrefUnitSpinBox() override;

protected:
    void wheelEvent(QWheelEvent* event) override;
    // restore from/save to parameters
    void restorePreferences() override;
    void savePreferences() override;
};

class PrefQuantitySpinBoxPrivate;

/**
 * The PrefQuantitySpinBox class.
 * \author Werner Mayer
 */
class GuiExport PrefQuantitySpinBox: public QuantitySpinBox, public PrefWidget
{
    Q_OBJECT

    Q_PROPERTY(
        QByteArray prefEntry READ entryName WRITE setEntryName
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        int historySize READ historySize WRITE setHistorySize
    )  // clazy:exclude=qproperty-without-notify

public:
    explicit PrefQuantitySpinBox(QWidget* parent = nullptr);
    ~PrefQuantitySpinBox() override;

    /// set the input field to the last used value (works only if the setParamGrpPath() was called)
    void setToLastUsedValue();
    /// get the value of the history size property
    int historySize() const;
    /// set the value of the history size property
    void setHistorySize(int);

    /** @name history and default management */
    //@{
    /// push a new value to the history, if no string given the actual text of the input field is used.
    void pushToHistory(const QString& value = QString());
    /// get the history of the field, newest first
    QStringList getHistory() const;
    //@}

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    // restore from/save to parameters
    void restorePreferences() override;
    void savePreferences() override;

private:
    QScopedPointer<PrefQuantitySpinBoxPrivate> d_ptr;
    Q_DISABLE_COPY(PrefQuantitySpinBox)
    Q_DECLARE_PRIVATE(PrefQuantitySpinBox)
};

/** The PrefFontBox class.
 * \author wandererfan
 */
class GuiExport PrefFontBox: public QFontComboBox, public PrefWidget
{
    Q_OBJECT

    Q_PROPERTY(
        QByteArray prefEntry READ entryName WRITE setEntryName
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath
    )  // clazy:exclude=qproperty-without-notify

public:
    explicit PrefFontBox(QWidget* parent = nullptr);
    ~PrefFontBox() override;

protected:
    // restore from/save to parameters
    void restorePreferences() override;
    void savePreferences() override;
};

/**
 * The PrefCheckableGroupBox class allows a QGroupBox to act as a boolean preference.
 * Its 'checked' state is saved to and restored from the FreeCAD parameter system
 * using the 'prefEntry' and 'prefPath' dynamic properties set in the .ui file.
 * When the GroupBox is checked, its children are enabled; when unchecked, disabled (standard
 * QGroupBox behavior).
 */
class GuiExport PrefCheckableGroupBox: public QGroupBox, public PrefWidget
{
    Q_OBJECT

    Q_PROPERTY(
        QByteArray prefEntry READ entryName WRITE setEntryName
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath
    )  // clazy:exclude=qproperty-without-notify

public:
    explicit PrefCheckableGroupBox(QWidget* parent = nullptr);
    ~PrefCheckableGroupBox() override;

protected:
    // restore from/save to parameters
    void restorePreferences() override;
    void savePreferences() override;
};

}  // namespace Gui
