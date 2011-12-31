/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QApplication>
# include <QKeyEvent>
# include <QLabel>
# include <QTextCursor>
# include <QPlainTextEdit>
# include <QToolTip>
#endif

#include <CXX/Objects.hxx>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <App/Property.h>
#include <App/PropertyContainer.h>
#include <App/PropertyContainerPy.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentPy.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/DocumentPy.h>
#include "CallTips.h"

Q_DECLARE_METATYPE( Gui::CallTip ); //< allows use of QVariant

namespace Gui
{

/**
 * template class Temporary.
 * Allows variable changes limited to a scope.
 */
template <typename TYPE>
class Temporary
{
public:
    Temporary( TYPE &var, const TYPE tmpVal )
      : _var(var), _saveVal(var)
    { var = tmpVal; }

    ~Temporary( void )
    { _var = _saveVal; }

private:
    TYPE &_var;
    TYPE  _saveVal;
};

} /* namespace Gui */

using namespace Gui;

CallTipsList::CallTipsList(QPlainTextEdit* parent)
  :  QListWidget(parent), textEdit(parent), cursorPos(0), validObject(true), doCallCompletion(false)
{
    // make the user assume that the widget is active
    QPalette pal = parent->palette();
    pal.setColor(QPalette::Inactive, QPalette::Highlight, pal.color(QPalette::Active, QPalette::Highlight));
    pal.setColor(QPalette::Inactive, QPalette::HighlightedText, pal.color(QPalette::Active, QPalette::HighlightedText));
    parent->setPalette( pal );

    connect(this, SIGNAL(itemActivated(QListWidgetItem *)), 
            this, SLOT(callTipItemActivated(QListWidgetItem *)));

    hideKeys.append(Qt::Key_Space);
    hideKeys.append(Qt::Key_Exclam);
    hideKeys.append(Qt::Key_QuoteDbl);
    hideKeys.append(Qt::Key_NumberSign);
    hideKeys.append(Qt::Key_Dollar);
    hideKeys.append(Qt::Key_Percent);
    hideKeys.append(Qt::Key_Ampersand);
    hideKeys.append(Qt::Key_Apostrophe);
    hideKeys.append(Qt::Key_Asterisk);
    hideKeys.append(Qt::Key_Plus);
    hideKeys.append(Qt::Key_Comma);
    hideKeys.append(Qt::Key_Minus);
    hideKeys.append(Qt::Key_Period);
    hideKeys.append(Qt::Key_Slash);
    hideKeys.append(Qt::Key_Colon);
    hideKeys.append(Qt::Key_Semicolon);
    hideKeys.append(Qt::Key_Less);
    hideKeys.append(Qt::Key_Equal);
    hideKeys.append(Qt::Key_Greater);
    hideKeys.append(Qt::Key_Question);
    hideKeys.append(Qt::Key_At);
    hideKeys.append(Qt::Key_Backslash);

    compKeys.append(Qt::Key_ParenLeft);
    compKeys.append(Qt::Key_ParenRight);
    compKeys.append(Qt::Key_BracketLeft);
    compKeys.append(Qt::Key_BracketRight);
    compKeys.append(Qt::Key_BraceLeft);
    compKeys.append(Qt::Key_BraceRight);
}

CallTipsList::~CallTipsList()
{
}

void CallTipsList::keyboardSearch(const QString& wordPrefix)
{ 
    // first search for the item that matches perfectly
    for (int i=0; i<count(); ++i) {
        QString text = item(i)->text();
        if (text.startsWith(wordPrefix)) {
            setCurrentRow(i);
            return;
        }
    }

    // now do a case insensitive comparison
    for (int i=0; i<count(); ++i) {
        QString text = item(i)->text();
        if (text.startsWith(wordPrefix, Qt::CaseInsensitive)) {
            setCurrentRow(i);
            return;
        }
    }

    setItemSelected(currentItem(), false);
}

