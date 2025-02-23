// Created on: 2002-02-22
// Created by: Andrey BETENEV
// Copyright (c) 2002-2014 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.

#ifndef _Message_ProgressScope_HeaderFile
#define _Message_ProgressScope_HeaderFile

#include <Standard_Assert.hxx>
#include <Standard_TypeDef.hxx>
#include <Standard_DefineAlloc.hxx>
#include <Standard_Handle.hxx>
#include <Precision.hxx>
#include <TCollection_AsciiString.hxx>

class Message_ProgressRange;
class Message_ProgressIndicator;

//! Message_ProgressScope class provides convenient way to advance progress
//! indicator in context of complex program organized in hierarchical way,
//! where usually it is difficult (or even not possible) to consider process 
//! as linear with fixed step.
//!
//! On every level (sub-operation) in hierarchy of operations
//! the local instance of the Message_ProgressScope class is created.
//! It takes a part of the upper-level scope (via Message_ProgressRange) and provides 
//! a way to consider this part as independent scale with locally defined range.
//!
//! The position on the local scale may be advanced using the method Next(),
//! which allows iteration-like advancement. This method can take argument to 
//! advance by the specified value (with default step equal to 1). 
//! This method returns Message_ProgressRange object that takes responsibility 
//! of making the specified step, either directly at its destruction or by
//! delegating this task to another sub-scope created from that range object.
//!
//! It is important that sub-scope must have life time less than 
//! the life time of its parent scope that provided the range.
//! The usage pattern is to create scope objects as local variables in the
//! functions that do the job, and pass range objects returned by Next() to
//! the functions of the lower level, to allow them creating their own scopes.
//!
//! The scope has a name that can be used in visualization of the progress.
//! It can be null. Note that when C string literal is used as a name, then its
//! value is not copied, just pointer is stored. In other variants (char pointer
//! or a string class) the string is copied, which is additional overhead.
//!
//! The same instance of the progress scope! must not be used concurrently from different threads.
//! For the algorithm running its tasks in parallel threads, a common scope is 
//! created before the parallel execution, and the range objects produced by method 
//! Next() are used to initialise the data pertinent to each task. 
//! Then the progress is advanced within each task using its own range object. 
//! See example below.
//!
//! Note that while a range of the scope is specified using Standard_Real 
//! (double) parameter, it is expected to be a positive integer value.
//! If the range is not an integer, method Next() shall be called with
//! explicit step argument, and the rounded value returned by method Value()
//! may be not coherent with the step and range.
//!
//! A scope can be created with option "infinite". This is useful when
//! the number of steps is not known by the time of the scope creation.
//! In this case the progress will be advanced logarithmically, approaching
//! the end of the scope at infinite number of steps. The parameter Max
//! for infinite scope indicates number of steps corresponding to mid-range.
//!
//! A progress scope created with empty constructor is not connected to any
//! progress indicator, and passing the range created on it to any algorithm 
//! allows it executing safely without actual progress indication.
//!
//! Example of preparation of progress indicator:
//!
//! @code{.cpp}
//!   Handle(Message_ProgressIndicator) aProgress = ...; // assume it can be null
//!   func (Message_ProgressIndicator::Start (aProgress));
//! @endcode
//!
//! Example of usage in sequential process:
//!
//! @code{.cpp}
//!   Message_ProgressScope aWholePS(aRange, "Whole process", 100);
//!
//!   // do one step taking 20%
//!   func1 (aWholePS.Next (20)); // func1 will take 20% of the whole scope
//!   if (aWholePS.UserBreak()) // exit prematurely if the user requested break
//!     return;
//!
//!   // ... do next step taking 50%
//!   func2 (aWholePS.Next (50));
//!   if (aWholePS.UserBreak())
//!     return;
//! @endcode
//!
//! Example of usage in nested cycle:
//!
//! @code{.cpp}
//!   // Outer cycle
//!   Message_ProgressScope anOuter (theProgress, "Outer", nbOuter);
//!   for (Standard_Integer i = 0; i < nbOuter && anOuter.More(); i++)
//!   {
//!     // Inner cycle
//!     Message_ProgressScope anInner (anOuter.Next(), "Inner", nbInner);
//!     for (Standard_Integer j = 0; j < nbInner && anInner.More(); j++)
//!     {
//!       // Cycle body
//!       func (anInner.Next());
//!     }
//!   }
//! @endcode
//!
//! Example of use in function:
//!
//! @code{.cpp}
//! //! Implementation of iterative algorithm showing its progress
//! func (const Message_ProgressRange& theProgress)
//! {
//!   // Create local scope covering the given progress range.
//!   // Set this scope to count aNbSteps steps.
//!   Message_ProgressScope aScope (theProgress, "", aNbSteps);
//!   for (Standard_Integer i = 0; i < aNbSteps && aScope.More(); i++)
//!   {
//!     // Optional: pass range returned by method Next() to the nested algorithm 
//!     // to allow it to show its progress too (by creating its own scope object).
//!     // In any case the progress will advance to the next step by the end of the func2 call.
//!     func2 (aScope.Next());
//!   }
//! }
//! @endcode
//!
//! Example of usage in parallel process:
//!
//! @code{.cpp}
//! struct Task
//! {
//!   Data& Data;
//!   Message_ProgressRange Range;
//!
//!   Task (const Data& theData, const Message_ProgressRange& theRange)
//!     : Data (theData), Range (theRange) {}
//! };
//! struct Functor
//! {
//!   void operator() (Task& theTask) const
//!   {
//!     // Note: it is essential that this method is executed only once for the same Task object
//!     Message_ProgressScope aPS (theTask.Range, NULL, theTask.Data.NbItems);
//!     for (Standard_Integer i = 0; i < theTask.Data.NbSteps && aPS.More(); i++)
//!     {
//!       do_job (theTask.Data.Item[i], aPS.Next());
//!     }
//!   }
//! };
//! ...
//! {
//!   std::vector<Data> aData = ...;
//!   std::vector<Task> aTasks;
//!
//!   Message_ProgressScope aPS (aRootRange, "Data processing", aData.size());
//!   for (Standard_Integer i = 0; i < aData.size(); ++i)
//!     aTasks.push_back (Task (aData[i], aPS.Next()));
//!   
//!   OSD_Parallel::ForEach (aTasks.begin(), aTasks.end(), Functor());
//! }
//! @endcode
//!
//! For lightweight algorithms that do not need advancing the progress 
//! within individual tasks the code can be simplified to avoid inner scopes:
//!
//! @code
//! struct Functor
//! {
//!   void operator() (Task& theTask) const
//!   {
//!     if (theTask.Range.More())
//!     {
//!       do_job (theTask.Data);
//!       // advance the progress
//!       theTask.Range.Close();
//!     }
//!   }
//! };
//! @endcode
class Message_ProgressScope
{
public:
  class NullString; //!< auxiliary type for passing NULL name to Message_ProgressScope constructor
public: //! @name Preparation methods

