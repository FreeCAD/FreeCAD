/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#ifndef GUI_TASKVIEW_TaskTransformedParameters_H
#define GUI_TASKVIEW_TaskTransformedParameters_H

#include <QComboBox>

#include <Mod/Part/App/Part2DObject.h>

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/DocumentObserver.h>

#include "TaskFeatureParameters.h"
#include "TaskTransformedMessages.h"
#include "ViewProviderTransformed.h"

class QListWidget;

namespace Part {
class Feature;
}

namespace PartDesign {
class Transformed;
}

namespace PartDesignGui {

class TaskMultiTransformParameters;

/**
 * @brief The ComboLinks class is a helper class that binds to a combo box and
 * provides an interface to add links, retrieve links and select items by link
 * value
 */
class ComboLinks
{
public:
    /**
     * @brief ComboLinks constructor.
     * @param combo. It will be cleared as soon as it is bound. Don't add or
     * remove items from the combo directly, otherwise internal tracking list
     * will go out of sync, and crashes may result.
     */
    ComboLinks(QComboBox &combo);
    ComboLinks() {_combo = 0; doc = 0;}
    void setCombo(QComboBox &combo) {assert(_combo == 0); this->_combo = &combo; _combo->clear();}

    /**
     * @brief addLink adds an item to the combo. Doesn't check for duplicates.
     * @param lnk can be a link to NULL, which is usually used for special item "Select Reference"
     * @param itemText
     * @return
     */
    int addLink(const App::PropertyLinkSub &lnk, QString itemText);
    int addLink(App::DocumentObject* linkObj, std::string linkSubname, QString itemText);
    void clear();
    App::PropertyLinkSub& getLink(int index) const;

    /**
     * @brief getCurrentLink
     * @return the link corresponding to the selected item. May be null link,
     * which is usually used to indicate a "Select reference..." special item.
     * Otherwise, the link is automatically tested for validity (oif an object
     * doesn't exist in the document, an exception will be thrown.)
     */
    App::PropertyLinkSub& getCurrentLink() const;

    /**
     * @brief setCurrentLink selects the item with the link that matches the
     * argument. If there is no such link in the list, -1 is returned and
     * selected item is not changed. Signals from combo are blocked in this
     * function.
     * @param lnk
     * @return the index of an item that was selected, -1 if link is not in the list yet.
     */
    int setCurrentLink(const App::PropertyLinkSub &lnk);

    QComboBox& combo(void) const {assert(_combo); return *_combo;}

    ~ComboLinks() {_combo = 0; clear();}
private:
    QComboBox* _combo;
    App::Document* doc;
    std::vector<App::PropertyLinkSub*> linksInList;
};

/**
  The transformed subclasses will be used in two different modes:
  1. As a stand-alone feature
  2. As a container that stores transformation info for a MultiTransform feature. In this case
     the flag insideMultiTransform is set to true.
  Because in the second case there is no ViewProvider, some special methods are required to
  access the underlying FeatureTransformed object in two different ways.
  **/
class TaskTransformedParameters : public Gui::TaskView::TaskBox,
                                  public Gui::SelectionObserver,
                                  public Gui::DocumentObserver
{
    Q_OBJECT

public:
    /// Constructor for task with ViewProvider
    TaskTransformedParameters(ViewProviderTransformed *TransformedView, QWidget *parent = 0);
    /// Constructor for task with parent task (MultiTransform mode)
    TaskTransformedParameters(TaskMultiTransformParameters *parentTask);
    virtual ~TaskTransformedParameters();

    /// Get the TransformedFeature object associated with this task
    // Either through the ViewProvider or the currently active subFeature of the parentTask
    App::DocumentObject *getBaseObject() const;

    /// Get the sketch object of the first original either of the object associated with this feature or with the parent feature (MultiTransform mode)
    App::DocumentObject* getSketchObject() const;   

    void exitSelectionMode();

    virtual void apply() = 0;

    void setupTransaction();
    void setupCheckBox(QCheckBox *checkBox);

protected Q_SLOTS:
    /**
     * Returns the base transformation view provider
     * For stand alone features it will be view provider associated with this object
     * For features inside multitransform it will be the view provider of the multitransform object
     */
    PartDesignGui::ViewProviderTransformed *getTopTransformedView () const;

    /**
     * Returns the base transformed object
     * For stand alone features it will be objects associated with this object
     * For features inside multitransform it will be the base multitransform object
     */
    PartDesign::Transformed *getTopTransformedObject () const;

    /// Connect the subTask OK button to the MultiTransform task
    virtual void onSubTaskButtonOK() {}
    void onButtonAddFeature(const bool checked);
    void onButtonRemoveFeature(const bool checked);
    virtual void onFeatureDeleted(void);
    void onChangedSubTransform(bool);

protected:
    /**
     * Returns the base transformation
     * For stand alone features it will be objects associated with the view provider
     * For features inside multitransform it will be the parent's multitransform object
     */
    PartDesign::Transformed *getObject () const;

    bool originalSelected(const Gui::SelectionChanges& msg);
    void populate();
    void setupListWidget(QListWidget *listWidget);

    /// Recompute either this feature or the parent feature (MultiTransform mode)
    void recomputeFeature();

    void hideObject();
    void showObject();
    void hideBase();
    void showBase();

    void addReferenceSelectionGate(bool edge, bool face, bool planar=true, bool whole=false);    

    bool isViewUpdated() const;
    int getUpdateViewTimeout() const;

    void checkVisibility();

protected:
    /** Notifies when the object is about to be removed. */
    virtual void slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj);
    virtual void changeEvent(QEvent *e) = 0;
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg) = 0;
    virtual void clearButtons()=0;

    void fillAxisCombo(ComboLinks &combolinks, Part::Part2DObject *sketch);
    void fillPlanesCombo(ComboLinks &combolinks, Part::Part2DObject *sketch);

protected:
    QWidget* proxy;
    ViewProviderTransformed *TransformedView;

    enum selectionModes { none, addFeature, removeFeature, reference };
    selectionModes selectionMode;

    /// The MultiTransform parent task of this task
    TaskMultiTransformParameters* parentTask;
    /// Flag indicating whether this object is a container for MultiTransform
    bool insideMultiTransform;
    /// Lock updateUI(), applying changes to the underlying feature and calling recomputeFeature()
    bool blockUpdate;    

    QListWidget *listWidget = 0;
};

/// simulation dialog for the TaskView
class TaskDlgTransformedParameters : public PartDesignGui::TaskDlgFeatureParameters
{
    Q_OBJECT

public:
    TaskDlgTransformedParameters(
            ViewProviderTransformed *TransformedView, TaskTransformedParameters *parameter);

    virtual ~TaskDlgTransformedParameters() {}

    ViewProviderTransformed* getTransformedView() const
    { return static_cast<ViewProviderTransformed*>(vp); }

public:
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
protected:
    TaskTransformedParameters  *parameter;
    TaskTransformedMessages  *message;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
