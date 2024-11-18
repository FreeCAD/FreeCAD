/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#ifndef SKETCHERGUI_InformationOverlayCoinConverter_H
#define SKETCHERGUI_InformationOverlayCoinConverter_H

#include <vector>

#include "ViewProviderSketch.h"


namespace Base
{
template<typename T>
class Vector3;

class Vector2d;
}  // namespace Base

namespace Part
{
class Geometry;
}

namespace SketcherGui
{
class ViewProviderSketch;
struct OverlayParameters;
struct DrawingParameters;

/** @brief      Class for creating the Overlay information layer
 *  @details
 *
 * Responsibility:
 * To create and update the SoGroup provided as a constructor parameter,
 * taking into account the drawing and overlay parameters provided as
 * constructor parameters.
 *
 * Interface:
 * A single entry point convert(), performing the following flow:
 *
 * [Geometry] => Calculate => addUpdateNode
 *
 * Calculate is responsible for generating information directly usable by Coin (but with standard
 * types that would enable portability) in a predetermined internal Node structure format (e.g.
 * StringNode, PolygonNode) that can generically be used by the addUpdateNode.
 *
 * addUpdateNode is responsible for creating or updating the node structure (depending on
 * overlayParameters.rebuildInformationLayer)
 *
 * Supported:
 * Currently it only supports information of Part::Geometry objects and implements calculations only
 * for GeomBSplineCurve and GeomArc.
 *
 * Caveats:
 * - This class relies on the order of creation to perform the update. Any parallel execution that
 * does not deterministically maintain the order will result in undefined behaviour. This provides a
 * reasonable tradeoff between complexity and the fact that currently the information layer is
 * generally so small that no parallel execution would actually result in a performance gain.
 *
 */
class EditModeInformationOverlayCoinConverter
{
private:
    enum class CalculationType
    {
        BSplineDegree,
        BSplineControlPolygon,
        BSplineCurvatureComb,
        BSplineKnotMultiplicity,
        BSplinePoleWeight,
        ArcCircleHelper
    };
    enum class VisualisationType
    {
        Text,
        Polygon
    };

    using Vector3d = Base::Vector3<double>;

private:
    // A Coin Node follows a VisualisationType, which defines how
    // the information is represented, and which member must be
    // filled into the struct for representation
    //
    // A struct containing the information to represent VisualisationType
    // must be provided per CalculationType
    //
    // a struct Node template enables to define the VisualisationType and
    // the CalculationType so that uniform treatment can be provided
    template<VisualisationType vtype, CalculationType ctype>
    struct Node
    {
        static constexpr VisualisationType visualisationType = vtype;
        static constexpr CalculationType calculationType = ctype;
    };

    template<CalculationType ctype>
    struct NodeText: public Node<VisualisationType::Text, ctype>
    {
        std::vector<std::string> strings;
        std::vector<Vector3d> positions;
    };

    template<CalculationType ctype>
    struct NodePolygon: public Node<VisualisationType::Polygon, ctype>
    {
        std::vector<Vector3d> coordinates;
        std::vector<int> indices;
    };

private:
    // Node Position in the Coin Scenograph for the different types of nodes
    enum class TextNodePosition
    {
        TextCoordinates = 0,
        TextInformation = 3
    };

    enum class PolygonNodePosition
    {
        PolygonCoordinates = 1,
        PolygonLineSet = 2
    };

public:
    /** Constructs an InformationOverlayCoinConverter responsible for
     * generating (calculating) the information of a full geometry layer
     * overlay using the overlay and drawing parameters
     *
     * @param infogroup: The SoGroup to be populated with the coin nodes
     * generated from the calculated information.
     *
     * @param overlayparameters: Parameters for controlling the overlay
     * @param drawingparameters: Parameters for drawing the overlay information
     */
    EditModeInformationOverlayCoinConverter(ViewProviderSketch& vp,
                                            SoGroup* infogroup,
                                            OverlayParameters& overlayparameters,
                                            DrawingParameters& drawingparameters);

    /**
     * extracts information from the geometry and converts it into an information overlay in the
     * SoGroup provided in the constructor.
     *
     * @param geometry: the geometry to be processed
     */
    void convert(const Part::Geometry* geometry, int geoid);

private:
    template<CalculationType calculation>
    void calculate(const Part::Geometry* geometry, [[maybe_unused]] int geoid);

    template<typename Result>
    void addUpdateNode(const Result& result);

    template<CalculationType calculation>
    bool isVisible();

    template<typename Result>
    void setPolygon(const Result& result, SoLineSet* polygonlineset, SoCoordinate3* polygoncoords);

    template<int line = 1>
    void setText(const std::string& string, SoText2* text);

    void addToInfoGroup(SoSwitch* sw);

    template<typename Result>
    void clearCalculation(Result& result);

    template<typename Result>
    void addNode(const Result& result);

    template<typename Result>
    void updateNode(const Result& result);

private:
    /// Reference to ViewProviderSketch in order to access the public and the Attorney Interface
    ViewProviderSketch& viewProvider;

    SoGroup* infoGroup;
    OverlayParameters& overlayParameters;
    DrawingParameters& drawingParameters;

    // Calculations
    NodeText<CalculationType::BSplineDegree> degree;
    NodeText<CalculationType::BSplineKnotMultiplicity> knotMultiplicity;
    NodeText<CalculationType::BSplinePoleWeight> poleWeights;
    NodePolygon<CalculationType::BSplineControlPolygon> controlPolygon;
    NodePolygon<CalculationType::BSplineCurvatureComb> curvatureComb;
    NodePolygon<CalculationType::ArcCircleHelper> circleHelper;

    // Node Management
    int nodeId;
};


}  // namespace SketcherGui


#endif  // SKETCHERGUI_InformationOverlayCoinConverter_H
