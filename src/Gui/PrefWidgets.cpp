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
# include <QMessageBox>
#endif

#include <QWidgetAction>

#include <cstring>
#include <boost/algorithm/string/predicate.hpp>

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/Exception.h>
#include <App/Application.h>

#include "PrefWidgets.h"
#include "FileDialog.h"

using Base::Console;
using namespace Gui;

PrefParam::PrefParam() {
  hGrp = App::GetApplication().GetParameterGroupByPath(
      "User parameter:BaseApp/Preferences/General");
  _AutoSave = hGrp->GetBool("AutoApplyPreference", true);
  hGrp->Attach(this);
}

PrefParam::~PrefParam() {
  hGrp->Detach(this);
}

void PrefParam::OnChange(Base::Subject<const char*> &, const char * sReason) {
  if (boost::equals(sReason, "AutoApplyPreference")) {
    _AutoSave = hGrp->GetBool("AutoApplyPreference", true);
    for (auto entry : _entries)
      entry->setAutoSave(_AutoSave);
  }
}

void PrefParam::addEntry(PrefWidget *entry) {
  instance()->_entries.insert(entry);
}

void PrefParam::removeEntry(PrefWidget *entry) {
  instance()->_entries.erase(entry);
}

bool PrefParam::AutoSave() {
  return instance()->_AutoSave;
}

void PrefParam::setAutoSave(bool enable) {
  if (enable != AutoSave())
    instance()->hGrp->GetBool("AutoApplyPreference", enable);
}

PrefParam *PrefParam::instance() {
  static PrefParam *_instance;
  if (!_instance)
    _instance = new PrefParam;
  return _instance;
}

/** Constructs a preference widget.
 */
PrefWidget::PrefWidget()
 : WindowParameter("")
{
  PrefParam::addEntry(this);
}

/**
 * Destroys the widget and detaches it from its parameter group.
 */
PrefWidget::~PrefWidget()
{
  PrefParam::removeEntry(this);
  if (getWindowParameter().isValid())
    getWindowParameter()->Detach(this);
  if (m_EntryHandle)
    m_EntryHandle->Detach(this);
}

/** Sets the preference name to \a name. */
void PrefWidget::setEntryName( const QByteArray& name )
{
  m_sPrefName = name;
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
    if (m_Busy)
      return;
    if (std::strcmp(sReason,m_sPrefName) == 0) {
      Base::StateLocker lock(m_Busy);
      restorePreferences();
    } else {
      if (sReason && sReason[0] == 0)
        sReason = nullptr;
      for (auto &entry : m_SubEntries) {
        if (restoreSubEntry(entry, sReason))
          break;
      }
    }
}

/**
 * Saves the current preferences of the widget.
 * All preference widget attached to the same parameter group are notified.
 */
void PrefWidget::onSave()
{
  if (m_Busy)
    return;
  Base::StateLocker guard(m_Busy);
  savePreferences();
#ifdef FC_DEBUG
  if (m_SubEntries.isEmpty() && !getWindowParameter().isValid())
    qFatal( "No parameter group specified!" );
#endif
}

static inline ParameterGrp::handle _getEntryParameter() {
    return App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/WidgetEntries");
}

ParameterGrp::handle PrefWidget::getEntryParameter() {
  if (!m_EntryHandle) {
    m_EntryHandle = _getEntryParameter();
    m_EntryHandle->Attach(this);
  }
  return m_EntryHandle;
}

void PrefWidget::setSubEntryValidate(const SubEntryValidate &validate)
{
  m_Validate = validate;
}

void PrefWidget::resetSubEntries()
{
  _getEntryParameter()->Clear();
}

