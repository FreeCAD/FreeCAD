/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <cstdlib>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QScreen>
#include <QSettings>
#include <QSysInfo>
#include <QTextBrowser>
#include <QTextStream>
#include <QTimer>
#include <Inventor/C/basic.h>
#endif

#include <App/Application.h>
#include <App/Metadata.h>
#include <Base/Console.h>
#include <CXX/WrapPython.h>

#include <boost/filesystem.hpp>
#include <LibraryVersions.h>
#include <zlib.h>

#include "BitmapFactory.h"
#include "DlgAbout.h"
#include "MainWindow.h"
#include "SplashScreen.h"
#include "ui_AboutApplication.h"

using namespace Gui;
using namespace Gui::Dialog;
namespace fs = boost::filesystem;

static QString prettyProductInfoWrapper()
{
    auto productName = QSysInfo::prettyProductName();
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
#ifdef FC_OS_MACOSX
    auto macosVersionFile =
        QString::fromUtf8("/System/Library/CoreServices/.SystemVersionPlatform.plist");
    auto fi = QFileInfo(macosVersionFile);
    if (fi.exists() && fi.isReadable()) {
        auto plistFile = QFile(macosVersionFile);
        plistFile.open(QIODevice::ReadOnly);
        while (!plistFile.atEnd()) {
            auto line = plistFile.readLine();
            if (line.contains("ProductUserVisibleVersion")) {
                auto nextLine = plistFile.readLine();
                if (nextLine.contains("<string>")) {
                    QRegularExpression re(QString::fromUtf8("\\s*<string>(.*)</string>"));
                    auto matches = re.match(QString::fromUtf8(nextLine));
                    if (matches.hasMatch()) {
                        productName = QString::fromUtf8("macOS ") + matches.captured(1);
                        break;
                    }
                }
            }
        }
    }
#endif
#endif
#ifdef FC_OS_WIN64
    QSettings regKey {
        QString::fromUtf8("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
        QSettings::NativeFormat};
    if (regKey.contains(QString::fromUtf8("CurrentBuildNumber"))) {
        auto buildNumber = regKey.value(QString::fromUtf8("CurrentBuildNumber")).toInt();
        if (buildNumber > 0) {
            if (buildNumber < 9200) {
                productName = QString::fromUtf8("Windows 7 build %1").arg(buildNumber);
            }
            else if (buildNumber < 10240) {
                productName = QString::fromUtf8("Windows 8 build %1").arg(buildNumber);
            }
            else if (buildNumber < 22000) {
                productName = QString::fromUtf8("Windows 10 build %1").arg(buildNumber);
            }
            else {
                productName = QString::fromUtf8("Windows 11 build %1").arg(buildNumber);
            }
        }
    }
#endif
    return productName;
}

// ------------------------------------------------------------------------------

AboutDialogFactory* AboutDialogFactory::factory = nullptr;

AboutDialogFactory::~AboutDialogFactory() = default;

QDialog* AboutDialogFactory::create(QWidget* parent) const
{
    return new AboutDialog(parent);
}

const AboutDialogFactory* AboutDialogFactory::defaultFactory()
{
    static const AboutDialogFactory this_factory;
    if (factory) {
        return factory;
    }
    return &this_factory;
}

void AboutDialogFactory::setDefaultFactory(AboutDialogFactory* f)
{
    if (factory != f) {
        delete factory;
    }
    factory = f;
}

// ------------------------------------------------------------------------------

/* TRANSLATOR Gui::Dialog::AboutDialog */

/**
 *  Constructs an AboutDialog which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'WStyle_Customize|WStyle_NoBorder|WType_Modal'
 *
 *  The dialog will be modal.
 */
AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_AboutApplication)
{
    setModal(true);
    ui->setupUi(this);
    connect(ui->copyButton, &QPushButton::clicked, this, &AboutDialog::copyToClipboard);

    // remove the automatic help button in dialog title since we don't use it
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    layout()->setSizeConstraint(QLayout::SetFixedSize);
    QRect rect = QApplication::primaryScreen()->availableGeometry();

    // See if we have a custom About screen image set
    QPixmap image = aboutImage();

    // Fallback to the splashscreen image
    if (image.isNull()) {
        image = SplashScreen::splashImage();
    }

    // Make sure the image is not too big
    int denom = 2;
    if (image.height() > rect.height() / denom || image.width() > rect.width() / denom) {
        float scale = static_cast<float>(image.width()) / static_cast<float>(image.height());
        int width = std::min(image.width(), rect.width() / denom);
        int height = std::min(image.height(), rect.height() / denom);
        height = std::min(height, static_cast<int>(width / scale));
        width = static_cast<int>(scale * height);

        image = image.scaled(width, height);
    }
    ui->labelSplashPicture->setPixmap(image);
    ui->tabWidget->setCurrentIndex(0);  // always start on the About tab

    setupLabels();
    showCredits();
    showLicenseInformation();
    showLibraryInformation();
    showCollectionInformation();
    showPrivacyPolicy();
    showOrHideImage(rect);
}

