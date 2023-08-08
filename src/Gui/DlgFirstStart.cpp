#include <QtWidgets>

#include "DlgFirstStart.h"
#include "PreferencePages/DlgSettingsGeneral.h"
#include "PreferencePages/DlgSettingsNavigation.h"
#include "PreferencePages/DlgSettingsWorkbenchesImp.h"
#include <Base/UnitsApi.h>
#include <QComboBox>


using namespace Gui::Dialog;


FirstStartWizard::FirstStartWizard(QWidget *parent)
    : QWizard(parent)
{
    addPage(new IntroPage);
    addPage(new LookAndFeelPage);
    addPage(new WorkbenchesPage);
    addPage(new ConclusionPage);

    resize(600, 650);

    // setPixmap(QWizard::BannerPixmap, QPixmap(":/images/banner_big.png"));
    // setPixmap(QWizard::BackgroundPixmap, QPixmap(""));

    setWindowTitle(tr("First start wizard"));
    setOption(QWizard::NoCancelButton, true);
}

IntroPage::IntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    setPixmap(QWizard::LogoPixmap, QPixmap(""));
    setPixmap(QWizard::BannerPixmap, QPixmap(":/images/banner_big.png"));
    // setTitle(tr("Welcome"));
    // setSubTitle(tr(" "));

    QLabel* label2 = new QLabel();
    label2->setTextFormat(Qt::MarkdownText);
    label2->setText(tr("# Welcome to FreeCAD"));

    QLabel* label = new QLabel();
    label->setTextFormat(Qt::MarkdownText);
    label->setText(tr(R"""(
## Localization

It looks like it's your first time here?
Let's start by setting up the your locale
    )"""));
    label->setWordWrap(true);

    QLabel* langLabel = new QLabel(tr("Select Language:"));
    QComboBox* selectLanguage = new QComboBox();
    DlgSettingsGeneral::setupLanguageSelector(selectLanguage);

    // QGroupBox* languageBox = new QGroupBox(tr("Language"));

    QLabel* setUnitLabel = new QLabel(tr("Select Units:"));
    QComboBox* selectUnitSchema = new QComboBox();
    DlgSettingsGeneral::setupSchemaSelector(selectUnitSchema);


    QLabel* setNavigationStyle = new QLabel(tr("Select Navigation Style"));
    QComboBox* selectNavigationStyle = new QComboBox();
    DlgSettingsNavigation::setupNavigationSelector(selectNavigationStyle);
    selectNavigationStyle->setCurrentIndex(1);

    QGridLayout* localizationBoxLayout = new QGridLayout;
    localizationBoxLayout->addWidget(langLabel, 0, 0);
    localizationBoxLayout->addWidget(selectLanguage, 0, 1);
    localizationBoxLayout->addWidget(setUnitLabel, 1, 0);
    localizationBoxLayout->addWidget(selectUnitSchema, 1, 1);
    localizationBoxLayout->addWidget(setNavigationStyle, 2, 0);
    localizationBoxLayout->addWidget(selectNavigationStyle, 2, 1);

    QGroupBox* localizationBox = new QGroupBox();
    localizationBox->setLayout(localizationBoxLayout);

    connect(
        selectLanguage,
        qOverload<int>(&QComboBox::currentIndexChanged),
        [selectLanguage]() { DlgSettingsGeneral::setLanguage(selectLanguage); }
    );
    connect(
        selectUnitSchema,
        qOverload<int>(&QComboBox::currentIndexChanged),
        [selectUnitSchema]() { DlgSettingsGeneral::setSchema(selectUnitSchema); }
    );
    connect(
        selectNavigationStyle,
        qOverload<int>(&QComboBox::currentIndexChanged),
        [selectNavigationStyle]() { DlgSettingsNavigation::setNavigation(selectNavigationStyle); }
    );

    selectUnitSchema->setCurrentIndex(0);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label2);
    layout->addWidget(label);
    layout->addWidget(localizationBox);
    setLayout(layout);
}

