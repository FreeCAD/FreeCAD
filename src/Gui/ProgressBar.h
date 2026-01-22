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

#pragma once

#include <QProgressBar>
#ifdef QT_WINEXTRAS_LIB
# include <QWinTaskbarButton>
# include <QWinTaskbarProgress>
#endif

#include <Base/Sequencer.h>


namespace Gui
{

struct SequencerBarPrivate;
struct ProgressBarPrivate;
class ProgressBar;

/**
 * FreeCAD's progress bar for long operations
 * To see how to use the progress bar have a look at the following examples:
 *
 * \code
 * unsigned long steps = ...
 * Base::SequencerLauncher seq("Starting progress bar", 0);
 *
 * for (unsigned long i=0; i<steps;i++)
 * {
 *   seq.next();
 *   // do one step of your algorithm
 * }
 *
 * \endcode
 *
 * The example below shows how to use two nested progresses.
 *
 * \code
 * void function1()
 * {
 *   unsigned long steps = ...
 *   Base::SequencerLauncger seq("Starting progress bar", 0);
 *
 *   for (unsigned long i=0; i<steps;i++)
 *   {
 *     seq.next();
 *     // do one step of your algorithm
 *   }
 *
 * }
 *
 * void function2()
 * {
 *   unsigned long steps = ...
 *   Base::SequencerLauncher seq("Starting progress bar", 0);
 *
 *   for (unsigned long i=0; i<steps;i++)
 *   {
 *     seq.next();
 *     // do one step of your algorithm calling function1
 *     function1();
 *   }
 * }
 *
 * \endcode
 *
 * If the total number of steps cannot be determined before, use 0 instead of to show
 * just a busy indicator instead of percentage steps.
 * @author Werner Mayer
 */
class GuiExport SequencerBar: public Base::SequencerBase
{
public:
    /** Returns the sequencer object. */
    static SequencerBar* instance();
    /** This restores the last overridden cursor and release the keyboard while the progress bar
     * is running. This is useful e.g. if a modal dialog appears while a long operation is performed
     * to indicate that the user can click on the dialog. Every pause() must eventually be followed
     * by a corresponding @ref resume().
     */
    void pause() override;
    /** This sets the wait cursor again and grabs the keyboard. @see pause() */
    void resume() override;
    bool isBlocking() const override;
    /** Returns an instance of the progress bar. It creates one if needed. */
    QProgressBar* getProgressBar(QWidget* parent = nullptr);

    void checkAbort() override;

protected:
    /** Construction */
    SequencerBar();
    /** Destruction */
    ~SequencerBar() override;

    /** Puts text to the status bar */
    void setText(const char* pszTxt) override;
    /** Starts the progress bar */
    void startStep() override;
    /** Increase the progress bar. */
    void nextStep(bool canAbort) override;
    /** Sets the progress indicator to a certain position. */
    void setProgress(size_t) override;
    /** Resets the sequencer */
    void resetData() override;
    void showRemainingTime();

private:
    /** @name for internal use only */
    //@{
    void setValue(int step);
    /** Throws an exception to stop the pending operation. */
    void abort();
    //@}
    SequencerBarPrivate* d;
    static SequencerBar* _pclSingleton;

    friend class ProgressBar;
};

class ProgressBar: public QProgressBar
{
    Q_OBJECT

public:
    /** Construction */
    ProgressBar(SequencerBar* s, QWidget* parent = nullptr);
    /** Destruction */
    ~ProgressBar() override;

    /** Handles all incoming events while the progress bar is running. All key and mouse
     * events are ignored to block user input.
     */
    bool eventFilter(QObject* o, QEvent* e) override;
    /** Returns the time in milliseconds that must pass before the progress bar appears.
     */
    int minimumDuration() const;

private Q_SLOTS:
    void resetEx();
    void setRangeEx(int minimum, int maximum);
    void setValueEx(int value);


public Q_SLOTS:
    /** Sets the time that must pass before the progress bar appears to \a ms.
     */
    void setMinimumDuration(int ms);

public:
    bool canAbort() const;

protected:
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;

protected Q_SLOTS:
    /* Shows the progress bar if it is still hidden after the operation has been started
     * and minimumDuration milliseconds have passed.
     */
    void delayedShow();
    void aboutToShow();
    void aboutToHide();

private:
    /** @name for internal use only */
    //@{
    void resetObserveEventFilter();
    /** Gets the events under control */
    void enterControlEvents(bool);
    /** Loses the control over incoming events*/
    void leaveControlEvents(bool);


    //@}
    ProgressBarPrivate* d;
    SequencerBar* sequencer;

#ifdef QT_WINEXTRAS_LIB
    /* Set up the taskbar progress in windows */
    void setupTaskBarProgress(void);
    QWinTaskbarProgress* m_taskbarProgress;
    QWinTaskbarButton* m_taskbarButton;
#endif
    friend class SequencerBar;
};

}  // namespace Gui
