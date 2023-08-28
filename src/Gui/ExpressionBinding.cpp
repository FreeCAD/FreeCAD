/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QLineEdit>
# include <QPixmapCache>
# include <QStyle>
#endif

#include "BitmapFactory.h"
#include "Command.h"
#include "ExpressionBinding.h"
#include "QuantitySpinBox_p.h"

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Expression.h>
#include <App/ObjectIdentifier.h>
#include <App/PropertyGeo.h>
#include <Base/Tools.h>


FC_LOG_LEVEL_INIT("Expression",true,true)

using namespace Gui;
using namespace App;
namespace sp = std::placeholders;

ExpressionBinding::ExpressionBinding() = default;

ExpressionBinding::~ExpressionBinding() = default;

bool ExpressionBinding::isBound() const
{
    return path.getDocumentObject() != nullptr;
}

void ExpressionBinding::unbind()
{
    expressionchanged.disconnect();
    objectdeleted.disconnect();
    path = App::ObjectIdentifier();
}

void Gui::ExpressionBinding::setExpression(std::shared_ptr<Expression> expr)
{
    DocumentObject * docObj = path.getDocumentObject();

    if (expr) {
        const std::string error = docObj->ExpressionEngine.validateExpression(path, expr);

        if (!error.empty())
            throw Base::RuntimeError(error.c_str());

    }

    lastExpression = getExpression();

    bool transaction = !App::GetApplication().getActiveTransaction();
    if(transaction) {
        std::ostringstream ss;
        ss << (expr?"Set":"Discard") << " expression " << docObj->Label.getValue();
        App::GetApplication().setActiveTransaction(ss.str().c_str());
    }

    docObj->ExpressionEngine.setValue(path, expr);

    if(m_autoApply)
        apply();

    if(transaction)
        App::GetApplication().closeActiveTransaction();

}

void ExpressionBinding::bind(const App::ObjectIdentifier &_path)
{
    const Property * prop = _path.getProperty();

    Q_ASSERT(prop != nullptr);

    path = prop->canonicalPath(_path);

    //connect to be informed about changes
    DocumentObject * docObj = path.getDocumentObject();
    if (docObj) {
        //NOLINTBEGIN
        expressionchanged = docObj->ExpressionEngine.expressionChanged.connect(std::bind(&ExpressionBinding::expressionChange, this, sp::_1));
        App::Document* doc = docObj->getDocument();
        objectdeleted = doc->signalDeletedObject.connect(std::bind(&ExpressionBinding::objectDeleted, this, sp::_1));
        //NOLINTEND
    }
}

void ExpressionBinding::bind(const Property &prop)
{
    bind(App::ObjectIdentifier(prop));
}

bool ExpressionBinding::hasExpression() const
{
    return isBound() && getExpression() != nullptr;
}

std::shared_ptr<App::Expression> ExpressionBinding::getExpression() const
{
    DocumentObject * docObj = path.getDocumentObject();

    Q_ASSERT(isBound() && docObj != nullptr);

    return docObj->getExpression(path).expression;
}

std::string ExpressionBinding::getExpressionString(bool no_throw) const
{
    try {
        if (!getExpression())
            throw Base::RuntimeError("No expression found.");
        return getExpression()->toString();
    } catch (Base::Exception &e) {
        if(no_throw)
            FC_ERR("failed to get expression string: " << e.what());
        else
            throw;
    } catch (std::exception &e) {
        if(no_throw)
            FC_ERR("failed to get expression string: " << e.what());
        else
            throw;
    } catch (...) {
        if(no_throw)
            FC_ERR("failed to get expression string: unknown exception");
        else
            throw;
    }
    return {};
}

std::string ExpressionBinding::getEscapedExpressionString() const
{
    return Base::Tools::escapedUnicodeFromUtf8(getExpressionString(false).c_str());
}

bool ExpressionBinding::assignToProperty(const std::string & propName, double value)
{
    if (isBound()) {
        const App::ObjectIdentifier & path = getPath();
        const Property * prop = path.getProperty();

        /* Skip update if property is bound and we know it is read-only */
        if (prop && prop->isReadOnly())
            return true;

        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyPlacement::getClassTypeId())) {
            std::string p = path.getSubPathStr();
            if (p == ".Rotation.Angle") {
                value = Base::toRadians(value);
            }
        }
    }

    Gui::Command::doCommand(Gui::Command::Doc, "%s = %f", propName.c_str(), value);
    return true;
}

