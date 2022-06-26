/***************************************************************************
 *   Copyright (c) 2022 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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


#ifndef SKETCHERGUI_DrawSketchDefaultHandler_H
#define SKETCHERGUI_DrawSketchDefaultHandler_H

#include <Inventor/events/SoKeyboardEvent.h>

#include <Base/Exception.h>

#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/PythonConverter.h>
#include <Mod/Sketcher/App/SolverGeometryExtension.h>

#include "DrawSketchHandler.h"

#include "Utils.h"

namespace bp = boost::placeholders;

namespace SketcherGui {

/*********************** Ancillary classes for DrawSketch Hierarchy *******************************/

namespace StateMachines {

enum class OneSeekEnd {
    SeekFirst,
    End // MUST be the last one
};

enum class TwoSeekEnd {
    SeekFirst,
    SeekSecond,
    End // MUST be the last one
};

enum class ThreeSeekEnd {
    SeekFirst,
    SeekSecond,
    SeekThird,
    End // MUST be the last one
};

enum class FourSeekEnd {
    SeekFirst,
    SeekSecond,
    SeekThird,
    SeekFourth,
    End // MUST be the last one
};

enum class TwoSeekDoEnd {
    SeekFirst,
    SeekSecond,
    Do,
    End // MUST be the last one
};

} // namespace StateMachines

/** @brief A state machine to encapsulate a state
 *
 * @details
 *
 * A template class for a state machine defined by template type SelectModeT,
 * automatically initialised to the first state, encapsulating the actual state,
 * and enabling to change the state, while generating a call to onModeChanged()
 * after every change.
 *
 * the getNextMode() returns the next mode in the state machine, unless it is in
 * End mode, in which End mode is returned.
 *
 * NOTE: The machine provided MUST include a last state named End.
 */
template <typename SelectModeT>
class StateMachine
{
public:
    StateMachine():Mode(static_cast<SelectModeT>(0)) {}
    virtual ~StateMachine(){}

protected:
    void setState(SelectModeT mode) {
        Mode = mode;
        onModeChanged();
    }

    void ensureState(SelectModeT mode) {
        if(Mode != mode) {
            Mode = mode;
            onModeChanged();
        }
    }

    /** Ensure the state machine is the provided mode
     * but only if the mode is an earlier state.
     *
     * This allows to return to previous states (e.g.
     * for modification), only if that state has previously
     * been completed.
     */
    void ensureStateIfEarlier(SelectModeT mode) {
        if(Mode != mode) {
            if(mode < Mode) {
                Mode = mode;
                onModeChanged();
            }
        }
    }

    SelectModeT state() const {
        return Mode;
    }

    bool isState(SelectModeT state) const { return Mode == state;}

    bool isFirstState() const { return Mode == (static_cast<SelectModeT>(0));}

    bool isLastState() const { return Mode == SelectModeT::End;}

    constexpr SelectModeT getFirstState() const {
        return static_cast<SelectModeT>(0);
    }

    SelectModeT getNextMode() const {
        auto modeint = static_cast<int>(state());


        if(modeint < maxMode) {
            auto newmode = static_cast<SelectModeT>(modeint+1);
            return newmode;
        }
        else {
            return SelectModeT::End;
        }
    }

    void moveToNextMode() {
        setState(getNextMode());
    }

    void reset() {
        setState(static_cast<SelectModeT>(0));
    }

    virtual void onModeChanged() {};

private:
    SelectModeT Mode;
    static const constexpr int maxMode = static_cast<int>(SelectModeT::End);

};

namespace ConstructionMethods {

enum class DefaultConstructionMethod {
    End // Must be the last one
};

} // namespace ConstructionMethods

template <typename ConstructionMethodT>
class ConstructionMethodMachine
{
public:
    ConstructionMethodMachine(ConstructionMethodT constructionmethod = static_cast<ConstructionMethodT>(0)):ConstructionMode(constructionmethod) {}
    virtual ~ConstructionMethodMachine(){}

protected:
    void setConstructionMethod(ConstructionMethodT mode) {
        ConstructionMode = mode;
        onConstructionMethodChanged();
    }