void CallTipsList::validateCursor()
{
    QTextCursor cursor = textEdit->textCursor();
    int currentPos = cursor.position();
    if (currentPos < this->cursorPos) {
        hide();
    }
    else {
        cursor.setPosition(this->cursorPos);
        cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        QString word = cursor.selectedText();
        if (!word.isEmpty()) {
            // the following text might be an operator, brackets, ...
            const QChar underscore =  QLatin1Char('_');
            const QChar ch = word.at(0);
            if (!ch.isLetterOrNumber() && ch != underscore)
                word.clear();
        }
        if (currentPos > this->cursorPos+word.length()) {
            hide();
        }
        else if (!word.isEmpty()){
            // If the word is empty we should not allow to do a search,
            // otherwise we may select the next item which is not okay in this
            // context. This might happen if e.g. Shift is pressed.
            keyboardSearch(word);
        }
    }
}

QString CallTipsList::extractContext(const QString& line) const
{
    int len = line.size();
    int index = len-1;
    for (int i=0; i<len; i++) {
        int pos = len-1-i;
        const char ch = line.at(pos).toAscii();
        if ((ch >= 48 && ch <= 57)  ||  // Numbers
            (ch >= 65 && ch <= 90)  ||  // Uppercase letters
            (ch >= 97 && ch <= 122) ||  // Lowercase letters
            (ch == '.') || (ch == '_'))
            index = pos;
        else 
            break;
    }

    return line.mid(index);
}

QMap<QString, CallTip> CallTipsList::extractTips(const QString& context) const
{
    Base::PyGILStateLocker lock;
    QMap<QString, CallTip> tips;
    if (context.isEmpty())
        return tips;

    try {
        QStringList items = context.split(QLatin1Char('.'));
        Py::Module module("__main__");
        Py::Dict dict = module.getDict();
        QString modname = items.front();
        items.pop_front();
        if (!dict.hasKey(std::string(modname.toAscii())))
            return tips; // unknown object

        // get the Python object we need
        Py::Object obj = dict.getItem(std::string(modname.toAscii()));
        while (!items.isEmpty()) {
            QByteArray name = items.front().toAscii();
            std::string attr = name.constData();
            items.pop_front();
            if (obj.hasAttr(attr))
                obj = obj.getAttr(attr);
            else
                return tips;
        }
        
        // Checks whether the type is a subclass of PyObjectBase because to get the doc string
        // of a member we must get it by its type instead of its instance otherwise we get the
        // wrong string, namely that of the type of the member. 
        // Note: 3rd party libraries may use their own type object classes so that we cannot 
        // reliably use Py::Type. To be on the safe side we should use Py::Object to assign
        // the used type object to.
        //Py::Object type = obj.type();
        Py::Object type(PyObject_Type(obj.ptr()), true);
        Py::Object inst = obj; // the object instance 
        union PyType_Object typeobj = {&Base::PyObjectBase::Type};
        bool subclass = (PyObject_IsSubclass(type.ptr(), typeobj.o) == 1);
        if (subclass) obj = type;
        
        // If we have an instance of PyObjectBase then determine whether it's valid or not
        if (PyObject_IsInstance(inst.ptr(), typeobj.o) == 1) {
            Base::PyObjectBase* baseobj = static_cast<Base::PyObjectBase*>(inst.ptr());
            const_cast<CallTipsList*>(this)->validObject = baseobj->isValid();
        }
        else {
            // PyObject_IsInstance might set an exception
            PyErr_Clear();
        }

        Py::List list(PyObject_Dir(obj.ptr()), true);

        // If we derive from PropertyContainerPy we can search for the properties in the
        // C++ twin class.
        union PyType_Object proptypeobj = {&App::PropertyContainerPy::Type};
        if (PyObject_IsSubclass(type.ptr(), proptypeobj.o) == 1) {
            // These are the attributes of the instance itself which are NOT accessible by
            // its type object
            extractTipsFromProperties(inst, tips);
        }

        // If we derive from App::DocumentPy we have direct access to the objects by their internal
        // names. So, we add these names to the list, too.
        union PyType_Object appdoctypeobj = {&App::DocumentPy::Type};
        if (PyObject_IsSubclass(type.ptr(), appdoctypeobj.o) == 1) {
            App::DocumentPy* docpy = (App::DocumentPy*)(inst.ptr());
            App::Document* document = docpy->getDocumentPtr();
            // Make sure that the C++ object is alive
            if (document) {
                std::vector<App::DocumentObject*> objects = document->getObjects();
                Py::List list;
                for (std::vector<App::DocumentObject*>::iterator it = objects.begin(); it != objects.end(); ++it)
                    list.append(Py::String((*it)->getNameInDocument()));
                extractTipsFromObject(inst, list, tips);
            }
        }

        // If we derive from Gui::DocumentPy we have direct access to the objects by their internal
        // names. So, we add these names to the list, too.
        union PyType_Object guidoctypeobj = {&Gui::DocumentPy::Type};
        if (PyObject_IsSubclass(type.ptr(), guidoctypeobj.o) == 1) {
            Gui::DocumentPy* docpy = (Gui::DocumentPy*)(inst.ptr());
            if (docpy->getDocumentPtr()) {
                App::Document* document = docpy->getDocumentPtr()->getDocument();
                // Make sure that the C++ object is alive
                if (document) {
                    std::vector<App::DocumentObject*> objects = document->getObjects();
                    Py::List list;
                    for (std::vector<App::DocumentObject*>::iterator it = objects.begin(); it != objects.end(); ++it)
                        list.append(Py::String((*it)->getNameInDocument()));
                    extractTipsFromObject(inst, list, tips);
                }
            }
        }

        // These are the attributes from the type object
        extractTipsFromObject(obj, list, tips);
    }
    catch (Py::Exception& e) {
        // Just clear the Python exception
        e.clear();
    }

    return tips;
}

