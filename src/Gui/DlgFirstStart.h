#ifndef CLASSWIZARD_H
#define CLASSWIZARD_H

#include <QWizard>

class QCheckBox;
class QGroupBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QRadioButton;
class QListWidget;

namespace Gui::Dialog {
class FirstStartWizard : public QWizard
{
    Q_OBJECT

public:
    FirstStartWizard(QWidget *parent = 0);
};

class IntroPage : public QWizardPage
{
    Q_OBJECT

public:
    IntroPage(QWidget *parent = 0);

private:
    QLabel *label;
};

class LookAndFeelPage : public QWizardPage
{
    Q_OBJECT

public:
    LookAndFeelPage(QWidget *parent = 0);

private:
    QLabel *setthemeLabel;
    QLabel *setHighlights;
    QComboBox *selectHighlightsCom;
    QGroupBox *ThemeAndStyleBox;

};
//! [2]

//! [3]
class WorkbenchesPage : public QWizardPage
{
    Q_OBJECT

public:
    WorkbenchesPage(QWidget *parent = 0);

protected:
    void createListWidget();

private:
    QListWidget *workbenchlist;
    QLabel *workbenchexplain;

};


class ConclusionPage : public QWizardPage
{
    Q_OBJECT

public:
    ConclusionPage(QWidget *parent = 0);

protected:
    // void initializePage();

private:
    QLabel *label;
};

};
#endif