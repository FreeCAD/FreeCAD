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

#include <Base/Console.h>

#include "PrefWidgets.h"
#include "FileDialog.h"
#include <cstring>

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
 * This method is called if one ore more values in the parameter settings are changed 
 * where getParamGrp() points to. 
 * Note: This method is called for each parameter inside the parameter group. So
 * you have to filter out the appropriate parameter with the name \a sReason.
 * \a rCaller calls this method.
 */
void PrefWidget::OnChange(Base::Subject<const char*> &rCaller, const char * sReason)
{
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
    Console().Warning("Cannot restore!\n");
    return;
  }

  int nVal = getWindowParameter()->GetInt( entryName(), QSpinBox::value() );
  setValue( nVal );
}

void PrefSpinBox::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  getWindowParameter()->SetInt( entryName() , (int)value() );
}

QByteArray PrefSpinBox::entryName () const
{
  return PrefWidget::entryName();
}

QByteArray PrefSpinBox::paramGrpPath () const
{
  return PrefWidget::paramGrpPath();
}

void PrefSpinBox::setEntryName ( const QByteArray& name )
{
  PrefWidget::setEntryName(name);
}

void PrefSpinBox::setParamGrpPath ( const QByteArray& name )
{
  PrefWidget::setParamGrpPath(name);
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
    Console().Warning("Cannot restore!\n");
    return;
  }

  double fVal = (double)getWindowParameter()->GetFloat( entryName() , value() );
  setValue(fVal);
}

void PrefDoubleSpinBox::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  getWindowParameter()->SetFloat( entryName(), value() );
}

QByteArray PrefDoubleSpinBox::entryName () const
{
  return PrefWidget::entryName();
}

QByteArray PrefDoubleSpinBox::paramGrpPath () const
{
  return PrefWidget::paramGrpPath();
}

void PrefDoubleSpinBox::setEntryName ( const QByteArray& name )
{
  PrefWidget::setEntryName(name);
}

void PrefDoubleSpinBox::setParamGrpPath ( const QByteArray& name )
{
  PrefWidget::setParamGrpPath(name);
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
    Console().Warning("Cannot restore!\n");
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
    Console().Warning("Cannot save!\n");
    return;
  }

  getWindowParameter()->SetASCII(entryName(), text().toUtf8());
}

QByteArray PrefLineEdit::entryName () const
{
  return PrefWidget::entryName();
}

QByteArray PrefLineEdit::paramGrpPath () const
{
  return PrefWidget::paramGrpPath();
}

void PrefLineEdit::setEntryName ( const QByteArray& name )
{
  PrefWidget::setEntryName(name);
}

void PrefLineEdit::setParamGrpPath ( const QByteArray& name )
{
  PrefWidget::setParamGrpPath(name);
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
    Console().Warning("Cannot restore!\n");
    return;
  }

  QString txt = QString::fromUtf8(getWindowParameter()->GetASCII(entryName(), fileName().toUtf8()).c_str());
  setFileName(txt);
}

void PrefFileChooser::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  getWindowParameter()->SetASCII(entryName(), fileName().toUtf8());
}

QByteArray PrefFileChooser::entryName () const
{
  return PrefWidget::entryName();
}

QByteArray PrefFileChooser::paramGrpPath () const
{
  return PrefWidget::paramGrpPath();
}

void PrefFileChooser::setEntryName ( const QByteArray& name )
{
  PrefWidget::setEntryName(name);
}

void PrefFileChooser::setParamGrpPath ( const QByteArray& name )
{
  PrefWidget::setParamGrpPath(name);
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
    Console().Warning("Cannot restore!\n");
    return;
  }

  int index = getWindowParameter()->GetInt(entryName(), currentIndex());
  setCurrentIndex(index);
}

void PrefComboBox::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  getWindowParameter()->SetInt(entryName() , currentIndex());
}

QByteArray PrefComboBox::entryName () const
{
  return PrefWidget::entryName();
}

QByteArray PrefComboBox::paramGrpPath () const
{
  return PrefWidget::paramGrpPath();
}

void PrefComboBox::setEntryName ( const QByteArray& name )
{
  PrefWidget::setEntryName(name);
}

void PrefComboBox::setParamGrpPath ( const QByteArray& name )
{
  PrefWidget::setParamGrpPath(name);
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
    Console().Warning("Cannot restore!\n");
    return;
  }

  bool enable = getWindowParameter()->GetBool( entryName(), isChecked() );
  setChecked(enable);
}

void PrefCheckBox::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  getWindowParameter()->SetBool( entryName(), isChecked() );
}

QByteArray PrefCheckBox::entryName () const
{
  return PrefWidget::entryName();
}

QByteArray PrefCheckBox::paramGrpPath () const
{
  return PrefWidget::paramGrpPath();
}

void PrefCheckBox::setEntryName ( const QByteArray& name )
{
  PrefWidget::setEntryName(name);
}

void PrefCheckBox::setParamGrpPath ( const QByteArray& name )
{
  PrefWidget::setParamGrpPath(name);
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
    Console().Warning("Cannot restore!\n");
    return;
  }

  bool enable = getWindowParameter()->GetBool( entryName(), isChecked() );
  setChecked(enable);
}

void PrefRadioButton::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  getWindowParameter()->SetBool( entryName() , isChecked() );
}

QByteArray PrefRadioButton::entryName () const
{
  return PrefWidget::entryName();
}

QByteArray PrefRadioButton::paramGrpPath () const
{
  return PrefWidget::paramGrpPath();
}

void PrefRadioButton::setEntryName ( const QByteArray& name )
{
  PrefWidget::setEntryName(name);
}

void PrefRadioButton::setParamGrpPath ( const QByteArray& name )
{
  PrefWidget::setParamGrpPath(name);
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
    Console().Warning("Cannot restore!\n");
    return;
  }

  int nVal = getWindowParameter()->GetInt(entryName(), QSlider::value());
  setValue(nVal);
}

void PrefSlider::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  getWindowParameter()->SetInt(entryName() , (int)value());
}

QByteArray PrefSlider::entryName () const
{
  return PrefWidget::entryName();
}

QByteArray PrefSlider::paramGrpPath () const
{
  return PrefWidget::paramGrpPath();
}

void PrefSlider::setEntryName ( const QByteArray& name )
{
  PrefWidget::setEntryName(name);
}

void PrefSlider::setParamGrpPath ( const QByteArray& name )
{
  PrefWidget::setParamGrpPath(name);
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
    Console().Warning("Cannot restore!\n");
    return;
  }

  QColor col = color();

  unsigned long lcol = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);

  lcol = getWindowParameter()->GetUnsigned( entryName(), lcol );
  int r = (lcol >> 24)&0xff;
  int g = (lcol >> 16)&0xff;
  int b = (lcol >>  8)&0xff;

  setColor(QColor(r,g,b));
}

void PrefColorButton::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  QColor col = color();
  // (r,g,b,a) with a = 255 (opaque)
  unsigned long lcol = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8) | 255;

  getWindowParameter()->SetUnsigned( entryName(), lcol );
}

QByteArray PrefColorButton::entryName () const
{
  return PrefWidget::entryName();
}

QByteArray PrefColorButton::paramGrpPath () const
{
  return PrefWidget::paramGrpPath();
}

void PrefColorButton::setEntryName ( const QByteArray& name )
{
  PrefWidget::setEntryName(name);
}

void PrefColorButton::setParamGrpPath ( const QByteArray& name )
{
  PrefWidget::setParamGrpPath(name);
}

#include "moc_PrefWidgets.cpp"