  //! Creates dummy scope.
  //! It can be safely passed to algorithms; no progress indication will be done.
  Message_ProgressScope()
  : myProgress (0),
    myParent (0),
    myName (0),
    myStart (0.),
    myPortion (1.),
    myMax (1.),
    myValue (0.),
    myIsActive (false),
    myIsOwnName (false),
    myIsInfinite (false)
  {}

  //! Creates a new scope taking responsibility of the part of the progress 
  //! scale described by theRange. The new scope has own range from 0 to 
  //! theMax, which is mapped to the given range.
  //!
  //! The topmost scope is created and owned by Message_ProgressIndicator
  //! and its pointer is contained in the Message_ProgressRange returned by the Start() method of progress indicator.
  //!
  //! @param theRange [in][out] range to fill (will be disarmed)
  //! @param theName  [in]      new scope name
  //! @param theMax   [in]      number of steps in scope
  //! @param isInfinite [in]    infinite flag
  Message_ProgressScope (const Message_ProgressRange& theRange,
                         const TCollection_AsciiString& theName,
                         Standard_Real theMax,
                         Standard_Boolean isInfinite = false);

  //! Creates a new scope taking responsibility of the part of the progress 
  //! scale described by theRange. The new scope has own range from 0 to 
  //! theMax, which is mapped to the given range.
  //!
  //! The topmost scope is created and owned by Message_ProgressIndicator
  //! and its pointer is contained in the Message_ProgressRange returned by the Start() method of progress indicator.
  //!
  //! @param theRange [in][out] range to fill (will be disarmed)
  //! @param theName  [in]      new scope name constant (will be stored by pointer with no deep copy)
  //! @param theMax   [in]      number of steps in scope
  //! @param isInfinite [in]    infinite flag
  template<size_t N>
  Message_ProgressScope (const Message_ProgressRange& theRange,
                         const char (&theName)[N],
                         Standard_Real theMax,
                         Standard_Boolean isInfinite = false);

