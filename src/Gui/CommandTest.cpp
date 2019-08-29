/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QEventLoop>
# include <QFileDialog>
# include <QMutex>
# include <QMutexLocker>
# include <QThread>
# include <QTimer>
# include <QMdiArea>
# include <QMdiSubWindow>
# include <QWaitCondition>
# include <QTranslator>
# include <QRunnable>
# include <QThreadPool>
#endif

#include <Base/Console.h>
#include "Application.h"
#include "MainWindow.h"
#include "MDIView.h"
#include "Command.h"
#include "Language/Translator.h"

#include "ProgressBar.h"


using namespace Gui;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//===========================================================================
// Std_TestQM
//===========================================================================
DEF_STD_CMD(Std_TestQM);

Std_TestQM::Std_TestQM()
  : Command("Std_TestQM")
{
    sGroup        = "Standard-Test";
    sMenuText     = "Test translation files...";
    sToolTipText  = "Test function to check .qm translation files";
    sWhatsThis    = "Std_TestQM";
    sStatusTip    = sToolTipText;
}

void Std_TestQM::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    QStringList files = QFileDialog::getOpenFileNames(getMainWindow(),
        QString::fromLatin1("Test translation"), QString(),
        QString::fromLatin1("Translation (*.qm)"));
    if (!files.empty()) {
        Translator::instance()->activateLanguage("English");
        QList<QTranslator*> i18n = qApp->findChildren<QTranslator*>();
        for (QList<QTranslator*>::Iterator it = i18n.begin(); it != i18n.end(); ++it)
            qApp->removeTranslator(*it);
        for (QStringList::Iterator it = files.begin(); it != files.end(); ++it) {
            QTranslator* translator = new QTranslator(qApp);
            if (translator->load(*it)) {
                qApp->installTranslator(translator);
            }
            else {
                delete translator;
            }
        }
    }
}

//===========================================================================
// Std_TestReloadQM
//===========================================================================
DEF_STD_CMD(Std_TestReloadQM);

Std_TestReloadQM::Std_TestReloadQM()
  : Command("Std_TestReloadQM")
{
    sGroup        = "Standard-Test";
    sMenuText     = "Reload translation files";
    sToolTipText  = "Test function to check .qm translation files";
    sWhatsThis    = "Std_TestReloadQM";
    sStatusTip    = sToolTipText;
}

void Std_TestReloadQM::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    Translator::instance()->activateLanguage(Translator::instance()->activeLanguage().c_str());
}

//===========================================================================
// Std_Test1
//===========================================================================
DEF_STD_CMD_A(FCCmdTest1);

FCCmdTest1::FCCmdTest1()
  : Command("Std_Test1")
{
    sGroup        = "Standard-Test";
    sMenuText     = "Test1";
    sToolTipText  = "Test function 1";
    sWhatsThis    = "Std_Test1";
    sStatusTip    = sToolTipText;
    sPixmap       = "Std_Tool1";
    sAccel        = "Ctrl+T";
}

void FCCmdTest1::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
}

bool FCCmdTest1::isActive(void)
{
    //return (GetActiveOCCDocument()!=NULL);
    return true;
}

//===========================================================================
// Std_Test2
//===========================================================================
DEF_STD_CMD_A(FCCmdTest2);

FCCmdTest2::FCCmdTest2()
  : Command("Std_Test2")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Test2";
    sToolTipText    = "Test function 2";
    sWhatsThis      = "Std_Test2";
    sStatusTip      = sToolTipText;
    sPixmap         = "Std_Tool2";
}


void FCCmdTest2::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
}

bool FCCmdTest2::isActive(void)
{
    return (getDocument()!=NULL);
}

//===========================================================================
// Std_Test3
//===========================================================================
DEF_STD_CMD_A(FCCmdTest3);

