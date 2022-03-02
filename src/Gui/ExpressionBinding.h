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

#ifndef EXPRESSIONBINDING_H
#define EXPRESSIONBINDING_H

#include <memory>
#include <string>
#include <App/ObjectIdentifier.h>
#include <QPalette>
#include <boost_signals2.hpp>


namespace App {
class Expression;
}

class ExpressionLabel;
class QLineEdit;

namespace Gui {

class GuiExport ExpressionBinding
{
public:
    ExpressionBinding();
    virtual ~ExpressionBinding();

    virtual void bind(const App::ObjectIdentifier & _path);
    virtual void bind(const App::Property & prop);
    bool isBound() const;
    void unbind();
    virtual bool apply(const std::string &propName);
    virtual bool apply();
    bool hasExpression() const;

    QPixmap getIcon(const char *name, const QSize &size) const;

    //auto apply means that the python code is issued not only on apply() but
    //also on setExpression
    bool autoApply() const {return m_autoApply;}
    void setAutoApply(bool value) {m_autoApply = value;}

protected:
    const App::ObjectIdentifier & getPath() const { return path; }
    std::shared_ptr<App::Expression> getExpression() const;
    std::string getExpressionString(bool no_throw=true) const;
    std::string getEscapedExpressionString() const;
    virtual void setExpression(std::shared_ptr<App::Expression> expr);

    //gets called when the bound expression is changed, either by this binding or any external action
    virtual void onChange() {}

    void makeLabel(QLineEdit* parent);

private:
    App::ObjectIdentifier path;
    std::shared_ptr<App::Expression> lastExpression;

protected:
    ExpressionLabel* iconLabel;
    QPalette defaultPalette;
    int iconHeight;

    void expressionChange(const App::ObjectIdentifier& id);
    void objectDeleted(const App::DocumentObject&);
    boost::signals2::scoped_connection expressionchanged;
    boost::signals2::scoped_connection objectdeleted;
    bool m_autoApply;
};

}

#endif // EXPRESSIONBINDING_H
