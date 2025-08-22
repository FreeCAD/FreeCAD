# A primer FreeCAD's transaction system

## Purpose

The purpose of the transaction system is to allow tracking commands and enabling the undo-redo functionnality. Each transaction is a set of actions that are recorded and that can be played backward (undo) and played forward (redo).

FreeCAD's transaction system is opt-in which means that transactions must be explicitly created when calling commands in order to be recorded. Such flexibility is great as there may be some actions that should not be recorded, however that means that **actions that affect the file can take place outside the transaction system** and it is up to the individual workbench developpers (be they core workbenches or addons) to make sure that the undo-redo functionnality is implemented and functionnal. See **Best practices** for a few recommandations on how to handle transactions at the workbench level.

### Commit and abort

There are 2 ways of closing a transaction: commit and abort. Commit is used when the operation was successful and adds the transaction to the undo stack. Abort is used when the operation was a failure, it rolls-back the changes made during the transaction and does not add it to the undo stack. Ideally the state of the file should be the same as before the command started after it aborts, if that's not the case, it's a bug.

### Scope

There are 2 possible transaction scopes: document and global.

#### Document scope

This is a transaction that applies to a document, there can be an arbitrary number of document scoped transactions opened at the same time (up to the number of opened documents).

#### Application scope

This is a transaction that applies to every document, in versions 1.1 and prior, all transactions were application-scoped. In versions 1.2 onward application-scoped transactions are implemented as a special document-scoped transaction to which any document can attach. **They should not be used in new code**

## Inner workings

### ID

A transaction is represented by a positive, non-zero integer that is unique within the application run. Since transactions are not saved in the file, the transactions ID (often abreviated `tid` in code) start at 1 when the first transaction is created and a counter is incremented to generate the next IDs.

This ID can then be used to manipulate the transaction or the document:

* commit
* abort
* set the name
* undo up to
* redo up to

### Atomic changes

`DocumentObject`s are `TransactionalObject`s which means that they contain properties and record changes to them. These properties are aware that they are contained in a `TransactionalObject` through their `father` pointer and can notify their container when they change so that it may be recorded. When notified, the `TransactionalObject` forwards the change to the `DocumentObject`'s `Document`'s `onBeforeChangeProperty` function so that the change can be recorded in the currently setuped transaction (if it exists)

The callback chain looks something like this, assuming a transaction is setup, the DocumentObject is attached to a Document and everything goes well:
`Property::setValue` => `Property::aboutToSetValue` [`father`]=> `DocumentObject::onBeforeChange` => `TransactionalObject::onBeforeChangeProperty` => `Document::onBeforeChangeProperty` => `Transaction::addObjectChange`

FreeCAD's transaction system works by recording *atomic* changes to properties. Properties are wrappers around types (distance, 3d vector, link to object, etc.) which allow such monitoring.

The 4 changes that can be recorded on a property are
* Create
* Delete
* Rename
* Set value

Properties are exposed to python and setting a value

```
dimension.X = 5
```

will internally call the callback chain that records the property change.
### Putting it all together

#### Creation

When creating a transaction from user code the only thing created is the ID which is returned to the call site for further uses. This ID is set into `App::Document::d->bookedTransactionId` variable. At this point no transaction is created, but an ID is booked in the `App::Document` object.

#### Use

When a `Property` is somehow modified, the `App::Document` class handles that logic and will create the actual `Transaction` which will hold the changes. When it creates the `Transaction`, it first checks if the `bookedTransactionId` is valid (non-zero) otherwise it will check if a global transaction ID is valid (non-zero) and assign this value to it's `bookedTransactionId`. If no ID is valid, then the change happens outside the transaction system. If an ID is found the `Transaction` gets the ID and the name that is stored alongside the ID in the App::Application class. When additional properties are changed the current `Transaction` which is called `App::Document::d->activeUndoTransaction` is used.

#### Close