void CallTipsList::extractTipsFromObject(Py::Object& obj, Py::List& list, QMap<QString, CallTip>& tips) const
{
    try {
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            Py::String attrname(*it);
            Py::Object attr = obj.getAttr(attrname.as_string());

            CallTip tip;
            QString str = QString::fromAscii(attrname.as_string().c_str());
            tip.name = str;

            if (attr.isCallable()) {
                union PyType_Object basetype = {&PyBaseObject_Type};
                if (PyObject_IsSubclass(attr.ptr(), basetype.o) == 1) {
                    tip.type = CallTip::Class;
                }
                else {
                    PyErr_Clear(); // PyObject_IsSubclass might set an exception
                    tip.type = CallTip::Method;
                }
            }
            else if (PyModule_Check(attr.ptr())) {
                tip.type = CallTip::Module;
            }
            else {
                tip.type = CallTip::Member;
            }

            if (str == QLatin1String("__doc__") && attr.isString()) {
                Py::Object help = attr;
                if (help.isString()) {
                    Py::String doc(help);
                    QString longdoc = QString::fromUtf8(doc.as_string().c_str());
                    int pos = longdoc.indexOf(QLatin1Char('\n'));
                    pos = qMin(pos, 70);
                    if (pos < 0) 
                        pos = qMin(longdoc.length(), 70);
                    tip.description = stripWhiteSpace(longdoc);
                    tip.parameter = longdoc.left(pos);
                }
            }
            else if (attr.hasAttr("__doc__")) {
                Py::Object help = attr.getAttr("__doc__");
                if (help.isString()) {
                    Py::String doc(help);
                    QString longdoc = QString::fromUtf8(doc.as_string().c_str());
                    int pos = longdoc.indexOf(QLatin1Char('\n'));
                    pos = qMin(pos, 70);
                    if (pos < 0) 
                        pos = qMin(longdoc.length(), 70);
                    tip.description = stripWhiteSpace(longdoc);
                    tip.parameter = longdoc.left(pos);
                }
            }
            tips[str] = tip;
        }
    }
    catch (Py::Exception& e) {
        // Just clear the Python exception
        e.clear();
    }
}

