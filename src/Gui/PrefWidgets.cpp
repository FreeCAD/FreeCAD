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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QContextMenuEvent>
# include <QMenu>
#endif

#include <cstring>

#include <Base/Console.h>
#include <Base/Tools.h>

#include "PrefWidgets.h"


using Base::Console;
using namespace Gui;

/** Constructs a preference widget.
 */
PrefWidget::PrefWidget()
 : WindowParameter("")
{
}

/**
 * Destroys the widget and detaches it from its parameter group.
 */
PrefWidget::~PrefWidget()
{
  if (getWindowParameter().isValid())
    getWindowParameter()->Detach(this);
}

/** Sets the preference name to \a name. */
void PrefWidget::setEntryName( const QByteArray& name )
{
  m_sPrefName = name;
}

/** Sets the preference name to \a name. */
void PrefWidget::setPrefEntry(const QByteArray& name)
{
  setEntryName(name);
}

/** Returns the widget's preference name. */
QByteArray PrefWidget::entryName() const
{
  return m_sPrefName;
}

/** Sets the preference path to \a path. */
void PrefWidget::setParamGrpPath( const QByteArray& path )
{
#ifdef FC_DEBUG
  if (getWindowParameter().isValid())
  {
    if ( paramGrpPath() != path )
      Base::Console().Warning("Widget already attached\n");
  }
#endif

  if ( paramGrpPath() != path )
  {
    if ( setGroupName( path ) )
    {
      m_sPrefGrp = path;
      assert(getWindowParameter().isValid());
      getWindowParameter()->Attach(this);
    }
  }
}

/** Sets the preference path to \a path. */
void PrefWidget::setPrefPath(const QByteArray& name)
{
  setParamGrpPath(name);
}

/** Returns the widget's preferences path. */
QByteArray PrefWidget::paramGrpPath() const
{
  return m_sPrefGrp;
}

/**
 * This method is called if one or more values in the parameter settings are changed
 * where getParamGrp() points to.
 * Note: This method is called for each parameter inside the parameter group. So
 * you have to filter out the appropriate parameter with the name \a sReason.
 * \a rCaller calls this method.
 */
void PrefWidget::OnChange(Base::Subject<const char*> &rCaller, const char * sReason)
{
    Q_UNUSED(rCaller);
    if (std::strcmp(sReason,m_sPrefName) == 0)
        restorePreferences();
}

/**
 * Saves the current preferences of the widget.
 * All preference widget attached to the same parameter group are notified.
 */
void PrefWidget::onSave()
{
  savePreferences();
  if (getWindowParameter().isValid())
    getWindowParameter()->Notify( entryName() );
#ifdef FC_DEBUG
  else
    qFatal( "No parameter group specified!" );
#endif
}

/**
 * Restores the preferences of the widget.
 */
void PrefWidget::onRestore()
{
#ifdef FC_DEBUG
  if (getWindowParameter().isNull())
    qWarning( "No parameter group specified!" );
#endif
  restorePreferences();
}

void PrefWidget::failedToSave(const QString& name) const
{
    QByteArray objname = name.toLatin1();
    if (objname.isEmpty())
        objname = "Undefined";
    Console().Warning("Cannot save %s (%s)\n", typeid(*this).name(), objname.constData());
}

void PrefWidget::failedToRestore(const QString& name) const
{
    QByteArray objname = name.toLatin1();
    if (objname.isEmpty())
        objname = "Undefined";
    Console().Warning("Cannot restore %s (%s)\n", typeid(*this).name(), objname.constData());
}

// --------------------------------------------------------------------

PrefSpinBox::PrefSpinBox ( QWidget * parent )
  : QSpinBox(parent), PrefWidget()
{
}

PrefSpinBox::~PrefSpinBox()
{
}

void PrefSpinBox::restorePreferences()
{
  if ( getWindowParameter().isNull() )
  {
    failedToRestore(objectName());
    return;
  }

  int nVal = getWindowParameter()->GetInt( entryName(), QSpinBox::value() );
  setValue( nVal );
}

void PrefSpinBox::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToSave(objectName());
    return;
  }

  getWindowParameter()->SetInt( entryName() , (int)value() );
}

// --------------------------------------------------------------------

PrefDoubleSpinBox::PrefDoubleSpinBox ( QWidget * parent )
  : QDoubleSpinBox(parent), PrefWidget()
{
}

