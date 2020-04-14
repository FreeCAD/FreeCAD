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


#ifndef BASE_SEQUENCER_H
#define BASE_SEQUENCER_H

#include <vector>
#include <memory>
#include <CXX/Extensions.hxx>

#include "Exception.h"

namespace Base
{

class AbortException;
class SequencerLauncher;

/**
 * \brief This class gives the user an indication of the progress of an operation and
 * it is used to reassure him that the application is still running.
 *
 * Here are some code snippets of how to use the sequencer:
 *  \code
 *
 *  #include <Base/Sequencer.h>
 *
 *  //first example
 *  Base::SequencerLauncher seq("my text", 10)
 *  for (int i=0; i<10; i++)
 *  {
 *    // do something
 *    seq.next ();
 *  }
 *
 *  //second example
 *  Base::SequencerLauncher seq("my text", 10)
 *  do
 *  {
 *    // do something
 *  }
 *  while (seq.next());
 *
 *  \endcode
 *
 * The implementation of this class also supports several nested instances
 * at a time. But note, that only the first instance has an effect. Any further
 * sequencer instance doesn't influence the total number of iteration steps. This
 * is simply because it's impossible to get the exact number of iteration steps
 * for nested instances and thus we have either too few steps estimated then the 
 * sequencer may indicate 100% but the algorithm still running or we have too many
 * steps estimated so that the an algorithm may stop long before the sequencer
 * reaches 100%. 
 *
 *  \code
 *  try {
 *    //start the first operation
 *    Base::SequencerLauncher seq1("my text", 10)
 *    for (int i=0; i<10, i++)
 *    {
 *      // do something
 *
 *      // start the second operation while the first one is still running
 *      Base::SequencerLauncher seq2("another text", 10);
 *      for (int j=0; j<10; j++)
 *      {
 *        // do something different
 *        seq2.next ();
 *      }
 *
 *      seq1.next ( true ); // allow to cancel
 *    }
 *  }
 *  catch(const Base::AbortException&){
 *    // cleanup your data if needed
 *  }
 *
 *  \endcode
 *
 * \note If using the sequencer with SequencerLauncher.next(\a true) then you must
 * take into account that the exception \a AbortException could be thrown, e.g. in
 * case the ESC button was pressed. So in this case it's always a good idea to use
 * the sequencer within a try-catch block.
 *
 * \note Instances of SequencerLauncher should always be created on the stack.
 * This is because if an exception somewhere is thrown the destructor is auto-
 * matically called to clean-up internal data.
 *
 * \note It's not supported to create an instance of SequencerBase or a sub-class
 * in another thread than the main thread. But you can create SequencerLauncher
 * instances in other threads.
 *
 * \author Werner Mayer
 */
class BaseExport SequencerBase
{
    friend class SequencerLauncher;

public:
    /**
     * Returns the last created sequencer instance.
     * If you create an instance of a class inheriting SequencerBase
     * this object is retrieved instead.
     *
     * This mechanism is very useful to have an own sequencer for each layer of FreeCAD.
     * For example, if FreeCAD is running in server mode you have/need no GUI layer
     * and therewith no (graphical) progress bar; in this case ConsoleSequencer is taken.
     * But in cases FreeCAD is running with GUI the @ref Gui::ProgressBar is taken instead.
     * @see Sequencer
     */
    static SequencerBase& Instance();
    /**
     * Returns true if the running sequencer is blocking any user input.
     * This might be only of interest of the GUI where the progress bar or dialog
     * is used from a thread. If started from a thread this method should return
     * false, otherwise true. The default implementation always returns true.
     */
    virtual bool isBlocking() const;
    /** If \a bLock is true then the sequencer gets locked. startStep() and nextStep()
     * don't get invoked any more until the sequencer gets unlocked again.
     * This method returns the previous lock state.
     */
    bool setLocked(bool bLock);
    /** Returns true if the sequencer was locked, false otherwise. */
    bool isLocked() const;
    /** Returns true if the sequencer is running, otherwise returns false. */
    bool isRunning() const;
    /**
     * Returns true if the pending operation was canceled.
     */
    bool wasCanceled() const;

