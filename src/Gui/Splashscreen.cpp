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
# include <cstdlib>
# include <QApplication>
# include <QClipboard>
# include <QDesktopWidget>
# include <QDesktopServices>
# include <QDialogButtonBox>
# include <QLocale>
# include <QMutex>
# include <QTextBrowser>
# include <QProcess>
# include <QProcessEnvironment>
# include <QScreen>
# include <QSysInfo>
# include <QTextStream>
# include <QWaitCondition>
# include <Inventor/C/basic.h>
#endif


#include <LibraryVersions.h>
#include <zlib.h>
#include <boost/version.hpp>

#include "Splashscreen.h"
#include "ui_AboutApplication.h"
#include <Base/Console.h>
#include <CXX/WrapPython.h>
#include <App/Application.h>
#include <Gui/MainWindow.h>


using namespace Gui;
using namespace Gui::Dialog;

namespace Gui {
/** Displays all messages at startup inside the splash screen.
 * \author Werner Mayer
 */
class SplashObserver : public Base::ILogger
{
public:
    SplashObserver(QSplashScreen* splasher=0)
      : splash(splasher), alignment(Qt::AlignBottom|Qt::AlignLeft), textColor(Qt::black)
    {
        Base::Console().AttachObserver(this);

        // allow to customize text position and color
        const std::map<std::string,std::string>& cfg = App::GetApplication().Config();
        std::map<std::string,std::string>::const_iterator al = cfg.find("SplashAlignment");
        if (al != cfg.end()) {
            QString alt = QString::fromLatin1(al->second.c_str());
            int align=0;
            if (alt.startsWith(QLatin1String("VCenter")))
                align = Qt::AlignVCenter;
            else if (alt.startsWith(QLatin1String("Top")))
                align = Qt::AlignTop;
            else
                align = Qt::AlignBottom;

            if (alt.endsWith(QLatin1String("HCenter")))
                align += Qt::AlignHCenter;
            else if (alt.endsWith(QLatin1String("Right")))
                align += Qt::AlignRight;
            else
                align += Qt::AlignLeft;

            alignment = align;
        }

        // choose text color
        std::map<std::string,std::string>::const_iterator tc = cfg.find("SplashTextColor");
        if (tc != cfg.end()) {
            QColor col; col.setNamedColor(QString::fromLatin1(tc->second.c_str()));
            if (col.isValid())
                textColor = col;
        }
    }
    virtual ~SplashObserver()
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
    void Log (const char * s)
    {
        QString msg(QString::fromUtf8(s));
        QRegExp rx;
        // ignore 'Init:' and 'Mod:' prefixes
        rx.setPattern(QLatin1String("^\\s*(Init:|Mod:)\\s*"));
        int pos = rx.indexIn(msg);
        if (pos != -1) {
            msg = msg.mid(rx.matchedLength());
        }
        else {
            // ignore activation of commands
            rx.setPattern(QLatin1String("^\\s*(\\+App::|Create|CmdC:|CmdG:|Act:)\\s*"));
            pos = rx.indexIn(msg);
            if (pos == 0)
                return;
        }

        splash->showMessage(msg.replace(QLatin1String("\n"), QString()), alignment, textColor);
        QMutex mutex;
        QMutexLocker ml(&mutex);
        QWaitCondition().wait(&mutex, 50);
    }

private:
    QSplashScreen* splash;
    int alignment;
    QColor textColor;
};
} // namespace Gui

// ------------------------------------------------------------------------------

/**
 * Constructs a splash screen that will display the pixmap.
 */
SplashScreen::SplashScreen(  const QPixmap & pixmap , Qt::WindowFlags f )
    : QSplashScreen(pixmap, f)
{
    // write the messages to splasher
    messages = new SplashObserver(this);
}

/** Destruction. */
SplashScreen::~SplashScreen()
{
    delete messages;
}

/**
 * Draws the contents of the splash screen using painter \a painter. The default
 * implementation draws the message passed by message().
 */
void SplashScreen::drawContents ( QPainter * painter )
{
    QSplashScreen::drawContents(painter);
}

// ------------------------------------------------------------------------------

AboutDialogFactory* AboutDialogFactory::factory = 0;

AboutDialogFactory::~AboutDialogFactory()
{
}

QDialog *AboutDialogFactory::create(QWidget *parent) const
{
    return new AboutDialog(parent);
}