  //! Creates a new scope taking responsibility of the part of the progress 
  //! scale described by theRange. The new scope has own range from 0 to 
  //! theMax, which is mapped to the given range.
  //!
  //! The topmost scope is created and owned by Message_ProgressIndicator
  //! and its pointer is contained in the Message_ProgressRange returned by the Start() method of progress indicator.
  //!
  //! @param theRange [in][out] range to fill (will be disarmed)
  //! @param theName  [in]      empty scope name (only NULL is accepted as argument)
  //! @param theMax   [in]      number of steps in scope
  //! @param isInfinite [in]    infinite flag
  Message_ProgressScope (const Message_ProgressRange& theRange,
                         const NullString* theName,
                         Standard_Real theMax,
                         Standard_Boolean isInfinite = false);

  //! Sets the name of the scope.
  void SetName (const TCollection_AsciiString& theName)
  {
    if (myIsOwnName)
    {
      Standard::Free (myName);
      myIsOwnName = false;
    }
    myName = NULL;
    if (!theName.IsEmpty())
    {
      myIsOwnName = true;
      myName = (char* )Standard::Allocate (theName.Length() + 1);
      char* aName = (char* )myName;
      memcpy (aName, theName.ToCString(), theName.Length());
      aName[theName.Length()] = '\0';
    }
  }

  //! Sets the name of the scope; can be null.
  //! Note! Just pointer to the given string is copied,
  //! so do not pass string from a temporary variable whose
  //! lifetime is less than that of this object.
  template<size_t N>
  void SetName (const char (&theName)[N])
  {
    if (myIsOwnName)
    {
      Standard::Free (myName);
      myIsOwnName = false;
    }
    myName = theName;
  }

public: //! @name Advance by iterations

  //! Returns true if ProgressIndicator signals UserBreak
  Standard_Boolean UserBreak() const;

  //! Returns false if ProgressIndicator signals UserBreak
  Standard_Boolean More() const
  {
    return !UserBreak();
  }

  //! Advances position by specified step and returns the range
  //! covering this step
  Message_ProgressRange Next (Standard_Real theStep = 1.);

public: //! @name Auxiliary methods to use in ProgressIndicator

  //! Force update of presentation of the progress indicator.
  //! Should not be called concurrently.
  void Show();

  //! Returns true if this progress scope is attached to some indicator.
  Standard_Boolean IsActive() const
  {
    return myIsActive;
  }

  //! Returns the name of the scope (may be null).
  //! Scopes with null name (e.g. root scope) should
  //! be bypassed when reporting progress to the user.
  Standard_CString Name() const
  {
    return myName;
  }

  //! Returns parent scope (null for top-level scope)
  const Message_ProgressScope* Parent() const
  {
    return myParent;
  }

  //! Returns the maximal value of progress in this scope
  Standard_Real MaxValue() const
  {
    return myMax;
  }

  //! Returns the current value of progress in this scope.
  //!
  //! The value is computed by mapping current global progress into 
  //! this scope range; the result is rounded up to integer.
  //! Note that if MaxValue() is not an integer, Value() can be 
  //! greater than MaxValue() due to that rounding.
  //!
  //! This method should not be called concurrently while the progress
  //! is advancing, except from implementation of method Show() in
  //! descendant of Message_ProgressIndicator.
  Standard_Real Value() const;

  //! Returns the infinite flag
  Standard_Boolean IsInfinite() const
  {
    return myIsInfinite;
  }

  //! Get the portion of the indicator covered by this scope (from 0 to 1)
  Standard_Real GetPortion() const
  {
    return myPortion;
  }

public: //! @name Destruction, allocation

  //! Destructor - closes the scope and adds its scale to the total progress
  ~Message_ProgressScope()
  {
    Close();
    if (myIsOwnName)
    {
      Standard::Free (myName);
      myIsOwnName = false;
      myName = NULL;
    }
  }