When ending a transaction, the transaction ID is used to address it. The `App::Application` class goes through all opened documents and checks if their current `bookedTransaction` matches the ID they are added to a deletion vector (it is a vector because multiple documents can have transactions with the same ID). Additional checks are performed on the vector to ensure that it can be closed. Cases where it is not possible include `DocumentObject` deletion operations where there is a critical section that must execute in it's entirety and transactions already in the process of aborting. If the transaction may be closed, for each targeted document, the `activeUndoTransaction` is moved to it's undo vector (each document has it's own), then `activeUndoTransaction` and `bookedTransactionId` nullified and ready for the next transaction.

#### Undo

When undoing, the last transaction in the `mUndoVector` of the `App::Document` is done backward (actualy in the code, it kind of looks like it is done forward, but that's because it is recorded backward). It is then popped off. And added to a different vector the `mRedoVector`.

#### Redo

The code to redo is almost identical, only the roles of the `mRedoVector` and `mUndoVector` are reversed

## API

There are multiple ways to interract with the transaction system, here are guideline on how to use different API that will all do essentially the same thing.

Note: functions that interact with the transaction system in Gui:: will use the word `command`, but in the App:: they will use `transaction`. They refer to the same thing and the variable and it is recommended to use a variable named `tid` for the transaction ID in App:: AND Gui::

### Command

Here `Command` does not refer to a transaction (sorry), it is a class that acts as a link between the transaction system and Qt's actions and many operations in FreeCAD are implemented as a Command. Commands implement an `activated` function where a transaction is often created.

* `Command::openCommand(name, tmpName, tid)` => opens a transaction on the current active document and store the transaction ID.
* `Command::commitCommand` => commits the transaction created with `Command::openCommand`
* `Command::abortCommand` => aborts the transaction created with `Command::openCommand`
* `Command::openCommandActiveDocument(name)` => [static] opens a transaction on the current active document and returns the transaction ID.

### Gui::Document (exposed to python)

This API is useful if you have access to the Gui::Document, in a ViewProvider for instance.

* `Gui::Document::openCommand` => opens a transaction on the document and returns the transaction ID.
* `Gui::Document::commitCommand` => commits the currently opened/booked transaction on the document
* `Gui::Document::abortCommand` => aborts the currently opened/booked transaction on the document

### App::Document (exposed to python)

This API is useful if you must touch multiple documents at once because it allows you to specify a transaction ID. This means that you can open a transaction on the first document and reuse the ID on the remaining documents. As established, transactions that have the same ID accross documents can be undone as a group.

* `App::Document::openTransaction(name, tmpName, tid)`
* `App::Document::commitTransaction()`
* `App::Document::abortTransaction()`

### App::Application (exposed to python)

* `App::Application::setActiveTransaction` => opens a transaction on the current active document or opens a global transaction if no document is active
* `App::Application::openGlobalTransaction() [c++ only]` => opens a global transaction
* `App::Application::closeActiveTransaction(abort, tid)` => closes the transaction that matches the ID or the transaction of the current active document if the `tid` is null
* `App::Application::abortTransaction(tid) [c++ only]` => alias for `App::Application::closeActiveTransaction(abort, tid)`
* `App::Application::commit transaction(tid) [c++ only]` => alias for `App::Application::closeActiveTransaction(abort, tid)`

## Best practices

In general when opening a new transaction you should know where it is closed. This is not crucial so to speak as opening a new transaction will close the previous transaction on the document. Not all of FreeCAD is conform, but new code should follow this advice as best as possible.

In general you should also know the breadth of the transaction and not open a global transaction, but rather open a group transaction on targeted documents.

If the transaction extends to multiple documents, yoou should use the `App::Document` API and pass the same `tid` to all `openTransaction` calls.

See it used in a Command in `StdCmdLinkReplace::activated`.

### C++ workbench

#### Command

For single document self contained commands, you should use the command API. You should not use it if the transaction is not started and closed in the `activated` function.

See it used in `CmdPartDesignClone`

If the transaction extends beyond the bounds of the `activated` function (e.g. it starts an edit dialog), the transaction should be created by any API and the resulting `tid` should be passed to the dialog. Alternatively, the target document can be passed along. Idealy the dialog should not have to guess which document's transaction to close (usualy take the activeDocument).

See this patern used in Sketcher's `CommandConstraints` which invokes a `EditDatumDialog`

#### ViewProvider and other call sites

You should use the API that is the most natural at the call site. For instance, if a dialog receives an `App::Document` to do it's normal duties, the `App::Document` API is natural. For ViewProviders of document objects, the `Gui::Document` API is natural most of the time because the gui document is easy to access.

### Python workbench

Many external addons do not make use of the transaction system, which can lead to user confusion, to use it there are 2 main approaches

* Using the `App::Document` API like the BIM workbench and built-in assembly
* Using the `App::Application` API like the Assembly3 workbench
