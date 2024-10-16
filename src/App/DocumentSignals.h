
#ifndef APP_DOCUMENTSIGNALS_H
#define APP_DOCUMENTSIGNALS_H

#include <boost/signals2.hpp>


namespace App {
struct Document::Public {
    /// signal before changing an doc property
    boost::signals2::signal<void (const App::Document&, const App::Property&)> signalBeforeChange;
    /// signal on changed doc property
    boost::signals2::signal<void (const App::Document&, const App::Property&)> signalChanged;
    /// signal on new Object
    boost::signals2::signal<void (const App::DocumentObject&)> signalNewObject;
    //boost::signals2::signal<void (const App::DocumentObject&)>     m_sig;
    /// signal on deleted Object
    boost::signals2::signal<void (const App::DocumentObject&)> signalDeletedObject;
    /// signal before changing an Object
    boost::signals2::signal<void (const App::DocumentObject&, const App::Property&)> signalBeforeChangeObject;
    /// signal on changed Object
    boost::signals2::signal<void (const App::DocumentObject&, const App::Property&)> signalChangedObject;
    /// signal on manually called DocumentObject::touch()
    boost::signals2::signal<void (const App::DocumentObject&)> signalTouchedObject;
    /// signal on relabeled Object
    boost::signals2::signal<void (const App::DocumentObject&)> signalRelabelObject;
    /// signal on activated Object
    boost::signals2::signal<void (const App::DocumentObject&)> signalActivatedObject;
    /// signal on created object
    boost::signals2::signal<void (const App::DocumentObject&, Transaction*)> signalTransactionAppend;
    /// signal on removed object
    boost::signals2::signal<void (const App::DocumentObject&, Transaction*)> signalTransactionRemove;
    /// signal on undo
    boost::signals2::signal<void (const App::Document&)> signalUndo;
    /// signal on redo
    boost::signals2::signal<void (const App::Document&)> signalRedo;
    /** signal on load/save document
     * this signal is given when the document gets streamed.
     * you can use this hook to write additional information in
     * the file (like the Gui::Document does).
     */
    boost::signals2::signal<void (Base::Writer   &)> signalSaveDocument;
    boost::signals2::signal<void (Base::XMLReader&)> signalRestoreDocument;
    boost::signals2::signal<void (const std::vector<App::DocumentObject*>&,
                                  Base::Writer   &)> signalExportObjects;
    boost::signals2::signal<void (const std::vector<App::DocumentObject*>&,
                                  Base::Writer   &)> signalExportViewObjects;
    boost::signals2::signal<void (const std::vector<App::DocumentObject*>&,
                                  Base::XMLReader&)> signalImportObjects;
    boost::signals2::signal<void (const std::vector<App::DocumentObject*>&, Base::Reader&,
                                  const std::map<std::string, std::string>&)> signalImportViewObjects;
    boost::signals2::signal<void (const std::vector<App::DocumentObject*>&)> signalFinishImportObjects;
    //signal starting a save action to a file
    boost::signals2::signal<void (const App::Document&, const std::string&)> signalStartSave;
    //signal finishing a save action to a file
    boost::signals2::signal<void (const App::Document&, const std::string&)> signalFinishSave;
    boost::signals2::signal<void (const App::Document&)> signalBeforeRecompute;
    boost::signals2::signal<void (const App::Document&, const std::vector<App::DocumentObject*>&)> signalRecomputed;
    boost::signals2::signal<void (const App::DocumentObject&)> signalRecomputedObject;
    //signal a new opened transaction
    boost::signals2::signal<void (const App::Document&, std::string)> signalOpenTransaction;
    // signal a committed transaction
    boost::signals2::signal<void (const App::Document&)> signalCommitTransaction;
    // signal an aborted transaction
    boost::signals2::signal<void (const App::Document&)> signalAbortTransaction;
    boost::signals2::signal<void (const App::Document&, const std::vector<App::DocumentObject*>&)> signalSkipRecompute;
    boost::signals2::signal<void (const App::DocumentObject&)> signalFinishRestoreObject;
    boost::signals2::signal<void (const App::Document&,const App::Property&)> signalChangePropertyEditor;
    //@}
    boost::signals2::signal<void (std::string)> signalLinkXsetValue;
};
}


#endif