void CallTipsList::extractTipsFromProperties(Py::Object& obj, QMap<QString, CallTip>& tips) const
{
    App::PropertyContainerPy* cont = (App::PropertyContainerPy*)(obj.ptr());
    App::PropertyContainer* container = cont->getPropertyContainerPtr();
    // Make sure that the C++ object is alive
    if (!container) return;
    std::map<std::string,App::Property*> Map;
    container->getPropertyMap(Map);

    for (std::map<std::string,App::Property*>::const_iterator It=Map.begin();It!=Map.end();++It) {
        CallTip tip;
        QString str = QString::fromAscii(It->first.c_str());
        tip.name = str;
        tip.type = CallTip::Property;
        QString longdoc = QString::fromUtf8(container->getPropertyDocumentation(It->second));
        // a point, mesh or shape property
        if (It->second->isDerivedFrom(Base::Type::fromName("App::PropertyComplexGeoData"))) {
            Py::Object data(It->second->getPyObject(), true);
            if (data.hasAttr("__doc__")) {
                Py::Object help = data.getAttr("__doc__");
                if (help.isString()) {
                    Py::String doc(help);
                    longdoc = QString::fromUtf8(doc.as_string().c_str());
                }
            }
        }
        if (!longdoc.isEmpty()) {
            int pos = longdoc.indexOf(QLatin1Char('\n'));
            pos = qMin(pos, 70);
            if (pos < 0) 
                pos = qMin(longdoc.length(), 70);
            tip.description = stripWhiteSpace(longdoc);
            tip.parameter = longdoc.left(pos);
        }
        tips[str] = tip;
    }
}

void CallTipsList::showTips(const QString& line)
{
    // search only once
    static QPixmap type_module_icon = BitmapFactory().pixmap("ClassBrowser/type_module");
    static QPixmap type_class_icon = BitmapFactory().pixmap("ClassBrowser/type_class");
    static QPixmap method_icon = BitmapFactory().pixmap("ClassBrowser/method");
    static QPixmap member_icon = BitmapFactory().pixmap("ClassBrowser/member");
    static QPixmap property_icon = BitmapFactory().pixmap("ClassBrowser/property");

    // object is in error state
    static const char * const forbidden_xpm[]={
            "8 8 3 1",
            ". c None",
            "# c #ff0000",
            "a c #ffffff",
            "..####..",
            ".######.",
            "########",
            "#aaaaaa#",
            "#aaaaaa#",
            "########",
            ".######.",
            "..####.."};
    static QPixmap forbidden_icon(forbidden_xpm);
    static QPixmap forbidden_type_module_icon = BitmapFactory().merge(type_module_icon,forbidden_icon,BitmapFactoryInst::BottomLeft);
    static QPixmap forbidden_type_class_icon = BitmapFactory().merge(type_class_icon,forbidden_icon,BitmapFactoryInst::BottomLeft);
    static QPixmap forbidden_method_icon = BitmapFactory().merge(method_icon,forbidden_icon,BitmapFactoryInst::BottomLeft);
    static QPixmap forbidden_member_icon = BitmapFactory().merge(member_icon,forbidden_icon,BitmapFactoryInst::BottomLeft);
    static QPixmap forbidden_property_icon = BitmapFactory().merge(property_icon,forbidden_icon,BitmapFactoryInst::BottomLeft);

    this->validObject = true;
    QString context = extractContext(line);
    QMap<QString, CallTip> tips = extractTips(context);
    clear();
    for (QMap<QString, CallTip>::Iterator it = tips.begin(); it != tips.end(); ++it) {
        addItem(it.key());
        QListWidgetItem *item = this->item(this->count()-1);
        item->setData(Qt::ToolTipRole, QVariant(it.value().description));
        item->setData(Qt::UserRole, qVariantFromValue( it.value() )); //< store full CallTip data
        switch (it.value().type)
        {
        case CallTip::Module:
            {
                item->setIcon((this->validObject ? type_module_icon : forbidden_type_module_icon));
            }   break;
        case CallTip::Class:
            {
                item->setIcon((this->validObject ? type_class_icon : forbidden_type_class_icon));
            }   break;
        case CallTip::Method:
            {
                item->setIcon((this->validObject ? method_icon : forbidden_method_icon));
            }   break;
        case CallTip::Member:
            {
                item->setIcon((this->validObject ? member_icon : forbidden_member_icon));
            }   break;
        case CallTip::Property:
            {
                item->setIcon((this->validObject ? property_icon : forbidden_property_icon));
            }   break;
        default:
            break;
        }
    }

    if (count()==0)
        return; // nothing found

    // get the minimum width and height of the box
    int h = 0;
    int w = 0;
    for (int i = 0; i < count(); ++i) {
        QRect r = visualItemRect(item(i));
        w = qMax(w, r.width());
        h += r.height();
    }

    // Add an offset
    w += 2*frameWidth();
    h += 2*frameWidth();

    // get the start position of the word prefix
    QTextCursor cursor = textEdit->textCursor();
    this->cursorPos = cursor.position();
    QRect rect = textEdit->cursorRect(cursor);
    int posX = rect.x();
    int posY = rect.y();
    int boxH = h;

    // Decide whether to show downstairs or upstairs
    if (posY > textEdit->viewport()->height()/2) {
        h = qMin(qMin(h,posY), 250);
        if (h < boxH)
            w += textEdit->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
        setGeometry(posX,posY-h, w, h);
    }
    else {
        h = qMin(qMin(h,textEdit->viewport()->height()-fontMetrics().height()-posY), 250);
        if (h < boxH)
            w += textEdit->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
        setGeometry(posX, posY+fontMetrics().height(), w, h);
    }
    
    setCurrentRow(0);
    show();
}

