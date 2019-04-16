/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer@users.sourceforge.net>        *
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


#ifndef GUI_PROGRESSDIALOG_H
#define GUI_PROGRESSDIALOG_H

#ifndef __Qt4All__
# include "Qt4All.h"
#endif

#include <Base/Sequencer.h>

namespace Gui {

struct SequencerDialogPrivate;

class ProgressDialog;
class SequencerDialog : public Base::SequencerBase
{
public:
    static SequencerDialog* instance();
    void pause();
    void resume();
    bool isBlocking() const;

    virtual void checkAbort() override;

protected:
    /** Construction */
    SequencerDialog ();
    /** Destruction */
    ~SequencerDialog ();

    /** Puts text to the progress dialog */
    void setText (const char* pszTxt);
    /** Starts the progress dialog */
    void startStep();
    /** Increase the step indicator of the progress dialog. */
    void nextStep(bool canAbort);
    /** Resets the sequencer */
    void resetData();
    void showRemainingTime();

private:
    /** @name for internal use only */
    //@{
    void setProgress(int step);
    /** Throws an exception to stop the pending operation. */
    void abort();
    //@}

    SequencerDialogPrivate* d;
    static SequencerDialog* _pclSingleton;

    friend class ProgressDialog;
};

class ProgressDialog : public QProgressDialog
{
    Q_OBJECT

public:
    /** Construction */
    ProgressDialog (SequencerDialog* s, QWidget * parent=0);
    /** Destruction */
    ~ProgressDialog ();

    /** Handles all incoming events while the progress bar is running. All key
     * and mouse events are ignored to block user input.
     */
    bool eventFilter(QObject* o, QEvent* e);

protected Q_SLOTS:
    void onCancel();

protected:
    bool canAbort() const;
    /** Gets the events under control */
    void enterControlEvents();
    /** Loses the control over incoming events*/
    void leaveControlEvents();

private:
    SequencerDialog* sequencer;

    friend class SequencerDialog;
};

} // namespace Gui

#endif // GUI_PROGRESSDIALOG_H
