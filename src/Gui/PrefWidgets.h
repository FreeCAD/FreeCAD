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


#ifndef GUI_PREFWIDGETS_H
#define GUI_PREFWIDGETS_H

#include <memory>
#include <QVector>
#include <QCheckBox>
#include <QComboBox>
#include <QRadioButton>
#include <QFontComboBox>
#include <QFont>
#include <QTimer>
#include <Base/Parameter.h>
#include "Widgets.h"
#include "Window.h"
#include "SpinBox.h"
#include "FileDialog.h"
#include "QuantitySpinBox.h"

class QSplitter;

namespace Gui {
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
class GuiExport PrefWidget : public WindowParameter
{
public:
  virtual void setEntryName( const QByteArray& name );
  QByteArray entryName() const;

  virtual void setParamGrpPath( const QByteArray& path );
  QByteArray paramGrpPath() const;

  virtual void OnChange(Base::Subject<const char*> &rCaller, const char * sReason);
  void onSave();
  void onRestore();

  enum EntryType {
      EntryBool,
      EntryInt,
      EntryDouble,
      EntryString,
  };
  struct SubEntry {
      QByteArray name;
      QString displayName;
      EntryType type;
      QVariant defvalue;
  };
  void setSubEntries(QObject *base, const QVector<SubEntry> &entires);

  enum EntryFlag {
    EntryDecimals = 1,
    EntryMinimum = 2,
    EntryMaximum = 4,
    EntryStep = 8,
    EntryAll = EntryDecimals|EntryMinimum|EntryMaximum|EntryStep,
  };
  void setupSubEntries(QObject *base, int entires = EntryDecimals|EntryStep);

  typedef std::function<bool(const QByteArray &, QVariant &)>  SubEntryValidate;
  void setSubEntryValidate(const SubEntryValidate &);

  const QVector<SubEntry> & subEntries() const;

  static QVariant getSubEntryValue(const QByteArray &entryName,
                                   const QByteArray &name,
                                   EntryType type,
                                   const QVariant &defvalue);
  static void resetSubEntries();

  void restoreSubEntries();
  void saveSubEntries();

  virtual void setAutoSave(bool enable) = 0;

  template<class F, class O>
  void autoSave(bool enable, O *o, F f, int delay=100) {
    if (m_Conn) {
      QObject::disconnect(m_Conn);
      if (!enable)
          return;
    }
    if (delay) {
      if (!m_Timer) {
          m_Timer.reset(new QTimer);
          m_Timer->setSingleShot(true);
          QObject::connect(m_Timer.get(), &QTimer::timeout, [this](){onSave();});
      }
      m_Conn = QObject::connect(o, f, [this, delay](){
        if (!m_Busy && m_Restored)
            m_Timer->start(delay);
      });
    } else {
      m_Conn = QObject::connect(o, f, [this](){
        if (!m_Busy && m_Restored)
            onSave();
      });
    }
  }

protected:
  /** Restores the preferences
   * Must be reimplemented in any subclasses.
   */
  virtual void restorePreferences() = 0;
  /** Save the preferences
   * Must be reimplemented in any subclasses.
   */
  virtual void savePreferences()    = 0;
  /** Print warning that saving failed.
   */
  void failedToSave(const QString&) const;
  /** Print warning that restoring failed.
   */
  void failedToRestore(const QString&) const;

  void buildContextMenu(QMenu *menu);

  bool restoreSubEntry(const SubEntry &, const char *change = nullptr);

  QByteArray entryPrefix();
  ParameterGrp::handle getEntryParameter();

  PrefWidget();
  virtual ~PrefWidget();

private:
  QObject *m_Base = nullptr;
  QByteArray m_sPrefName;
  QByteArray m_sPrefGrp;
  QVector<SubEntry> m_SubEntries;
  SubEntryValidate m_Validate;
  ParameterGrp::handle m_EntryHandle;

  // friends
  friend class Gui::WidgetFactoryInst;

  QMetaObject::Connection m_Conn;
  std::unique_ptr<QTimer> m_Timer;
  bool m_Busy = false;

protected:
  bool m_Restored = false;
};


/// Convenient class for accessing and tracking common parameter settings of PrefWidgets
class PrefParam: public ParameterGrp::ObserverType {
public:
  PrefParam();
  ~PrefParam();

  void OnChange(Base::Subject<const char*> &, const char * sReason);
  static void addEntry(PrefWidget *entry);
  static void removeEntry(PrefWidget *entry);
  static bool AutoSave();
  static void setAutoSave(bool);
  static PrefParam *instance();

private:
  ParameterGrp::handle hGrp;
  bool _AutoSave;
  std::set<PrefWidget*> _entries;
};


/// Convenient class to help widget save/restore states
class GuiExport PrefWidgetStates : public QObject
{
  Q_OBJECT
public:
  PrefWidgetStates(QWidget *widget, bool manageSize=true, const char *name = nullptr);
  virtual ~PrefWidgetStates();
  void addSplitter(QSplitter *, const char *name = nullptr);

protected:
  bool eventFilter(QObject *o, QEvent *e);
  void saveSettings();
  void restoreSettings();
  
protected:
  ParameterGrp::handle hParam;
  bool geometryRestored = false;
  bool manageSize = true;
  std::map<QSplitter*, std::string> splitters;
  QWidget *widget = nullptr;
};

/** The PrefSpinBox class.
 * \author Werner Mayer
 */
class GuiExport PrefSpinBox : public IntSpinBox, public PrefWidget
{
  typedef IntSpinBox inherited;

  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefSpinBox ( QWidget * parent = 0 );
  virtual ~PrefSpinBox();