  //! Closes the scope and advances the progress to its end.
  //! Closed scope should not be used.
  void Close();

  DEFINE_STANDARD_ALLOC

private: //! @name Internal methods
  
  //! Creates a top-level scope with default range [0,1] and step 1.
  //! Called only by Message_ProgressIndicator constructor.
  Message_ProgressScope (Message_ProgressIndicator* theProgress);

  //! Convert value from this scope to global scale, but disregarding
  //! start position of the scope, in the range [0, myPortion]
  Standard_Real localToGlobal(const Standard_Real theVal) const;

private:
  //! Copy constructor is prohibited
  Message_ProgressScope (const Message_ProgressScope& theOther);

  //! Copy assignment is prohibited
  Message_ProgressScope& operator= (const Message_ProgressScope& theOther);

private:

  Message_ProgressIndicator* myProgress; //!< Pointer to progress indicator instance
  const Message_ProgressScope* myParent; //!< Pointer to parent scope
  Standard_CString   myName;        //!< Name of the operation being done in this scope, or null

  Standard_Real      myStart;       //!< Start position on the global scale [0, 1]
  Standard_Real      myPortion;     //!< The portion of the global scale covered by this scope [0, 1]

  Standard_Real      myMax;         //!< Maximal value of progress in this scope
  Standard_Real      myValue;       //!< Current position advanced within this scope [0, Max]

  Standard_Boolean   myIsActive;    //!< flag indicating armed/disarmed state
  Standard_Boolean   myIsOwnName;   //!< flag indicating if name was allocated or not
  Standard_Boolean   myIsInfinite;  //!< Option to advance by hyperbolic law

private:
  friend class Message_ProgressIndicator;
  friend class Message_ProgressRange;
};

#include <Message_ProgressRange.hxx>

//=======================================================================
//function : Message_ProgressScope
//purpose  :
//=======================================================================
inline Message_ProgressScope::Message_ProgressScope (Message_ProgressIndicator* theProgress)
: myProgress(theProgress),
  myParent(0),
  myName(0),
  myStart(0.),
  myPortion(1.),
  myMax(1.),
  myValue(0.),
  myIsActive(theProgress != NULL),
  myIsOwnName(false),
  myIsInfinite(false)
{
}

//=======================================================================
//function : Message_ProgressScope
//purpose  :
//=======================================================================
inline Message_ProgressScope::Message_ProgressScope (const Message_ProgressRange& theRange,
                                                     const TCollection_AsciiString& theName,
                                                     Standard_Real theMax,
                                                     Standard_Boolean isInfinite)
: myProgress (theRange.myParentScope != NULL ? theRange.myParentScope->myProgress : NULL),
  myParent (theRange.myParentScope),
  myName (NULL),
  myStart (theRange.myStart),
  myPortion (theRange.myDelta),
  myMax (Max (1.e-6, theMax)), // protection against zero range
  myValue (0.),
  myIsActive (myProgress != NULL && !theRange.myWasUsed),
  myIsOwnName (false),
  myIsInfinite (isInfinite)
{
  SetName (theName);
  Standard_ASSERT_VOID (! theRange.myWasUsed, "Message_ProgressRange is used to initialize more than one scope");
  theRange.myWasUsed = true; // Disarm the range
}

//=======================================================================
//function : Message_ProgressScope
//purpose  :
//=======================================================================
template<size_t N>
Message_ProgressScope::Message_ProgressScope (const Message_ProgressRange& theRange,
                                              const char (&theName)[N],
                                              Standard_Real theMax,
                                              Standard_Boolean isInfinite)
: myProgress (theRange.myParentScope != NULL ? theRange.myParentScope->myProgress : NULL),
  myParent (theRange.myParentScope),
  myName (theName),
  myStart (theRange.myStart),
  myPortion (theRange.myDelta),
  myMax (Max (1.e-6, theMax)), // protection against zero range
  myValue (0.),
  myIsActive (myProgress != NULL && !theRange.myWasUsed),
  myIsOwnName (false),
  myIsInfinite (isInfinite)
{
  Standard_ASSERT_VOID (! theRange.myWasUsed, "Message_ProgressRange is used to initialize more than one scope");
  theRange.myWasUsed = true; // Disarm the range
}

