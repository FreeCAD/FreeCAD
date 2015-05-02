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
# include <QApplication>
# include <QClipboard>
# include <QDialogButtonBox>
# include <QMutex>
# include <QProcess> 
# include <QSysInfo>
# include <QTextStream>
# include <QWaitCondition>
# include <Inventor/C/basic.h>
#endif

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
    SplashObserver(QSplashScreen* splasher=0, const char* name=0)
      : splash(splasher), alignment(Qt::AlignBottom|Qt::AlignLeft), textColor(Qt::black)
    {
        Base::Console().AttachObserver(this);

        // allow to customize text position and color
        const std::map<std::string,std::string>& cfg = App::GetApplication().Config();
        std::map<std::string,std::string>::const_iterator al = cfg.find("SplashAlignment");
        if (al != cfg.end()) {
            QString alt = QString::fromAscii(al->second.c_str());
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
            QColor col; col.setNamedColor(QString::fromAscii(tc->second.c_str()));
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
#endif
    }
    void Message(const char * s)
    {
#ifdef FC_DEBUG
        Log(s);
#endif
    }
    void Error  (const char * s)
    {
#ifdef FC_DEBUG
        Log(s);
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
        mutex.lock();
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
SplashScreen::SplashScreen(  const QPixmap & pixmap , Qt::WFlags f )
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
    setModal(true);
    ui->setupUi(this);
    ui->labelSplashPicture->setPixmap(getMainWindow()->splashImage());
    if (!showLic)
        ui->licenseButton->hide();
    setupLabels();
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
#if defined (Q_OS_WIN32)
    switch(QSysInfo::windowsVersion())
    {
        case QSysInfo::WV_NT:
            return QString::fromAscii("Windows NT");
        case QSysInfo::WV_2000:
            return QString::fromAscii("Windows 2000");
        case QSysInfo::WV_XP:
            return QString::fromAscii("Windows XP");
        case QSysInfo::WV_2003:
            return QString::fromAscii("Windows Server 2003");
        case QSysInfo::WV_VISTA:
            return QString::fromAscii("Windows Vista");
        case QSysInfo::WV_WINDOWS7:
            return QString::fromAscii("Windows 7");
#if QT_VERSION >= 0x040800
        case QSysInfo::WV_WINDOWS8:
            return QString::fromAscii("Windows 8");
#endif
#if ((QT_VERSION >= 0x050200) || (QT_VERSION >= 0x040806 && QT_VERSION < 0x050000))
        case QSysInfo::WV_WINDOWS8_1:
            return QString::fromAscii("Windows 8.1");
#endif
        default:
            return QString::fromAscii("Windows");
    }
#elif defined (Q_OS_MAC)
    switch(QSysInfo::MacVersion())
    {
        case QSysInfo::MV_10_3:
            return QString::fromAscii("Mac OS X 10.3");
        case QSysInfo::MV_10_4:
            return QString::fromAscii("Mac OS X 10.4");
        case QSysInfo::MV_10_5:
            return QString::fromAscii("Mac OS X 10.5");
#if QT_VERSION >= 0x040700
        case QSysInfo::MV_10_6:
            return QString::fromAscii("Mac OS X 10.6");
#endif
#if QT_VERSION >= 0x040800
        case QSysInfo::MV_10_7:
            return QString::fromAscii("Mac OS X 10.7");
        case QSysInfo::MV_10_8:
            return QString::fromAscii("Mac OS X 10.8");
        case QSysInfo::MV_10_9:
            return QString::fromAscii("Mac OS X 10.9");
#endif
        default:
            return QString::fromAscii("Mac OS X");
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
        return QString::fromAscii((const char*)info);
    }

    return QString::fromAscii("Linux");
#elif defined (Q_OS_UNIX)
    return QString::fromAscii("UNIX");
#else
    return QString();
#endif
}

#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
#endif
static int getWordSizeOfOS()
{
#if defined(Q_OS_WIN64)
    return 64; // 64-bit process running on 64-bit windows
#elif defined(Q_OS_WIN32)

    // determine if 32-bit process running on 64-bit windows in WOW64 emulation
    // or 32-bit process running on 32-bit windows
    // default bIsWow64 to FALSE for 32-bit process on 32-bit windows

    BOOL bIsWow64 = FALSE; // must default to FALSE

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
        setStyleSheet(QString::fromAscii("Gui--Dialog--AboutDialog QLabel {font-size: %1pt;}").arg(fontSize));
    }
    
    QString exeName = qApp->applicationName();
    std::map<std::string, std::string>& config = App::Application::Config();
    std::map<std::string,std::string>::iterator it;
    QString banner  = QString::fromUtf8(config["CopyrightInfo"].c_str());
    banner = banner.left( banner.indexOf(QLatin1Char('\n')) );
    QString major  = QString::fromAscii(config["BuildVersionMajor"].c_str());
    QString minor  = QString::fromAscii(config["BuildVersionMinor"].c_str());
    QString build  = QString::fromAscii(config["BuildRevision"].c_str());
    QString disda  = QString::fromAscii(config["BuildRevisionDate"].c_str());
    QString mturl  = QString::fromAscii(config["MaintainerUrl"].c_str());

    QString author = ui->labelAuthor->text();
    author.replace(QString::fromAscii("Unknown Application"), exeName);
    author.replace(QString::fromAscii("(c) Unknown Author"), banner);
    ui->labelAuthor->setText(author);
    ui->labelAuthor->setUrl(mturl);

    QString version = ui->labelBuildVersion->text();
    version.replace(QString::fromAscii("Unknown"), QString::fromAscii("%1.%2").arg(major).arg(minor));
    ui->labelBuildVersion->setText(version);

    QString revision = ui->labelBuildRevision->text();
    revision.replace(QString::fromAscii("Unknown"), build);
    ui->labelBuildRevision->setText(revision);

    QString date = ui->labelBuildDate->text();
    date.replace(QString::fromAscii("Unknown"), disda);
    ui->labelBuildDate->setText(date);

    QString os = ui->labelBuildOS->text();
    os.replace(QString::fromAscii("Unknown"), SystemInfo::getOperatingSystem());
    ui->labelBuildOS->setText(os);

    QString platform = ui->labelBuildPlatform->text();
    platform.replace(QString::fromAscii("Unknown"),
        QString::fromAscii("%1-bit").arg(QSysInfo::WordSize));
    ui->labelBuildPlatform->setText(platform);
    
    // branch name
    it = config.find("BuildRevisionBranch");
    if (it != config.end()) {
        QString branch = ui->labelBuildBranch->text();
        branch.replace(QString::fromAscii("Unknown"), QString::fromAscii(it->second.c_str()));
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
        hash.replace(QString::fromAscii("Unknown"), QString::fromAscii(it->second.c_str()));
        ui->labelBuildHash->setText(hash);
    }
    else {
        ui->labelHash->hide();
        ui->labelBuildHash->hide();
    }
}