  virtual void setEntryName( const QByteArray& name );
  virtual void setAutoSave(bool enable);

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();
  void contextMenuEvent(QContextMenuEvent *event);

private:
  int m_Default;
};

/** The PrefDoubleSpinBox class.
 * \author Werner Mayer
 */
class GuiExport PrefDoubleSpinBox : public DoubleSpinBox, public PrefWidget
{
  typedef DoubleSpinBox inherited;

  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefDoubleSpinBox ( QWidget * parent = 0 );
  virtual ~PrefDoubleSpinBox();

  virtual void setEntryName( const QByteArray& name );
  virtual void setAutoSave(bool enable);

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();
  void contextMenuEvent(QContextMenuEvent *event);

private:
  double m_Default;
};

/**
 * The PrefLineEdit class.
 * \author Werner Mayer
 */
class GuiExport PrefLineEdit : public QLineEdit, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefLineEdit ( QWidget * parent = 0 );
  virtual ~PrefLineEdit();
  virtual void setAutoSave(bool enable);

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();

private:
  QString m_Default;
};

/**
 * The PrefFileChooser class.
 * \author Werner Mayer
 */
class GuiExport PrefFileChooser : public FileChooser, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefFileChooser ( QWidget * parent = 0 );
  virtual ~PrefFileChooser();
  virtual void setAutoSave(bool enable);

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();

private:
  QString m_Default;
};

/**
 * The PrefComboBox class.
 * \author Werner Mayer
 *
 * The PrefComboBox supports restoring/saving variant type of item data. You
 * can add a property named 'prefType' with the type you want. If not such
 * property is found, the class defaults to restore/save the item index.
 *
 * Note that there is special handling for 'prefType' of QString, which means
 * to restore/save the item text. This allows the combox to be editable, and
 * accepts user entered value. Use QByteArray if you want to restore/save a
 * non translatable string stored as item data.
 */
class GuiExport PrefComboBox : public QComboBox, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefComboBox ( QWidget * parent = 0 );
  virtual ~PrefComboBox();
  virtual void setAutoSave(bool enable);

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();

private:
  QVariant m_Default;
  int m_DefaultIndex;
  QString m_DefaultText;
};

/**
 * The PrefCheckBox class.
 * \author Werner Mayer
 */
class GuiExport PrefCheckBox : public QCheckBox, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefCheckBox ( QWidget * parent = 0 );
  virtual ~PrefCheckBox();
  virtual void setAutoSave(bool enable);

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();

private:
  bool m_Default;
};

/**
 * The PrefRadioButton class.
 * \author Werner Mayer
 */
class GuiExport PrefRadioButton : public QRadioButton, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefRadioButton ( QWidget * parent = 0 );
  virtual ~PrefRadioButton();
  virtual void setAutoSave(bool enable);

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();

private:
  bool m_Default;
};

/**
 * The PrefSlider class.
 * \author Werner Mayer
 */
class GuiExport PrefSlider : public QSlider, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefSlider ( QWidget * parent = 0 );
  virtual ~PrefSlider();
  virtual void setAutoSave(bool enable);

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();

private:
  int m_Default;
};

/**
 * The PrefColorButton class.
 * \author Werner Mayer
 */
class GuiExport PrefColorButton : public ColorButton, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefColorButton ( QWidget * parent = 0 );
  virtual ~PrefColorButton();
  virtual void setAutoSave(bool enable);

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();

private:
  QColor m_Default;
};

/** The PrefUnitSpinBox class.
 * \author wandererfan
 * a simple Unit aware spin box.
 * See also \ref PrefQuantitySpinBox
 */
class GuiExport PrefUnitSpinBox : public QuantitySpinBox, public PrefWidget
{
    Q_OBJECT

    Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
    Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
    PrefUnitSpinBox ( QWidget * parent = 0 );
    virtual ~PrefUnitSpinBox();

    virtual void setEntryName( const QByteArray& name );
    virtual void setAutoSave(bool enable);

protected:
    // restore from/save to parameters
    void restorePreferences();
    void savePreferences();
    void contextMenuEvent(QContextMenuEvent *event);

private:
  double m_Default;
};

class PrefQuantitySpinBoxPrivate;

/**
 * The PrefQuantitySpinBox class.
 * \author Werner Mayer
 */
class GuiExport PrefQuantitySpinBox : public PrefUnitSpinBox
{
    Q_OBJECT

    Q_PROPERTY(int historySize READ historySize WRITE setHistorySize)

public:
    PrefQuantitySpinBox (QWidget * parent = 0);
    virtual ~PrefQuantitySpinBox();

    /// set the input field to the last used value (works only if the setParamGrpPath() was called)
    void setToLastUsedValue();
    /// get the value of the history size property
    int historySize() const;
    /// set the value of the history size property
    void setHistorySize(int);

    /** @name history and default management */
    //@{
    /// push a new value to the history, if no string given the actual text of the input field is used.
    void pushToHistory();
    /// get the history of the field, newest first
    QStringList getHistory() const;
    //@}

    virtual void setParamGrpPath( const QByteArray& path );

protected:
    virtual void contextMenuEvent(QContextMenuEvent * event);
    void restorePreferences();
    void savePreferences();

private:
    int _historySize;
    Q_DISABLE_COPY(PrefQuantitySpinBox)
};

/** The PrefFontBox class.
 * \author wandererfan
 */
class GuiExport PrefFontBox : public QFontComboBox, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefFontBox ( QWidget * parent = 0 );
  virtual ~PrefFontBox();
  virtual void setAutoSave(bool enable);

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();

private:
  QFont m_Default;
};

} // namespace Gui

#endif // GUI_PREFWIDGETS_H
