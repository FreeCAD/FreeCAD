/***************************************************************************
 *   Copyright (c) 2012 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_MANUALALIGNMENT_H
#define GUI_MANUALALIGNMENT_H

#include <QPointer>
#include <Base/BoundBox.h>
#include <Base/Placement.h>
#include <Base/Vector3D.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderDocumentObject.h>

class SbVec3f;
class SoPickedPoint;
class SoEventCallback;

namespace Gui {
class Document;
class AlignmentView;
class View3DInventorViewer;

class PickedPoint {
public:
    PickedPoint() = default;
    PickedPoint(const Base::Vector3d& p, const Base::Vector3d& n) : point(p), normal(n) {}
    Base::Vector3d point;
    Base::Vector3d normal;
};

/**
 * The AlignemntGroup class is the base for fixed and movable groups.
 * @author Werner Mayer
 */
class GuiExport AlignmentGroup
{
protected:
    AlignmentGroup();
    ~AlignmentGroup();

public:
    /**
     * Add a mesh to the group.
     */
    void addView(App::DocumentObject*);
    std::vector<App::DocumentObject*> getViews() const;
    /**
     * Checks for the view provider of one of the added views.
     */
    bool hasView(Gui::ViewProviderDocumentObject*) const;
    /**
     * Remove a previously added view by its view provider.
     */
    void removeView(Gui::ViewProviderDocumentObject*);
    /**
     * Add the group and therefore all its added view providers to the Inventor tree.
     */
    void addToViewer(Gui::View3DInventorViewer*) const;
    /**
     * Remove all the view providers from the Inventor tree.
     */
    void removeFromViewer(Gui::View3DInventorViewer*) const;
    void setRandomColor();
    /**
     * Returns the document of the added views.
     */
    Gui::Document* getDocument() const;
    /**
     * Add a point to an array of picked points.
     */
    void addPoint(const PickedPoint&);
    /**
     * Remove last point from array of picked points.
     */
    void removeLastPoint();
    /**
     * Count the number of picked points.
     */
    int countPoints() const;
    /**
     * Return an array of picked points.
     */
    const std::vector<PickedPoint>& getPoints() const;
    /**
     * Clear all picked points.
     */
    void clearPoints();
    /**
     * Set or unset the alignable mode for the added views. If a view is not alignable it also not pickable.
     */
    void setAlignable(bool);
    void moveTo(AlignmentGroup&);
    /**
     * Clear the list of added views.
     */
    void clear();
    /**
     * Checks whether the list of added views is empty or not.
     */
    bool isEmpty() const;
    /**
     * Return the number of added views.
     */
    int count() const;
    /**
     * Get the overall bounding box of all views.
     */
    Base::BoundBox3d getBoundingBox() const;

protected:
    std::vector<PickedPoint> _pickedPoints;
    std::vector<Gui::ViewProviderDocumentObject*> _views;
};

/**
 * The FixedGroup class can be used for a fixed group of views.
 * @author Werner Mayer
 */
class GuiExport MovableGroup : public AlignmentGroup
{
public:
    MovableGroup();
    ~MovableGroup();
};

/**
 * The FixedGroup class can be used for a fixed group of views.
 * @author Werner Mayer
 */
class GuiExport FixedGroup : public AlignmentGroup
{
public:
    FixedGroup();
    ~FixedGroup();
};

/**
 * The MovableGroupModel class keeps an array of movable groups.
 * @author Werner Mayer
 */
class GuiExport MovableGroupModel
{
public:
    MovableGroupModel();
    ~MovableGroupModel();

    void addGroup(const MovableGroup&);
    void addGroups(const std::map<int, MovableGroup>&);
    MovableGroup& activeGroup();
    const MovableGroup& activeGroup() const;
    void continueAlignment();
    void clear();
    bool isEmpty() const;
    int count() const;
    const MovableGroup& getGroup(int i) const;
    Base::BoundBox3d getBoundingBox() const;

protected:
    void removeActiveGroup();

private:
    std::vector<MovableGroup> _groups;
};

/**
 * @author Werner Mayer
 */
class GuiExport ManualAlignment : public QObject
{
    Q_OBJECT

protected:
    ManualAlignment();
    ~ManualAlignment() override;

public:
    static ManualAlignment* instance();
    static void destruct();
    static bool hasInstance();

    void setMinPoints(int minPoints);
    void setFixedGroup(const FixedGroup&);
    void setModel(const MovableGroupModel&);
    void clearAll();

    void setViewingDirections(const Base::Vector3d& view1, const Base::Vector3d& up1,
                              const Base::Vector3d& view2, const Base::Vector3d& up2);
    void startAlignment(Base::Type mousemodel);
    void finish();
    void align();
    bool canAlign() const;
    void cancel();

    const Base::Placement & getTransform() const
    { return myTransform; }
    void alignObject(App::DocumentObject*);

    // Observer stuff
    /// Checks if the given object is about to be removed
    void slotDeletedDocument(const Gui::Document& Doc);
    /// Checks if the given document is about to be closed
    void slotDeletedObject(const Gui::ViewProvider& Obj);

protected:
    bool computeAlignment(const std::vector<PickedPoint>& movPts, const std::vector<PickedPoint>& fixPts);
    void continueAlignment();
    void showInstructions();
    /** @name Probe picking */
    //@{
    static void probePickedCallback(void * ud, SoEventCallback * n);
    bool applyPickedProbe(Gui::ViewProviderDocumentObject*, const SoPickedPoint* pnt);
    //@}

protected Q_SLOTS:
    void reset();
    void onAlign();
    void onRemoveLastPointMoveable();
    void onRemoveLastPointFixed();
    void onClear();
    void onCancel();

Q_SIGNALS:
    void emitCanceled();
    void emitFinished();

private:
    SoNode* pickedPointsSubGraph(const SbVec3f& p, const SbVec3f& n, int id);
    void closeViewer();

    static ManualAlignment* _instance;

    using Connection = boost::signals2::connection;
    Connection connectApplicationDeletedDocument;
    Connection connectDocumentDeletedObject;

    FixedGroup myFixedGroup;
    MovableGroupModel myAlignModel;
    QPointer<Gui::AlignmentView> myViewer;
    Gui::Document* myDocument;
    int myPickPoints;
    Base::Placement myTransform;

    class Private;
    Private* d;
};

} // namespace Gui


#endif // GUI_MANUALALIGNMENT_H