void CallTipsList::showEvent(QShowEvent* e)
{
    QListWidget::showEvent(e);
    // install this object to filter timer events for the tooltip label
    qApp->installEventFilter(this);
}

void CallTipsList::hideEvent(QHideEvent* e)
{
    QListWidget::hideEvent(e);
    qApp->removeEventFilter(this);
}

/** 
 * Get all incoming events of the text edit and redirect some of them, like key up and
 * down, mouse press events, ... to the widget itself.
 */
bool CallTipsList::eventFilter(QObject * watched, QEvent * event)
{
    // This is a trick to avoid to hide the tooltip window after the defined time span
    // of 10 seconds. We just filter out all timer events to keep the label visible.
    if (watched->inherits("QLabel")) {
        QLabel* label = qobject_cast<QLabel*>(watched);
        // Ignore the timer events to prevent from being closed
        if (label->windowFlags() & Qt::ToolTip && event->type() == QEvent::Timer)
            return true;
    }
    if (isVisible() && watched == textEdit->viewport()) {
        if (event->type() == QEvent::MouseButtonPress)
            hide();
    }
    else if (isVisible() && watched == textEdit) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* ke = (QKeyEvent*)event;
            if (ke->key() == Qt::Key_Up || ke->key() == Qt::Key_Down) {
                keyPressEvent(ke);
                return true;
            }
            else if (ke->key() == Qt::Key_PageUp || ke->key() == Qt::Key_PageDown) {
                keyPressEvent(ke);
                return true;
            }
            else if (ke->key() == Qt::Key_Escape) {
                hide();
                return true;
            }
            else if (this->hideKeys.indexOf(ke->key()) > -1) {
                itemActivated(currentItem());
                return false;
            }
            else if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
                itemActivated(currentItem());
                return true;
            }
            else if (ke->key() == Qt::Key_Tab) {
                // enable call completion for activating items
                Temporary<bool> tmp( this->doCallCompletion, true ); //< previous state restored on scope exit
                itemActivated( currentItem() );
                return true;
            }
            else if (this->compKeys.indexOf(ke->key()) > -1) {
                itemActivated(currentItem());
                return false;
            }
            else if (ke->key() == Qt::Key_Shift || ke->key() == Qt::Key_Control || 
                       ke->key() == Qt::Key_Meta || ke->key() == Qt::Key_Alt || 
                       ke->key() == Qt::Key_AltGr) {
                // filter these meta keys to avoid to call keyboardSearch()
                return true;
            }
        }
        else if (event->type() == QEvent::KeyRelease) {
            QKeyEvent* ke = (QKeyEvent*)event;
            if (ke->key() == Qt::Key_Up || ke->key() == Qt::Key_Down ||
                ke->key() == Qt::Key_PageUp || ke->key() == Qt::Key_PageDown) {
                QList<QListWidgetItem *> items = selectedItems();
                if (!items.isEmpty()) {
                    QPoint p(width(), 0);
                    QString text = items.front()->toolTip();
                    if (!text.isEmpty()){
                        QToolTip::showText(mapToGlobal(p), text);
                    } else {
                        QToolTip::showText(p, QString());
                    }
                }
                return true;
            }
        }
        else if (event->type() == QEvent::FocusOut) {
            if (!hasFocus())
                hide();
        }
    }

    return QListWidget::eventFilter(watched, event);
}

