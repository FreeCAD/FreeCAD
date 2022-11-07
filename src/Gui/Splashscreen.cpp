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
#include <Inventor/C/basic.h>
#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QLocale>
#include <QMutex>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QScreen>
#include <QSysInfo>
#include <QTextBrowser>
#include <QTextStream>
#include <QWaitCondition>
#include <cstdlib>
#endif

#include <App/Application.h>
#include <App/Metadata.h>
#include <Base/Console.h>
#include <CXX/WrapPython.h>

#include <LibraryVersions.h>
#include <boost/filesystem.hpp>
#include <zlib.h>

#include "MainWindow.h"
#include "Splashscreen.h"
#include "ui_AboutApplication.h"

#include <sstream>
#include <strstream>

using namespace Gui;
using namespace Gui::Dialog;
namespace fs = boost::filesystem;

namespace Gui
{


/** Displays all messages at startup inside the splash screen.
 * \author Werner Mayer
 */
class SplashObserver: public Base::ILogger
{
public:
    SplashObserver(const SplashObserver&) = delete;
    SplashObserver(SplashObserver&&) = delete;
    SplashObserver& operator=(const SplashObserver&) = delete;
    SplashObserver& operator=(SplashObserver&&) = delete;

    explicit SplashObserver(QSplashScreen& splasher) : splash {splasher}
    {
        Base::Console().AttachObserver(this);
    }
    ~SplashObserver() override
    {
        Base::Console().DetachObserver(this);
    }
    const char* Name() override
    {
        return "SplashObserver";
    }

    void SendLog(const std::string& msg, Base::LogStyle level) override
    {
#ifdef FC_DEBUG
        Log(msg.c_str());
        Q_UNUSED(level)
#else
        if (level == Base::LogStyle::Log) {
            Log(msg.c_str());
        }
#endif
    }
    void Log(const char* text)
    {
        QString msg(QString::fromUtf8(text));
        QRegularExpression rx;
        // ignore 'Init:' and 'Mod:' prefixes
        rx.setPattern(QLatin1String("^\\s*(Init:|Mod:)\\s*"));
        auto match = rx.match(msg);
        if (match.hasMatch()) {
            msg = msg.mid(match.capturedLength());
        }
        else {
            // ignore activation of commands
            rx.setPattern(QLatin1String("^\\s*(\\+App::|Create|CmdC:|CmdG:|Act:)\\s*"));
            match = rx.match(msg);
            if (match.hasMatch() && match.capturedStart() == 0) {
                return;
            }
        }
        msg.replace(QLatin1String("\n"), QString());

        /**
         * Previously SplashObserver got involved with the formatting of a message.
         * This has been removed. SplashObserver has no responsibility for formatting (SRP)
         */
        splash.showMessage(msg);

        QMutex mutex;
        QMutexLocker ml(&mutex);
        QWaitCondition().wait(&mutex, 50);
    }

private:
    QSplashScreen& splash;
};
}// namespace Gui

// ------------------------------------------------------------------------------

/**
 * Constructs a splash screen that will display the pixmap.
 */
SplashScreen::SplashScreen(const QPixmap& pixmap, Qt::WindowFlags f) : QSplashScreen {pixmap, f}
{
    const std::map<std::string, std::string>& config = App::Application::Config();

    /**
     * Breaking up code by SRP highlighted conditions that were not taken into account
     * Even so, it is dubious that the following has to happen here
     * This is a dependency that could perhaps better be injected
     */

    std::string colStr = fromConfig(config, "SplashTextColor");
    if (!colStr.empty()) {
        auto color = colorFromString(colStr);
        if (color.isValid()) {
            textColor = color;
        }
    }

    std::string alignStr = fromConfig(config, "SplashAlignment");
    if (!alignStr.empty()) {
        alignment = alignStrToInt(alignStr);
    }

    splashObserver = new SplashObserver(*this);
}

/** Destruction. */
SplashScreen::~SplashScreen()
{
    delete splashObserver;
}

void SplashScreen::showMessage(const QString& message)
{
    QSplashScreen::showMessage(message, alignment, textColor);
}

std::string SplashScreen::fromConfig(const std::map<std::string, std::string>& config,
                                     const std::string value)
{
    auto al = config.find(value);
    return al == config.end() ? "" : al->second.c_str();
}

QColor SplashScreen::colorFromString(const std::string str)
{
    QColor color {};
    color.setNamedColor(QString::fromStdString(str));
    return color;
}