LookAndFeelPage::LookAndFeelPage(QWidget *parent)
    : QWizardPage(parent)
{
    // setTitle(tr("Look and Feel"));
   //* FirstStartWizard::setTitleFormat(BOLD_FONTTYPE)
    // setSubTitle(tr("Set the look and feel of freecad"));
    setPixmap(QWizard::LogoPixmap, QPixmap(":/images/icon-freecad.png"));
    setPixmap(QWizard::BannerPixmap, QPixmap(":/images/banner.png"));

    QLabel* label = new QLabel();
    label->setTextFormat(Qt::MarkdownText);
    label->setText(R"""(
# Visuals

Now, how should it look and feel?
    )""");

    setthemeLabel = new QLabel(tr("Select theme mode"), this);
    QComboBox* selectTheme = new QComboBox(this);
    // DlgSettingsGeneral::setupStyleSheetSelector(selectTheme);

    // setHighlights = new QLabel(tr("Select Highlight"));
    // selectHighlightsCom = new QComboBox();
    // selectHighlightsCom->addItem("Blue");
    // selectHighlightsCom->addItem("Red");
    // selectHighlightsCom->addItem("Pink");
    // selectHighlightsCom->setCurrentIndex(0);

    QLabel* setIconSize = new QLabel(tr("Select Icon size"), this);
    QComboBox* selectIconSize = new QComboBox(this);
    DlgSettingsGeneral::setupIconSizeSelector(selectIconSize);
    selectIconSize->setCurrentIndex(1);

    // QComboBox* selectGraphics = new QComboBox();
    // selectGraphics->addItem(tr("Potato"));
    // selectGraphics->addItem(tr("Normal"));

    connect(
        selectIconSize,
        qOverload<int>(&QComboBox::currentIndexChanged),
        [selectIconSize]() { DlgSettingsGeneral::setIconSize(selectIconSize); }
    );
    connect(
        selectTheme,
        qOverload<int>(&QComboBox::currentIndexChanged),
        [selectTheme]() { /*DlgSettingsGeneral::setStyleSheet(selectTheme);*/ }
    );
    selectTheme->setCurrentIndex(1);

    ThemeAndStyleBox = new QGroupBox(this);

    QGridLayout *ThemeAndStyleBoxLayout = new QGridLayout;
    ThemeAndStyleBoxLayout->addWidget(selectTheme, 0, 1);
    ThemeAndStyleBoxLayout->addWidget(setthemeLabel, 0, 0);
    // ThemeAndStyleBoxLayout->addWidget(selectHighlightsCom, 1, 1);
    // ThemeAndStyleBoxLayout->addWidget(setHighlights, 1, 0);
    ThemeAndStyleBoxLayout->addWidget(selectIconSize, 2, 1);
    ThemeAndStyleBoxLayout->addWidget(setIconSize, 2, 0);
    ThemeAndStyleBox->setLayout(ThemeAndStyleBoxLayout);


    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(label);
    layout->addWidget(ThemeAndStyleBox);

    setLayout(layout);
}