PrefDoubleSpinBox::~PrefDoubleSpinBox()
{
}

void PrefDoubleSpinBox::restorePreferences()
{
  if ( getWindowParameter().isNull() )
  {
    failedToRestore(objectName());
    return;
  }

  double fVal = (double)getWindowParameter()->GetFloat( entryName() , value() );
  setValue(fVal);
}

void PrefDoubleSpinBox::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToSave(objectName());
    return;
  }

  getWindowParameter()->SetFloat( entryName(), value() );
}

// --------------------------------------------------------------------

PrefLineEdit::PrefLineEdit ( QWidget * parent )
  : QLineEdit(parent), PrefWidget()
{
}

PrefLineEdit::~PrefLineEdit()
{
}

void PrefLineEdit::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToRestore(objectName());
    return;
  }

  QString text = this->text();
  text = QString::fromUtf8(getWindowParameter()->GetASCII(entryName(), text.toUtf8()).c_str());
  setText(text);
}

void PrefLineEdit::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToSave(objectName());
    return;
  }

  getWindowParameter()->SetASCII(entryName(), text().toUtf8());
}

// --------------------------------------------------------------------

PrefTextEdit::PrefTextEdit(QWidget* parent)
    : QTextEdit(parent), PrefWidget()
{
}

PrefTextEdit::~PrefTextEdit()
{
}

void PrefTextEdit::restorePreferences()
{
    if (getWindowParameter().isNull())
    {
        failedToRestore(objectName());
        return;
    }

    QString text = this->toPlainText();
    text = QString::fromUtf8(getWindowParameter()->GetASCII(entryName(), text.toUtf8()).c_str());
    setText(text);
}

void PrefTextEdit::savePreferences()
{
    if (getWindowParameter().isNull())
    {
        failedToSave(objectName());
        return;
    }

    QString text = this->toPlainText();
    getWindowParameter()->SetASCII(entryName(), text.toUtf8());
}

// --------------------------------------------------------------------

PrefFileChooser::PrefFileChooser ( QWidget * parent )
  : FileChooser(parent), PrefWidget()
{
}

PrefFileChooser::~PrefFileChooser()
{
}

void PrefFileChooser::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToRestore(objectName());
    return;
  }

  QString txt = QString::fromUtf8(getWindowParameter()->GetASCII(entryName(), fileName().toUtf8()).c_str());
  setFileName(txt);
}

void PrefFileChooser::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToSave(objectName());
    return;
  }

  getWindowParameter()->SetASCII(entryName(), fileName().toUtf8());
}

// --------------------------------------------------------------------

PrefComboBox::PrefComboBox ( QWidget * parent )
  : QComboBox(parent), PrefWidget()
{
}

PrefComboBox::~PrefComboBox()
{
}

void PrefComboBox::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToRestore(objectName());
    return;
  }

  int index = getWindowParameter()->GetInt(entryName(), currentIndex());
  setCurrentIndex(index);
}

void PrefComboBox::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToSave(objectName());
    return;
  }

  getWindowParameter()->SetInt(entryName() , currentIndex());
}

// --------------------------------------------------------------------

PrefCheckBox::PrefCheckBox ( QWidget * parent )
  : QCheckBox(parent), PrefWidget()
{
}

PrefCheckBox::~PrefCheckBox()
{
}

void PrefCheckBox::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToRestore(objectName());
    return;
  }

  bool enable = getWindowParameter()->GetBool( entryName(), isChecked() );
  setChecked(enable);
}

void PrefCheckBox::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToSave(objectName());
    return;
  }

  getWindowParameter()->SetBool( entryName(), isChecked() );
}

// --------------------------------------------------------------------

PrefRadioButton::PrefRadioButton ( QWidget * parent )
  : QRadioButton(parent), PrefWidget()
{
}

PrefRadioButton::~PrefRadioButton()
{
}

void PrefRadioButton::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToRestore(objectName());
    return;
  }

  bool enable = getWindowParameter()->GetBool( entryName(), isChecked() );
  setChecked(enable);
}

void PrefRadioButton::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToSave(objectName());
    return;
  }

  getWindowParameter()->SetBool( entryName() , isChecked() );
}

// --------------------------------------------------------------------

PrefSlider::PrefSlider ( QWidget * parent )
  : QSlider(parent), PrefWidget()
{
}

