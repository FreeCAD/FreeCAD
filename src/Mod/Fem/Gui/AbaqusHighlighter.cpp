/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <QTextCharFormat>
#endif

#include "AbaqusHighlighter.h"


using namespace FemGui;

/**
 * Constructs a syntax highlighter.
 */
AbaqusHighlighter::AbaqusHighlighter(QObject* parent)
    : SyntaxHighlighter(parent)
{}

/** Destroys this object. */
AbaqusHighlighter::~AbaqusHighlighter() = default;

void AbaqusHighlighter::highlightBlock(const QString& text)
{
    // Find a syntax file for the Abaqus format here
    // http://notepad-plus.sourceforge.net/commun/userDefinedLang/userDefineLang_Abaqus.xml
    //
    enum
    {
        NormalState = -1,
        Definition,
        BeforeKey,
        InideKey,
        BeforeValue,
        InsideValue,
        Text,
        Number
    };

    int state = NormalState;
    int start = 0;

    QColor keywordColor(102, 0, 227);  // this->colorByType(SyntaxHighlighter::Keyword)
    QColor defnameColor(0, 119, 255);  // this->colorByType(SyntaxHighlighter::Defname)
    QColor operateColor(153, 0, 102);  // this->colorByType(SyntaxHighlighter::Operator)
    QColor valueColor(0, 0, 0);        // this->colorByType(SyntaxHighlighter::String)
    QColor numberColor(0, 127, 127);   // this->colorByType(SyntaxHighlighter::Number)
    QColor commentColor = this->colorByType(SyntaxHighlighter::Comment);

    for (int i = 0; i < text.length(); ++i) {
        // highlight the supported operators
        if (text[i] == QLatin1Char(',') || text[i] == QLatin1Char('=')) {
            setFormat(i, 1, operateColor);
        }

        if (state == Definition) {
            if (!text[i].isLetterOrNumber() && !text[i].isSpace()) {
                QTextCharFormat keywordFormat;
                keywordFormat.setForeground(keywordColor);
                keywordFormat.setFontWeight(QFont::Bold);
                setFormat(start, i - start, keywordFormat);

                start = i;
                state = BeforeKey;
            }
        }
        else if (state == BeforeKey) {
            if (text[i].isLetterOrNumber()) {
                start = i;
                state = InideKey;
            }
        }
        else if (state == InideKey) {
            if (!text[i].isLetterOrNumber() && !text[i].isSpace()) {
                QTextCharFormat keyFormat;
                keyFormat.setForeground(defnameColor);
                setFormat(start, i - start, keyFormat);

                start = i;
                state = BeforeValue;
            }
        }
        else if (state == BeforeValue) {
            if (text[i].isLetterOrNumber()) {
                start = i;
                state = InsideValue;
            }
        }
        else if (state == InsideValue) {
            if (!text[i].isLetterOrNumber() && !text[i].isSpace()) {
                QTextCharFormat valueFormat;
                valueFormat.setForeground(valueColor);
                setFormat(start, i - start, valueFormat);

                start = i;
                state = BeforeKey;
            }
        }
        // Number
        else if (state == Number) {
            if (!text[i].isNumber() && text[i] != QLatin1Char('.')) {
                QTextCharFormat numberFormat;
                numberFormat.setForeground(numberColor);
                setFormat(start, i - start, numberFormat);

                start = i;
                state = NormalState;
            }
        }
        else if (text[i].isNumber() || text[i] == QLatin1Char('-')) {
            if (state == NormalState) {
                start = i;
                state = Number;
            }
        }
        // Comment lines
        else if (text.mid(i, 2) == QLatin1String("**")) {
            QTextCharFormat commentFormat;
            commentFormat.setForeground(commentColor);
            commentFormat.setFontItalic(true);
            setFormat(i, text.length() - i, commentFormat);
            break;
        }
        // Definition
        else if (text[i] == QLatin1Char('*')) {
            start = i;
            state = Definition;
        }
        else if (text[i].isLetterOrNumber()) {
            if (state == NormalState) {
                start = i;
                state = Text;
            }
        }
        else {
            state = NormalState;
        }
    }

    if (state == Definition) {
        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(keywordColor);
        keywordFormat.setFontWeight(QFont::Bold);
        setFormat(start, text.length() - start, keywordFormat);
    }
    else if (state == InideKey) {
        QTextCharFormat keyFormat;
        keyFormat.setForeground(defnameColor);
        setFormat(start, text.length() - start, keyFormat);
    }
    else if (state == InsideValue) {
        QTextCharFormat valueFormat;
        valueFormat.setForeground(valueColor);
        setFormat(start, text.length() - start, valueFormat);
    }
    else if (state == Number) {
        QTextCharFormat numberFormat;
        numberFormat.setForeground(numberColor);
        setFormat(start, text.length() - start, numberFormat);
    }
}