    ConstructionMethodT constructionMethod() const {
        return ConstructionMode;
    }

    bool isConstructionMethod(ConstructionMethodT state) const { return ConstructionMode == state;}

    void resetConstructionMode() {
        ConstructionMode = static_cast<ConstructionMethodT>(0);
    }

    void initConstructionMethod(ConstructionMethodT mode) {
        ConstructionMode = mode;
    }

    // Cyclically iterate construction methods
    ConstructionMethodT getNextMethod() const {
        auto modeint = static_cast<int>(ConstructionMode);


        if(modeint < (maxMode-1)) {
            auto newmode = static_cast<ConstructionMethodT>(modeint+1);
            return newmode;
        }
        else {
            return static_cast<ConstructionMethodT>(0);
        }
    }

    void iterateToNextConstructionMethod() {
        if(ConstructionMethodsCount() > 1)
            setConstructionMethod(getNextMethod());
    }

    virtual void onConstructionMethodChanged() {};

    static constexpr int ConstructionMethodsCount() {return maxMode;}

private:
    ConstructionMethodT ConstructionMode;
    static const constexpr int maxMode = static_cast<int>(ConstructionMethodT::End);
};

/** @brief A state machine DrawSketchHandler for geometry.
 *
 * @details
 * A state machine DrawSketchHandler defining a EditCurve and AutoConstraints, providing:
 * - generic initialisation including setting the cursor
 * - structured command finalisation
 * - handling of continuous creation mode
 *
 * Two ways of using it:
 * 1. By instanting and specialising functions.
 * 2. By creating a new class deriving from this one
 *
 * You need way 2 if you must add additional data members to your class.
 *
 * This class provides an NVI interface for extension. Alternatively, specialisation of those functions
 * may be effected if it is opted to instantiate and specialise.
 *
 * Template Types/Parameters:
 * PTool : Parameter to specialise behaviour to a specific tool
 * SelectModeT : The type of statemachine to be used (see namespace StateMachines above).
 * PInitEditCurveSize : Initial size of the EditCurve vector
 * PInitAutoConstraintSize : Initial size of the AutoConstraint vector
 *
 * Question 1: Do I need to use this handler or derive from this handler to make a new hander?
 *
 * No, you do not NEED to. But you are encouraged to. Structuring a handler following this NVI, apart
 * from savings in amount of code typed, enables a much easier and less verbose implementation of a handler
 * using a default widget (toolwidget).
 *
 * For handlers using a custom widget it will also help by structuring the code in a way consistent with other handlers.
 * It will result in an easier to maintain code.
 *
 * Question 2: I want to use the default widget, do I need to use this handler or derive from this handler?
 *
 * You should use DrawSketchDefaultWidgetHandler instead. However, both clases use the same interface, so if you derive from
 * this class when implementing your handler and then decide to use the tool widget, all you have to do is to change
 * the base class from DrawSketchDefaultHandler to DrawSketchDefaultWidgetHandler. Then you will have to implement the code that
 * is exclusively necessary for the default widget to work.
 */
template < typename HandlerT,           // The geometry tool for which the template is created (See GeometryTools above)
           typename SelectModeT,        // The state machine defining the states that the handle iterates
           int PInitEditCurveSize,      // The initial size of the EditCurve
           int PInitAutoConstraintSize, // The initial size of the AutoConstraint>
           typename ConstructionMethodT = ConstructionMethods::DefaultConstructionMethod >
class DrawSketchDefaultHandler: public DrawSketchHandler, public StateMachine<SelectModeT>, public ConstructionMethodMachine<ConstructionMethodT>
{
public:
    DrawSketchDefaultHandler(ConstructionMethodT constructionmethod = static_cast<ConstructionMethodT>(0)):
        ConstructionMethodMachine<ConstructionMethodT>(constructionmethod)
        ,initialEditCurveSize(PInitEditCurveSize)
        , EditCurve(PInitEditCurveSize)
        ,sugConstraints(PInitAutoConstraintSize)
    {
        applyCursor();
    }

    virtual ~DrawSketchDefaultHandler() {}

