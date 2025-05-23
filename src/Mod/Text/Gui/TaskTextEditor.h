/***************************************************************************
 *   Copyright (c) 2024 Martin Rodriguez Reboredo <yakoyoku@gmail.com>     *
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

#ifndef TEXTGUI_TaskTextEditor_H
#define TEXTGUI_TaskTextEditor_H

#include <QFont>

#include <Gui/TaskView/TaskDialog.h>
#include <Mod/Text/App/ShapeText.h>


namespace TextGui {

class ViewProviderShapeText;
class Ui_TaskTextEditor;
class TextGuiExport TextEditor : public QWidget
{
    Q_OBJECT

public:
    explicit TextEditor(Text::ShapeText* shapeText, QWidget *parent = nullptr);
    ~TextEditor() override;

private:
    void setupDialog();

    void onTextEditChanged();
    void onFontNameChanged(const QFont &font);
    void onFontFileSelected(const QString &file);
    void onSizeChanged(double val);
    void onJustificationChanged(int index);

protected:
    std::unique_ptr<Ui_TaskTextEditor> ui;
    App::WeakPtrT<Text::ShapeText> shapeText;
};

class TextGuiExport TaskTextEditor : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskTextEditor(ViewProviderShapeText* shapeTextView);
    ~TaskTextEditor() override;
    ViewProviderShapeText* getShapeTextView() const
    {
        return shapeTextView;
    }

public:
    /// is called the TaskView when the dialog is opened
    void open() override;
    /// is called by the framework if an button is clicked which has no accept or reject role
    void clicked(int) override;
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    bool isAllowedAlterDocument() const override
    {
        return false;
    }

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Close;
    }

private:
    ViewProviderShapeText* shapeTextView;
};

} //namespace TextGui


#endif // TEXTGUI_TaskTextEditor_H