QVariant PrefWidget::getSubEntryValue(const QByteArray &entryName,
                                      const QByteArray &name,
                                      EntryType type,
                                      const QVariant &defvalue)
{
  static ParameterGrp::handle _handle;
  if (!_handle)
    _handle = _getEntryParameter();
  QVariant v;
  QByteArray n;
  if (entryName.size())
    n = entryName + "_" + name;
  else
    n = name;
  switch(type) {
    case EntryBool:
      v = _handle->GetBool(n, defvalue.toBool());
      break;
    case EntryInt:
      v = (int)_handle->GetInt(n, defvalue.toInt());
      break;
    case EntryDouble:
      v = _handle->GetFloat(n, defvalue.toReal());
      break;
    case EntryString:
      v = QString::fromUtf8(_handle->GetASCII(
            n, defvalue.toString().toUtf8().constData()).c_str());
      break;
  }
  return v;
}

QByteArray PrefWidget::entryPrefix()
{
  QByteArray prefix = entryName();
  if (prefix.isEmpty() && getWindowParameter().isValid())
    prefix = getWindowParameter()->GetGroupName();
  if (!prefix.isEmpty())
    prefix += "_";
  return prefix;
}
      
const QVector<PrefWidget::SubEntry> & PrefWidget::subEntries() const
{
  return m_SubEntries;
}

void PrefWidget::setSubEntries(QObject *base, const QVector<SubEntry> &entries)
{
  m_Base = base;
  m_SubEntries.clear();
  for (auto &entry : entries) {
    if (base->property(entry.name).isValid())
      m_SubEntries.push_back(entry);
  }
  restoreSubEntries();
}

void PrefWidget::setupSubEntries(QObject *base, int flags)
{
    QVector<SubEntry> entries;

    if (flags & EntryDecimals) {
      SubEntry info;
      info.name = "decimals";
      info.defvalue = base->property(info.name);
      if (info.defvalue.isValid()) {
        info.displayName = QObject::tr("Decimals");
        info.type = PrefWidget::EntryInt;
        entries.push_back(info);
      }
    }

    if (flags & EntryMinimum) {
      SubEntry info;
      info.name = "minimum";
      info.defvalue = base->property(info.name);
      if (info.defvalue.isValid()) {
        info.displayName = QObject::tr("Minimum");
        info.type = PrefWidget::EntryDouble;
        entries.push_back(info);
      }
    }

    if (flags & EntryMaximum) {
      SubEntry info;
      info.name = "maximum";
      info.defvalue = base->property(info.name);
      if (info.defvalue.isValid()) {
        info.displayName = QObject::tr("Maximum");
        info.type = PrefWidget::EntryDouble;
        entries.push_back(info);
      }
    }

    if (flags & EntryStep) {
      SubEntry info;
      info.name = "singleStep";
      info.defvalue = base->property(info.name);
      if (info.defvalue.isValid()) {
        info.displayName = QObject::tr("Single step");
        info.type = PrefWidget::EntryDouble;
        entries.push_back(info);
      }
    }

    setSubEntries(base, entries);
}