void CallTipsList::callTipItemActivated(QListWidgetItem *item)
{
    hide();
    if (!isItemSelected(item)) return;
    
    QString text = item->text();
    QTextCursor cursor = textEdit->textCursor();
    cursor.setPosition(this->cursorPos);
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    QString sel = cursor.selectedText();
    if (!sel.isEmpty()) {
        // in case the cursor moved too far on the right side
        const QChar underscore =  QLatin1Char('_');
        const QChar ch = sel.at(sel.count()-1);
        if (!ch.isLetterOrNumber() && ch != underscore)
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
    }
    cursor.insertText( text );

    // get CallTip from item's UserRole-data
    const CallTip &callTip = qVariantValue<CallTip>( item->data(Qt::UserRole) );

    // if call completion enabled and we've something callable (method or class constructor) ...
    if (this->doCallCompletion
     && (callTip.type == CallTip::Method || callTip.type == CallTip::Class))
    {
      cursor.insertText( QLatin1String("()") ); //< just append parenthesis to identifier even inserted.

      /**
       * Try to find out if call needs arguments.
       * For this we search the description for appropriate hints ...
       */
      QRegExp argumentMatcher( QRegExp::escape( callTip.name ) + QLatin1String("\\s*\\(\\s*\\w+.*\\)") );
      argumentMatcher.setMinimal( true ); //< set regex non-greedy!
      if (argumentMatcher.indexIn( callTip.description ) != -1)
      {
        // if arguments are needed, we just move the cursor one left, to between the parentheses.
        cursor.movePosition( QTextCursor::Left, QTextCursor::MoveAnchor, 1 );
        textEdit->setTextCursor( cursor );
      }
    }
    textEdit->ensureCursorVisible();

    QRect rect = textEdit->cursorRect(cursor);
    int posX = rect.x();
    int posY = rect.y();

    QPoint p(posX, posY);
    p = textEdit->mapToGlobal(p);
    QToolTip::showText( p, callTip.parameter );
}

QString CallTipsList::stripWhiteSpace(const QString& str) const
{
    QString stripped = str;
    QStringList lines = str.split(QLatin1String("\n"));
    int minspace=INT_MAX;
    int line=0;
    for (QStringList::iterator it = lines.begin(); it != lines.end(); ++it, ++line) {
        if (it->count() > 0 && line > 0) {
            int space = 0;
            for (int i=0; i<it->count(); i++) {
                if ((*it)[i] == QLatin1Char('\t'))
                    space++;
                else
                    break;
            }

            if (it->count() > space)
                minspace = std::min<int>(minspace, space);
        }
    }

    // remove all leading tabs from each line
    if (minspace > 0 && minspace < INT_MAX) {
        int line=0;
        QStringList strippedlines;
        for (QStringList::iterator it = lines.begin(); it != lines.end(); ++it, ++line) {
            if (line == 0 && !it->isEmpty()) {
                strippedlines << *it;
            }
            else if (it->count() > 0 && line > 0) {
                strippedlines << it->mid(minspace);
            }
        }

        stripped = strippedlines.join(QLatin1String("\n"));
    }

    return stripped;
}

#include "moc_CallTips.cpp" 
