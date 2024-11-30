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

#include <Base/Sequencer.h>
#include <QProgressDialog>
#ifdef QT_WINEXTRAS_LIB
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif


namespace Gui {

struct SequencerDialogPrivate;

class ProgressDialog;
class GuiExport SequencerDialog : public Base::SequencerBase
{
public:
    static SequencerDialog* instance();
    void pause() override;
    void resume() override;
    bool isBlocking() const override;
    bool canAbort() const;

protected:
    /** Construction */
    SequencerDialog ();
    /** Destruction */
    ~SequencerDialog () override;

    /** Puts text to the progress dialog */
    void setText (const char* pszTxt) override;
    /** Starts the progress dialog */
    void startStep() override;
    /** Increase the step indicator of the progress dialog. */
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

    SequencerDialogPrivate* d;
    static SequencerDialog* _pclSingleton;

    friend class ProgressDialog;
};

class ProgressDialog : public QProgressDialog
{
    Q_OBJECT

public:
    /** Construction */
    explicit ProgressDialog (SequencerDialog* s, QWidget * parent=nullptr);
    /** Destruction */
    ~ProgressDialog () override;

protected Q_SLOTS:
    void onCancel();

private Q_SLOTS:
    void resetEx();
    void setRangeEx(int minimum, int maximum);
    void setValueEx(int value);
    void aboutToShow();
    void aboutToHide();
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;

protected:
    bool canAbort() const;

private:
    SequencerDialog* sequencer;

#ifdef QT_WINEXTRAS_LIB
    /* Set up the taskbar progress in windows */
    void setupTaskBarProgress(void);
    QWinTaskbarProgress* m_taskbarProgress;
    QWinTaskbarButton* m_taskbarButton;
#endif
    friend class SequencerDialog;
};

} // namespace Gui

#endif // GUI_PROGRESSDIALOG_H