void PrefWidget::buildContextMenu(QMenu *menu)
{
  if (m_SubEntries.isEmpty())
    return;
  menu->addSeparator();
  QByteArray prefix = entryPrefix();
  for (auto &entry : m_SubEntries) {
    auto action = new QWidgetAction(menu);
    action->setText(entry.displayName);
    QWidget *widget = nullptr;
    QHBoxLayout *layout = nullptr;
    QByteArray name = entry.name;
    if (entry.type != EntryBool) {
        widget = new QWidget(menu);
        layout = new QHBoxLayout(widget);
        auto label = new QLabel(widget);
        label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        label->setText(entry.displayName);
        layout->addWidget(label);
    }
    switch(entry.type) {
      case EntryBool: {
        auto checkbox = new QCheckBox(menu);
        widget = checkbox;
        checkbox->setChecked(m_Base->property(entry.name).toBool());
        checkbox->setText(entry.displayName);
        QObject::connect(checkbox, &QCheckBox::toggled,
          [=](bool value) {
             QVariant v = value;
             if (!m_Validate || m_Validate(name, v))
               getEntryParameter()->SetBool(prefix + name, v.toBool());
          });
        break;
      }
      case EntryInt: {
        auto spinbox = new QSpinBox(widget);
        spinbox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        layout->addWidget(spinbox);
        spinbox->setValue(m_Base->property(entry.name).toInt());
        spinbox->setRange(INT_MIN, INT_MAX);
        QObject::connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
          [=](int value) {
             QVariant v = value;
             if (!m_Validate || m_Validate(name, v))
               getEntryParameter()->SetInt(prefix + name, v.toInt());
          });
        break;
      }
      case EntryDouble: {
        auto spinbox = new QDoubleSpinBox(widget);
        spinbox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        layout->addWidget(spinbox);
        spinbox->setValue(m_Base->property(entry.name).toReal());
        spinbox->setRange(-DBL_MAX, DBL_MAX);
        int decimals = m_Base->property("decimals").toInt();
        spinbox->setDecimals(std::max(16, decimals));
        QObject::connect(spinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          [=](double value) {
             QVariant v = value;
             if (!m_Validate || m_Validate(name, v))
               getEntryParameter()->SetFloat(prefix + name, v.toReal());
          });
        break;
      }
      case EntryString: {
        auto lineedit = new QLineEdit(widget);
        lineedit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        layout->addWidget(lineedit);
        lineedit->setText(m_Base->property(entry.name).toString());
        QObject::connect(lineedit, &QLineEdit::textChanged,
          [=](const QString &value) {
             QVariant v = value;
             if (!m_Validate || m_Validate(name, v))
               getEntryParameter()->SetASCII(prefix + name, v.toString().toUtf8().constData());
          });
        break;
      }
    }
    action->setDefaultWidget(widget);
    menu->addAction(action);
  }
}

void PrefWidget::restoreSubEntries()
{
  for (auto &entry : m_SubEntries)
    restoreSubEntry(entry);
}