int SplashScreen::alignStrToInt(const std::string str)
{
    // c++20 has .ends_with and .starts_with !!
    QString qstr {QString::fromStdString(str)};
    int align {};
    if (qstr.startsWith(QLatin1String("VCenter"))) {
        align = Qt::AlignVCenter;
    }
    else if (qstr.startsWith(QLatin1String("Top"))) {
        align = Qt::AlignTop;
    }
    else {
        align = Qt::AlignBottom;
    }

    if (qstr.endsWith(QLatin1String("HCenter"))) {
        align += Qt::AlignHCenter;
    }
    else if (qstr.endsWith(QLatin1String("Right"))) {
        align += Qt::AlignRight;
    }
    else {
        align += Qt::AlignLeft;
    }
    return align;
}

/**
 * Draws the contents of the splash screen using painter \a painter. The default
 * implementation draws the message passed by message().
 */
void SplashScreen::drawContents(QPainter* painter)
{
    QSplashScreen::drawContents(painter);
}

// ------------------------------------------------------------------------------

constexpr bool use3D =
#ifdef _USE_3DCONNEXION_SDK
    true;
#else
    false;
#endif

// ------------------------------------------------------------------------------

QDialog* AboutDialogFactory::create(QWidget* parent) const
{
    return new AboutDialog(use3D, parent);
}

const AboutDialogFactory* AboutDialogFactory::defaultFactory()
{
    static const AboutDialogFactory this_factory;
    return factory ? factory : &this_factory;
}