PrefSlider::~PrefSlider()
{
}

void PrefSlider::restorePreferences()
{
  if ( getWindowParameter().isNull() )
  {
    failedToRestore(objectName());
    return;
  }

  int nVal = getWindowParameter()->GetInt(entryName(), QSlider::value());
  setValue(nVal);
}

void PrefSlider::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToSave(objectName());
    return;
  }

  getWindowParameter()->SetInt(entryName() , (int)value());
}

// --------------------------------------------------------------------

PrefColorButton::PrefColorButton ( QWidget * parent )
  : ColorButton(parent), PrefWidget()
{
}

PrefColorButton::~PrefColorButton()
{
}

void PrefColorButton::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToRestore(objectName());
    return;
  }

  QColor col = color();

  unsigned int icol = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);

  unsigned long lcol = static_cast<unsigned long>(icol);
  lcol = getWindowParameter()->GetUnsigned( entryName(), lcol );
  icol = static_cast<unsigned int>(lcol);
  int r = (icol >> 24)&0xff;
  int g = (icol >> 16)&0xff;
  int b = (icol >>  8)&0xff;

  setColor(QColor(r,g,b));
}

void PrefColorButton::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToSave(objectName());
    return;
  }

  QColor col = color();
  // (r,g,b,a) with a = 255 (opaque)
  unsigned int icol = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8) | 255;
  unsigned long lcol = static_cast<unsigned long>(icol);
  getWindowParameter()->SetUnsigned( entryName(), lcol );
}

// --------------------------------------------------------------------

PrefUnitSpinBox::PrefUnitSpinBox ( QWidget * parent )
  : QuantitySpinBox(parent), PrefWidget()
{
}

PrefUnitSpinBox::~PrefUnitSpinBox()
{
}

void PrefUnitSpinBox::restorePreferences()
{
    if (getWindowParameter().isNull()) {
        failedToRestore(objectName());
        return;
    }

    double fVal = (double)getWindowParameter()->GetFloat( entryName() ,rawValue() );
    setValue(fVal);
}

void PrefUnitSpinBox::savePreferences()
{
    if (getWindowParameter().isNull()) {
        failedToSave(objectName());
        return;
    }

    double q = rawValue();
    getWindowParameter()->SetFloat( entryName(), q );
}

// --------------------------------------------------------------------

namespace Gui {
class HistoryList {
    QStringList list;
    int max_size = 5;
public:
    const QStringList& asStringList() const {
        return list;
    }
    int maximumSize() const {
        return max_size;
    }
    void setMaximumSize(int num) {
        max_size = num;
        while (list.size() > num)
            list.pop_front();
    }
    void clear() {
        list.clear();
    }
    void append(const QString& value) {
        if (!list.isEmpty() && list.back() == value)
            return;
        auto it = std::find(list.begin(), list.end(), value);
        if (it != list.end())
            list.erase(it);
        else if (list.size() == max_size)
            list.pop_front();
        list.push_back(value);
    }
};

class PrefQuantitySpinBoxPrivate
{
public:
    HistoryList history;
    bool isSaving = false;

    QByteArray getHistoryGroupName(QByteArray name) const {
        return name + "_History";
    }

    void restoreHistory(ParameterGrp::handle hGrp) {
        std::vector<std::string> hist = hGrp->GetASCIIs("Hist");
        for (const auto& it : hist)
            history.append(QString::fromStdString(it));
    }
    void clearHistory(ParameterGrp::handle hGrp) {
        std::vector<std::string> hist = hGrp->GetASCIIs("Hist");
        for (const auto& it : hist)
            hGrp->RemoveASCII(it.c_str());
    }
    void saveHistory(ParameterGrp::handle hGrp) {
        clearHistory(hGrp);

        const QStringList& list = history.asStringList();
        for (int i = 0; i < list.size(); i++) {
            QByteArray key("Hist");
            key.append(QByteArray::number(i));
            hGrp->SetASCII(key, list[i].toUtf8());
        }
    }
};
}

PrefQuantitySpinBox::PrefQuantitySpinBox (QWidget * parent)
  : QuantitySpinBox(parent)
  , d_ptr(new PrefQuantitySpinBoxPrivate())
{
}

PrefQuantitySpinBox::~PrefQuantitySpinBox()
{
}