bool PrefWidget::restoreSubEntry(const SubEntry &entry, const char *change)
{
  if (!m_Base || m_SubEntries.isEmpty())
    return true;
  if (change && !boost::ends_with(change, entry.name.constData()))
    return false;
  QByteArray name = entryPrefix() + entry.name;
  if (change && name != change)
    return false;
  QVariant v;
  switch(entry.type) {
  case EntryBool:
    v = getEntryParameter()->GetBool(name, entry.defvalue.toBool());
    break;
  case EntryInt:
    v = (int)getEntryParameter()->GetInt(name, entry.defvalue.toInt());
    break;
  case EntryDouble:
    v = getEntryParameter()->GetFloat(name, entry.defvalue.toReal());
    break;
  case EntryString:
    v = QString::fromUtf8(getEntryParameter()->GetASCII(
          name, entry.defvalue.toString().toUtf8().constData()).c_str());
    break;
  }
  if (!m_Validate || m_Validate(entry.name, v))
    m_Base->setProperty(entry.name, v);
  return true;
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
  if (m_Busy)
    return;
  Base::StateLocker guard(m_Busy);
  restorePreferences();
  m_Restored = true;
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
  : IntSpinBox(parent), PrefWidget()
{
    LineEditStyle::setup(lineEdit());
    setAutoSave(PrefParam::AutoSave());
}

PrefSpinBox::~PrefSpinBox()
{
}

void PrefSpinBox::setAutoSave(bool enable)
{
    autoSave(enable, this, QOverload<int>::of(&PrefSpinBox::valueChanged));
}

void PrefSpinBox::setEntryName( const QByteArray& name )
{
  PrefWidget::setEntryName(name);
  if (subEntries().isEmpty())
    setupSubEntries(this);
}

void PrefSpinBox::contextMenuEvent(QContextMenuEvent *event)
{
  if (subEntries().isEmpty()) {
    inherited::contextMenuEvent(event);
    return;
  }
    
  auto edit = lineEdit();
  QPointer<QMenu> menu = edit->createStandardContextMenu();
  if (!menu)
    return;

  buildContextMenu(menu);
  const QPoint pos = (event->reason() == QContextMenuEvent::Mouse)
      ? event->globalPos() : mapToGlobal(QPoint(event->pos().x(), 0)) + QPoint(width() / 2, height() / 2);
  menu->exec(pos);
  delete static_cast<QMenu *>(menu);
  event->accept();
}

void PrefSpinBox::restorePreferences()
{
  if ( getWindowParameter().isNull() )
  {
    failedToRestore(objectName());
    return;
  }
  if (!m_Restored)
    m_Default = value();

  int nVal = getWindowParameter()->GetInt( entryName(), m_Default);
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
  : DoubleSpinBox(parent), PrefWidget()
{
    LineEditStyle::setup(lineEdit());
    setAutoSave(PrefParam::AutoSave());
}

PrefDoubleSpinBox::~PrefDoubleSpinBox()
{
}

void PrefDoubleSpinBox::setAutoSave(bool enable)
{
    autoSave(enable, this, QOverload<double>::of(&PrefDoubleSpinBox::valueChanged));
}

void PrefDoubleSpinBox::setEntryName( const QByteArray& name )
{
  PrefWidget::setEntryName(name);
  if (subEntries().isEmpty())
    setupSubEntries(this);
}

void PrefDoubleSpinBox::contextMenuEvent(QContextMenuEvent *event)
{
  if (subEntries().isEmpty()) {
    inherited::contextMenuEvent(event);
    return;
  }
    
  auto edit = lineEdit();
  QPointer<QMenu> menu = edit->createStandardContextMenu();
  if (!menu)
    return;

  buildContextMenu(menu);
  const QPoint pos = (event->reason() == QContextMenuEvent::Mouse)
      ? event->globalPos() : mapToGlobal(QPoint(event->pos().x(), 0)) + QPoint(width() / 2, height() / 2);
  menu->exec(pos);
  delete static_cast<QMenu *>(menu);
  event->accept();
}

void PrefDoubleSpinBox::restorePreferences()
{
  if ( getWindowParameter().isNull() )
  {
    failedToRestore(objectName());
    return;
  }

  if (!m_Restored)
    m_Default = value();

  double fVal = (double)getWindowParameter()->GetFloat( entryName() , m_Default);
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
    LineEditStyle::setup(this);
    setAutoSave(PrefParam::AutoSave());
}

PrefLineEdit::~PrefLineEdit()
{
}

void PrefLineEdit::setAutoSave(bool enable)
{
    autoSave(enable, this, &PrefLineEdit::editingFinished);
}

void PrefLineEdit::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToRestore(objectName());
    return;
  }

  if (!m_Restored)
    m_Default = this->text();

  setText(QString::fromUtf8(getWindowParameter()->GetASCII(
          entryName(), m_Default.toUtf8()).c_str()));
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

PrefFileChooser::PrefFileChooser ( QWidget * parent )
  : FileChooser(parent), PrefWidget()
{
    setAutoSave(PrefParam::AutoSave());
}

PrefFileChooser::~PrefFileChooser()
{
}

void PrefFileChooser::setAutoSave(bool enable)
{
    autoSave(enable, this, &PrefFileChooser::fileNameSelected);
}

void PrefFileChooser::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToRestore(objectName());
    return;
  }
  if (!m_Restored)
    m_Default = fileName();

  QString txt = QString::fromUtf8(getWindowParameter()->GetASCII(entryName(), m_Default.toUtf8()).c_str());
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
    setAutoSave(PrefParam::AutoSave());
}

PrefComboBox::~PrefComboBox()
{
}

void PrefComboBox::setAutoSave(bool enable)
{
    autoSave(enable, this, QOverload<int>::of(&PrefComboBox::currentIndexChanged));
}