void AboutDialogFactory::setDefaultFactory(AboutDialogFactory* f)
{
    if (factory != f)
        delete factory;
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
AboutDialog::AboutDialog(bool showLic, QWidget* parent)
    : QDialog(parent)
{
    Q_UNUSED(showLic);

    ui = new Ui_AboutApplication;
    summaryReport new SummaryReport;
    setModal(true);
    ui->setupUi(this);

    // remove the automatic help button in dialog title since we don't use it
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    layout()->setSizeConstraint(QLayout::SetFixedSize);
    QRect rect = QApplication::primaryScreen()->availableGeometry();

    // See if we have a custom About screen image set
    QPixmap image = getMainWindow()->aboutImage();

    // Fallback to the splashscreen image
    if (image.isNull()) {
        image = getMainWindow()->splashImage();
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
    ui->tabWidget->setCurrentIndex(0);// always start on the About tab

    setupLabels();
    showCredits();
    showLicenseInformation();
    showLibraryInformation();
    showCollectionInformation();
    showOrHideImage(rect);
}

/**
 *  Destroys the object and frees any allocated resources
 */
AboutDialog::~AboutDialog()
{
    // no need to delete child widgets, Qt does it all for us
    delete summaryReport;
    delete ui;
}

void AboutDialog::showOrHideImage(const QRect& rect)
{
    adjustSize();
    if (height() > rect.height()) {
        ui->labelSplashPicture->hide();
    }
}

/**
 * the following code smells ripe for improvement, but was left alone, for now.
 * Likely issues with DRY and SRP.
 */
void AboutDialog::setupLabels()
{
    //fonts are rendered smaller on Mac so point size can't be the same for all platforms
    int fontSize = 8;
#ifdef Q_OS_MAC
    fontSize = 11;
#endif
    //avoid overriding user set style sheet
    if (qApp->styleSheet().isEmpty()) {
        setStyleSheet(QString::fromLatin1("Gui--Dialog--AboutDialog QLabel {font-size: %1pt;}")
                          .arg(fontSize));
    }

    QString exeName = qApp->applicationName();
    std::map<std::string, std::string>& config = App::Application::Config();
    std::map<std::string, std::string>::iterator it;
    QString banner = QString::fromUtf8(config["CopyrightInfo"].c_str());
    banner = banner.left(banner.indexOf(QLatin1Char('\n')));
    QString major = QString::fromLatin1(config["BuildVersionMajor"].c_str());
    QString minor = QString::fromLatin1(config["BuildVersionMinor"].c_str());
    QString build = QString::fromLatin1(config["BuildRevision"].c_str());
    QString disda = QString::fromLatin1(config["BuildRevisionDate"].c_str());
    QString mturl = QString::fromLatin1(config["MaintainerUrl"].c_str());

    // we use replace() to keep label formatting, so a label with text "<b>Unknown</b>"
    // gets replaced to "<b>FreeCAD</b>", for example

    QString author = ui->labelAuthor->text();
    author.replace(QString::fromLatin1("Unknown Application"), exeName);
    author.replace(QString::fromLatin1("(c) Unknown Author"), banner);
    ui->labelAuthor->setText(author);
    ui->labelAuthor->setUrl(mturl);

    if (qApp->styleSheet().isEmpty()) {
        ui->labelAuthor->setStyleSheet(
            QString::fromLatin1("Gui--UrlLabel {color: #0000FF;text-decoration: "
                                "underline;font-weight: 600;font-family: MS Shell Dlg 2;}"));
    }

    QString version = ui->labelBuildVersion->text();
    version.replace(QString::fromLatin1("Unknown"), QString::fromLatin1("%1.%2").arg(major, minor));
    ui->labelBuildVersion->setText(version);

    QString revision = ui->labelBuildRevision->text();
    revision.replace(QString::fromLatin1("Unknown"), build);
    ui->labelBuildRevision->setText(revision);

    QString date = ui->labelBuildDate->text();
    date.replace(QString::fromLatin1("Unknown"), disda);
    ui->labelBuildDate->setText(date);

    QString os = ui->labelBuildOS->text();
    os.replace(QString::fromLatin1("Unknown"), QSysInfo::prettyProductName());
    ui->labelBuildOS->setText(os);

    QString platform = ui->labelBuildPlatform->text();
    platform.replace(QString::fromLatin1("Unknown"),
                     QString::fromLatin1("%1-bit").arg(QSysInfo::WordSize));
    ui->labelBuildPlatform->setText(platform);

    // branch name
    it = config.find("BuildRevisionBranch");
    if (it != config.end()) {
        QString branch = ui->labelBuildBranch->text();
        branch.replace(QString::fromLatin1("Unknown"), QString::fromUtf8(it->second.c_str()));
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
            QString::fromLatin1(it->second.c_str()).left(7));// Use the 7-char abbreviated hash
        ui->labelBuildHash->setText(hash);
        if (auto url_itr = config.find("BuildRepositoryURL"); url_itr != config.end()) {
            auto url = QString::fromStdString(url_itr->second);

            if (int space = url.indexOf(QChar::fromLatin1(' ')); space != -1)
                url = url.left(space);// Strip off the branch information to get just the repo

            if (url == QString::fromUtf8("Unknown"))
                url = QString::fromUtf8("https://github.com/FreeCAD/FreeCAD");// Just take a guess

            // This may only create valid URLs for Github,
            // but some other hosts use the same format so give it a shot...
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

/**
 * Replace four (almost) identical code blocks with one (DRY)
 */
QTextBrowser* AboutDialog::showCommon(const char* name, const char* tabName,
                                      const bool openExternalLinks, const bool openLinks)
{

    auto tab_license = new QWidget();
    tab_license->setObjectName(QString::fromLatin1(name));
    ui->tabWidget->addTab(tab_license, tr(tabName));

    auto textField = new QTextBrowser(tab_license);
    textField->setOpenExternalLinks(openExternalLinks);// default: false
    textField->setOpenLinks(openLinks);                // default: true

    auto hlayout = new QVBoxLayout(tab_license);
    hlayout->addWidget(textField);

    return textField;
}

void AboutDialog::showCredits()
{
    QFile creditsFile {QLatin1String(":/doc/CONTRIBUTORS")};

    if (!creditsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextBrowser* textBrowser = showCommon("tab_credits", "Credits", false, false);

    QString creditsHTML = QString::fromLatin1("<html><body><h1>");
    //: Header for the Credits tab of the About screen
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
    textBrowser->setHtml(creditsHTML);
}

void AboutDialog::showLicenseInformation()
{
    QString licenseFileURL = QString::fromLatin1("%1/LICENSE.html")
                                 .arg(QString::fromUtf8(App::Application::getHelpDir().c_str()));
    QFile licenseFile(licenseFileURL);

    if (licenseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString licenseHTML = QString::fromUtf8(licenseFile.readAll());
        const auto placeholder =
            QString::fromUtf8("<!--PLACEHOLDER_FOR_ADDITIONAL_LICENSE_INFORMATION-->");
        licenseHTML.replace(placeholder, getAdditionalLicenseInformation());

        ui->tabWidget->removeTab(1);// Hide the license placeholder widget
        QTextBrowser* textBrowser = showCommon("tab_license", "License", true, true);
        textBrowser->setHtml(licenseHTML);
    }
    else {
        QString info(QLatin1String("SUCH DAMAGES.<hr/>"));
        info += getAdditionalLicenseInformation();
        QString lictext = ui->textBrowserLicense->toHtml();
        lictext.replace(QString::fromLatin1("SUCH DAMAGES.<hr/>"), info);
        ui->textBrowserLicense->setHtml(lictext);
    }
}

/**
 * A whole function just for additional license information?
 * Is this really necessary?
  */
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

/**
 * macros are ugly!!
 * Lets move these back a bit from the action
 */
constexpr const char* excludeOCC =
#if defined(HAVE_OCC_VERSION)
    "";
#else
    "exclude";
#endif

constexpr const char* smeshVer =
#if defined(SMESH_VERSION_STR)
    SMESH_VERSION_STR;
#else
    "";
#endif

/**
 * replacing code with data
 *
 * name, href, url, version, exclude
 */
constexpr const char* libInfoData[][5] {
    {"Boost", "#_TocBoost", "https://www.boost.org", BOOST_LIB_VERSION},
    {"Coin3D", "#_TocCoin3D", "https://coin3d.github.io", COIN_VERSION},
    {"Eigen", "#_TocEigen", "https://eigen.tuxfamily.org", FC_EIGEN3_VERSION},
    {"FreeType", "#_TocFreeType", "https://freetype.org", FC_FREETYPE_VERSION},
    {"KDL", "#_TocKDL", "https://www.orocos.org/kdl"},
    {"libarea", "#_TocLibArea", "https://github.com/danielfalck/libarea"},
    {"Open CASCADE Technology", "#_TocOCCT", "https://www.opencascade.com/open-cascade-technology/",
     OCC_VERSION_STRING_EXT, excludeOCC},
    {"Point Cloud Library", "#_TocPcl", "https://www.pointclouds.org", FC_PCL_VERSION},
    {"PyCXX", "#_TocPyCXX", "http://cxx.sourceforge.net", FC_PYCXX_VERSION},
    {"Python", "#_TocPython", "https://www.python.org", PY_VERSION},
    {"Qt for Python (PySide)", "#_TocPySide", "https://wiki.qt.io/Qt_for_Python",
     FC_PYSIDE_VERSION},
    {"Qt", "#_TocQt", "https://www.qt.io", QT_VERSION_STR},
    {"Salome SMESH", "#_TocSalomeSMESH", "https://salome-platform.org", smeshVer},
    {"Qt for Python (Shiboken)", "#_TocPySide", "https://wiki.qt.io/Qt_for_Python",
     FC_SHIBOKEN_VERSION},
    {"vtk", "#_TocVtk", "https://www.vtk.org", FC_VTK_VERSION},
    {"Xerces-C", "#_TocXercesC", "https://xerces.apache.org/xerces-c", FC_XERCESC_VERSION},
    {"Zipios++", "#_TocZipios", "https://zipios.sourceforge.io"},
    {"zlib", "#_TocZlib", "https://zlib.net", ZLIB_VERSION},
};

QString AboutDialog::libraryInfoAsHtml()
{
    QString html;
    QTextStream out(&html);
    QString baseUrl = QString::fromLatin1("file:///%1/ThirdPartyLibraries.html")
                          .arg(QString::fromUtf8(App::Application::getHelpDir().c_str()));

    out << "<html><head/><body style=\" font-family:'MS Shell Dlg 2'; font-size:8.25pt; "
           "font-weight:400; font-style:normal;\">"
           "<p>"
        << tr("This software uses open source components whose copyright and other "
              "proprietary rights belong to their respective owners:")
        << "<br/></p>\n<ul>\n";

    /**
     * ****************************************************************
     * Replace a whole bunch of repeating code, with one (+ data) (DRY)
     * ****************************************************************
     * MUCH more maintainable
     */
    for (const auto& info : libInfoData) {
        if (!info[4]) {
            out << "<li>"
                << "<p>" << info[0] << " " << info[2] << "</p>"
                << "<p><a href=\"" << baseUrl << info[1] << "\">" << info[3] << "</a><br></p>"
                << "</li>\n";
        }
    }
    out << "</ul>\n</body>\n</html>";
    return html;
}

void AboutDialog::showLibraryInformation()
{
    QTextBrowser* textBrowser = showCommon("tab_library", "Libraries", false, false);
    connect(textBrowser, &QTextBrowser::anchorClicked, this, &AboutDialog::linkActivated);
}

void AboutDialog::showCollectionInformation()
{
    QString doc = QString::fromUtf8(App::Application::getHelpDir().c_str());
    QString path = doc + QLatin1String("Collection.html");
    if (!QFile::exists(path)) {
        return;
    }

    QTextBrowser* textBrowser = showCommon("tab_collection", "Collection", true);
    textBrowser->setSource(path);
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


/**
 * Having got this far in the refactoring (mainly SRP), it is clear that this
 * function is very much the tail wagging the dog, ie demanding info from all over the app,
 * violating "Tell, Don't Ask".
 *
 * It has far too many responsibilities, and far too many dependencies.
 *
 * Without a working system this function is impossible to test.
 *
 * At the end of the day, this function is just combining disparate information strings. And that's
 * all it should be doing.
 *
 * The config items should be put in config. But currently config is just a variable, so it
 * should be wrapped in a class with various io handlers (value in/out as stdString, QString
 * etc), itemExists() function, valueEmpty() functionn etc.
*/
std::string AboutDialog::makeSummaryReport()
{

    // should be moved to where config is managed
    // at this level we should not need to know the internals of config
    auto configFind = [](std::string name) {
        std::map<std::string, std::string>& config = App::Application::Config();
        auto it = config.find(name);
        return it == config.end() ? "" : it->second;
    };

    // should be moved to where config is managed
    // at this level we should not need to know the internals of config
    auto configFetch = [](std::string name) {
        return App::Application::Config()[name];
    };

    auto sysEnv = [](const std::string & name) {
        return QProcessEnvironment::systemEnvironment()
            .value(QString::fromStdString(name), QString())
            .toStdString();
    };

    auto deskInfo = [&sysEnv] {
        std::string deskEnv = sysEnv("XDG_CURRENT_DESKTOP");
        std::string deskSess = sysEnv("DESKTOP_SESSION");

        std::string result {};
        if (!(deskEnv.empty() && deskSess.empty())) {
            result += " (" + deskEnv;
            if (deskEnv.empty() || deskSess.empty()) {
                result += deskSess;
            }
            else {
                result += "/" + deskSess;
            }
            result += ")";
        }
        result += '\n';
        return result;
    };

    auto wordSize = [](const char* title) {
        int wordSize = static_cast<int>(QSysInfo::WordSize);
        std::string result = title + App::Application::getExecutableName() + ": ";
        result += wordSize;
        result += "-bit\n";
        return result;
    };

    auto version = [&configFetch](const char* title) {
        return title + configFetch("BuildVersionMajor") + "." + configFetch("BuildVersionMinor")
            + "." + configFetch("BuildRevision");
    };

    auto ifOcc = [](const char* title) {
        std::string result {};
#if defined(HAVE_OCC_VERSION)
        result = title;
        result += OCC_VERSION_MAJOR;
        result += ".";
        result += OCC_VERSION_MINOR;
        result += ".";
        result += OCC_VERSION_MAINTENANCE;
#ifdef OCC_VERSION_DEVELOPMENT
        result += OCC_VERSION_DEVELOPMENT
#endif
#endif
            return result;
    };

    auto ifAppImage = [](const std::string title) {
        std::string result {};
        auto* appimage = getenv("APPIMAGE");
        if (appimage) {
            result = title;
        }
        return result;
    };

    auto ifSnap = [](const std::string title) {
        std::string result {};
        std::string snap = getenv("SNAP_REVISION");
        if (!snap.empty()) {
            result = title + snap;
        }
        return result;
    };

    auto ifBuildRevBranch = [&configFind](const std::string title) {
        std::string result {};
        auto rev = configFind("BuildRevisionBranch");
        if (!rev.empty()) {
            result = title + rev + '\n';
        }
        return result;
    };

    auto ifBuildRevHash = [&configFind](const std::string title) {
        std::string result {};
        auto hash = configFind("BuildRevisionBranch");
        if (!hash.empty()) {
            result = title + hash + '\n';
        }
        return result;
    };

    auto locale = [](const std::string title) {
        auto asStr = [](const QLocale& loc) {
            return loc.languageToString(loc.language()).toStdString() + "/"
                + loc.countryToString(loc.country()).toStdString() + " (" + loc.name().toStdString()
                + ")";
        };
        QLocale loc {};
        std::string result = title + asStr(loc);
        QLocale sysLoc = QLocale::system();
        if (loc != sysLoc) {
            result += " [ OS: " + asStr(sysLoc) + "]";
        }
        return result + '\n';
    };

    // should be moved to where modules are managed
    // at this level we should not need to know the internals of modules
    auto modulePaths = [] {
        std::vector<fs::path> paths {};
        auto modDir {fs::path(App::Application::getUserAppDataDir()) / "Mod"};
        if (fs::exists(modDir) && fs::is_directory(modDir)) {
            for (const auto& mod : fs::directory_iterator(modDir)) {
                if (mod.path().leaf().string()[0] != '.') {
                    paths.emplace_back(mod.path());
                }
            };
        }
        return paths;
    };

    // should be moved to where modules are managed
    // at this level we should not need to know the internals of modules
    auto moduleInfo = [](const fs::path& path) {
        std::string version {};
        std::string disabled {};
        std::string name = path.leaf().string();
        const auto metadataFile {path / "package.xml"};
        if (fs::exists(metadataFile)) {
            App::Metadata metadata {metadataFile};
            if (metadata.version() != App::Meta::Version()) {
                version = " " + metadata.version().str();
            }
        }
        if (fs::exists(path / "ADDON_DISABLED")) {
            disabled = " (Disabled)";
        }
        return name + version + disabled;
    };

    auto moduleInfoStrs = [&modulePaths, &moduleInfo] {
        std::vector<fs::path> paths = modulePaths();
        std::vector<std::string> strs = {};
        for (const auto& path : paths) {
            strs.emplace_back(moduleInfo(path));
        }
        return strs;
    };

    // this now cares not about modules management
    auto ifInstalledModuleInfo = [&moduleInfoStrs](const std::string title) {
        const std::vector<std::string> strs = moduleInfoStrs();
        std::string res {};
        if (!strs.empty()) {
            res += title;
            for (const auto& str : strs) {
                res += "  * " + str + '\n';
            }
        }
        return res;
    };

    // Smells like recreating the wheel
    auto buildType = [](std::string title) -> std::string {
        std::string bld
        {
#if defined(_DEBUG) || defined(DEBUG)
            "Debug"
#elif defined(NDEBUG)
            "Release"
#elif defined(CMAKE_BUILD_TYPE)
            CMAKE_BUILD_TYPE
#else
            "Unknown"
#endif
        };
        return title + bld + '\n';
    };


    /**
     * This is this function's CORE responsibility: assemble info strings SRP
     * all the rest is just undesirable responsibilities)
     *
     * Even before moving all the above closures out, use them to populate report
     */
    summaryReport.addItem(QSysInfo::prettyProductName().toStdString());
    summaryReport.addItem(deskInfo());
    summaryReport.addItem(wordSize("Word size of "));
    summaryReport.addItem(version("Version: "));
    summaryReport.addItem(ifAppImage(" AppImage"));
    summaryReport.addItem(ifSnap(" Snap ") + '\n');
    summaryReport.addItem(buildType("Build type: "));
    summaryReport.addItem(ifBuildRevBranch("Branch: "));
    summaryReport.addItem(ifBuildRevHash("Hash: "));
    summaryReport.addItem("Python " PY_VERSION ", ");
    summaryReport.addItem("Qt " QT_VERSION_STR ", ");
    summaryReport.addItem("Coin " COIN_VERSION ", ");
    summaryReport.addItem("Vtk " + std::string(FC_VTK_VERSION) + ", ");
    summaryReport.addItem(ifOcc("OCC "));
    summaryReport.addItem("\n");
    summaryReport.addItem(locale("Locale: "));
    summaryReport.addItem(ifInstalledModuleInfo("Installed mods: \n"));
}


void AboutDialog::on_copyButton_clicked()
{
    QClipboard* cb = QApplication::clipboard();
    std::string wrapped = summaryReport.codeWrap(summaryReport.asStdString());
    cb->setText(QString::fromStdString(wrapped));
}

// ----------------------------------------------------------------------------

    void SummaryReport::addItem(std::string item) {
        if(!item.empty()) {
            items.emplace_back(item);
        }
    }

    std::string SummaryReport::asStdString(){
        std::ostringstream ss;
        for (const auto & item : items){
            ss << item;
        }
        return ss.str();
    }

    // this smells like it should live someplace else
    std::string SummaryReport::codeWrap(std::string wrappee)
    {
        return "[code]\n" + wrappee + "[/code]\n";
    }

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::LicenseView */

LicenseView::LicenseView(QWidget* parent) : MDIView(nullptr, parent, Qt::WindowFlags())
{
    browser = new QTextBrowser(this);
    browser->setOpenExternalLinks(true);
    browser->setOpenLinks(true);
    setCentralWidget(browser);
}

LicenseView::~LicenseView()
{}

void LicenseView::setSource(const QUrl& url)
{
    browser->setSource(url);
}

#include "moc_Splashscreen.cpp"