WorkbenchesPage::WorkbenchesPage(QWidget *parent)
    : QWizardPage(parent)
{
    QLabel* label = new QLabel();
    label->setTextFormat(Qt::MarkdownText);
    label->setText(R"""(
# Workflow

How are you planning to use FreeCAD?
    )""");
    setPixmap(QWizard::LogoPixmap, QPixmap(":/images/icon-freecad.png"));
    setPixmap(QWizard::BannerPixmap, QPixmap(":/images/banner.png"));

    QRadioButton* workbenchSelectorMechanic = new QRadioButton(tr("Mechanical part design"), this);
    QRadioButton* workbenchSelectorArchiBim = new QRadioButton(tr("Architecture and BIM"), this);
    QRadioButton* workbenchSelectorMachining = new QRadioButton(tr("Machining"), this);
    QRadioButton* workbenchSelectorAll = new QRadioButton(tr("All"), this);
    QRadioButton* workbenchSelectorCustom = new QRadioButton(tr("Custom"), this);

    WorkbenchList* workbenchList = new WorkbenchList(true, this);
    workbenchList->loadSettings();

    QStringList mechanicWorkbenches;
    mechanicWorkbenches << "DraftWorkbench"
                        << "FemWorkbench"
                        << "PartDesignWorkbench"
                        << "PartWorkbench"
                        << "SketcherWorkbench"
                        << "SpreadsheetWorkbench"
                        << "SurfaceWorkbench"
                        << "TechDrawWorkbench"
                        << "StartWorkbench";

    QStringList archiBimWorkbenches;
    archiBimWorkbenches << "ArchWorkbench"
                        << "DraftWorkbench"
                        << "PartDesignWorkbench"
                        << "PartWorkbench"
                        << "SketcherWorkbench"
                        << "SpreadsheetWorkbench"
                        << "TechDrawWorkbench"
                        << "StartWorkbench";

    QStringList machiningWorkbenches;
    machiningWorkbenches << "DraftWorkbench"
                         << "PartDesignWorkbench"
                         << "PartWorkbench"
                         << "PathWorkbench"
                         << "SketcherWorkbench"
                         << "SpreadsheetWorkbench"
                         << "TechDrawWorkbench"
                         << "StartWorkbench";

    connect(workbenchSelectorMechanic, &QRadioButton::clicked, [workbenchList, mechanicWorkbenches]() {
        QSignalBlocker blocker(workbenchList);
        workbenchList->disableWorkbenches();
        workbenchList->enableWorkbenches(mechanicWorkbenches);
    });
    connect(workbenchSelectorArchiBim, &QRadioButton::clicked, [workbenchList, archiBimWorkbenches]() {
        QSignalBlocker blocker(workbenchList);
        workbenchList->disableWorkbenches();
        workbenchList->enableWorkbenches(archiBimWorkbenches);
    });
    connect(workbenchSelectorMachining, &QRadioButton::clicked, [workbenchList, machiningWorkbenches]() {
        QSignalBlocker blocker(workbenchList);
        workbenchList->disableWorkbenches();
        workbenchList->enableWorkbenches(machiningWorkbenches);
    });
    connect(workbenchSelectorAll, &QRadioButton::clicked, [workbenchList]() {
        QSignalBlocker blocker(WorkbenchList);
        workbenchList->enableWorkbenches();
    });
    connect(workbenchList, &WorkbenchList::wbToggled, [workbenchSelectorCustom]() {
        workbenchSelectorCustom->click();
    });

    workbenchSelectorMechanic->click();

    // createListWidget();
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(label);
    layout->addWidget(workbenchSelectorMechanic);
    layout->addWidget(workbenchSelectorArchiBim);
    layout->addWidget(workbenchSelectorMachining);
    layout->addWidget(workbenchSelectorAll);
    layout->addWidget(workbenchSelectorCustom);
    layout->addWidget(workbenchList);

    // layout->addWidget(workbenchlist);
    setLayout(layout);
}

ConclusionPage::ConclusionPage(QWidget *parent)
    : QWizardPage(parent)
{
    // setTitle(tr("You are ready to go"));
    // setSubTitle(tr("Press exit to exit the wizard and start using Freecad"));
    // setPixmap(QWizard::LogoPixmap, QPixmap(":/images/icon-freecad.png"));
    // setPixmap(QWizard::BannerPixmap, QPixmap(":/images/banner.png"));

    QLabel* label = new QLabel();
    label->setTextFormat(Qt::MarkdownText);
    label->setText(R"""(
# Enjoy!

You are ready to go! Remember, you can always change these settings later in Edit > Preferences.

FreeCAD is completely free and made by volunteers having spent countless hours of their freetime, please take a brief moment to acknowledge them.
    )""");
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    setLayout(layout);
}