void PrefComboBox::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToRestore(objectName());
    return;
  }
  if (!m_Restored) {
    m_Default = currentData();
    m_DefaultText = currentText();
    m_DefaultIndex = currentIndex();
  }
  int index = -1;
  switch(static_cast<int>(property("prefType").type())) {
  case QMetaType::Int:
    index = findData((int)getWindowParameter()->GetInt(entryName(), m_Default.toInt()));
    break;
  case QMetaType::UInt:
    index = findData((uint)getWindowParameter()->GetUnsigned(entryName(), m_Default.toUInt()));
    break;
  case QMetaType::Bool:
    index = findData(getWindowParameter()->GetBool(entryName(), m_Default.toBool()));
    break;
  case QMetaType::Double:
    index = findData(getWindowParameter()->GetFloat(entryName(), m_Default.toDouble()));
    break;
  case QMetaType::QString:
    index = findText(QString::fromUtf8(
          getWindowParameter()->GetASCII(entryName(), m_DefaultText.toUtf8().constData()).c_str()));
    break;
  case QMetaType::QByteArray:
    index = findData(QByteArray(getWindowParameter()->GetASCII(entryName(),
          m_Default.toByteArray().constData()).c_str()));
    break;
  default:
    index = getWindowParameter()->GetInt(entryName(), m_DefaultIndex);
    break;
  }
  if (index >= 0 && index < count())
    setCurrentIndex(index);
}

void PrefComboBox::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToSave(objectName());
    return;
  }

  switch(static_cast<int>(property("prefType").type())) {
  case QMetaType::Int:
    getWindowParameter()->SetInt(entryName(), currentData().toInt());
    break;
  case QMetaType::UInt:
    getWindowParameter()->SetUnsigned(entryName(), currentData().toUInt());
    break;
  case QMetaType::Bool:
    getWindowParameter()->SetBool(entryName(), currentData().toBool());
    break;
  case QMetaType::Double:
    getWindowParameter()->SetFloat(entryName(), currentData().toDouble());
    break;
  case QMetaType::QString:
    getWindowParameter()->SetASCII(entryName(), currentText().toUtf8().constData());
    break;
  case QMetaType::QByteArray:
    getWindowParameter()->SetASCII(entryName(), currentData().toByteArray().constData());
    break;
  default:
    getWindowParameter()->SetInt(entryName(), currentIndex());
    break;
  }
}

// --------------------------------------------------------------------

PrefCheckBox::PrefCheckBox ( QWidget * parent )
  : QCheckBox(parent), PrefWidget()
{
    setAutoSave(PrefParam::AutoSave());
}

PrefCheckBox::~PrefCheckBox()
{
}

void PrefCheckBox::setAutoSave(bool enable)
{
    autoSave(enable, this, &PrefCheckBox::toggled);
}

void PrefCheckBox::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToRestore(objectName());
    return;
  }

  if (!m_Restored)
    m_Default = isChecked();

  bool enable = getWindowParameter()->GetBool( entryName(), m_Default);
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
    setAutoSave(PrefParam::AutoSave());
}

PrefRadioButton::~PrefRadioButton()
{
}

void PrefRadioButton::setAutoSave(bool enable)
{
    autoSave(enable, this, &PrefRadioButton::toggled);
}

void PrefRadioButton::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToRestore(objectName());
    return;
  }

  if (!m_Restored)
    m_Default = isChecked();

  bool enable = getWindowParameter()->GetBool( entryName(), m_Default );
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
    setAutoSave(PrefParam::AutoSave());
}

PrefSlider::~PrefSlider()
{
}

void PrefSlider::setAutoSave(bool enable)
{
    autoSave(enable, this, &PrefSlider::valueChanged);
}

void PrefSlider::restorePreferences()
{
  if ( getWindowParameter().isNull() )
  {
    failedToRestore(objectName());
    return;
  }

  if (!m_Restored)
    m_Default = value();

  int nVal = getWindowParameter()->GetInt(entryName(), m_Default);
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
    setAutoSave(PrefParam::AutoSave());
}

PrefColorButton::~PrefColorButton()
{
}

void PrefColorButton::setAutoSave(bool enable)
{
    setAutoChangeColor(enable);
    autoSave(enable, this, &PrefColorButton::changed);
}