/**
 *  Destroys the object and frees any allocated resources
 */
AboutDialog::~AboutDialog()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

QPixmap AboutDialog::aboutImage() const
{
    // See if we have a custom About screen image set
    QPixmap about_image;
    QFileInfo fi(QString::fromLatin1("images:about_image.png"));
    if (fi.isFile() && fi.exists()) {
        about_image.load(fi.filePath(), "PNG");
    }

    std::string about_path = App::Application::Config()["AboutImage"];
    if (!about_path.empty() && about_image.isNull()) {
        QString path = QString::fromStdString(about_path);
        if (QDir(path).isRelative()) {
            QString home = QString::fromStdString(App::Application::getHomePath());
            path = QFileInfo(QDir(home), path).absoluteFilePath();
        }
        about_image.load(path);

        // Now try the icon paths
        if (about_image.isNull()) {
            about_image = Gui::BitmapFactory().pixmap(about_path.c_str());
        }
    }

    return about_image;
}

void AboutDialog::showOrHideImage(const QRect& rect)
{
    adjustSize();
    if (height() > rect.height()) {
        ui->labelSplashPicture->hide();
    }
}

void AboutDialog::setupLabels()
{
    // fonts are rendered smaller on Mac so point size can't be the same for all platforms
    int fontSize = 8;
#ifdef Q_OS_MAC
    fontSize = 11;
#endif
    // avoid overriding user set style sheet
    if (qApp->styleSheet().isEmpty()) {
        setStyleSheet(QString::fromLatin1("Gui--Dialog--AboutDialog QLabel {font-size: %1pt;}")
                          .arg(fontSize));
    }

    QString exeName = qApp->applicationName();
    std::map<std::string, std::string>& config = App::Application::Config();
    std::map<std::string, std::string>::iterator it;
    QString banner = QString::fromStdString(config["CopyrightInfo"]);
    banner = banner.left(banner.indexOf(QLatin1Char('\n')));
    QString major = QString::fromStdString(config["BuildVersionMajor"]);
    QString minor = QString::fromStdString(config["BuildVersionMinor"]);
    QString point = QString::fromStdString(config["BuildVersionPoint"]);
    QString suffix = QString::fromStdString(config["BuildVersionSuffix"]);
    QString build = QString::fromStdString(config["BuildRevision"]);
    QString disda = QString::fromStdString(config["BuildRevisionDate"]);
    QString mturl = QString::fromStdString(config["MaintainerUrl"]);

    // we use replace() to keep label formatting, so a label with text "<b>Unknown</b>"
    // gets replaced to "<b>FreeCAD</b>", for example

    QString author = ui->labelAuthor->text();
    author.replace(QString::fromLatin1("Unknown Application"), exeName);
    author.replace(QString::fromLatin1("(c) Unknown Author"), banner);
    ui->labelAuthor->setText(author);
    ui->labelAuthor->setUrl(mturl);

    if (qApp->styleSheet().isEmpty()) {
        ui->labelAuthor->setStyleSheet(QString::fromLatin1(
            "Gui--UrlLabel {color: #0000FF;text-decoration: underline;font-weight: 600;}"));
    }

    QString version = ui->labelBuildVersion->text();
    version.replace(QString::fromLatin1("Unknown"),
                    QString::fromLatin1("%1.%2.%3%4").arg(major, minor, point, suffix));
    ui->labelBuildVersion->setText(version);

    QString revision = ui->labelBuildRevision->text();
    revision.replace(QString::fromLatin1("Unknown"), build);
    ui->labelBuildRevision->setText(revision);

    QString date = ui->labelBuildDate->text();
    date.replace(QString::fromLatin1("Unknown"), disda);
    ui->labelBuildDate->setText(date);

    QString os = ui->labelBuildOS->text();
    os.replace(QString::fromLatin1("Unknown"), prettyProductInfoWrapper());
    ui->labelBuildOS->setText(os);

    QString architecture = ui->labelBuildRunArchitecture->text();
    if (QSysInfo::buildCpuArchitecture() == QSysInfo::currentCpuArchitecture()) {
        architecture.replace(QString::fromLatin1("Unknown"), QSysInfo::buildCpuArchitecture());
    }
    else {
        architecture.replace(
            QString::fromLatin1("Unknown"),
            QString::fromLatin1("%1 (running on: %2)")
                .arg(QSysInfo::buildCpuArchitecture(), QSysInfo::currentCpuArchitecture()));
    }
    ui->labelBuildRunArchitecture->setText(architecture);

    // branch name
    it = config.find("BuildRevisionBranch");
    if (it != config.end()) {
        QString branch = ui->labelBuildBranch->text();
        branch.replace(QString::fromLatin1("Unknown"), QString::fromStdString(it->second));
        ui->labelBuildBranch->setText(branch);
    }
    else {
        ui->labelBranch->hide();
        ui->labelBuildBranch->hide();
    }

    // hash id
    it = config.find("BuildRevisionHash");
    if (it != config.end()) {
        QString hash = ui->labelBuildHash->text();
        hash.replace(
            QString::fromLatin1("Unknown"),
            QString::fromStdString(it->second).left(7));  // Use the 7-char abbreviated hash
        ui->labelBuildHash->setText(hash);
        if (auto url_itr = config.find("BuildRepositoryURL"); url_itr != config.end()) {
            auto url = QString::fromStdString(url_itr->second);

            if (int space = url.indexOf(QChar::fromLatin1(' ')); space != -1) {
                url = url.left(space);  // Strip off the branch information to get just the repo
            }

            if (url == QString::fromUtf8("Unknown")) {
                url = QString::fromUtf8("https://github.com/FreeCAD/FreeCAD");  // Just take a guess
            }

            // This may only create valid URLs for Github, but some other hosts use the same format
            // so give it a shot...
            auto https = url.replace(QString::fromUtf8("git://"), QString::fromUtf8("https://"));
            https.replace(QString::fromUtf8(".git"), QString::fromUtf8(""));
            ui->labelBuildHash->setUrl(https + QString::fromUtf8("/commit/")
                                       + QString::fromStdString(it->second));
        }
    }
    else {
        ui->labelHash->hide();
        ui->labelBuildHash->hide();
    }
}