    /// Check if the  operation is aborted by user
    virtual void checkAbort() {}

protected:
    /**
     * Starts a new operation, returns false if there is already a pending operation,
     * otherwise it returns true.
     * In this method startStep() gets invoked that can be reimplemented in sub-classes.
     */
    bool start(const char* pszStr, size_t steps);
    /** Returns the number of steps. */
    size_t numberOfSteps() const;
    /** Returns the current state of progress in percent. */
    int progressInPercent() const;
    /**
     * Performs the next step and returns true if the operation is not yet finished.
     * But note, when 0 was passed to start() as the number of total steps this method
     * always returns false.
     *
     * In this method nextStep() gets invoked that can be reimplemented in sub-classes.
     * If \a canAbort is true then the operations can be aborted, otherwise (the default)
     * the operation cannot be aborted. In case it gets aborted an exception AbortException
     * is thrown.
     */
    bool next(bool canAbort = false);
    /**
     * Stops the sequencer if all operations are finished. It returns false if
     * there are still pending operations, otherwise it returns true.
     */
    bool stop();
    /**
     * Breaks the sequencer if needed. The default implementation does nothing.
     * Every pause() must eventually be followed by a corresponding @ref resume().
     * @see Gui::ProgressBar.
     */
    virtual void pause();
    /**
     * Continues with progress. The default implementation does nothing.
     * @see pause(), @see Gui::ProgressBar.
     */
    virtual void resume();
    /**
     * Try to cancel the pending operation(s).
     * E.g. @ref Gui::ProgressBar calls this method after the ESC button was pressed.
     */
    void tryToCancel();
    /**
     * If you tried to cancel but then decided to continue the operation.
     * E.g. in @ref Gui::ProgressBar a dialog appears asking if you really want to
     * cancel. If you decide to continue this method must be called.
     */
    void rejectCancel();

protected:
    /** construction */
    SequencerBase();
    /** Destruction */
    virtual ~SequencerBase();
    /**
     * Sets a text what the pending operation is doing. The default implementation
     * does nothing.
     */
    virtual void setText (const char* pszTxt);
    /**
     * This method can be reimplemented in sub-classes to give the user a feedback
     * when a new sequence starts. The default implementation does nothing.
     */
    virtual void startStep();
    /**
     * This method can be reimplemented in sub-classes to give the user a feedback
     * when the next is performed. The default implementation does nothing. If \a canAbort
     * is true then the pending operation can aborted, otherwise not. Depending on the
     * re-implementation this method can throw an AbortException if canAbort is true.
     */
    virtual void nextStep(bool canAbort);
    /**
     * Sets the progress indicator to a certain position.
     */
    virtual void setProgress(size_t);
    /**
     * Resets internal data.
     * If you want to reimplement this method, it is very important to call it in
     * the re-implemented method.
     */
    virtual void resetData();

protected:
    size_t nProgress; /**< Stores the current amount of progress.*/
    size_t nTotalSteps; /**< Stores the total number of steps */

private:
    bool _bLocked; /**< Lock/unlock sequencer. */
    bool _bCanceled; /**< Is set to true if the last pending operation was canceled */
    int _nLastPercentage; /**< Progress in percent. */
};

/** This special sequencer might be useful if you want to suppress any indication
 * of the progress to the user.
 */
class BaseExport EmptySequencer : public Base::SequencerBase
{
public:
    /** construction */
    EmptySequencer();
    /** Destruction */
    ~EmptySequencer();
};

/**
 * \brief This class writes the progress to the console window.
 */
class BaseExport ConsoleSequencer : public SequencerBase
{
public:
    /** construction */
    ConsoleSequencer ();
    /** Destruction */
    ~ConsoleSequencer ();

protected:
    /** Starts the sequencer */
    void startStep();
    /** Writes the current progress to the console window. */
    void nextStep(bool canAbort);

private:
    /** Puts text to the console window */
    void setText (const char* pszTxt);
    /** Resets the sequencer */
    void resetData();
};

/** The SequencerLauncher class is provided for convenience. It allows you to run an instance of the
 * sequencer by instantiating an object of this class -- most suitable on the stack. So this mechanism
 * can be used for try-catch-blocks to destroy the object automatically if the C++ exception mechanism
 * cleans up the stack.
 *
 * This class has been introduced to simplify the use with the sequencer. In the FreeCAD Gui layer there
 * is a subclass of SequencerBase called ProgressBar that grabs the keyboard and filters most of the incoming
 * events. If the programmer uses the API of SequencerBase directly to start an instance without due diligence
 * with exceptions then a not handled exception could block the whole application -- the user has to kill the
 * application then.
 *
 * Below is an example of a not correctly used sequencer.
 *
 * \code
 *
 *  #include <Base/Sequencer.h>
 *
 *  void runOperation();
 *
 *  void myTest()
 *  {
 *    try{
 *       runOperation();
 *    } catch(...) {
 *       // the programmer forgot to stop the sequencer here
 *       // Under circumstances the sequencer never gets stopped so the keyboard never gets ungrabbed and
 *       // all Gui events still gets filtered.
 *    }
 *  }
 *
 *  void runOperation()
 *  {
 *    Base::Sequencer().start ("my text", 10);
 *
 *    for (int i=0; i<10; i++)
 *    {
 *      // do something where an exception be thrown
 *      ...
 *      Base::Sequencer().next ();
 *    }
 *
 *    Base::Sequencer().stop ();
 *  }
 *
 * \endcode
 *
 * To avoid such problems the SequencerLauncher class can be used as follows:
 *
 * \code
 *
 *  #include <Base/Sequencer.h>
 *
 *  void runOperation();
 *
 *  void myTest()
 *  {
 *    try{
 *       runOperation();
 *    } catch(...) {
 *       // the programmer forgot to halt the sequencer here
 *       // If SequencerLauncher leaves its scope the object gets destructed automatically and
 *       // stops the running sequencer.
 *    }
 *  }
 *
 *  void runOperation()
 *  {
 *    // create an instance on the stack (not on any terms on the heap)
 *    SequencerLauncher seq("my text", 10);
 *
 *    for (int i=0; i<10; i++)
 *    {
 *      // do something (e.g. here can be thrown an exception)
 *      ...
 *      seq.next ();
 *    }
 *  }
 *
 * \endcode
 *
 * @author Werner Mayer
 */
class BaseExport SequencerLauncher
{
public:
    SequencerLauncher(const char* pszStr, size_t steps);
    ~SequencerLauncher();
    size_t numberOfSteps() const;
    void setText (const char* pszTxt);
    bool next(bool canAbort = false);
    void setProgress(size_t);
    bool wasCanceled() const;
};

/** Access to the only SequencerBase instance */
inline SequencerBase& Sequencer ()
{
    return SequencerBase::Instance();
}

class BaseExport ProgressIndicatorPy : public Py::PythonExtension<ProgressIndicatorPy>
{
public:
    static void init_type(void);    // announce properties and methods

    ProgressIndicatorPy();
    ~ProgressIndicatorPy();

    Py::Object repr();

    Py::Object start(const Py::Tuple&);
    Py::Object next(const Py::Tuple&);
    Py::Object stop(const Py::Tuple&);

private:
    static PyObject *PyMake(struct _typeobject *, PyObject *, PyObject *);

private:
    std::unique_ptr<SequencerLauncher> _seq;
};

} // namespace Base

#endif // BASE_SEQUENCER_H
