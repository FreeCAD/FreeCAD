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
# include <QSysInfo>
# include <QTextStream>
# include <QWaitCondition>
# include <Inventor/C/basic.h>
#endif

#if QT_VERSION < 0x050000
# include <QGLContext>
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
class SplashObserver : public Base::ConsoleObserver
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
    const char* Name()
    {
        return "SplashObserver";
    }
    void Warning(const char * s)
    {
#ifdef FC_DEBUG
        Log(s);
#else
        Q_UNUSED(s);
#endif
    }
    void Message(const char * s)
    {
#ifdef FC_DEBUG
        Log(s);
#else
        Q_UNUSED(s);
#endif
    }
    void Error  (const char * s)
    {
#ifdef FC_DEBUG
        Log(s);
#else
        Q_UNUSED(s);
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

#if QT_VERSION < 0x050000
        const QGLContext* ctx = QGLContext::currentContext();
        if (!ctx)
#endif
        {
            splash->showMessage(msg.replace(QLatin1String("\n"), QString()), alignment, textColor);
            QMutex mutex;
            QMutexLocker ml(&mutex);
            QWaitCondition().wait(&mutex, 50);
        }
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
#ifdef _USE_3DCONNEXION_SDK
    return new AboutDialog(true, parent);
#else
    return new AboutDialog(false, parent);
#endif
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
 *  Constructs an AboutDialog which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'WStyle_Customize|WStyle_NoBorder|WType_Modal'
 *
 *  The dialog will be modal.
 */
AboutDialog::AboutDialog(bool showLic, QWidget* parent)
  : QDialog(parent, Qt::FramelessWindowHint), ui(new Ui_AboutApplication)
{
    Q_UNUSED(showLic);

    setModal(true);
    ui->setupUi(this);
    QRect rect = QApplication::desktop()->availableGeometry();
    QPixmap image = getMainWindow()->splashImage();

    // Make sure the image is not too big
    if (image.height() > rect.height()/2 || image.width() > rect.width()/2) {
        float scale = static_cast<float>(image.width()) / static_cast<float>(image.height());
        int width = std::min(image.width(), rect.width()/2);
        int height = std::min(image.height(), rect.height()/2);
        height = std::min(height, static_cast<int>(width / scale));
        width = static_cast<int>(scale * height);

        image = image.scaled(width, height);
    }
    ui->labelSplashPicture->setPixmap(image);
//    if (showLic) { // currently disabled. Additional license blocks are always shown.
        QString info(QLatin1String("SUCH DAMAGES.<hr/>"));
        // any additional piece of text to be added after the main license text goes below.
        // Please set title in <h2> tags, license text in <p> tags
        // and add an <hr/> tag at the end to nicely separate license blocks
#ifdef _USE_3DCONNEXION_SDK
        info += QString::fromLatin1(
            "<h2>3D Mouse Support</h2>"
            "<p>Development tools and related technology provided under license from 3Dconnexion."
            "(c) 1992 - 2012 3Dconnexion. All rights reserved</p>"
            "<hr/>"
            );
#endif
        QString lictext = ui->textBrowserLicense->toHtml();
        lictext.replace(QString::fromLatin1("SUCH DAMAGES."),info);
        ui->textBrowserLicense->setHtml(lictext);
//    }
    ui->tabWidget->setCurrentIndex(0); // always start on the About tab
    setupLabels();
    showLicenseInformation();
}

/**
 *  Destroys the object and frees any allocated resources
 */
AboutDialog::~AboutDialog()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

class SystemInfo {
public:
static QString getOperatingSystem()
{
#if QT_VERSION >= 0x050400
    return QSysInfo::prettyProductName();
#endif

#if defined (Q_OS_WIN32)
    switch(QSysInfo::windowsVersion())
    {
        case QSysInfo::WV_NT:
            return QString::fromLatin1("Windows NT");
        case QSysInfo::WV_2000:
            return QString::fromLatin1("Windows 2000");
        case QSysInfo::WV_XP:
            return QString::fromLatin1("Windows XP");
        case QSysInfo::WV_2003:
            return QString::fromLatin1("Windows Server 2003");
        case QSysInfo::WV_VISTA:
            return QString::fromLatin1("Windows Vista");
        case QSysInfo::WV_WINDOWS7:
            return QString::fromLatin1("Windows 7");
#if QT_VERSION >= 0x040800
        case QSysInfo::WV_WINDOWS8:
            return QString::fromLatin1("Windows 8");
#endif
#if ((QT_VERSION >= 0x050200) || (QT_VERSION >= 0x040806 && QT_VERSION < 0x050000))
        case QSysInfo::WV_WINDOWS8_1:
            return QString::fromLatin1("Windows 8.1");
#endif
#if QT_VERSION >= 0x040807
        case QSysInfo::WV_WINDOWS10:
            return QString::fromLatin1("Windows 10");
#endif
        default:
            return QString::fromLatin1("Windows");
    }
#elif defined (Q_OS_MAC)
    switch(QSysInfo::MacVersion())
    {
        case QSysInfo::MV_10_3:
            return QString::fromLatin1("Mac OS X 10.3");
        case QSysInfo::MV_10_4:
            return QString::fromLatin1("Mac OS X 10.4");
        case QSysInfo::MV_10_5:
            return QString::fromLatin1("Mac OS X 10.5");
#if QT_VERSION >= 0x040700
        case QSysInfo::MV_10_6:
            return QString::fromLatin1("Mac OS X 10.6");
#endif
#if QT_VERSION >= 0x040800
        case QSysInfo::MV_10_7:
            return QString::fromLatin1("Mac OS X 10.7");
        case QSysInfo::MV_10_8:
            return QString::fromLatin1("Mac OS X 10.8");
        case QSysInfo::MV_10_9:
            return QString::fromLatin1("Mac OS X 10.9");
        case QSysInfo::MV_10_10:
            return QString::fromLatin1("Mac OS X 10.10");
#endif
#if QT_VERSION >= 0x050500
        case QSysInfo::MV_10_11:
            return QString::fromLatin1("Mac OS X 10.11");
#endif
#if QT_VERSION >= 0x050600
        case QSysInfo::MV_10_12:
            return QString::fromLatin1("Mac OS X 10.12");
#endif
        default:
            return QString::fromLatin1("Mac OS X");
    }
#elif defined (Q_OS_LINUX)
    QString exe(QLatin1String("lsb_release"));
    QStringList args;
    args << QLatin1String("-ds");
    QProcess proc;
    proc.setEnvironment(QProcess::systemEnvironment());
    proc.start(exe, args);
    if (proc.waitForStarted() && proc.waitForFinished()) {
        QByteArray info = proc.readAll();
        info.replace('\n',"");
        return QString::fromLatin1((const char*)info);
    }

    return QString::fromLatin1("Linux");
#elif defined (Q_OS_UNIX)
    return QString::fromLatin1("UNIX");
#else
    return QString();
#endif
}

static int getWordSizeOfOS()
{
#if defined(Q_OS_WIN64)
    return 64; // 64-bit process running on 64-bit windows
#elif defined(Q_OS_WIN32)

    // determine if 32-bit process running on 64-bit windows in WOW64 emulation
    // or 32-bit process running on 32-bit windows
    // default bIsWow64 to false for 32-bit process on 32-bit windows

    BOOL bIsWow64 = false; // must default to false
    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
        GetModuleHandle("kernel32"), "IsWow64Process");

    if (NULL != fnIsWow64Process) {
        if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64)) {
            assert(false); // something went majorly wrong
        }
    }
    return bIsWow64 ? 64 : 32;