FCCmdTest3::FCCmdTest3()
  : Command("Std_Test3")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Test3";
    sToolTipText    = "Test function 3";
    sWhatsThis      = "Std_Test3";
    sStatusTip      = sToolTipText;
    sPixmap         = "Std_Tool3";
}

void FCCmdTest3::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    App::Document *pcDoc = getDocument();
    if (!pcDoc) return;
}


bool FCCmdTest3::isActive(void)
{
    return (getDocument()!=NULL);
}

//===========================================================================
// Std_Test4
//===========================================================================

DEF_STD_CMD_A(FCCmdTest4);

FCCmdTest4::FCCmdTest4()
  : Command("Std_Test4")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Test4";
    sToolTipText    = "Test function 4";
    sWhatsThis      = "Std_Test4";
    sStatusTip      = sToolTipText;
    sPixmap         = "Std_Tool4";
}

void FCCmdTest4::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    App::Document *pcDoc = getDocument();
    if(!pcDoc) return;
}


bool FCCmdTest4::isActive(void)
{
    return (getDocument()!=NULL);
}

//===========================================================================
// Std_Test5
//===========================================================================
DEF_STD_CMD_A(FCCmdTest5);

FCCmdTest5::FCCmdTest5()
  : Command("Std_Test5")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Test5";
    sToolTipText    = "Test function 5";
    sWhatsThis      = "Std_Test5";
    sStatusTip      = sToolTipText;
    sPixmap         = "Std_Tool5";
}

void FCCmdTest5::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    App::Document *pcDoc = getDocument();
    if(!pcDoc) return;
}

bool FCCmdTest5::isActive(void)
{
  return (getDocument()!=NULL);
}


//===========================================================================
// Std_Test6
//===========================================================================
DEF_STD_CMD_A(FCCmdTest6);

FCCmdTest6::FCCmdTest6()
  : Command("Std_Test6")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Test6";
    sToolTipText    = "Test function 6";
    sWhatsThis      = "Std_Test6";
    sStatusTip      = sToolTipText;
    sPixmap         = "Std_Tool6";
}

void FCCmdTest6::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    App::Document *pcDoc = getDocument();
    if(!pcDoc) return;
}

bool FCCmdTest6::isActive(void)
{
    return (getDocument()!=NULL);
}

//===========================================================================
// Std_TestProgress1
//===========================================================================
DEF_STD_CMD_A(CmdTestProgress1);

CmdTestProgress1::CmdTestProgress1()
  : Command("Std_TestProgress1")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Breakable bar";
    sToolTipText    = "Test a breakable progress bar";
    sWhatsThis      = "Std_TestProgress1";
    sStatusTip      = sToolTipText;
    sPixmap         = "Std_Tool7";
}

void CmdTestProgress1::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    QMutex mutex;
    QMutexLocker ml(&mutex);
    try
    {
        unsigned long steps = 1000;
        Base::SequencerLauncher seq("Starting progress bar", steps);

        for (unsigned long i=0; i<steps;i++)
        {
            seq.next(true);
            QWaitCondition().wait(&mutex, 30);
        }
    }
    catch (...)
    {
    }
}

bool CmdTestProgress1::isActive(void)
{
    return (!Base::Sequencer().isRunning());
}

//===========================================================================
// Std_TestProgress2
//===========================================================================
DEF_STD_CMD_A(CmdTestProgress2);

CmdTestProgress2::CmdTestProgress2()
  : Command("Std_TestProgress2")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Unbreakable bar";
    sToolTipText    = "Test a unbreakable progress bar";
    sWhatsThis      = "Std_TestProgress2";
    sStatusTip      = sToolTipText;
    sPixmap         = "Std_Tool7";
}

void CmdTestProgress2::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    QMutex mutex;
    QMutexLocker ml(&mutex);

    try
    {
        unsigned long steps = 1000;
        Base::SequencerLauncher seq("Starting progress bar", steps);

        for (unsigned long i=0; i<steps;i++)
        {
            seq.next(false);
            QWaitCondition().wait(&mutex, 10);
        }
    }
    catch (...)
    {
    }
}

