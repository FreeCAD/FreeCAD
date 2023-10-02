/***************************************************************************
 *   Copyright (c) 2023 Pierre-Louis Boyer <pierrelouis.boyer@gmail.com>   *
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
 *   SnapManager initially funded by the Open Toolchain Foundation         *
 ***************************************************************************/

#ifndef SKETCHERGUI_SnapManager_H
#define SKETCHERGUI_SnapManager_H


#include <App/Application.h>


namespace SketcherGui
{

class ViewProviderSketch;


class ViewProviderSketchSnapAttorney
{
private:
    static inline int getPreselectPoint(const ViewProviderSketch& vp);
    static inline int getPreselectCross(const ViewProviderSketch& vp);
    static inline int getPreselectCurve(const ViewProviderSketch& vp);

    friend class SnapManager;
};

/* This class is used to manage the overriding of mouse pointer coordinates in Sketcher
 *  (in Edit-Mode) depending on the situation. Those situations are in priority order :
 *  1 - Snap at angle: For tools like Slot, Arc, Line, Ellipse, this enables to constrain the angle
 * at steps of 5° (or customized angle). This is useful to make features at a certain angle (45° for
 * example) 2 - Snap to object: This snaps the mouse pointer onto objects. 3 - Snap to grid: This
 * snaps the mouse pointer on the grid.
 */
class SnapManager
{

    /** @brief      Class for monitoring changes in parameters affecting Snapping
     *  @details
     *
     * This nested class is a helper responsible for attaching to the parameters relevant for
     * SnapManager, initialising the SnapManager to the current configuration
     * and handle in real time any change to their values.
     */
    class ParameterObserver: public ParameterGrp::ObserverType
    {
    public:
        explicit ParameterObserver(SnapManager& client);
        ~ParameterObserver() override;

        void subscribeToParameters();

        void unsubscribeToParameters();

        /** Observer for parameter group. */
        void OnChange(Base::Subject<const char*>& rCaller, const char* sReason) override;

    private:
        void initParameters();
        void updateSnapParameter(const std::string& parametername);
        void updateSnapToObjectParameter(const std::string& parametername);
        void updateSnapToGridParameter(const std::string& parametername);
        void updateSnapAngleParameter(const std::string& parametername);

        static ParameterGrp::handle getParameterGrpHandle();

    private:
        std::map<std::string, std::function<void(const std::string&)>> str2updatefunction;
        SnapManager& client;
    };

public:
    explicit SnapManager(ViewProviderSketch& vp);
    ~SnapManager();

    bool snap(double& x, double& y);
    bool snapAtAngle(double& x, double& y);
    bool snapToObject(double& x, double& y);
    bool snapToGrid(double& x, double& y);

    bool snapToLineMiddle(Base::Vector3d& pointToOverride, const Part::GeomLineSegment* line);
    bool snapToArcMiddle(Base::Vector3d& pointToOverride, const Part::GeomArcOfCircle* arc);

    void setAngleSnapping(bool enable, Base::Vector2d referencepoint);

private:
    /// Reference to ViewProviderSketch in order to access the public and the Attorney Interface
    ViewProviderSketch& viewProvider;

    bool angleSnapRequested;
    bool snapRequested;
    bool snapToObjectsRequested;
    bool snapToGridRequested;

    Base::Vector2d referencePoint;
    double lastMouseAngle;

    double snapAngle;

    /// Observer to track all the needed parameters.
    std::unique_ptr<SnapManager::ParameterObserver> pObserver;
};


}  // namespace SketcherGui


#endif  // SKETCHERGUI_SnapManager_H