const AboutDialogFactory *AboutDialogFactory::defaultFactory()
{
    static const AboutDialogFactory this_factory;
    if (factory)
        return factory;
    return &this_factory;
}

void AboutDialogFactory::setDefaultFactory(AboutDialogFactory *f)
{
    if (factory != f)
        delete factory;
    factory = f;
}

// ------------------------------------------------------------------------------

/* TRANSLATOR Gui::Dialog::AboutDialog */

/**
 *  Constructs an AboutDialog which is a child of 'parent'.
 *
 *  The dialog will be modal.
 */
AboutDialog::AboutDialog(QWidget* parent)
  : QDialog(parent), ui(new Ui_AboutApplication)
{
    setModal(true);
    ui->setupUi(this);
    layout()->setSizeConstraint(QLayout::SetFixedSize);
    QRect rect = QApplication::primaryScreen()->availableGeometry();
    QPixmap image = getMainWindow()->splashImage();

    // Make sure the image is not too big
    if (image.height() > rect.height()/2 || image.width() > rect.width()/2) {
        image = image.scaled (rect.width()/2, rect.height()/2, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    ui->labelSplashPicture->setPixmap(image);

    ui->tabWidget->setCurrentIndex(0); // always start on the About tab
    setupLabels();
    showCredits();
    showLicenseInformation();
    showLibraryInformation();
    showCollectionInformation();
}

void AboutDialog::setupLabels()
{
    //fonts are rendered smaller on Mac so point size can't be the same for all platforms
    int fontSize = 8;
#ifdef Q_OS_MAC
    fontSize = 11;
#endif
    //avoid overriding user set style sheet
    if (qApp->styleSheet().isEmpty()) {
        setStyleSheet(QString::fromLatin1("Gui--Dialog--AboutDialog QLabel {font-size: %1pt;}").arg(fontSize));
    }

    QString exeName = qApp->applicationName();
    std::map<std::string, std::string>& config = App::Application::Config();
    QString banner  = QString::fromUtf8(config["CopyrightInfo"].c_str());
    banner = banner.left( banner.indexOf(QLatin1Char('\n')) );
    QString major  = QString::fromStdString(config["BuildVersionMajor"]);
    QString minor  = QString::fromLatin1(config["BuildVersionMinor"].c_str());
    QString build  = QString::fromLatin1(config["BuildRevision"].c_str());
    QString disda  = QString::fromLatin1(config["BuildRevisionDate"].c_str());
    QString mturl  = QString::fromLatin1(config["MaintainerUrl"].c_str());

    // we use replace() to keep label formatting, so a label with text "<b>Unknown</b>"
    // gets replaced to "<b>FreeCAD</b>", for example

    QString author = ui->labelAuthor->text();
    author.replace(QString::fromLatin1("Unknown Application"), exeName);
    author.replace(QString::fromLatin1("(c) Unknown Author"), banner);
    ui->labelAuthor->setText(author);
    ui->labelAuthor->setUrl(mturl);

    if (qApp->styleSheet().isEmpty()) {
        ui->labelAuthor->setStyleSheet(QString::fromLatin1("Gui--UrlLabel {color: #0000FF;text-decoration: underline;font-weight: 600;font-family: MS Shell Dlg 2;}"));
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
    if (auto it = config.find("BuildRevisionBranch"); it != config.end()) {
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
        hash.replace(QString::fromLatin1("Unknown"), QString::fromLatin1(it->second.c_str()));
        ui->labelBuildHash->setText(hash);
    }
    else {
        ui->labelHash->hide();
        ui->labelBuildHash->hide();
    }
}

class AboutDialog::LibraryInfo {
public:
    QString name;
    QString version;
    QString href;
    QString url;
};

void AboutDialog::showCredits()
{
    QString creditsFileURL = QString::fromLatin1("%1/CONTRIBUTORS")
        .arg(QString::fromUtf8(App::Application::getHelpDir().c_str()));
    QFile creditsFile(creditsFileURL);

    if (!creditsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QWidget* tab_credits = new QWidget();
    tab_credits->setObjectName(QString::fromLatin1("tab_credits"));
    ui->tabWidget->addTab(tab_credits, tr("Credits"));
    QVBoxLayout* hlayout = new QVBoxLayout(tab_credits);
    QTextBrowser* textField = new QTextBrowser(tab_credits);
    textField->setOpenExternalLinks(false);
    textField->setOpenLinks(false);
    hlayout->addWidget(textField);

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
        .arg(QString::fromUtf8(App::Application::getHelpDir().c_str()));
    QFile licenseFile(licenseFileURL);

    if (!licenseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ///////////////////////////////////////////////////////////////////////////
        // Leave the existing license placeholder there if we can't find our license html file
        QString info(QLatin1String("SUCH DAMAGES.<hr/>"));
        info += getAdditionalLicenseInformation();
        QString lictext = ui->textBrowserLicense->toHtml();
        lictext.replace(QString::fromLatin1("SUCH DAMAGES.<hr/>"),info);
        ui->textBrowserLicense->setHtml(lictext);
        ///////////////////////////////////////////////////////////////////////////
    } 
    else {
        ui->tabWidget->removeTab (1); // Hide the license placeholder widget

        QWidget* tab_license = new QWidget();
        tab_license->setObjectName(QString::fromLatin1("tab_license"));
        ui->tabWidget->addTab(tab_license, tr("License"));
        QVBoxLayout* hlayout = new QVBoxLayout(tab_license);
        QTextBrowser* textField = new QTextBrowser(tab_license);
        textField->setOpenExternalLinks(true);
        textField->setOpenLinks(true);
        hlayout->addWidget(textField);

        QString licenseHTML = QString::fromUtf8(licenseFile.readAll());

        QString additionalLicenseInfo = getAdditionalLicenseInformation();
        licenseHTML.replace(QString::fromLatin1("FREECAD_ADDITIONAL_LICENSE_INFORMATION"), additionalLicenseInfo);

        textField->setHtml(licenseHTML);
    }
}

void AboutDialog::showLibraryInformation()
{
    QWidget *tab_library = new QWidget();
    tab_library->setObjectName(QString::fromLatin1("tab_library"));
    ui->tabWidget->addTab(tab_library, tr("Libraries"));
    QVBoxLayout* hlayout = new QVBoxLayout(tab_library);
    QTextBrowser* textField = new QTextBrowser(tab_library);
    textField->setOpenExternalLinks(false);
    textField->setOpenLinks(false);
    hlayout->addWidget(textField);

    QList<LibraryInfo> libInfo;
    LibraryInfo li;
    QString baseurl = QString::fromLatin1("file:///%1/ThirdPartyLibraries.html")
            .arg(QString::fromUtf8(App::Application::getHelpDir().c_str()));

    // Boost
    li.name = QLatin1String("Boost");
    li.href = baseurl + QLatin1String("#_TocBoost");
    li.url = QLatin1String("http://www.boost.org");
    li.version = QLatin1String(BOOST_LIB_VERSION);
    libInfo << li;

    // Coin3D
    li.name = QLatin1String("Coin3D");
    li.href = baseurl + QLatin1String("#_TocCoin3D");
    li.url = QLatin1String("https://bitbucket.org/Coin3D/coin/");
    li.version = QLatin1String(COIN_VERSION);
    libInfo << li;

    // Eigen3
    li.name = QLatin1String("Eigen3");
    li.href = baseurl + QLatin1String("#_TocEigen3");
    li.url = QLatin1String("http://eigen.tuxfamily.org/");
    li.version = QString::fromLatin1(FC_EIGEN3_VERSION);
    libInfo << li;

    // FreeType
    li.name = QLatin1String("FreeType");
    li.href = baseurl + QLatin1String("#_TocFreeType");
    li.url = QLatin1String("http://freetype.org");
    li.version = QString::fromLatin1(FC_FREETYPE_VERSION);
    libInfo << li;

    // KDL
    li.name = QLatin1String("KDL");
    li.href = baseurl + QLatin1String("#_TocKDL");
    li.url = QLatin1String("http://www.orocos.org/kdl");
    li.version.clear();
    libInfo << li;

    // libarea
    li.name = QLatin1String("libarea");
    li.href = baseurl + QLatin1String("#_TocLibArea");
    li.url = QLatin1String("https://github.com/danielfalck/libarea");
    li.version.clear();
    libInfo << li;

    // OCCT
#if defined(HAVE_OCC_VERSION)
    li.name = QLatin1String("Open CASCADE Technology");
    li.href = baseurl + QLatin1String("#_TocOCCT");
    li.url = QLatin1String("http://www.opencascade.com");
    li.version = QLatin1String(OCC_VERSION_STRING_EXT);
    libInfo << li;
#endif

    // pcl
    li.name = QLatin1String("Point Cloud Library");
    li.href = baseurl + QLatin1String("#_TocPcl");
    li.url = QLatin1String("http://www.pointclouds.org");
    li.version = QString::fromLatin1(FC_PCL_VERSION);
    libInfo << li;

    // PyCXX
    li.name = QLatin1String("PyCXX");
    li.href = baseurl + QLatin1String("#_TocPyCXX");
    li.url = QLatin1String("http://cxx.sourceforge.net");
    li.version = QString::fromLatin1(FC_PYCXX_VERSION);
    libInfo << li;

    // Python
    li.name = QLatin1String("Python");
    li.href = baseurl + QLatin1String("#_TocPython");
    li.url = QLatin1String("http://www.python.org");
    li.version = QLatin1String(PY_VERSION);
    libInfo << li;

    // PySide
    li.name = QLatin1String("PySide");
    li.href = baseurl + QLatin1String("#_TocPySide");
    li.url = QLatin1String("http://www.pyside.org");
    li.version = QString::fromLatin1(FC_PYSIDE_VERSION);
    libInfo << li;

    // Qt
    li.name = QLatin1String("Qt");
    li.href = baseurl + QLatin1String("#_TocQt");
    li.url = QLatin1String("http://www.qt.io");
    li.version = QLatin1String(QT_VERSION_STR);
    libInfo << li;

    // Salome SMESH
    li.name = QLatin1String("Salome SMESH");
    li.href = baseurl + QLatin1String("#_TocSalomeSMESH");
    li.url = QLatin1String("http://salome-platform.org");
    li.version.clear();
    libInfo << li;

    // Shiboken
    li.name = QLatin1String("Shiboken");
    li.href = baseurl + QLatin1String("#_TocPySide");
    li.url = QLatin1String("http://www.pyside.org");
    li.version = QString::fromLatin1(FC_SHIBOKEN_VERSION);
    libInfo << li;

    // vtk
    li.name = QLatin1String("vtk");
    li.href = baseurl + QLatin1String("#_TocVtk");
    li.url = QLatin1String("https://www.vtk.org");
    li.version = QString::fromLatin1(FC_VTK_VERSION);
    libInfo << li;

    // Xerces-C
    li.name = QLatin1String("Xerces-C");
    li.href = baseurl + QLatin1String("#_TocXercesC");
    li.url = QLatin1String("https://xerces.apache.org/xerces-c");
    li.version = QString::fromLatin1(FC_XERCESC_VERSION);
    libInfo << li;

    // Zipios++
    li.name = QLatin1String("Zipios++");
    li.href = baseurl + QLatin1String("#_TocZipios");
    li.url = QLatin1String("http://zipios.sourceforge.net");
    li.version.clear();
    libInfo << li;

    // zlib
    li.name = QLatin1String("zlib");
    li.href = baseurl + QLatin1String("#_TocZlib");
    li.url = QLatin1String("http://zlib.net");
    li.version = QLatin1String(ZLIB_VERSION);
    libInfo << li;


    QString msg = tr("This software uses open source components whose copyright and other "
                     "proprietary rights belong to their respective owners:");
    QString html;
    QTextStream out(&html);
    out << "<html><head/><body"
        << "<p>" << msg << "<br/></p>\n<ul>\n";
    for (QList<LibraryInfo>::iterator it = libInfo.begin(); it != libInfo.end(); ++it) {
        out << "<li><p>" << it->name << " " << it->version << "</p>"
               "<p><a href=\"" << it->href << "\">" << it->url
            << "</a><br/></p></li>\n";
    }
    out << "</ul>\n</body>\n</html>";
    textField->setHtml(html);

    connect(textField, SIGNAL(anchorClicked(QUrl)), this, SLOT(linkActivated(QUrl)));
}

void AboutDialog::showCollectionInformation()
{
    QString doc = QString::fromUtf8(App::Application::getHelpDir().c_str());
    QString path = doc + QLatin1String("Collection.html");
    if (!QFile::exists(path))
        return;

    QWidget *tab_collection = new QWidget();
    tab_collection->setObjectName(QString::fromLatin1("tab_collection"));
    ui->tabWidget->addTab(tab_collection, tr("Collection"));
    QVBoxLayout* hlayout = new QVBoxLayout(tab_collection);
    QTextBrowser* textField = new QTextBrowser(tab_collection);
    textField->setOpenExternalLinks(true);
    hlayout->addWidget(textField);
    textField->setSource(path);
}

QString AboutDialog::getAdditionalLicenseInformation()
{
    // Any additional piece of text to be added after the main license text goes below.
    // Please set title in <h2> tags, license text in <p> tags
    // and add an <hr/> tag at the end to nicely separate license blocks
    QString info;
#ifdef _USE_3DCONNEXION_SDK
    info += QString::fromLatin1(
        "<h2>3D Mouse Support</h2>"
        "<p>Development tools and related technology provided under license from 3Dconnexion."
        "Copyright © 1992-2012 3Dconnexion. All rights reserved.</p>"
        "<hr/>"
        );
#endif
    return info;
}

void AboutDialog::linkActivated(const QUrl& link)
{
#if defined(Q_OS_WIN) && QT_VERSION < 0x050602
    LicenseView* licenseView = new LicenseView();
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
#else
    QDesktopServices::openUrl(link);
#endif
}

void AboutDialog::on_copyButton_clicked()
{
    QString data;
    QTextStream str(&data);
    std::map<std::string, std::string>& config = App::Application::Config();
    std::map<std::string,std::string>::iterator it;
    QString exe = QString::fromLatin1(App::GetApplication().getExecutableName());

    QString major  = QString::fromLatin1(config["BuildVersionMajor"].c_str());
    QString minor  = QString::fromLatin1(config["BuildVersionMinor"].c_str());
    QString build  = QString::fromLatin1(config["BuildRevision"].c_str());

    QString deskEnv = QProcessEnvironment::systemEnvironment().value(QString::fromLatin1("XDG_CURRENT_DESKTOP"),QString::fromLatin1(""));
    QString deskSess = QProcessEnvironment::systemEnvironment().value(QString::fromLatin1("DESKTOP_SESSION"),QString::fromLatin1(""));
    QString deskInfo = QString::fromLatin1("");

    if (!deskEnv.isEmpty() && !deskSess.isEmpty())
      deskInfo = QString::fromLatin1(" (%1/%2)").arg(deskEnv, deskSess);
    else if (!deskEnv.isEmpty())
      deskInfo = QString::fromLatin1(" (%1)").arg(deskEnv);
    else if (!deskSess.isEmpty())
      deskInfo = QString::fromLatin1(" (%1)").arg(deskSess);

    str << "OS: " << QSysInfo::prettyProductName() << deskInfo << '\n';

    str << "Version: " << major << "." << minor << "." << build;
    char *appimage = getenv("APPIMAGE");
    if (appimage)
        str << " AppImage";
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
    if (it != config.end())
        str << "Branch: " << QString::fromUtf8(it->second.c_str()) << '\n';
    it = config.find("BuildRevisionHash");
    if (it != config.end())
        str << "Hash: " << it->second.c_str() << '\n';
    // report also the version numbers of the most important libraries in FreeCAD
    str << "Python version: " << PY_VERSION << '\n';
    str << "Qt version: " << QT_VERSION_STR << '\n';
    str << "Coin version: " << COIN_VERSION << '\n';
#if defined(HAVE_OCC_VERSION)
    str << "OCC version: "
        << OCC_VERSION_MAJOR << "."
        << OCC_VERSION_MINOR << "."
        << OCC_VERSION_MAINTENANCE
#ifdef OCC_VERSION_DEVELOPMENT
        << "." OCC_VERSION_DEVELOPMENT
#endif
        << '\n';
#endif
    QLocale loc;
    str << "Locale: " << loc.languageToString(loc.language()) << "/"
        << loc.countryToString(loc.country())
        << " (" << loc.name() << ")\n";

    QClipboard* cb = QApplication::clipboard();
    cb->setText(data);
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::LicenseView */

LicenseView::LicenseView(QWidget* parent)
    : MDIView(0,parent,Qt::WindowFlags())
{
    browser = new QTextBrowser(this);
    browser->setOpenExternalLinks(true);
    browser->setOpenLinks(true);
    setCentralWidget(browser);
}

LicenseView::~LicenseView()
{
}

void LicenseView::setSource(const QUrl& url)
{
    browser->setSource(url);
}

#include "moc_Splashscreen.cpp"
