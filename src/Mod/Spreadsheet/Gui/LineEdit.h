#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <Gui/ExpressionCompleter.h>
#include <QWidget>
#include <QModelIndex>

namespace SpreadsheetGui {

class LineEdit : public Gui::ExpressionLineEdit
{
    Q_OBJECT
public:
    explicit LineEdit(QWidget *parent = 0);

    bool event(QEvent *event);
    void setIndex(QModelIndex _current);
    QModelIndex next() const;

private:
    QModelIndex current;
    int deltaCol;
    int deltaRow;
};

class TextEdit : public Gui::ExpressionTextEdit
{
    Q_OBJECT
public:
    explicit TextEdit(QWidget *parent = 0);

    bool event(QEvent *event);
    void setIndex(QModelIndex _current);
    QModelIndex next() const;

Q_SIGNALS:
    void returnPressed();

private:
    QModelIndex current;
    int deltaCol;
    int deltaRow;
};


}

#endif // LINEEDIT_H