void PrefQuantitySpinBox::contextMenuEvent(QContextMenuEvent *event)
{
    Q_D(PrefQuantitySpinBox);

    QMenu *editMenu = lineEdit()->createStandardContextMenu();
    editMenu->setTitle(tr("Edit"));
    std::unique_ptr<QMenu> menu(new QMenu(QString::fromLatin1("PrefQuantitySpinBox")));

    menu->addMenu(editMenu);
    menu->addSeparator();

    // data structure to remember actions for values
    QStringList history = d->history.asStringList();
    for (QStringList::const_iterator it = history.begin();it != history.end(); ++it) {
        QAction* action = menu->addAction(*it);
        action->setProperty("history_value", *it);
    }

    // add the save value portion of the menu
    menu->addSeparator();
    QAction *saveValueAction = menu->addAction(tr("Save value"));
    QAction *clearListAction = menu->addAction(tr("Clear list"));
    clearListAction->setDisabled(history.empty());

    // call the menu
    QAction *userAction = menu->exec(event->globalPos());

    // look what the user has chosen
    if (userAction == saveValueAction) {
        pushToHistory(this->text());
    }
    else if (userAction == clearListAction) {
        d->history.clear();
    }
    else if (userAction) {
        QVariant prop = userAction->property("history_value");
        if (prop.isValid()) {
            lineEdit()->setText(prop.toString());
        }
    }
}

void PrefQuantitySpinBox::restorePreferences()
{
    Q_D(PrefQuantitySpinBox);

    // Do not restore values while saving them
    if (d->isSaving)
        return;

    if (getWindowParameter().isNull() || entryName().isEmpty()) {
        failedToRestore(objectName());
        return;
    }

    QString text = this->text();
    text = QString::fromUtf8(getWindowParameter()->GetASCII(entryName(), text.toUtf8()).c_str());
    lineEdit()->setText(text);

    // Restore history
    auto hGrp = getWindowParameter()->GetGroup(d->getHistoryGroupName(entryName()));
    d->restoreHistory(hGrp);
}

void PrefQuantitySpinBox::savePreferences()
{
    Q_D(PrefQuantitySpinBox);
    if (getWindowParameter().isNull() || entryName().isEmpty()) {
        failedToSave(objectName());
        return;
    }

    getWindowParameter()->SetASCII( entryName(), text().toUtf8() );

    // Save history
    auto hGrp = getWindowParameter()->GetGroup(d->getHistoryGroupName(entryName()));
    d->saveHistory(hGrp);
}

void PrefQuantitySpinBox::pushToHistory(const QString &value)
{
    Q_D(PrefQuantitySpinBox);
    d->history.append(value.isEmpty() ? this->text() : value);

    Base::StateLocker lock(d->isSaving);
    onSave();
}

QStringList PrefQuantitySpinBox::getHistory() const
{
    Q_D(const PrefQuantitySpinBox);
    return d->history.asStringList();
}

void PrefQuantitySpinBox::setToLastUsedValue()
{
    QStringList hist = getHistory();
    if (!hist.empty())
        lineEdit()->setText(hist.front());
}

int PrefQuantitySpinBox::historySize() const
{
    Q_D(const PrefQuantitySpinBox);
    return d->history.maximumSize();
}

void PrefQuantitySpinBox::setHistorySize(int i)
{
    Q_D(PrefQuantitySpinBox);
    d->history.setMaximumSize(i);
}

// --------------------------------------------------------------------

PrefFontBox::PrefFontBox ( QWidget * parent )
  : QFontComboBox(parent), PrefWidget()
{
}

PrefFontBox::~PrefFontBox()
{
}

void PrefFontBox::restorePreferences()
{
  if ( getWindowParameter().isNull() )
  {
    failedToRestore(objectName());
    return;
  }

  QFont currFont = currentFont();                         //QFont from selector widget
  QString currName = currFont.family();

  std::string prefName = getWindowParameter()->GetASCII(entryName(), currName.toUtf8());  //font name from cfg file

  currFont.setFamily(QString::fromStdString(prefName));
  setCurrentFont(currFont);                               //set selector widget to name from cfg file
}

void PrefFontBox::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToSave(objectName());
    return;
  }

  QFont currFont = currentFont();
  QString currName = currFont.family();
  getWindowParameter()->SetASCII( entryName() , currName.toUtf8() );
}

#include "moc_PrefWidgets.cpp"