#elif defined (Q_OS_LINUX)
    // http://stackoverflow.com/questions/246007/how-to-determine-whether-a-given-linux-is-32-bit-or-64-bit
    QString exe(QLatin1String("getconf"));
    QStringList args;
    args << QLatin1String("LONG_BIT");
    QProcess proc;
    proc.setEnvironment(QProcess::systemEnvironment());
    proc.start(exe, args);
    if (proc.waitForStarted() && proc.waitForFinished()) {
        QByteArray info = proc.readAll();
        info.replace('\n',"");
        return info.toInt();
    }

    return 0; // failed

#elif defined (Q_OS_UNIX) || defined (Q_OS_MAC)
    QString exe(QLatin1String("uname"));
    QStringList args;
    args << QLatin1String("-m");
    QProcess proc;
    proc.setEnvironment(QProcess::systemEnvironment());
    proc.start(exe, args);
    if (proc.waitForStarted() && proc.waitForFinished()) {
        QByteArray info = proc.readAll();
        info.replace('\n',"");
        if (info.indexOf("x86_64") >= 0)
            return 64;
        else if (info.indexOf("amd64") >= 0)
            return 64;
        else if (info.indexOf("ia64") >= 0)
            return 64;
        else if (info.indexOf("ppc64") >= 0)
            return 64;
        else if (info.indexOf("i386") >= 0)
            return 32;
        else if (info.indexOf("i686") >= 0)
            return 32;
        else if (info.indexOf("x86") >= 0)
            return 32;
    }

    return 0; // failed
#else
    return 0; // unknown