    /** @name public DrawSketchHandler interface
     * NOTE: Not intended to be specialised. It calls some functions intended to be
     * overridden/specialised instead.
     */
    //@{
    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        updateDataAndDrawToPosition(onSketchPos);
    }

    virtual bool pressButton(Base::Vector2d onSketchPos) override
    {

        onButtonPressed(onSketchPos);
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos) override {
        Q_UNUSED(onSketchPos);
        finish();
        return true;
    }

    virtual void registerPressedKey(bool pressed, int key) override {
        if (key == SoKeyboardEvent::M && pressed && !this->isLastState())
            this->iterateToNextConstructionMethod();
    }
    //@}


protected:
    using SelectMode = SelectModeT;
    using ModeStateMachine = StateMachine<SelectModeT>;
    using ConstructionMethod = ConstructionMethodT;
    using ConstructionMachine = ConstructionMethodMachine<ConstructionMethodT>;

    /** @name functions NOT intended for specialisation or to be hidden
        These functions define a predefined structure and are extendable using NVI.
        1. Call them from your handle
        2. Do not hide them or otherwise redefine them UNLESS YOU HAVE A REALLY GOOD REASON TO
        3. EXTEND their functionality, if needed, using the NVI interface (or if you do not need to derive, by specialising these functions).*/
    //@{
    /** @brief This function finalises the creation operation. It works only if the state machine is in state End.
    *
    * @details
    * The functionality need to be provided by extending these virtual private functions:
    * 1. executeCommands() : Must be provided with the Commands to create the geometry
    * 2. generateAutoConstraints() : When using AutoConstraints vector, this function populates the AutoConstraints vector, ensuring no redundant autoconstraints
    * 2. beforeCreateAutoConstraints() : Enables derived clases to define specific actions before executeCommands and createAutoConstraints (optional).
    * 3. createAutoConstraints() : Must be provided with the commands to create autoconstraints
    *
    * It recomputes if not solves and handles continuous mode automatically
    */
    void finish() {

        if(this->isState(SelectMode::End)) {
            unsetCursor();
            resetPositionText();

            try {
                executeCommands();

                generateAutoConstraints();

                beforeCreateAutoConstraints();

                createAutoConstraints();

                tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

            }
            catch (const Base::RuntimeError& e) {
                e.ReportException();
            }

            handleContinuousMode();
        }
    }

    /** @brief This function resets the handler to the initial state.
    *
    * @details
    * The functionality can be extended using these virtual private function:
    * 1. onReset() : Any further initialisation applicable to your handler
    *
    * It clears the edit curve, resets the state machine and resizes edit curve and autoconstraints
    * to initial size. Reapplies the cursor bitmap.
    */
    void reset() {
        EditCurve.clear();
        drawEdit(EditCurve);

        ModeStateMachine::reset();
        EditCurve.resize(initialEditCurveSize);
        for(auto & ac : sugConstraints)
            ac.clear();

        AutoConstraints.clear();
        ShapeGeometry.clear();
        ShapeConstraints.clear();

        onReset();
        applyCursor();
    }

    /** @brief This function handles the geometry continuous mode.
    *
    * @details
    * The functionality can be extended using the virtual private function called from reset(), namely:
    * 1. onReset() : Any further initialisation applicable to your handler
    *
    * It performs all the operations in reset().
    */
    void handleContinuousMode() {

        if(continuousMode){
            // This code enables the continuous creation mode.
            reset();
            // It is ok not to call to purgeHandler in continuous creation mode because the
            // handler is destroyed by the quit() method on pressing the right button of the mouse
        }
        else{
            sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider
        }
    }
    //@}

private:
    /** @name functions are intended to be overridden/specialised to extend basic functionality
        NVI interface. See documentation of the functions above.*/
    //@{
    virtual void onReset() { }
    virtual void executeCommands() {}
    virtual void generateAutoConstraints() {}
    virtual void beforeCreateAutoConstraints() {}
    virtual void createAutoConstraints() {}

    virtual void onConstructionMethodChanged() override {};

    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) {Q_UNUSED(onSketchPos)};

    // function intended to populate ShapeGeometry and ShapeConstraints
    virtual void createShape(bool onlyeditoutline) {Q_UNUSED(onlyeditoutline)}
    //@}