//=======================================================================
//function : Message_ProgressScope
//purpose  :
//=======================================================================
inline Message_ProgressScope::Message_ProgressScope (const Message_ProgressRange& theRange,
                                                     const NullString* ,
                                                     Standard_Real theMax,
                                                     Standard_Boolean isInfinite)
: myProgress (theRange.myParentScope != NULL ? theRange.myParentScope->myProgress : NULL),
  myParent (theRange.myParentScope),
  myName (NULL),
  myStart (theRange.myStart),
  myPortion (theRange.myDelta),
  myMax (Max (1.e-6, theMax)), // protection against zero range
  myValue (0.),
  myIsActive (myProgress != NULL && !theRange.myWasUsed),
  myIsOwnName (false),
  myIsInfinite (isInfinite)
{
  Standard_ASSERT_VOID (! theRange.myWasUsed, "Message_ProgressRange is used to initialize more than one scope");
  theRange.myWasUsed = true; // Disarm the range
}

//=======================================================================
//function : Close
//purpose  :
//=======================================================================
inline void Message_ProgressScope::Close()
{
  if (!myIsActive)
  {
    return;
  }

  // Advance indicator to the end of the scope
  Standard_Real aCurr = localToGlobal (myValue);
  myValue = (myIsInfinite ? Precision::Infinite() : myMax);
  Standard_Real aDelta = myPortion - aCurr;
  if (aDelta > 0.)
  {
    myProgress->Increment (aDelta, *this);
  }
  Standard_ASSERT_VOID (myParent == 0 || myParent->myIsActive,
    "Parent progress scope has been closed before child");

  myIsActive = false;
}

//=======================================================================
//function : UserBreak
//purpose  :
//=======================================================================
inline Standard_Boolean Message_ProgressScope::UserBreak() const
{
  return myProgress && myProgress->UserBreak();
}

//=======================================================================
//function : Next
//purpose  :
//=======================================================================
inline Message_ProgressRange Message_ProgressScope::Next (Standard_Real theStep)
{
  if (myIsActive && theStep > 0.)
  {
    Standard_Real aCurr = localToGlobal(myValue);
    Standard_Real aNext = localToGlobal(myValue += theStep);
    Standard_Real aDelta = aNext - aCurr;
    if (aDelta > 0.)
    {
      return Message_ProgressRange(*this, myStart + aCurr, aDelta);
    }
  }
  return Message_ProgressRange();
}

//=======================================================================
//function : Show
//purpose  :
//=======================================================================

inline void Message_ProgressScope::Show ()
{
  if (myIsActive)
  {
    myProgress->Show (*this, Standard_True);
  }
}

//=======================================================================
//function : localToGlobal
//purpose  :
//=======================================================================
inline Standard_Real Message_ProgressScope::localToGlobal (const Standard_Real theVal) const
{
  if (theVal <= 0.)
    return 0.;

  if (!myIsInfinite)
  {
    if (myMax - theVal < RealSmall())
      return myPortion;
    return myPortion * theVal / myMax;
  }

  double x = theVal / myMax;
  // return myPortion * ( 1. - std::exp ( -x ) ); // exponent
  return myPortion * x / (1. + x);  // hyperbola
}

//=======================================================================
//function : Value
//purpose  :
//=======================================================================

inline Standard_Real Message_ProgressScope::Value () const
{
  if (!myIsActive)
  {
    return myIsInfinite ? Precision::Infinite() : myMax;
  }

  // get current progress on the global scale counted 
  // from the start of this scope
  Standard_Real aVal = myProgress->GetPosition() - myStart;

  // if progress has not reached yet the start of this scope, return 0
  if (aVal <= 0.)
    return 0.;

  // if at end of the scope (or behind), report the maximum
  Standard_Real aDist = myPortion - aVal;
  if (aDist <= Precision::Confusion())
    return myIsInfinite ? Precision::Infinite() : myMax;

  // map the value to the range of this scope [0, Max],
  // rounding up to integer, with small correction applied
  // to avoid rounding errors
  return std::ceil (myMax * aVal / (myIsInfinite ? aDist : myPortion) - Precision::Confusion());
}

#endif // _Message_ProgressScope_HeaderFile