#endif
}
};

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
    std::map<std::string,std::string>::iterator it;
    QString banner  = QString::fromUtf8(config["CopyrightInfo"].c_str());
    banner = banner.left( banner.indexOf(QLatin1Char('\n')) );
    QString major  = QString::fromLatin1(config["BuildVersionMajor"].c_str());
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
    os.replace(QString::fromLatin1("Unknown"), SystemInfo::getOperatingSystem());
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

void AboutDialog::showLicenseInformation()
{
    QWidget *tab_license = new QWidget();
    tab_license->setObjectName(QString::fromLatin1("tab_license"));
    ui->tabWidget->addTab(tab_license, tr("Libraries"));
    QVBoxLayout* hlayout = new QVBoxLayout(tab_license);
    QTextBrowser* textField = new QTextBrowser(tab_license);
    textField->setOpenExternalLinks(false);
    textField->setOpenLinks(false);
    hlayout->addWidget(textField);

    QList<LibraryInfo> libInfo;
    LibraryInfo li;
    QString baseurl = QString::fromLatin1("file:///%1/ThirdPartyLibraries.html")
            .arg(QString::fromUtf8(App::Application::getHelpDir().c_str()));

    //FIXME: Put all needed information into LibraryVersions.h
    //

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
    li.version.clear();
    libInfo << li;

    // FreeType
    li.name = QLatin1String("FreeType");
    li.href = baseurl + QLatin1String("#_TocFreeType");
    li.url = QLatin1String("http://freetype.org");
    li.version.clear();
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
    li.version.clear();
    libInfo << li;

    // PyCXX
    li.name = QLatin1String("PyCXX");
    li.href = baseurl + QLatin1String("#_TocPyCXX");
    li.url = QLatin1String("http://cxx.sourceforge.net");
    li.version.clear();
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
    li.version.clear();
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
    li.version.clear();
    libInfo << li;

    // vtk
    li.name = QLatin1String("vtk");
    li.href = baseurl + QLatin1String("#_TocVtk");
    li.url = QLatin1String("https://www.vtk.org");
    li.version.clear();
    libInfo << li;

    // Xerces-C
    li.name = QLatin1String("Xerces-C");
    li.href = baseurl + QLatin1String("#_TocXercesC");
    li.url = QLatin1String("https://xerces.apache.org/xerces-c");
    li.version.clear();
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
    out << "<html><head/><body style=\" font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;\">"
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

void AboutDialog::linkActivated(const QUrl& link)
{
//#if defined(Q_OS_WIN) && QT_VERSION < 0x050602
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
//#else
//    QDesktopServices::openUrl(link);
//#endif
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
    str << "OS: " << SystemInfo::getOperatingSystem() << endl;
    int wordSize = SystemInfo::getWordSizeOfOS();
    if (wordSize > 0) {
        str << "Word size of OS: " << wordSize << "-bit" << endl;
    }
    str << "Word size of " << exe << ": " << QSysInfo::WordSize << "-bit" << endl;
    str << "Version: " << major << "." << minor << "." << build;
    char *appimage = getenv("APPIMAGE");
    if (appimage)
        str << " AppImage";
    str << endl;

#if defined(_DEBUG) || defined(DEBUG)
    str << "Build type: Debug" << endl;
#elif defined(NDEBUG)
    str << "Build type: Release" << endl;
#elif defined(CMAKE_BUILD_TYPE)
    str << "Build type: " << CMAKE_BUILD_TYPE << endl;
#else
    str << "Build type: Unknown" << endl;
#endif
    it = config.find("BuildRevisionBranch");
    if (it != config.end())
        str << "Branch: " << QString::fromUtf8(it->second.c_str()) << endl;
    it = config.find("BuildRevisionHash");
    if (it != config.end())
        str << "Hash: " << it->second.c_str() << endl;
    // report also the version numbers of the most important libraries in FreeCAD
    str << "Python version: " << PY_VERSION << endl;
    str << "Qt version: " << QT_VERSION_STR << endl;
    str << "Coin version: " << COIN_VERSION << endl;
#if defined(HAVE_OCC_VERSION)
    str << "OCC version: "
        << OCC_VERSION_MAJOR << "."
        << OCC_VERSION_MINOR << "."
        << OCC_VERSION_MAINTENANCE
#ifdef OCC_VERSION_DEVELOPMENT
        << "." OCC_VERSION_DEVELOPMENT
#endif
        << endl;
#endif
    QLocale loc;
    str << "Locale: " << loc.languageToString(loc.language()) << "/"
        << loc.countryToString(loc.country())
        << " (" << loc.name() << ")" << endl;

    QClipboard* cb = QApplication::clipboard();
    cb->setText(data);
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::LicenseView */

LicenseView::LicenseView(QWidget* parent)
    : MDIView(0,parent,0)
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
