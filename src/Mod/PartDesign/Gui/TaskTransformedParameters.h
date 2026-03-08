// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
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


#pragma once

#include <Gui/ComboLinks.h>
#include <Gui/DocumentObserver.h>
#include <Gui/Selection/Selection.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/PartDesign/Gui/EnumFlags.h>

#include "TaskFeatureParameters.h"
#include "TaskTransformedMessages.h"
#include "ViewProviderTransformed.h"

class QListWidget;

class Ui_TaskTransformedParameters;

namespace Part
{
class Feature;
}

namespace PartDesign
{
class Transformed;
}

namespace PartDesignGui
{

class TaskMultiTransformParameters;

/**
  The transformed subclasses will be used in two different modes:
  1. As a stand-alone feature
  2. As a container that stores transformation info for a MultiTransform feature. In this case
     the flag insideMultiTransform is set to true.
  Because in the second case there is no ViewProvider, some special methods are required to
  access the underlying FeatureTransformed object in two different ways.
  **/
class TaskTransformedParameters: public Gui::TaskView::TaskBox,
                                 public Gui::SelectionObserver,
                                 public Gui::DocumentObserver
{
    Q_OBJECT

public:
    /// Constructor for task with ViewProvider
    explicit TaskTransformedParameters(
        ViewProviderTransformed* TransformedView,
        QWidget* parent = nullptr
    );
    /// Constructor for task with parent task (MultiTransform mode)
    explicit TaskTransformedParameters(TaskMultiTransformParameters* parentTask);
    ~TaskTransformedParameters() override;

    /// Apply changes for python console
    virtual void apply() = 0;

    /*!
     * \brief setEnabledTransaction
     * The transaction handling of this panel can be disabled if there is another
     * instance that does it already, e.g. TaskDlgMultiTransformParameters.
     * By default, transactions are enabled.
     */
    void setEnabledTransaction(bool /*unused*/);

    /// Exit the selection mode of the associated task panel
    void exitSelectionMode();

    static void removeItemFromListWidget(QListWidget* widget, const QString& itemstr);

protected:
    /** Setup the standalone UI.
     * Call this in the derived destructor with ViewProvider.
     */
    void setupUI();

    /**
     * Returns the base transformation
     * For standalone features it will be object associated with the view provider
     * For features inside MultiTransform it will be the parent MultiTransform's sub feature object
     */
    PartDesign::Transformed* getObject() const;

    template<class T>
    T* getObject() const
    {
        return freecad_cast<T*>(getObject());
    }

    /// Get the sketch object of the first original either of the object associated with this
    /// feature or with the parent feature (MultiTransform mode)
    App::DocumentObject* getSketchObject() const;

    /** Handle adding/removing of selected features
     * Returns true if a selected feature was added/removed.
     */
    bool originalSelected(const Gui::SelectionChanges& msg);

    /// Recompute either this feature or the parent MultiTransform feature
    void recomputeFeature();

    /// Hide the top transformed object (see getTopTransformedObject())
    void hideObject();
    /// Show the top transformed object (see getTopTransformedObject())
    void showObject();
    /// Hide the base transformation object (see getObject())
    void hideBase();
    /// Show the base transformation object (see getObject())
    void showBase();

    void addReferenceSelectionGate(AllowSelectionFlags);

    int getUpdateViewTimeout() const;

    /** Notifies when the object is about to be removed. */
    void slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj) override;

    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

    /// Fill combobox with the axis from the sketch and the own bodys origin axis
    void fillAxisCombo(Gui::ComboLinks& combolinks, Part::Part2DObject* sketch);
    /// Fill combobox with the planes from the sketch and the own bodys origin planes
    void fillPlanesCombo(Gui::ComboLinks& combolinks, Part::Part2DObject* sketch);

    /**
     * Returns the base transformed objectfromStdString
     * For stand alone features it will be objects associated with this object
     * For features inside multitransform it will be the base multitransform object
     */
    PartDesign::Transformed* getTopTransformedObject() const;

    bool isEnabledTransaction() const;
    void setupTransaction();

private Q_SLOTS:
    virtual void onUpdateView(bool /*unused*/) = 0;

    void onButtonAddFeature(bool checked);
    void onButtonRemoveFeature(bool checked);
    void onFeatureDeleted();
    void indexesMoved();
    void onModeChanged(int mode_id);

private:
    /** Setup the parameter UI.
     * This is called to create the parameter UI in the specified widget.
     * Call this in the derived constructor with MultiTransform parent.
     */
    virtual void setupParameterUI(QWidget* widget) = 0;

    /// Change translation of the parameter UI
    virtual void retranslateParameterUI(QWidget* widget) = 0;

    void addObject(App::DocumentObject*);
    void removeObject(App::DocumentObject*);
    void clearButtons();
    void checkVisibility();

    /// Return the base object of the base transformed object (see getTopTransformedObject())
    // Either through the ViewProvider or the currently active subFeature of the parentTask
    App::DocumentObject* getBaseObject() const;

    /**
     * Returns the base transformation view provider
     * For stand alone features it will be view provider associated with this object
     * For features inside multitransform it will be the view provider of the multitransform object
     */
    PartDesignGui::ViewProviderTransformed* getTopTransformedView() const;

    void changeEvent(QEvent* event) override;

protected:
    enum class SelectionMode
    {
        None,
        AddFeature,
        RemoveFeature,
        Reference
    };

    ViewProviderTransformed* TransformedView = nullptr;
    SelectionMode selectionMode = SelectionMode::None;

    /// Lock updateUI(), applying changes to the underlying feature and calling recomputeFeature()
    bool blockUpdate = false;

private:
    int transactionID = 0;
    bool enableTransaction = true;
    /// The MultiTransform parent task of this task
    TaskMultiTransformParameters* parentTask = nullptr;
    /// Flag indicating whether this object is a container for MultiTransform
    bool insideMultiTransform = false;
    /// Widget holding the transform task UI
    QWidget* proxy = nullptr;
    std::unique_ptr<Ui_TaskTransformedParameters> ui;
};

/// simulation dialog for the TaskView
class TaskDlgTransformedParameters: public PartDesignGui::TaskDlgFeatureParameters
{
    Q_OBJECT

public:
    explicit TaskDlgTransformedParameters(ViewProviderTransformed* TransformedView);

    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;

protected:
    TaskTransformedParameters* parameter = nullptr;
    TaskTransformedMessages* message = nullptr;
};

}  // namespace PartDesignGui