bool CmdTestProgress2::isActive(void)
{
    return ( !Base::Sequencer().isRunning() );
}

//===========================================================================
// Std_TestProgress3
//===========================================================================
DEF_STD_CMD_A(CmdTestProgress3);

CmdTestProgress3::CmdTestProgress3()
  : Command("Std_TestProgress3")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Nested progress bar";
    sToolTipText    = "Test nested progress bar";
    sWhatsThis      = "Std_TestProgress3";
    sStatusTip      = sToolTipText;
    sPixmap         = "Std_Tool8";
}

void CmdTestProgress3::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    QMutex mutex;
    QMutexLocker ml(&mutex);
    
    try
    {
        // level 1
        unsigned long level1 = 5;
        Base::SequencerLauncher seq1("Starting progress bar", level1);
        for (unsigned long i=0; i<level1;i++)
        {
            QWaitCondition().wait(&mutex, 200);
            seq1.next(true);

            // level 2
            unsigned long level2 = 6;
            Base::SequencerLauncher seq2("Starting progress bar", level2);
            for (unsigned long j=0; j<level2;j++)
            {
                QWaitCondition().wait(&mutex, 150);
                seq2.next(true);

                // level 3
                unsigned long level3 = 7;
                Base::SequencerLauncher seq3("Starting progress bar", level3);
                for (unsigned long k=0; k<level3;k++)
                {
                    QWaitCondition().wait(&mutex, 100);
                    seq3.next(true);

                    // level 4
                    unsigned long level4 = 8;
                    Base::SequencerLauncher seq4("Starting progress bar", level4);
                    for (unsigned long l=0; l<level4;l++)
                    {
                        QWaitCondition().wait(&mutex, 5);
                        seq4.next(true);
                    }
                }
            }
        }
    }
    catch (...)
    {
    }
}

bool CmdTestProgress3::isActive(void)
{
    return ( !Base::Sequencer().isRunning() );
}

//===========================================================================
// Std_TestProgress4
//===========================================================================
DEF_STD_CMD_A(CmdTestProgress4);

CmdTestProgress4::CmdTestProgress4()
  : Command("Std_TestProgress4")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Mixed nested bar";
    sToolTipText    = "Test a mixed up nested progress bar";
    sWhatsThis      = "Std_TestProgress4";
    sStatusTip      = sToolTipText;
    sPixmap         = "Std_Tool7";
}

void CmdTestProgress4::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    QMutex mutex;
    QMutexLocker ml(&mutex);

    try
    {
        unsigned long steps = 50;
        Base::SequencerLauncher* seq = new Base::SequencerLauncher("Starting progress bar", steps);

        for (unsigned long i=0; i<steps;i++)
        {
            QWaitCondition().wait(&mutex, 5);
            if (i == 45) {
                delete seq;
                seq = 0;
            }
            if (seq) {
                seq->next(false);
            }
            Base::SequencerLauncher seq2("Starting second progress bar", steps);
            for (unsigned long j=0; j<steps;j++)
            {
                QWaitCondition().wait(&mutex, (seq ? 5 : 50));
                seq2.next(true);
            }
        }
    }
    catch (...)
    {
    }
}

bool CmdTestProgress4::isActive(void)
{
    return (!Base::Sequencer().isRunning());
}

//===========================================================================
// Std_TestProgress5
//===========================================================================
DEF_STD_CMD_A(CmdTestProgress5);

CmdTestProgress5::CmdTestProgress5()
  : Command("Std_TestProgress5")
{
    sGroup          = "Standard-Test";
    sMenuText       = "From thread";
    sToolTipText    = "Test a progress bar from a thread";
    sWhatsThis      = "Std_TestProgress5";
    sStatusTip      = sToolTipText;
    sPixmap         = "Std_Tool7";
}