void AboutDialog::showCredits()
{
    auto creditsFileURL = QLatin1String(":/doc/CONTRIBUTORS");
    QFile creditsFile(creditsFileURL);

    if (!creditsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    auto tab_credits = new QWidget();
    tab_credits->setObjectName(QString::fromLatin1("tab_credits"));
    ui->tabWidget->addTab(tab_credits, tr("Credits"));
    auto hlayout = new QVBoxLayout(tab_credits);
    auto textField = new QTextBrowser(tab_credits);
    textField->setOpenExternalLinks(true);
    hlayout->addWidget(textField);

    QString creditsHTML = QString::fromLatin1("<html><body><p>");
    //: Header for bgbsww
    creditsHTML +=
        tr("This version of FreeCAD is dedicated to the memory of Brad McLean, aka bgbsww.");
    //: Header for the Credits tab of the About screen
    creditsHTML += QString::fromLatin1("</p><h1>");
    creditsHTML += tr("Credits");
    creditsHTML += QString::fromLatin1("</h1><p>");
    creditsHTML += tr("FreeCAD would not be possible without the contributions of");
    creditsHTML += QString::fromLatin1(":</p><h2>");
    //: Header for the list of individual people in the Credits list.
    creditsHTML += tr("Individuals");
    creditsHTML += QString::fromLatin1("</h2><ul>");

    QTextStream stream(&creditsFile);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    stream.setCodec("UTF-8");
#endif
    QString line;
    while (stream.readLineInto(&line)) {
        if (!line.isEmpty()) {
            if (line == QString::fromLatin1("Firms")) {
                creditsHTML += QString::fromLatin1("</ul><h2>");
                //: Header for the list of companies/organizations in the Credits list.
                creditsHTML += tr("Organizations");
                creditsHTML += QString::fromLatin1("</h2><ul>");
            }
            else {
                creditsHTML += QString::fromLatin1("<li>") + line + QString::fromLatin1("</li>");
            }
        }
    }
    creditsHTML += QString::fromLatin1("</ul></body></html>");
    textField->setHtml(creditsHTML);
}

void AboutDialog::showLicenseInformation()
{
    QString licenseFileURL = QString::fromLatin1("%1/LICENSE.html")
                                 .arg(QString::fromStdString(App::Application::getHelpDir()));
    QFile licenseFile(licenseFileURL);

    if (licenseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString licenseHTML = QString::fromUtf8(licenseFile.readAll());
        const auto placeholder =
            QString::fromUtf8("<!--PLACEHOLDER_FOR_ADDITIONAL_LICENSE_INFORMATION-->");
        licenseHTML.replace(placeholder, getAdditionalLicenseInformation());

        ui->tabWidget->removeTab(1);  // Hide the license placeholder widget

        auto tab_license = new QWidget();
        tab_license->setObjectName(QString::fromLatin1("tab_license"));
        ui->tabWidget->addTab(tab_license, tr("License"));
        auto hlayout = new QVBoxLayout(tab_license);
        auto textField = new QTextBrowser(tab_license);
        textField->setOpenExternalLinks(true);
        textField->setOpenLinks(true);
        hlayout->addWidget(textField);

        textField->setHtml(licenseHTML);
    }
    else {
        QString info(QLatin1String("SUCH DAMAGES.<hr/>"));
        info += getAdditionalLicenseInformation();
        QString lictext = ui->textBrowserLicense->toHtml();
        lictext.replace(QString::fromLatin1("SUCH DAMAGES.<hr/>"), info);
        ui->textBrowserLicense->setHtml(lictext);
    }
}

QString AboutDialog::getAdditionalLicenseInformation() const
{
    // Any additional piece of text to be added after the main license text goes below.
    // Please set title in <h2> tags, license text in <p> tags
    // and add an <hr/> tag at the end to nicely separate license blocks
    QString info;
#ifdef _USE_3DCONNEXION_SDK
    info += QString::fromUtf8(
        "<h2>3D Mouse Support</h2>"
        "<p>Development tools and related technology provided under license from 3Dconnexion.<br/>"
        "Copyright &#169; 1992&ndash;2012 3Dconnexion. All rights reserved.</p>"
        "<hr/>");
#endif
    return info;
}

void AboutDialog::showLibraryInformation()
{
    auto tab_library = new QWidget();
    tab_library->setObjectName(QString::fromLatin1("tab_library"));
    ui->tabWidget->addTab(tab_library, tr("Libraries"));
    auto hlayout = new QVBoxLayout(tab_library);
    auto textField = new QTextBrowser(tab_library);
    textField->setOpenExternalLinks(false);
    textField->setOpenLinks(false);
    hlayout->addWidget(textField);

    QString baseurl = QString::fromLatin1("file:///%1/ThirdPartyLibraries.html")
                          .arg(QString::fromStdString(App::Application::getHelpDir()));

    textField->setSource(QUrl(baseurl));
}

void AboutDialog::showCollectionInformation()
{
    QString doc = QString::fromStdString(App::Application::getHelpDir());
    QString path = doc + QLatin1String("Collection.html");
    if (!QFile::exists(path)) {
        return;
    }

    auto tab_collection = new QWidget();
    tab_collection->setObjectName(QString::fromLatin1("tab_collection"));
    ui->tabWidget->addTab(tab_collection, tr("Collection"));
    auto hlayout = new QVBoxLayout(tab_collection);
    auto textField = new QTextBrowser(tab_collection);
    textField->setOpenExternalLinks(true);
    hlayout->addWidget(textField);
    textField->setSource(path);
}

void AboutDialog::showPrivacyPolicy()
{
    auto policyFileURL = QLatin1String(":/doc/PRIVACY_POLICY");
    QFile policyFile(policyFileURL);

    if (!policyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    auto text = QString::fromUtf8(policyFile.readAll());
    auto tabPrivacyPolicy = new QWidget();
    tabPrivacyPolicy->setObjectName(QString::fromLatin1("tabPrivacyPolicy"));
    ui->tabWidget->addTab(tabPrivacyPolicy, tr("Privacy Policy"));
    auto hLayout = new QVBoxLayout(tabPrivacyPolicy);
    auto textField = new QTextBrowser(tabPrivacyPolicy);
    textField->setOpenExternalLinks(true);
    hLayout->addWidget(textField);

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    // We can't actually render the markdown, so just display it as text
    textField->setText(text);
#else
    textField->setMarkdown(text);
#endif
}

void AboutDialog::linkActivated(const QUrl& link)
{
    auto licenseView = new LicenseView();
    licenseView->setAttribute(Qt::WA_DeleteOnClose);
    licenseView->show();
    QString title = tr("License");
    QString fragment = link.fragment();
    if (fragment.startsWith(QLatin1String("_Toc"))) {
        QString prefix = fragment.mid(4);
        title = QString::fromLatin1("%1 %2").arg(prefix, title);
    }
    licenseView->setWindowTitle(title);
    getMainWindow()->addWindow(licenseView);
    licenseView->setSource(link);
}

void AboutDialog::copyToClipboard()
{
    QString data;
    QTextStream str(&data);
    std::map<std::string, std::string>& config = App::Application::Config();
    std::map<std::string, std::string>::iterator it;
    QString exe = QString::fromStdString(App::Application::getExecutableName());

    QString major = QString::fromStdString(config["BuildVersionMajor"]);
    QString minor = QString::fromStdString(config["BuildVersionMinor"]);
    QString point = QString::fromStdString(config["BuildVersionPoint"]);
    QString suffix = QString::fromStdString(config["BuildVersionSuffix"]);
    QString build = QString::fromStdString(config["BuildRevision"]);

    QString deskEnv =
        QProcessEnvironment::systemEnvironment().value(QStringLiteral("XDG_CURRENT_DESKTOP"),
                                                       QString());
    QString deskSess =
        QProcessEnvironment::systemEnvironment().value(QStringLiteral("DESKTOP_SESSION"),
                                                       QString());
    QStringList deskInfoList;
    QString deskInfo;

    if (!deskEnv.isEmpty()) {
        deskInfoList.append(deskEnv);
    }
    if (!deskSess.isEmpty()) {
        deskInfoList.append(deskSess);
    }
    if (qGuiApp->platformName() != QLatin1String("windows")
        && qGuiApp->platformName() != QLatin1String("cocoa")) {
        deskInfoList.append(qGuiApp->platformName());
    }
    if (!deskInfoList.isEmpty()) {
        deskInfo = QLatin1String(" (") + deskInfoList.join(QLatin1String("/")) + QLatin1String(")");
    }

    str << "OS: " << prettyProductInfoWrapper() << deskInfo << '\n';
    if (QSysInfo::buildCpuArchitecture() == QSysInfo::currentCpuArchitecture()) {
        str << "Architecture: " << QSysInfo::buildCpuArchitecture() << "\n";
    }
    else {
        str << "Architecture: " << QSysInfo::buildCpuArchitecture()
            << "(running on: " << QSysInfo::currentCpuArchitecture() << ")\n";
    }
    str << "Version: " << major << "." << minor << "." << point << suffix << "." << build;
#ifdef FC_CONDA
    str << " Conda";
#endif
#ifdef FC_FLATPAK
    str << " Flatpak";
#endif
    char* appimage = getenv("APPIMAGE");
    if (appimage) {
        str << " AppImage";
    }
    char* snap = getenv("SNAP_REVISION");
    if (snap) {
        str << " Snap " << snap;
    }
    str << '\n';

#if defined(_DEBUG) || defined(DEBUG)
    str << "Build type: Debug\n";
#elif defined(NDEBUG)
    str << "Build type: Release\n";
#elif defined(CMAKE_BUILD_TYPE)
    str << "Build type: " << CMAKE_BUILD_TYPE << '\n';
#else
    str << "Build type: Unknown\n";
#endif
    it = config.find("BuildRevisionBranch");
    if (it != config.end()) {
        str << "Branch: " << QString::fromStdString(it->second) << '\n';
    }
    it = config.find("BuildRevisionHash");
    if (it != config.end()) {
        str << "Hash: " << QString::fromStdString(it->second) << '\n';
    }
    // report also the version numbers of the most important libraries in FreeCAD
    str << "Python " << PY_VERSION << ", ";
    str << "Qt " << QT_VERSION_STR << ", ";
    str << "Coin " << COIN_VERSION << ", ";
    str << "Vtk " << fcVtkVersion << ", ";
#if defined(HAVE_OCC_VERSION)
    str << "OCC " << OCC_VERSION_MAJOR << "." << OCC_VERSION_MINOR << "." << OCC_VERSION_MAINTENANCE
#ifdef OCC_VERSION_DEVELOPMENT
        << "." OCC_VERSION_DEVELOPMENT
#endif
        << '\n';
#endif
    QLocale loc;
    str << "Locale: " << QLocale::languageToString(loc.language()) << "/"
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
        << QLocale::countryToString(loc.country())
#else
        << QLocale::territoryToString(loc.territory())
#endif
        << " (" << loc.name() << ")";
    if (loc != QLocale::system()) {
        loc = QLocale::system();
        str << " [ OS: " << QLocale::languageToString(loc.language()) << "/"
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
            << QLocale::countryToString(loc.country())
#else
            << QLocale::territoryToString(loc.territory())
#endif
            << " (" << loc.name() << ") ]";
    }
    str << "\n";

    // Add Stylesheet/Theme/Qtstyle information
    std::string styleSheet =
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow")
            ->GetASCII("StyleSheet");
    std::string theme =
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow")
            ->GetASCII("Theme");
#if QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
    std::string style = qApp->style()->name().toStdString();
#else
    std::string style =
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow")
            ->GetASCII("QtStyle");
    if (style.empty()) {
        style = "Qt default";
    }
#endif
    if (styleSheet.empty()) {
        styleSheet = "unset";
    }
    if (theme.empty()) {
        theme = "unset";
    }

    str << "Stylesheet/Theme/QtStyle: " << QString::fromStdString(styleSheet) << "/"
        << QString::fromStdString(theme) << "/" << QString::fromStdString(style) << "\n";

    // Add installed module information:
    auto modDir = fs::path(App::Application::getUserAppDataDir()) / "Mod";
    bool firstMod = true;
    if (fs::exists(modDir) && fs::is_directory(modDir)) {
        for (const auto& mod : fs::directory_iterator(modDir)) {
            auto dirName = mod.path().filename().string();
            if (dirName[0] == '.') {  // Ignore dot directories
                continue;
            }
            if (firstMod) {
                firstMod = false;
                str << "Installed mods: \n";
            }
            str << "  * " << QString::fromStdString(mod.path().filename().string());
            auto metadataFile = mod.path() / "package.xml";
            if (fs::exists(metadataFile)) {
                App::Metadata metadata(metadataFile);
                if (metadata.version() != App::Meta::Version()) {
                    str << QLatin1String(" ") + QString::fromStdString(metadata.version().str());
                }
            }
            auto disablingFile = mod.path() / "ADDON_DISABLED";
            if (fs::exists(disablingFile)) {
                str << " (Disabled)";
            }

            str << "\n";
        }
    }

    QClipboard* cb = QApplication::clipboard();
    cb->setText(data);

    auto copytext = ui->copyButton->text();
    ui->copyButton->setText(tr("Copied!"));
    const int timeout = 2000;
    QTimer::singleShot(timeout, this, [this, copytext]() {
        ui->copyButton->setText(copytext);
    });
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::LicenseView */

LicenseView::LicenseView(QWidget* parent)
    : MDIView(nullptr, parent, Qt::WindowFlags())
{
    browser = new QTextBrowser(this);
    browser->setOpenExternalLinks(true);
    browser->setOpenLinks(true);
    setCentralWidget(browser);
}

LicenseView::~LicenseView() = default;

void LicenseView::setSource(const QUrl& url)
{
    browser->setSource(url);
}

#include "moc_DlgAbout.cpp"