protected:
    /** @name functions are intended to be overridden/specialised to extend basic functionality
        See documentation of the functions above*/
    //@{
    /// Handles avoid redundants and continuous mode, if overridden the base class must be called!
    virtual void activated() override {
        avoidRedundants = sketchgui->AvoidRedundant.getValue()  && sketchgui->Autoconstraints.getValue();

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");

        continuousMode = hGrp->GetBool("ContinuousCreationMode",true);
    }

    // Default implementation is that on every mouse click it redraws and the mode is changed to the next seek
    // On the last seek, it changes to SelectMode::End
    // If this behaviour is not acceptable, then the function must be specialised (or overloaded).
    virtual void onButtonPressed(Base::Vector2d onSketchPos) {
        this->updateDataAndDrawToPosition(onSketchPos);
        this->moveToNextMode();
    }

    virtual void onModeChanged() override {
        finish(); // internally checks that state is SelectMode::End, and only finishes then.
    };
    //@}

    /** @name Helper functions
        See documentation of the functions above*/
    //@{
    void generateAutoConstraintsOnElement(const std::vector<AutoConstraint> &autoConstrs,
                                          int geoId1, Sketcher::PointPos posId1)
    {
        if (!sketchgui->Autoconstraints.getValue())
            return;

        if (autoConstrs.size() > 0) {
            for (auto &ac : autoConstrs) {
                int geoId2 = ac.GeoId;

                switch (ac.Type)
                {
                case Sketcher::Coincident: {
                    if (posId1 == Sketcher::PointPos::none)
                        continue;

                    // find if there is already a matching tangency
                    auto result = std::find_if(AutoConstraints.begin(),AutoConstraints.end(),[&](const auto & ace){
                         return ace->Type == Sketcher::Tangent &&
                                ace->First == geoId1 &&
                                ace->Second == ac.GeoId;
                    });


                    if(result != AutoConstraints.end()) { // modify tangency to endpoint-to-endpoint
                        (*result)->FirstPos = posId1;
                        (*result)->SecondPos = ac.PosId;
                    }
                    else {
                        auto c = std::make_unique<Sketcher::Constraint>();
                        c->Type = Sketcher::Coincident;
                        c->First = geoId1; c->FirstPos = posId1;
                        c->Second = ac.GeoId; c->SecondPos = ac.PosId;
                        AutoConstraints.push_back(std::move(c));
                    }

                    } break;
                case Sketcher::PointOnObject: {
                    Sketcher::PointPos posId2 = ac.PosId;
                    if (posId1 == Sketcher::PointPos::none) {
                        // Auto constraining an edge so swap parameters
                        std::swap(geoId1,geoId2);
                        std::swap(posId1,posId2);
                    }

                    auto result = std::find_if(AutoConstraints.begin(),AutoConstraints.end(),[&](const auto & ace){
                         return ace->Type == Sketcher::Tangent &&
                                ace->First == geoId1 &&
                                ace->Second == ac.GeoId;
                    });

                    // if tangency, convert to point-to-edge tangency
                    if(result != AutoConstraints.end()) {
                        (*result)->FirstPos = posId1;

                        if( (*result)->First != geoId1 ) {
                            std::swap((*result)->Second, (*result)->First);
                        }
                    }
                    else {
                        auto c = std::make_unique<Sketcher::Constraint>();
                        c->Type = Sketcher::PointOnObject;
                        c->First = geoId1; c->FirstPos = posId1;
                        c->Second = geoId2;
                        AutoConstraints.push_back(std::move(c));
                    }
                    } break;
                // In special case of Horizontal/Vertical constraint, geoId2 is normally unused and should be 'Constraint::GeoUndef'
                // However it can be used as a way to require the function to apply these constraints on another geometry
                // In this case the caller as to set geoId2, then it will be used as target instead of geoId2
                case Sketcher::Horizontal: {
                    auto c = std::make_unique<Sketcher::Constraint>();
                    c->Type = Sketcher::Horizontal;
                    c->First = (geoId2 != Sketcher::GeoEnum::GeoUndef ? geoId2 : geoId1);
                    AutoConstraints.push_back(std::move(c));
                    } break;
                case Sketcher::Vertical: {
                    auto c = std::make_unique<Sketcher::Constraint>();
                    c->Type = Sketcher::Vertical;
                    c->First = (geoId2 != Sketcher::GeoEnum::GeoUndef ? geoId2 : geoId1);
                    AutoConstraints.push_back(std::move(c));
                    } break;
                case Sketcher::Tangent: {
                    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(sketchgui->getObject());

                    const Part::Geometry *geom1 = Obj->getGeometry(geoId1);
                    const Part::Geometry *geom2 = Obj->getGeometry(ac.GeoId);

                    // ellipse tangency support using construction elements (lines)
                    if( geom1 && geom2 &&
                        ( geom1->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                        geom2->getTypeId() == Part::GeomEllipse::getClassTypeId() )){

                        if(geom1->getTypeId() != Part::GeomEllipse::getClassTypeId())
                            std::swap(geoId1,geoId2);

                        // geoId1 is the ellipse
                        geom1 = Obj->getGeometry(geoId1);
                        geom2 = Obj->getGeometry(geoId2);

                        if( geom2->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                            geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                            geom2->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                            geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ) {
                            // in all these cases an intermediate element is needed
                            /*makeTangentToEllipseviaNewPoint(Obj,
                                                            static_cast<const Part::GeomEllipse *>(geom1),
                                                            geom2, geoId1, geoId2);*/
                            // NOTE: Temporarily deactivated
                            return;
                        }
                    }

                    // arc of ellipse tangency support using external elements
                    if( geom1 && geom2 &&
                        ( geom1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                        geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() )){

                        if(geom1->getTypeId() != Part::GeomArcOfEllipse::getClassTypeId())
                            std::swap(geoId1,geoId2);

                        // geoId1 is the arc of ellipse
                        geom1 = Obj->getGeometry(geoId1);
                        geom2 = Obj->getGeometry(geoId2);

                        if( geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                            geom2->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                            geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ) {
                            // in all these cases an intermediate element is needed
                            //makeTangentToArcOfEllipseviaNewPoint(Obj,
                            //                                    static_cast<const Part::GeomArcOfEllipse *>(geom1), geom2, geoId1, geoId2);
                            // NOTE: Temporarily deactivated
                            return;
                        }
                    }

                    auto resultcoincident = std::find_if(AutoConstraints.begin(),AutoConstraints.end(), [&](const auto & ace){
                         return ace->Type == Sketcher::Coincident &&
                                ace->First == geoId1 &&
                                ace->Second == ac.GeoId;
                    });

                    auto resultpointonobject = std::find_if(AutoConstraints.begin(),AutoConstraints.end(), [&](const auto & ace){
                         return ace->Type == Sketcher::PointOnObject &&
                                ((ace->First == geoId1 && ace->Second == ac.GeoId) ||
                                 (ace->First == ac.GeoId && ace->Second == geoId1));
                    });

                    if(resultcoincident != AutoConstraints.end()) { // endpoint-to-endpoint tangency
                        (*resultcoincident)->Type = Sketcher::Tangent;
                    }
                    else if(resultpointonobject != AutoConstraints.end()) { // endpoint-to-edge tangency
                        (*resultpointonobject)->Type = Sketcher::Tangent;
                    }
                    else { // regular edge to edge tangency
                        auto c = std::make_unique<Sketcher::Constraint>();
                        c->Type = Sketcher::Tangent;
                        c->First = geoId1;
                        c->Second = ac.GeoId;
                        AutoConstraints.push_back(std::move(c));
                    }
                    } break;
                default:
                    break;
                }
            }
        }
    }

    void createGeneratedAutoConstraints(bool owncommand)
    {
        // add auto-constraints
        if(owncommand)
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add auto constraints"));

        auto autoConstraints = toPointerVector(AutoConstraints);

        Gui::Command::doCommand(Gui::Command::Doc,
                            Sketcher::PythonConverter::convert(Gui::Command::getObjectCmd(sketchgui->getObject()), autoConstraints).c_str());

        if(owncommand)
            Gui::Command::commitCommand();
    }

    void removeRedundantAutoConstraints() {

        if(AutoConstraints.empty())
            return;

        auto sketchobject = getSketchObject();

        auto autoConstraints = toPointerVector(AutoConstraints);

        sketchobject->diagnoseAdditionalConstraints(autoConstraints);

        if(sketchobject->getLastHasRedundancies()) {
            Base::Console().Warning("Autoconstraints cause redundant: removing them\n");

            auto lastsketchconstraintindex = sketchobject->Constraints.getSize() - 1;

            auto redundants = sketchobject->getLastRedundant(); // redundants is always sorted

            for(int index = redundants.size()-1; index >= 0; index--) {
                int redundantconstraintindex = redundants[index] - 1;
                if(redundantconstraintindex > lastsketchconstraintindex) {
                    int removeindex = redundantconstraintindex - lastsketchconstraintindex - 1;
                    AutoConstraints.erase(std::next(AutoConstraints.begin(), removeindex));
                }
                else {
                    // This exception stops the procedure here, which means that:
                    // 1) Geometry (and constraints of the geometry in case of a multicurve shape) are created
                    // 2) No autoconstrains are actually added
                    // 3) No widget mandated constraints are added
                    THROWM(Base::RuntimeError,"Redundant is not an Autoconstraint - Please report!\n");
                }
            }

            // NOTE: If we removed all redundants in the list, then at this moment there are no redundants anymore
        }

        if(sketchobject->getLastHasConflicts())
            THROWM(Base::RuntimeError,"Autoconstraints cause conflicting constraints - Please report!\n");
    }

    /** This function is to be called when redundancies or conflicting constraints should not appear, and updated
     * solver information is required (for example, after addition of new constraints).
     * It does not modify the SketchObject properties.
     * It will throw if redundant or conflicting constraints arise*/
    void diagnoseWithAutoConstraints() {
        auto sketchobject = getSketchObject();

        auto autoConstraints = toPointerVector(AutoConstraints);

        sketchobject->diagnoseAdditionalConstraints(autoConstraints);

        if(sketchobject->getLastHasRedundancies() || sketchobject->getLastHasConflicts())
            THROWM(Base::RuntimeError,"Unexpected Redundancy/Conflicting constraint\n");
    }

    Sketcher::SolverGeometryExtension::PointParameterStatus getPointInfo(const Sketcher::GeoElementId & element) {

        auto sketchobject = getSketchObject();
        // it is important not to rely on Geometry attached extension from the solver, as geometry is not updated during
        // temporary diagnose of additional constraints
        const auto & solvedsketch = sketchobject->getSolvedSketch();

        auto solvext = solvedsketch.getSolverExtension(element.GeoId);

        if(solvext) {
            auto pointinfo = solvext->getPoint(element.Pos);

            return pointinfo;
        }

        THROWM(Base::ValueError, "Geometry does not have solver extension when trying to apply widget constraints!")
    }

    int getLineDoFs(int geoid)
    {
        auto startpointinfo = getPointInfo(Sketcher::GeoElementId(geoid, Sketcher::PointPos::start));
        auto endpointinfo = getPointInfo(Sketcher::GeoElementId(geoid, Sketcher::PointPos::end));

        int DoFs = startpointinfo.getDoFs();
        DoFs += endpointinfo.getDoFs();

        return DoFs;
    }

    Sketcher::SolverGeometryExtension::EdgeParameterStatus getEdgeInfo(int geoid) {

        auto sketchobject = getSketchObject();
        // it is important not to rely on Geometry attached extension from the solver, as geometry is not updated during
        // temporary diagnose of additional constraints
        const auto & solvedsketch = sketchobject->getSolvedSketch();

        auto solvext = solvedsketch.getSolverExtension(geoid);

        if(solvext) {
            Sketcher::SolverGeometryExtension::EdgeParameterStatus edgeinfo = solvext->getEdgeParameters();

            return edgeinfo;
        }

        THROWM(Base::ValueError, "Geometry does not have solver extension when trying to apply widget constraints!")
    }

    void addToShapeConstraints(Sketcher::ConstraintType type, int first, Sketcher::PointPos firstPos = Sketcher::PointPos::none, int second = -2000, Sketcher::PointPos secondPos = Sketcher::PointPos::none, int third = -2000, Sketcher::PointPos thirdPos = Sketcher::PointPos::none) {
        auto constr = std::make_unique<Sketcher::Constraint>();
        constr->Type = type;
        constr->First = first;
        constr->FirstPos = firstPos;
        constr->Second = second;
        constr->SecondPos = secondPos;
        constr->Third = third;
        constr->ThirdPos = thirdPos;
        ShapeConstraints.push_back(std::move(constr));
    }

    void addLineToShapeGeometry(Base::Vector3d p1, Base::Vector3d p2, bool constructionMode) {
        auto line = std::make_unique<Part::GeomLineSegment>();
        line->setPoints(p1, p2);
        Sketcher::GeometryFacade::setConstruction(line.get(), constructionMode);
        ShapeGeometry.push_back(std::move(line));
    }

    void addArcToShapeGeometry(Base::Vector3d p1, double start, double end, double radius, bool constructionMode) {
        auto arc = std::make_unique<Part::GeomArcOfCircle>();
        arc->setCenter(p1);
        arc->setRange(start, end, true);
        arc->setRadius(radius);
        Sketcher::GeometryFacade::setConstruction(arc.get(), constructionMode);
        ShapeGeometry.push_back(std::move(arc));
    }

    void addPointToShapeGeometry(Base::Vector3d p1, bool constructionMode) {
        auto point = std::make_unique<Part::GeomPoint>();
        point->setPoint(p1);
        Sketcher::GeometryFacade::setConstruction(point.get(), constructionMode);
        ShapeGeometry.push_back(std::move(point));
    }

    void addEllipseToShapeGeometry(Base::Vector3d centerPoint, Base::Vector3d majorAxisDirection, double majorRadius, double minorRadius, bool constructionMode) {
        auto ellipse = std::make_unique<Part::GeomEllipse>();
        ellipse->setMajorRadius(majorRadius);
        ellipse->setMinorRadius(minorRadius);
        ellipse->setMajorAxisDir(majorAxisDirection);
        ellipse->setCenter(centerPoint);
        Sketcher::GeometryFacade::setConstruction(ellipse.get(), constructionMode);
        ShapeGeometry.push_back(std::move(ellipse));
    }

    void addCircleToShapeGeometry(Base::Vector3d centerPoint, double radius, bool constructionMode) {
        auto circle = std::make_unique<Part::GeomCircle>();
        circle->setRadius(radius);
        circle->setCenter(centerPoint);
        Sketcher::GeometryFacade::setConstruction(circle.get(), constructionMode);
        ShapeGeometry.push_back(std::move(circle));
    }

    void commandAddShapeGeometryAndConstraints() {
            auto shapeGeometry = toPointerVector(ShapeGeometry);
            Gui::Command::doCommand(Gui::Command::Doc,
                Sketcher::PythonConverter::convert(Gui::Command::getObjectCmd(sketchgui->getObject()), shapeGeometry).c_str());

            auto shapeConstraints = toPointerVector(ShapeConstraints);
            Gui::Command::doCommand(Gui::Command::Doc,
                Sketcher::PythonConverter::convert(Gui::Command::getObjectCmd(sketchgui->getObject()), shapeConstraints).c_str());
    }

    void DrawShapeGeometry() {
        drawEdit(toPointerVector(ShapeGeometry));
    }

    void CreateAndDrawShapeGeometry() {
        createShape(true);
        drawEdit(toPointerVector(ShapeGeometry));
    }

    //@}

protected:
    // The initial size may need to change in some tools due to the configuration of the tool, so resetting may lead to a
    // different number than the compiled time value
    int initialEditCurveSize;

    std::vector<Base::Vector2d> EditCurve;
    std::vector<std::vector<AutoConstraint>> sugConstraints;

    std::vector<std::unique_ptr<Part::Geometry>> ShapeGeometry;
    std::vector<std::unique_ptr<Sketcher::Constraint>> ShapeConstraints;
    std::vector<std::unique_ptr<Sketcher::Constraint>> AutoConstraints;

    bool avoidRedundants;
    bool continuousMode;
};

} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchDefaultHandler_H