class BarThread : public QThread
{
public:
    BarThread(unsigned long s) : steps(s)
    {
    }
    ~BarThread()
    {
    }
    void run()
    {
        QMutex mutex;
        QMutexLocker ml(&mutex);
	
        try
        {
            Base::SequencerLauncher seq("Starting progress bar in thread", steps);

            for (unsigned long i=0; i<this->steps;i++)
            {
                seq.next(true);
                QWaitCondition().wait(&mutex, 5);
            }
        }
        catch (...)
        {
        }

        this->deleteLater();
        Base::Console().Message("Thread with %d steps finished\n",this->steps);
    }

private:
    unsigned long steps;
};

void CmdTestProgress5::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    QEventLoop loop;

    BarThread* thr1 = new BarThread(2000);
    QObject::connect(thr1, SIGNAL(finished()), &loop, SLOT(quit()));
    thr1->start();
    loop.exec();

    BarThread* thr2 = new BarThread(1500);

    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    thr2->start();
    timer.start(2000); // 2s timeout
    loop.exec();

    BarThread* thr3 = new BarThread(1000);
    thr3->start();
}

bool CmdTestProgress5::isActive(void)
{
    return (!Base::Sequencer().isRunning());
}

//===========================================================================
// Std_MDITest
//===========================================================================
DEF_STD_CMD_A(CmdTestMDI1);

CmdTestMDI1::CmdTestMDI1()
  : Command("Std_MDITest1")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Remove MDI 1";
    sToolTipText    = "Remove MDI from main window";
    sWhatsThis      = "Std_MDITest1";
    sStatusTip      = sToolTipText;
}

void CmdTestMDI1::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    MDIView* mdi = getMainWindow()->activeWindow();
    getMainWindow()->removeWindow(mdi);
}

bool CmdTestMDI1::isActive(void)
{
    return getMainWindow()->activeWindow();
}

DEF_STD_CMD_A(CmdTestMDI2);

CmdTestMDI2::CmdTestMDI2()
  : Command("Std_MDITest2")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Remove MDI 2";
    sToolTipText    = "Remove view from MDI area";
    sWhatsThis      = "Std_MDITest2";
    sStatusTip      = sToolTipText;
}

void CmdTestMDI2::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    QMdiArea* area = getMainWindow()->findChild<QMdiArea*>();
    if (area) {
        MDIView* mdi = getMainWindow()->activeWindow();
        area->removeSubWindow(mdi->parentWidget());
        mdi->parentWidget()->showNormal();
    }
}

bool CmdTestMDI2::isActive(void)
{
    return getMainWindow()->activeWindow();
}

DEF_STD_CMD_A(CmdTestMDI3);

CmdTestMDI3::CmdTestMDI3()
  : Command("Std_MDITest3")
{
    sGroup          = "Standard-Test";
    sMenuText       = "Remove MDI 3";
    sToolTipText    = "Unset parent and remove from main window";
    sWhatsThis      = "Std_MDITest3";
    sStatusTip      = sToolTipText;
}

void CmdTestMDI3::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    MDIView* mdi = getMainWindow()->activeWindow();
    getMainWindow()->removeWindow(mdi);
    mdi->setParent(0, Qt::Window | Qt::WindowTitleHint |
                   Qt::WindowSystemMenuHint | 
                   Qt::WindowMinMaxButtonsHint);
    mdi->show();
}

bool CmdTestMDI3::isActive(void)
{
    return getMainWindow()->activeWindow();
}

DEF_STD_CMD(CmdTestConsoleOutput);

CmdTestConsoleOutput::CmdTestConsoleOutput()
  : Command("Std_TestConsoleOutput")
{
    sGroup      = QT_TR_NOOP("Standard-Test");
    sMenuText   = QT_TR_NOOP("Test console output");
    sToolTipText= QT_TR_NOOP("Test console output");
    sStatusTip  = QT_TR_NOOP("Test console output");
}

