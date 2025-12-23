# Improve Postprocessors

## STATUS

Active

## Why it is a priority

Prior to 2022 it became clear that the existing postprocessors were hard
to maintain and develop.  An effort started in 2022 to develop a different approach
to postprocessors to share as much code as possible.

### Legacy postprocessors

The older postprocessors (hereafter referred to as "legacy" postprocessors)
were written with all of the code in a single file.

This has the following advantages:

- You can make changes to a single postprocessor without affecting anything else.
- You can copy and modify a single file to create a new postprocessor.

This has the following disadvantages:

- There is a lot of duplicate code in the postprocessors which makes it hard
to fix bugs or add new capabilities because changes must be made in many places.
- It is hard to test the postprocessors without also duplicating many tests.
- The size and complexity of the postprocessor files makes them harder to understand.
- The size and complexity of the postprocessor files makes it harder to figure out the
differences between the postprocessors (what makes them unique).

### Refactored postprocessors

A different approach to postprocessors was started that shared as much code as possible
(hereafter referred to as "refactored" postprocessors and the "refactored" code base).

The "refactored" postprocessors have the following advantages:

- The "refactored" postprocessor files are smaller and easier to understand.
- It is easier to figure out the differences between the "refactored" postprocessors.
- It is easier to customize a "refactored" postprocessor as long as the desired changes
have already been "designed in" to the "refactored" code base.
- It is easier to test the "refactored" code base since most of the code has one copy
and can be tested just once for many "refactored" postprocessors.
- It is easier to add new functionality to all (or many) of the "refactored"
postprocessors by adding code just once to the shared code in the "refactored" code base.
- Many bugs can be fixed for many "refactored" postprocessors by making changes
just once to the shared code in the "refactored" code base.
- The "refactored" postprocessors can share a single list of "command line options"
which are used to change the behavior of the postprocessor rather than each
postprocessor having its own (somewhat unique) list of command line options.

The "refactored" postprocessors have the following disadvantages:

- To customize/modify a "refactored" postprocessor often requires making changes to
the "shared" code in the "refactored" code base which affects all of the "refactored"
postprocessors.  The "refactored" code base must be modified to support making
the needed changes without affecting the other "refactored" postprocessors.

## Scope

### What has been done so far

In 2022 the existing versions of the "centroid", "grbl", "linuxcnc" and "mach3_mach4"
"legacy" postprocessors were copied and "merged" into the "starting" "refactored" code base.

Much refactoring of the "refactored" code base was done to identify shared code
and move the shared code to several shared files.
This "shrank" the sizes of the "refactored" postprocessor files a lot.

Some refactoring of the "refactored" code base was done to make the code style,
variable names, and function/routine/class parameters more consistent.
The "legacy" code was written and/or adapted by multiple people, each with their
own way of doing things.  The combination of code from multiple people was hard
to understand and maintain.

Many tests were written.  A "refactored_test" postprocessor was created which was
used to test the shared code base.  Very few additional tests were needed to test
the individual "refactored" postprocessors since most of the tests and code were shared.

Some of the shared code in the "refactored" code base was rewritten to an object
oriented style that allowed the "refactored" postprocessor files to get much smaller
and simpler by using inheritance.  Eventually this (object oriented) code was added
to the "PostProcessor" class in the "Processor.py" file to integrate the "refactored"
postprocessor code base into the rest of the CAM software.

The "Masso" postprocessor was developed based on the "refactored" code base.  As the
refactoring continued the "Masso" postprocessor file became much simpler, inheriting
from the "refactored_linuxcnc" postprocessor with one option set differently.

As of the time that this document was last modified, there are tests for many of the possible
"G" and "M" g-code commands, many of the command line options, and tests for other areas
of particular interest such as A, B, and C axis handling and F parameter handling.
While there are still some tests that might need to be written, the test suite now
covers a large part of the functionality.

As of the time that this document was last modified, there are "refactored" postprocessors
for "centroid", "grbl", "linuxcnc", "mach3_mach4", and "masso" controllers.

### What still needs to be done

#### Customizability

The main pain point left is the ability to create new or modified "refactored"
postprocessors without having to copy and modify lots of code.  When we went from
having all of the code in a separate file for each postprocessor to having most code
being shared between the "refactored" postprocessors we lost a lot of customizability.
We need to get "enough" customizability back into the "refactored" code base.

Much or all of the shared code in the "refactored" code base should be rewritten in
an object oriented style enough so that inheritance can be used to customize some
"refactored" postprocessors.  This should be a "light" rewrite since we have working
code with existing tests to exercise it in many cases and we don't need to make it
"more object oriented than necessary" :-)

It would be a good idea to look at the next few "most used" "legacy" postprocessors
and try to "port" them to the "refactored" code base one at a time.  Odds are that we
will learn something from each "legacy" postprocessor that is "ported" and will modify
the "refactored" code base to improve its customizability as a result.

It would be a good idea to look at porting or developing postprocessors that support
laser cutters, drag knife cutters, and controllers that use something other than g-code
as their control language (at least).  Odds are that we would learn something from each
attempt and the "refactored" code base would improve its customizability as a result.

#### What about "legacy" postprocessors?

We probably want to keep the ability to use "legacy" style postprocessors for a long
time to come.  Some "legacy" postprocessors may not be "ported" to the "refactored" code
base.  Other "legacy" postprocessors may need the full flexibility of having all of
their code in one file with no other postprocessors depending on their code.

Once we have working and tested (by the test suite and by people actually using
them) "refactored" postprocessors, do we still need to keep and maintain the "legacy"
versions of the same postprocessors?  It would be nice to reduce the support burden,
but we don't want to remove/reduce the ability to create new or modified postprocessors.
This puts extra emphasis on having "enough" customizability in the "refactored" code
base to meet "all" of the postprocessor needs so we can afford to deprecate and
(eventually) remove "unnecessary" legacy postprocessors.

#### Documentation

We will need documentation about and working examples of "how to write a custom postprocessor"
based on the "refactored" code base.  For example, a class hierarchy that shows what
methods are implemented where and which methods are able to be (or recommended to be)
overridden by child classes.  In addition to the existing "refactored" postprocessors
"ported" versions of some of the other "legacy" postprocessors would be good examples
of "how to write a custom postprocessor".

#### Testing

We will need to end up with a testing framework for the "refactored" postprocessors that is:

 1. Efficient
    1. Doesn't require loading files from the disk
    2. Atomic so we're only testing things once
 2. Documented well enough that new postprocessor authors can easily understand how to
    add tests
 3. Extensible so new postprocessor authors can easily add tests
 4. Complete enough so that we can start recommending the "refactored" postprocessors
    and deprecating the legacy postprocessors that also have a "refactored" version

#### Other changes

There will be bug fixes, new tests, and new features that will need to be added
to the "refactored" code base.  It is likely that the "rules will change" about what
the rest of the FreeCAD code will send to the postprocessors and/or the format of the
input to the postprocessors will change.

## Related Epics