void PrefColorButton::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    failedToRestore(objectName());
    return;
  }

  if (!m_Restored)
    m_Default = color();

  const QColor &col = m_Default;

  unsigned int icol = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8) | col.alpha();

  unsigned long lcol = static_cast<unsigned long>(icol);
  lcol = getWindowParameter()->GetUnsigned( entryName(), lcol );
  icol = static_cast<unsigned int>(lcol);
  int r = (icol >> 24)&0xff;
  int g = (icol >> 16)&0xff;
  int b = (icol >>  8)&0xff;
  int a = (icol      )&0xff;
  if (!this->allowTransparency())
      a = 0xff;
  setColor(QColor(r,g,b,a));
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
  unsigned int icol = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8) | col.alpha();
  unsigned long lcol = static_cast<unsigned long>(icol);
  getWindowParameter()->SetUnsigned( entryName(), lcol );
}

// --------------------------------------------------------------------

PrefUnitSpinBox::PrefUnitSpinBox ( QWidget * parent )
  : QuantitySpinBox(parent), PrefWidget()
{
    setAutoSave(PrefParam::AutoSave());
}

PrefUnitSpinBox::~PrefUnitSpinBox()
{
}

void PrefUnitSpinBox::setAutoSave(bool enable)
{
    autoSave(enable, this, QOverload<double>::of(&PrefUnitSpinBox::valueChanged));
}

void PrefUnitSpinBox::setEntryName( const QByteArray& name )
{
  PrefWidget::setEntryName(name);
  if (subEntries().isEmpty())
    setupSubEntries(this);
}

void PrefUnitSpinBox::contextMenuEvent(QContextMenuEvent *event)
{
  if (subEntries().isEmpty()) {
    QuantitySpinBox::contextMenuEvent(event);
    return;
  }
    
  auto edit = lineEdit();
  QPointer<QMenu> menu = edit->createStandardContextMenu();
  if (!menu)
    return;

  buildContextMenu(menu);
  const QPoint pos = (event->reason() == QContextMenuEvent::Mouse)
      ? event->globalPos() : mapToGlobal(QPoint(event->pos().x(), 0)) + QPoint(width() / 2, height() / 2);
  menu->exec(pos);
  delete static_cast<QMenu *>(menu);
  event->accept();
}

void PrefUnitSpinBox::restorePreferences()
{
    if (getWindowParameter().isNull()) {
        failedToRestore(objectName());
        return;
    }

    if (!m_Restored)
      m_Default = rawValue();

    double fVal = (double)getWindowParameter()->GetFloat( entryName() ,m_Default );
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

PrefQuantitySpinBox::PrefQuantitySpinBox (QWidget * parent)
  : PrefUnitSpinBox(parent), _historySize(5)
{
}

PrefQuantitySpinBox::~PrefQuantitySpinBox()
{
}

void PrefQuantitySpinBox::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *editMenu = lineEdit()->createStandardContextMenu();
    editMenu->setTitle(tr("Edit"));
    QMenu* menu = new QMenu(QString::fromLatin1("PrefQuantitySpinBox"));

    menu->addMenu(editMenu);

    buildContextMenu(menu);

    menu->addSeparator();

    // datastructure to remember actions for values
    std::vector<QString> values;
    std::vector<QAction *> actions;

    // add the history menu part...
    QStringList history = getHistory();

#if QT_VERSION >= 0x050100
    menu->setToolTipsVisible(true);
#endif

    for (QStringList::const_iterator it = history.begin();it!= history.end();++it) {
        QAction *action;
        if (it->size() > 50) {
            action = menu->addAction(QString::fromLatin1("%1...%2").arg(it->left(22), it->right(22)));
            if (it->size() < 1024)
                action->setToolTip(*it);
            else
                action->setToolTip(QString::fromLatin1("%1\n\n...\n\n%2").arg(
                            it->left(500), it->right(500)));
        } else
            action = menu->addAction(*it);
        actions.push_back(action);
        values.push_back(*it);
    }

    // add the save value portion of the menu
    menu->addSeparator();
    QAction *saveValueAction = menu->addAction(tr("Save value"));
    QAction *clearListAction = menu->addAction(tr("Clear list"));
    clearListAction->setDisabled(history.empty());

    // call the menu and wait until its back
    QAction *userAction = menu->exec(event->globalPos());

    // look what the user has chosen
    if (userAction == saveValueAction) {
        pushToHistory();
    }
    else if (userAction == clearListAction) {
        auto handle = getWindowParameter();
        if (handle.isValid())
            handle->Clear();
    }
    else {
        int i=0;
        for (std::vector<QAction *>::const_iterator it = actions.begin();it!=actions.end();++it,i++) {
            if (*it == userAction) {
                if (values[i].startsWith(QLatin1Char('='))) {
                    QString msg;
                    try {
                        setExpressionString(values[i].toUtf8().constData()+1);
                    } catch (Base::Exception &e) {
                        msg = QString::fromUtf8(e.what());
                    } catch (...) {
                        msg = tr("Failed to apply expression");
                    }
                    if (msg.size())
                        QMessageBox::critical(this, tr("Expression"), msg);
                } else
                    lineEdit()->setText(values[i]);
                break;
            }
        }
    }

    delete menu;
}