namespace Gui {
namespace Dialog {

class GuiExport LicenseDialog : public QDialog
{
public:
    LicenseDialog(QWidget *parent = 0) : QDialog(parent, Qt::FramelessWindowHint)
    {
        QString info;
#ifdef _USE_3DCONNEXION_SDK
        info = QString::fromAscii(
            "3D Mouse Support:\n"
            "Development tools and related technology provided under license from 3Dconnexion.\n"
            "(c) 1992 - 2012 3Dconnexion. All rights reserved");
#endif
        statusLabel = new QLabel(info);
        buttonBox = new QDialogButtonBox;
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);
        connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

        QHBoxLayout *topLayout = new QHBoxLayout;
        topLayout->addWidget(statusLabel);

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->addLayout(topLayout);
        mainLayout->addWidget(buttonBox);
        setLayout(mainLayout);

        setWindowTitle(tr("Copyright"));
    }
    ~LicenseDialog()
    {
    }

private:
    QLabel *statusLabel;
    QDialogButtonBox *buttonBox;
};

} // namespace Dialog
} // namespace Gui

void AboutDialog::on_licenseButton_clicked()
{
#ifdef _USE_3DCONNEXION_SDK
    LicenseDialog dlg(this);
    dlg.exec();
#endif
}

void AboutDialog::on_copyButton_clicked()
{
    QString data;
    QTextStream str(&data);
    std::map<std::string, std::string>& config = App::Application::Config();
    std::map<std::string,std::string>::iterator it;
    QString exe = QString::fromAscii(App::GetApplication().getExecutableName());

    QString major  = QString::fromAscii(config["BuildVersionMajor"].c_str());
    QString minor  = QString::fromAscii(config["BuildVersionMinor"].c_str());
    QString build  = QString::fromAscii(config["BuildRevision"].c_str());
    str << "OS: " << SystemInfo::getOperatingSystem() << endl;
    int wordSize = SystemInfo::getWordSizeOfOS();
    if (wordSize > 0) {
        str << "Word size of OS: " << wordSize << "-bit" << endl;
    }
    str << "Word size of " << exe << ": " << QSysInfo::WordSize << "-bit" << endl;
    str << "Version: " << major << "." << minor << "." << build << endl;
#if defined(_DEBUG)
    str << "Build type: Debug" << endl;
#elif defined(NDEBUG)
    str << "Build type: Release" << endl;
#else
    str << "Build type: Unknown" << endl;
#endif
    it = config.find("BuildRevisionBranch");
    if (it != config.end())
        str << "Branch: " << it->second.c_str() << endl;
    it = config.find("BuildRevisionHash");
    if (it != config.end())
        str << "Hash: " << it->second.c_str() << endl;
    // report also the version numbers of the most important libraries in FreeCAD
    str << "Python version: " << PY_VERSION << endl;
    str << "Qt version: " << QT_VERSION_STR << endl;
    str << "Coin version: " << COIN_VERSION << endl;
    it = config.find("OCC_VERSION");
    if (it != config.end())
        str << "OCC version: " << it->second.c_str() << endl;

    QClipboard* cb = QApplication::clipboard();
    cb->setText(data);
}

#include "moc_Splashscreen.cpp"
