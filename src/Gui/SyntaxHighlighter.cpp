/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <QApplication>
#include <QPalette>

#include "SyntaxHighlighter.h"


using namespace Gui;

namespace Gui {
class SyntaxHighlighterP
{
public:
    SyntaxHighlighterP()
    {
        cNormalText = qApp->palette().windowText().color();
        cComment.setRgb(0, 170, 0);
        cBlockcomment.setRgb(160, 160, 164);
        cLiteral.setRgb(255, 0, 0);
        cNumber.setRgb(0, 0, 255);
        cOperator.setRgb(160, 160, 164);
        cKeyword.setRgb(0, 0, 255);
        cClassName.setRgb(255, 170, 0);
        cDefineName.setRgb(255, 170, 0);
        cOutput.setRgb(170, 170, 127);
        cError.setRgb(255, 0, 0);
    }

    QColor cNormalText, cComment, cBlockcomment, cLiteral, cNumber,
    cOperator, cKeyword, cClassName, cDefineName, cOutput, cError;
};
} // namespace Gui

/**
 * Constructs a syntax highlighter.
 */
SyntaxHighlighter::SyntaxHighlighter(QObject* parent)
    : QSyntaxHighlighter(parent)
{
    d = new SyntaxHighlighterP;
}

/** Destroys this object. */
SyntaxHighlighter::~SyntaxHighlighter()
{
    delete d;
}

/** Sets the color \a col to the paragraph type \a type.
 * This method is provided for convenience to specify the paragraph type
 * by its name.
 */
void SyntaxHighlighter::setColor(const QString& type, const QColor& col)
{
    // Rehighlighting is very expensive, thus avoid it if this color is already set
    QColor old = color(type);
    if (!old.isValid())
        return; // no such type
    if (old == col)
        return;
    if (type == QLatin1String("Text"))
        d->cNormalText = col;
    else if (type == QLatin1String("Comment"))
        d->cComment = col;
    else if (type == QLatin1String("Block comment"))
        d->cBlockcomment = col;
    else if (type == QLatin1String("Number"))
        d->cNumber = col;
    else if (type == QLatin1String("String"))
        d->cLiteral = col;
    else if (type == QLatin1String("Keyword"))
        d->cKeyword = col;
    else if (type == QLatin1String("Class name"))
        d->cClassName = col;
    else if (type == QLatin1String("Define name"))
        d->cDefineName = col;
    else if (type == QLatin1String("Operator"))
        d->cOperator = col;
    else if (type == QLatin1String("Python output"))
        d->cOutput = col;
    else if (type == QLatin1String("Python error"))
        d->cError = col;
    colorChanged(type, col);
}

QColor SyntaxHighlighter::color(const QString& type)
{
    if (type == QLatin1String("Text"))
        return d->cNormalText;
    else if (type == QLatin1String("Comment"))
        return d->cComment;
    else if (type == QLatin1String("Block comment"))
        return d->cBlockcomment;
    else if (type == QLatin1String("Number"))
        return d->cNumber;
    else if (type == QLatin1String("String"))
        return d->cLiteral;
    else if (type == QLatin1String("Keyword"))
        return d->cKeyword;
    else if (type == QLatin1String("Class name"))
        return d->cClassName;
    else if (type == QLatin1String("Define name"))
        return d->cDefineName;
    else if (type == QLatin1String("Operator"))
        return d->cOperator;
    else if (type == QLatin1String("Python output"))
        return d->cOutput;
    else if (type == QLatin1String("Python error"))
        return d->cError;
    else
        return {}; // not found
}

QColor SyntaxHighlighter::colorByType(SyntaxHighlighter::TColor type)
{
    if (type == SyntaxHighlighter::Text)
        return d->cNormalText;
    else if (type == SyntaxHighlighter::Comment)
        return d->cComment;
    else if (type == SyntaxHighlighter::BlockComment)
        return d->cBlockcomment;
    else if (type == SyntaxHighlighter::Number)
        return d->cNumber;
    else if (type == SyntaxHighlighter::String)
        return d->cLiteral;
    else if (type == SyntaxHighlighter::Keyword)
        return d->cKeyword;
    else if (type == SyntaxHighlighter::Classname)
        return d->cClassName;
    else if (type == SyntaxHighlighter::Defname)
        return d->cDefineName;
    else if (type == SyntaxHighlighter::Operator)
        return d->cOperator;
    else if (type == SyntaxHighlighter::Output)
        return d->cOutput;
    else if (type == SyntaxHighlighter::Error)
        return d->cError;
    else
        return {}; // not found
}

void SyntaxHighlighter::colorChanged(const QString& type, const QColor& col)
{
    Q_UNUSED(type);
    Q_UNUSED(col);
    rehighlight();
}

int SyntaxHighlighter::maximumUserState() const
{
    return 8;
}