bool ExpressionBinding::apply(const std::string & propName)
{
    Q_UNUSED(propName);
    if (hasExpression()) {
        DocumentObject * docObj = path.getDocumentObject();

        if (!docObj)
            throw Base::RuntimeError("Document object not found.");

        bool transaction = !App::GetApplication().getActiveTransaction();
        if(transaction) {
            std::ostringstream ss;
            ss << "Set expression " << docObj->Label.getValue();
            App::GetApplication().setActiveTransaction(ss.str().c_str());
        }
        Gui::Command::doCommand(Gui::Command::Doc,"App.getDocument('%s').%s.setExpression('%s', u'%s')",
                                docObj->getDocument()->getName(),
                                docObj->getNameInDocument(),
                                path.toEscapedString().c_str(),
                                getEscapedExpressionString().c_str());
        if(transaction)
            App::GetApplication().closeActiveTransaction();
        return true;
    }
    else {
        if (isBound()) {
            DocumentObject * docObj = path.getDocumentObject();

            if (!docObj)
                throw Base::RuntimeError("Document object not found.");

            if (lastExpression) {
                bool transaction = !App::GetApplication().getActiveTransaction();
                if(transaction) {
                    std::ostringstream ss;
                    ss << "Discard expression " << docObj->Label.getValue();
                    App::GetApplication().setActiveTransaction(ss.str().c_str());
                }
                Gui::Command::doCommand(Gui::Command::Doc,"App.getDocument('%s').%s.setExpression('%s', None)",
                                        docObj->getDocument()->getName(),
                                        docObj->getNameInDocument(),
                                        path.toEscapedString().c_str());
                if(transaction)
                    App::GetApplication().closeActiveTransaction();
            }
        }

        return false;
    }
}

bool ExpressionBinding::apply()
{
    Property * prop(path.getProperty());

    assert(prop);
    Q_UNUSED(prop);

    DocumentObject * docObj(path.getDocumentObject());

    if (!docObj)
        throw Base::RuntimeError("Document object not found.");

    /* Skip updating read-only properties */
    if (prop->isReadOnly())
        return true;

    std::string _path = getPath().toEscapedString();
    const char *path = _path.c_str();
    if(path[0] == '.')
        ++path;
    return apply(Gui::Command::getObjectCmd(docObj) + "." + path);
}

void ExpressionBinding::expressionChange(const ObjectIdentifier& id) {

    if(id==path)
        onChange();
}

void ExpressionBinding::objectDeleted(const App::DocumentObject& obj)
{
    DocumentObject * docObj = path.getDocumentObject();
    if (docObj == &obj) {
        unbind();
    }
}

// ----------------------------------------------------------------------------

ExpressionWidget::ExpressionWidget() = default;

QPixmap ExpressionWidget::getIcon(const char* name, const QSize& size) const
{
    QString key = QString::fromLatin1("%1_%2x%3")
        .arg(QString::fromLatin1(name))
        .arg(size.width())
        .arg(size.height());
    QPixmap icon;
    if (QPixmapCache::find(key, &icon))
        return icon;

    icon = BitmapFactory().pixmapFromSvg(name, size);
    if (!icon.isNull())
        QPixmapCache::insert(key, icon);
    return icon;
}

void ExpressionWidget::makeLabel(QLineEdit* le)
{
    defaultPalette = le->palette();

    /* Icon for f(x) */
    QFontMetrics fm(le->font());
    int frameWidth = le->style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth);
    iconHeight = fm.height() - frameWidth;
    iconLabel = new ExpressionLabel(le);
    iconLabel->setCursor(Qt::ArrowCursor);
    QPixmap pixmap = getIcon(":/icons/bound-expression-unset.svg", QSize(iconHeight, iconHeight));
    iconLabel->setPixmap(pixmap);
    iconLabel->setStyleSheet(QString::fromLatin1("QLabel { border: none; padding: 0px; padding-top: %2px; width: %1px; height: %1px }").arg(iconHeight).arg(frameWidth/2));
    iconLabel->hide();
    iconLabel->setExpressionText(QString());
    le->setStyleSheet(QString::fromLatin1("QLineEdit { padding-right: %1px } ").arg(iconHeight+frameWidth));
}