namespace Gui {
class TestConsoleObserver : public Base::ConsoleObserver
{
    QMutex mutex;
public:
    int matchMsg, matchWrn, matchErr, matchLog;
    TestConsoleObserver() : matchMsg(0), matchWrn(0), matchErr(0), matchLog(0)
    {
    }
    virtual void Warning(const char * msg)
    {
        QMutexLocker ml(&mutex);
        matchWrn += strcmp(msg, "Write a warning to the console output.\n");
    }
    virtual void Message(const char * msg)
    {
        QMutexLocker ml(&mutex);
        matchMsg += strcmp(msg, "Write a message to the console output.\n");
    }
    virtual void Error(const char * msg)
    {
        QMutexLocker ml(&mutex);
        matchErr += strcmp(msg, "Write an error to the console output.\n");
    }
    virtual void Log(const char * msg)
    {
        QMutexLocker ml(&mutex);
        matchLog += strcmp(msg, "Write a log to the console output.\n");
    }
};

class ConsoleMessageTask : public QRunnable
{
public:
    void run()
    {
        for (int i=0; i<10; i++)
            Base::Console().Message("Write a message to the console output.\n");
    }
};

class ConsoleWarningTask : public QRunnable
{
public:
    void run()
    {
        for (int i=0; i<10; i++)
            Base::Console().Warning("Write a warning to the console output.\n");
    }
};

class ConsoleErrorTask : public QRunnable
{
public:
    void run()
    {
        for (int i=0; i<10; i++)
            Base::Console().Error("Write an error to the console output.\n");
    }
};

class ConsoleLogTask : public QRunnable
{
public:
    void run()
    {
        for (int i=0; i<10; i++)
            Base::Console().Log("Write a log to the console output.\n");
    }
};

}

void CmdTestConsoleOutput::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    TestConsoleObserver obs;
    Base::Console().AttachObserver(&obs);
    QThreadPool::globalInstance()->start(new ConsoleMessageTask);
    QThreadPool::globalInstance()->start(new ConsoleWarningTask);
    QThreadPool::globalInstance()->start(new ConsoleErrorTask);
    QThreadPool::globalInstance()->start(new ConsoleLogTask);
    QThreadPool::globalInstance()->waitForDone();
    Base::Console().DetachObserver(&obs);

    if (obs.matchMsg > 0 || obs.matchWrn > 0 || obs.matchErr > 0 || obs.matchLog > 0) {
        Base::Console().Error("Race condition in Console class\n");
    }
}


namespace Gui {

void CreateTestCommands(void)
{
    CommandManager &rcCmdMgr = Application::Instance->commandManager();

    rcCmdMgr.addCommand(new Std_TestQM());
    rcCmdMgr.addCommand(new Std_TestReloadQM());
    rcCmdMgr.addCommand(new FCCmdTest1());
    rcCmdMgr.addCommand(new FCCmdTest2());
    rcCmdMgr.addCommand(new FCCmdTest3());
    rcCmdMgr.addCommand(new FCCmdTest4());
    rcCmdMgr.addCommand(new FCCmdTest5());
    rcCmdMgr.addCommand(new FCCmdTest6());
    rcCmdMgr.addCommand(new CmdTestProgress1());
    rcCmdMgr.addCommand(new CmdTestProgress2());
    rcCmdMgr.addCommand(new CmdTestProgress3());
    rcCmdMgr.addCommand(new CmdTestProgress4());
    rcCmdMgr.addCommand(new CmdTestProgress5());
    rcCmdMgr.addCommand(new CmdTestMDI1());
    rcCmdMgr.addCommand(new CmdTestMDI2());
    rcCmdMgr.addCommand(new CmdTestMDI3());
    rcCmdMgr.addCommand(new CmdTestConsoleOutput());
}

} // namespace Gui
