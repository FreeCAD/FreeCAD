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


#ifndef GUI_EDITORVIEW_H
#define GUI_EDITORVIEW_H

#include "MDIView.h"
#include "Window.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPlainTextEdit;
class QPrinter;
class QHBoxLayout;
class QToolButton;
class QCheckBox;
class QSpacerItem;
QT_END_NAMESPACE

namespace Gui {

class EditorViewP;
class TextEdit;
class PythonTracingWatcher;

/**
 * A special view class which sends the messages from the application to
 * the editor and embeds it in a window.
 * @author Werner Mayer
 */
class GuiExport EditorView : public MDIView, public WindowParameter
{
    Q_OBJECT

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum DisplayName {
        FullName,
        FileName,
        BaseName
    };

    EditorView(TextEdit* editor, QWidget* parent);
    ~EditorView() override;

    QPlainTextEdit* getEditor() const;
    void setDisplayName(DisplayName);
    void OnChange(Base::Subject<const char*> &rCaller,const char* rcReason) override;

    const char *getName() const override {return "EditorView";}
    void onUpdate() override{}

    bool onMsg(const char* pMsg,const char** ppReturn) override;
    bool onHasMsg(const char* pMsg) const override;

    bool canClose() override;

    /** @name Standard actions of the editor */
    //@{
    bool open   (const QString &f);
    bool saveAs ();
    void cut    ();
    void copy   ();
    void paste  ();
    void undo   ();
    void redo   ();
    void print  () override;
    void printPdf() override;
    void printPreview() override;
    void print(QPrinter*) override;
    //@}

    QStringList undoActions() const override;
    QStringList redoActions() const override;
    QString fileName() const;

protected:
    void focusInEvent(QFocusEvent* e) override;
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;
    void closeEvent(QCloseEvent*) override;

private Q_SLOTS:
    void checkTimestamp();
    void contentsChange(int position, int charsRemoved, int charsAdded);
    void undoAvailable(bool);
    void redoAvailable(bool);

Q_SIGNALS:
    void changeFileName(const QString&);

private:
    void setCurrentFileName(const QString &fileName);
    bool saveFile();

private:
    EditorViewP* d;
};

class PythonEditor;
class GuiExport PythonEditorView : public EditorView
{
    Q_OBJECT

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PythonEditorView(PythonEditor* editor, QWidget* parent);
    ~PythonEditorView() override;

    bool onMsg(const char* pMsg,const char** ppReturn) override;
    bool onHasMsg(const char* pMsg) const override;

public Q_SLOTS:
    void executeScript();
    void startDebug();
    void toggleBreakpoint();
    void showDebugMarker(int line);
    void hideDebugMarker();

private:
    PythonEditor* _pye;
    PythonTracingWatcher* watcher;
};

class SearchBar : public QWidget
{
    Q_OBJECT

public:
    explicit SearchBar(QWidget* parent = nullptr);

    void setEditor(QPlainTextEdit *textEdit);

protected:
    void keyPressEvent(QKeyEvent*) override;
    void changeEvent(QEvent*) override;

public Q_SLOTS:
    void activate();
    void deactivate();
    void findPrevious();
    void findNext();
    void findCurrent();

private:
    void retranslateUi();
    void findText(bool skip, bool next, const QString& str);
    void updateButtons();

private:
    QPlainTextEdit* textEditor;
    QHBoxLayout* horizontalLayout;
    QSpacerItem* horizontalSpacer;
    QToolButton* closeButton;
    QLineEdit* searchText;
    QToolButton* prevButton;
    QToolButton* nextButton;
    QCheckBox* matchCase;
    QCheckBox* matchWord;
};

} // namespace Gui

#endif // GUI_EDITORVIEW_H