void PrefQuantitySpinBox::savePreferences()
{
    pushToHistory();
}

void PrefQuantitySpinBox::restorePreferences()
{
    setToLastUsedValue();
}

void PrefQuantitySpinBox::pushToHistory()
{
    QString val;
    if (hasExpression(false))
        val = QString::fromLatin1("=%1").arg(QString::fromUtf8(getExpressionString().c_str()));
    else
        val = this->text();

    if (val.isEmpty())
        return;

    std::string value(val.toUtf8().constData());
    auto handle = getWindowParameter();
    if (handle.isValid()) {
        try {
            // Search the history for the same value and move to the top if found.
            std::string tHist = handle->GetASCII("Hist0");
            if (tHist != value) {
                int offset = 0;
                QByteArray hist("Hist");
                tHist = value;
                for (int i = 0 ; i < _historySize ;++i) {
                    std::string tNext;
                    for (; i + offset < _historySize; ++offset) {
                        tNext = handle->GetASCII(hist+QByteArray::number(i+offset));
                        if (tNext != value)
                            break;
                    }
                    handle->SetASCII(hist+QByteArray::number(i), tHist);
                    tHist = std::move(tNext);
                }
            }
        }
        catch (const Base::Exception& e) {
            Console().Warning("pushToHistory: %s\n", e.what());
        }
    }
}

QStringList PrefQuantitySpinBox::getHistory() const
{
    QStringList res;

    auto handle = const_cast<PrefQuantitySpinBox*>(this)->getWindowParameter();
    if (handle.isValid()) {
        std::string tmp;
        for (int i = 0 ; i< _historySize ;i++) {
            QByteArray hist = "Hist";
            hist.append(QByteArray::number(i));
            tmp = handle->GetASCII(hist);
            if (!tmp.empty())
                res.push_back(QString::fromUtf8(tmp.c_str()));
            else
                break; // end of history reached
        }
    }

    return res;
}

void PrefQuantitySpinBox::setToLastUsedValue()
{
    QStringList hist = getHistory();
    if (!hist.empty())
        lineEdit()->setText(hist[0]);
}

int PrefQuantitySpinBox::historySize() const
{
    return _historySize;
}

void PrefQuantitySpinBox::setHistorySize(int i)
{
    _historySize = i;
}

void PrefQuantitySpinBox::setParamGrpPath( const QByteArray& path )
{
    PrefUnitSpinBox::setParamGrpPath(path);

    if (subEntries().isEmpty())
        setupSubEntries(this);
}

// --------------------------------------------------------------------

PrefFontBox::PrefFontBox ( QWidget * parent )
  : QFontComboBox(parent), PrefWidget()
{
    setAutoSave(PrefParam::AutoSave());
}

PrefFontBox::~PrefFontBox()
{
}

void PrefFontBox::setAutoSave(bool enable)
{
    autoSave(enable, this, &PrefFontBox::currentFontChanged);
}

void PrefFontBox::restorePreferences()
{
  if ( getWindowParameter().isNull() )
  {
    failedToRestore(objectName());
    return;
  }

  if (!m_Restored)
    m_Default = currentFont();

  QFont currFont = m_Default;                         //QFont from selector widget
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
// vim: noai:ts=2:sw=2
