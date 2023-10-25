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

#ifndef SKETCHERGUI_DrawSketchHandlerEllipse_H
#define SKETCHERGUI_DrawSketchHandlerEllipse_H

#include <GC_MakeEllipse.hxx>
#include <QDebug>
#include <cmath>

#include <Gui/Notifications.h>

#include "GeometryCreationMode.h"


namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

/* Ellipse ==============================================================================*/
/**
 * @brief This class handles user interaction to draw and save the ellipse
 *
 * Two construction methods are implemented:
 *   -Periapsis, apoapsis, and b; and
 *   -Center, periapsis, and b.
 *
 * The first method limits the ellipse to a circle, while the second method allows for
 * swapping of the semi-major and semi-minor axes.
 *
 * We use three reference frames in this class.  The first (and primary), is the cartesian
 * frame of the sketcher; all our work begins and ends in this frame.  The second is the
 * perifocal frame of the ellipse using polar coordinates.  We use this frame for naming
 * conventions and working with the ellipse.  The last is a rotated right-handed cartesian
 * frame centered at the ellipse center with the +X direction towards periapsis, +Z out of
 * screen.
 *
 * When working with an ellipse in the perifocal frame, the following equations are useful:
 *
 *    \f{eqnarray*}{
 *        r &\equiv& \textrm{ radial distance from the focus to a point on the ellipse}\\
 *        r_a &\equiv& \textrm{ radial distance from the focus to apopasis}\\
 *        r_p &\equiv& \textrm{ radial distance from the focus to periapsis}\\
 *        a &\equiv& \textrm{ length of the semi-major axis, colloquially 'radius'}\\
 *        b &\equiv& \textrm{ length of the semi-minor axis, colloquially 'radius'}\\
 *        e &\equiv& \textrm{ eccentricity of the ellipse}\\
 *        \theta_b &\equiv& \textrm{ angle to the intersection of the semi-minor axis and the
 * ellipse, relative to the focus}\\
 *        ae &\equiv& \textrm{ distance from the focus to the centroid}\\
 *        r &=& \frac{a(1-e^2)}{1+e\cos(\theta)} = \frac{r_a(1-e)}{1+e\cos(\theta)} =
 * \frac{r_p(1+e)}{1+e\cos(\theta)}\\
 *        r_a &=& a(1-e)\\
 *        r_p &=& a(1+e)\\
 *        a &=& \frac{r_p+r_a}{2}\\
 *        b &=& a\sqrt{1-e^2}\\
 *        e &=& \frac{r_a-r_p}{r_a+r_p} = \sqrt{1-\frac{b^2}{a^2}}\\
 *        \theta_b &=& \left[\pi - \arctan\left(\frac{b}{ae}\right)\right] \pm N\pi
 *   \f}
 *
 */
class DrawSketchHandlerEllipse: public DrawSketchHandler
{
public:
    explicit DrawSketchHandlerEllipse(int constructionMethod)
        : mode(STATUS_Close)
        , method(CENTER_PERIAPSIS_B)
        , constrMethod(constructionMethod)
        , a(0)
        , b(0)
        , e(0)
        , ratio(0)
        , ae(0)
        , num(0)
        , r(0)
        , theta(0)
        , phi(0)
        , editCurve(33)
        , fixedAxisLength(0)
    {}
    ~DrawSketchHandlerEllipse() override
    {}
    /// Mode table, describes what step of the process we are in
    enum SelectMode
    {
        STATUS_SEEK_PERIAPSIS, /**< enum value, looking for click to set periapsis. */
        STATUS_SEEK_APOAPSIS,  /**< enum value, looking for click to set apoapsis. */
        STATUS_SEEK_CENTROID,  /**< enum value, looking for click to set centroid. */
        STATUS_SEEK_A,         /**< enum value, looking for click to set a. */
        STATUS_SEEK_B,         /**< enum value, looking for click to set b. */
        STATUS_Close           /**< enum value, finalizing and saving ellipse. */
    };
    /// Construction methods, describes the method used to construct the ellipse
    enum ConstructionMethod
    {
        CENTER_PERIAPSIS_B,  /**< enum value, click on center, then periapsis, then b point. */
        PERIAPSIS_APOAPSIS_B /**< enum value, click on periapsis, then apoapsis, then b point. */
    };

    /**
     * @brief Updates the ellipse when the cursor moves
     * @param onSketchPos the position of the cursor on the sketch
     */
    void mouseMove(Base::Vector2d onSketchPos) override
    {
        if (method == PERIAPSIS_APOAPSIS_B) {
            if (mode == STATUS_SEEK_PERIAPSIS) {
                setPositionText(onSketchPos);
                if (seekAutoConstraint(sugConstr1,
                                       onSketchPos,
                                       Base::Vector2d(0.f, 0.f),
                                       AutoConstraint::CURVE)) {
                    renderSuggestConstraintsCursor(sugConstr1);
                    return;
                }
            }
            else if (mode == STATUS_SEEK_APOAPSIS) {
                solveEllipse(onSketchPos);
                approximateEllipse();

                // Display radius for user
                float semiMajorRadius = a * 2;
                if (showCursorCoords()) {
                    SbString text;
                    std::string majorString = lengthToDisplayFormat(semiMajorRadius, 1);
                    text.sprintf(" (R%s, R%s)", majorString.c_str(), majorString.c_str());
                    setPositionText(onSketchPos, text);
                }

                drawEdit(editCurve);
                // Suggestions for ellipse and curves are disabled because many tangent constraints
                // need an intermediate point or line.
                if (seekAutoConstraint(sugConstr2,
                                       onSketchPos,
                                       Base::Vector2d(0.f, 0.f),
                                       AutoConstraint::CURVE)) {
                    renderSuggestConstraintsCursor(sugConstr2);
                    return;
                }
            }
            else if (mode == STATUS_SEEK_B) {
                solveEllipse(onSketchPos);
                approximateEllipse();

                // Display radius for user
                if (showCursorCoords()) {
                    SbString text;
                    std::string aString = lengthToDisplayFormat(a, 1);
                    std::string bString = lengthToDisplayFormat(b, 1);
                    text.sprintf(" (R%s, R%s)", aString.c_str(), bString.c_str());
                    setPositionText(onSketchPos, text);
                }

                drawEdit(editCurve);
                if (seekAutoConstraint(sugConstr3,
                                       onSketchPos,
                                       Base::Vector2d(0.f, 0.f),
                                       AutoConstraint::CURVE)) {
                    renderSuggestConstraintsCursor(sugConstr3);
                    return;
                }
            }
        }
        else {  // method is CENTER_PERIAPSIS_B
            if (mode == STATUS_SEEK_CENTROID) {
                setPositionText(onSketchPos);
                if (seekAutoConstraint(sugConstr1,
                                       onSketchPos,
                                       Base::Vector2d(0.f, 0.f))) {  // TODO: ellipse prio 1
                    renderSuggestConstraintsCursor(sugConstr1);
                    return;
                }
            }
            else if (mode == STATUS_SEEK_PERIAPSIS) {
                solveEllipse(onSketchPos);
                approximateEllipse();

                // Display radius for user
                float semiMajorRadius = a * 2;
                if (showCursorCoords()) {
                    SbString text;
                    std::string majorString = lengthToDisplayFormat(semiMajorRadius, 1);
                    text.sprintf(" (R%s, R%s)", majorString.c_str(), majorString.c_str());
                    setPositionText(onSketchPos, text);
                }

                drawEdit(editCurve);
                if (seekAutoConstraint(sugConstr2,
                                       onSketchPos,
                                       onSketchPos - centroid,
                                       AutoConstraint::CURVE)) {
                    renderSuggestConstraintsCursor(sugConstr2);
                    return;
                }
            }
            else if ((mode == STATUS_SEEK_A) || (mode == STATUS_SEEK_B)) {
                solveEllipse(onSketchPos);
                approximateEllipse();

                // Display radius for user
                if (showCursorCoords()) {
                    SbString text;
                    std::string aString = lengthToDisplayFormat(a, 1);
                    std::string bString = lengthToDisplayFormat(b, 1);
                    text.sprintf(" (R%s, R%s)", aString.c_str(), bString.c_str());
                    setPositionText(onSketchPos, text);
                }

                drawEdit(editCurve);
                if (seekAutoConstraint(sugConstr3,
                                       onSketchPos,
                                       onSketchPos - centroid,
                                       AutoConstraint::CURVE)) {
                    renderSuggestConstraintsCursor(sugConstr3);
                    return;
                }
            }
        }
        applyCursor();
    }

    /**
     * @brief Changes drawing mode on user-click
     * @param onSketchPos the position of the cursor on the sketch
     * @return
     */
    bool pressButton(Base::Vector2d onSketchPos) override
    {
        if (method == PERIAPSIS_APOAPSIS_B) {
            if (mode == STATUS_SEEK_PERIAPSIS) {
                periapsis = onSketchPos;
                setAngleSnapping(true, periapsis);
                mode = STATUS_SEEK_APOAPSIS;
            }
            else if (mode == STATUS_SEEK_APOAPSIS) {
                apoapsis = onSketchPos;
                setAngleSnapping(false);
                mode = STATUS_SEEK_B;
            }
            else {
                mode = STATUS_Close;
            }
        }
        else {  // method is CENTER_PERIAPSIS_B
            if (mode == STATUS_SEEK_CENTROID) {
                centroid = onSketchPos;
                setAngleSnapping(true, centroid);
                mode = STATUS_SEEK_PERIAPSIS;
            }
            else if (mode == STATUS_SEEK_PERIAPSIS) {
                periapsis = onSketchPos;
                setAngleSnapping(false);
                mode = STATUS_SEEK_B;
            }
            else {
                mode = STATUS_Close;
            }
        }
        return true;
    }

    /**
     * @brief Calls \c saveEllipse() after last user input
     * @param onSketchPos the position of the cursor on the sketch
     * @return
     */
    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (mode == STATUS_Close) {
            saveEllipse();
            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);

            if (continuousMode) {
                if (constrMethod == 0) {
                    method = CENTER_PERIAPSIS_B;
                    mode = STATUS_SEEK_CENTROID;
                }
                else {
                    method = PERIAPSIS_APOAPSIS_B;
                    mode = STATUS_SEEK_PERIAPSIS;
                }
            }
        }
        return true;
    }

private:
    /**
     * @brief Slot called when the create ellipse command is activated
     * @param sketchgui A pointer to the active sketch
     */
    void activated() override
    {
        if (constrMethod == 0) {
            method = CENTER_PERIAPSIS_B;
            mode = STATUS_SEEK_CENTROID;
        }
        else {
            method = PERIAPSIS_APOAPSIS_B;
            mode = STATUS_SEEK_PERIAPSIS;
        }
    }
    QString getCrosshairCursorSVGName() const override
    {
        return QString::fromLatin1("Sketcher_Pointer_Create_Ellipse");
    }

protected:
    std::vector<AutoConstraint> sugConstr1, sugConstr2, sugConstr3;

private:
    SelectMode mode;
    /// the method of constructing the ellipse
    ConstructionMethod method;
    int constrMethod;
    /// periapsis position vector, in standard position in sketch coordinate system
    Base::Vector2d periapsis;
    /// apoapsis position vector, in standard position in sketch coordinate system
    Base::Vector2d apoapsis;
    /// centroid position vector, in standard position in sketch coordinate system
    Base::Vector2d centroid;
    /**
     * @brief position vector of positive b point, in standard position in sketch coordinate system
     * I.E. in polar perifocal system, the first intersection of the semiminor axis with the ellipse
     * as theta increases from 0. This always happens when:
     *    \f{eqnarray*}{
     *        \theta_b &=& \left[\pi - \arctan\left(\frac{b}{ae}\right)\right]  \pm N 2\pi
     *   \f}
     *
     * In a rotated R^3 cartesian system, centered at the centroid, +X towards periapsis, and
     * +Z coming out of the sketch, this b position is in the +Y direction from the centroid.
     */
    Base::Vector2d positiveB;
    /// the other b position
    Base::Vector2d negativeB;
    /// cart. position vector for primary focus
    Base::Vector2d f;
    /// cart. position vector for other focus
    Base::Vector2d fPrime;
    /// Unit vector for apse line
    Base::Vector2d apseHat;
    /// length of semimajor axis, i.e. 'radius' colloquially
    double a;
    /// length of semiminor axis, i.e. 'radius' colloquially
    double b;
    /// eccentricity [unitless]
    double e;
    /// optimization, holds a term that helps calculate b in terms of a and e
    double ratio;
    /// holds product of a * e
    double ae;
    /// holds numerator of orbit equation of form a(1-e^2)
    double num;
    /// holds a radial distance from f to the ellipse for a given theta
    double r;
    /// angle of a point in a perifocal frame centered at f
    double theta;
    /// angle of apse line relative to sketch coordinate system
    double phi;
    /// holds a position vector for a point on the ellipse from f
    Base::Vector2d pos;
    /// holds a position vector for a point on the ellipse from fPrime
    Base::Vector2d posPrime;
    /// holds position vectors for a points on the ellipse
    std::vector<Base::Vector2d> editCurve;
    /// local i_hat vector for ellipse, from centroid to periapsis
    Base::Vector3d iPrime;
    /// local j_hat vector for ellipse, from centroid to b point
    Base::Vector3d jPrime;
    /// length (radius) of the fixed axis
    double fixedAxisLength;
    /// position vector of fixed axis point in sketch coordinates
    Base::Vector2d fixedAxis;

    /**
     * @brief Computes a vector of 2D points representing an ellipse
     * @param onSketchPos Current position of the cursor on the sketch
     */
    void solveEllipse(Base::Vector2d onSketchPos)
    {
        const double GOLDEN_RATIO = 1.6180339887;
        Base::Vector3d k(0, 0, 1);

        if (method == PERIAPSIS_APOAPSIS_B) {
            if (mode == STATUS_SEEK_APOAPSIS) {
                apoapsis = onSketchPos;
            }
            a = (apoapsis - periapsis).Length() / 2;
            apseHat = (periapsis - apoapsis);
            apseHat.Normalize();
            centroid = apseHat;
            centroid.Scale(-1 * a);
            centroid = periapsis + centroid;
            if (mode == STATUS_SEEK_APOAPSIS) {
                // for first step, we draw an ellipse inscribed in a golden rectangle
                ratio = 1 / GOLDEN_RATIO;  // ~= 0.6180339887
                e = sqrt(ratio);           // ~= 0.7861513777
                b = a * ratio;
            }
            else if (mode == STATUS_SEEK_B) {
                // Get the closest distance from onSketchPos to apse line, as a 'requested' value
                // for b
                Base::Vector2d cursor =
                    Base::Vector2d(onSketchPos - f);  // vector from f to cursor pos
                // decompose cursor with a projection, then length of w_2 will give us b
                Base::Vector2d w_1 = cursor;
                w_1.ProjectToLine(
                    cursor,
                    (periapsis - apoapsis));  // projection of cursor line onto apse line
                Base::Vector2d w_2 = (cursor - w_1);
                b = w_2.Length();

                // limit us to ellipse or circles
                if (b > a) {
                    b = a;
                }

                e = sqrt(1 - ((b * b) / (a * a)));
                ratio = sqrt(1 - (e * e));
            }
            ae = a * e;
            f = apseHat;
            f.Scale(ae);
            f = centroid + f;
            fPrime = apseHat;
            fPrime.Scale(-1 * ae);
            fPrime = centroid + fPrime;
            phi = atan2(apseHat.y, apseHat.x);
            num = a * (1 - (e * e));
            // The ellipse is now solved
        }
        else {  // method == CENTER_PERIAPSIS_B
            if (mode == STATUS_SEEK_PERIAPSIS) {
                // solve the ellipse inscribed in a golden rectangle
                periapsis = onSketchPos;
                a = (centroid - periapsis).Length();
                iPrime.x = periapsis.x - centroid.x;
                iPrime.y = periapsis.y - centroid.y;
                iPrime.z = 0;
                jPrime = k % iPrime;  // j = k cross i

                // these are constant for any ellipse inscribed in a golden rectangle
                ratio = 1 / GOLDEN_RATIO;  // ~= 0.6180339887
                e = sqrt(ratio);           // ~= 0.7861513777

                b = a * ratio;
                ae = a * e;
                apseHat = (periapsis - centroid);
                apseHat.Normalize();
                f = apseHat;
                f.Scale(ae);
                f = centroid + f;
                fPrime = apseHat;
                fPrime.Scale(-1 * ae);
                fPrime = centroid + fPrime;
                apoapsis = apseHat;
                apoapsis.Scale(-1 * a);
                apoapsis = centroid + apoapsis;
                phi = atan2(apseHat.y, apseHat.x);
                num = a * (1 - (e * e));
                fixedAxisLength = a;
                fixedAxis = periapsis;
            }
            else if ((mode == STATUS_SEEK_B) || (mode == STATUS_SEEK_A)) {
                // while looking for the last click, we may switch back and forth
                // between looking for a b point and looking for periapsis, so ensure
                // we are in the right mode
                Base::Vector2d cursor =
                    Base::Vector2d(onSketchPos - centroid);  // vector from centroid to cursor pos
                // decompose cursor with a projection, then length of w_2 will give us b
                Base::Vector2d w_1 = cursor;
                w_1.ProjectToLine(
                    cursor,
                    (fixedAxis - centroid));  // projection of cursor line onto fixed axis line
                Base::Vector2d w_2 = (cursor - w_1);
                if (w_2.Length() > fixedAxisLength) {
                    // b is fixed, we are seeking a
                    mode = STATUS_SEEK_A;
                    jPrime.x = (fixedAxis - centroid).x;
                    jPrime.y = (fixedAxis - centroid).y;
                    jPrime.Normalize();
                    iPrime = jPrime % k;  // cross
                    b = fixedAxisLength;
                    a = w_2.Length();
                }
                else {
                    // a is fixed, we are seeking b
                    mode = STATUS_SEEK_B;
                    iPrime.x = (fixedAxis - centroid).x;
                    iPrime.y = (fixedAxis - centroid).y;
                    iPrime.Normalize();
                    jPrime = k % iPrime;  // cross
                    a = fixedAxisLength;
                    b = w_2.Length();
                }
                // now finish solving the ellipse
                periapsis.x = centroid.x + (iPrime * a).x;
                periapsis.y = centroid.y + (iPrime * a).y;
                e = sqrt(1 - ((b * b) / (a * a)));
                ratio = sqrt(1 - (e * e));
                ae = a * e;
                apseHat = (periapsis - centroid);
                apseHat.Normalize();
                f = apseHat;
                f.Scale(ae);
                f = centroid + f;
                fPrime = apseHat;
                fPrime.Scale(-1 * ae);
                fPrime = centroid + fPrime;
                apoapsis = apseHat;
                apoapsis.Scale(-1 * a);
                apoapsis = centroid + apoapsis;
                phi = atan2(apseHat.y, apseHat.x);
                num = a * (1 - (e * e));
            }
        }
    }


    /**
     * @brief Computes a sequence of 2D vectors to approximate the ellipse
     */
    void approximateEllipse()
    {
        // We will approximate the ellipse as a sequence of connected chords
        // Number of points per quadrant of the ellipse
        int n = static_cast<int>((editCurve.size() - 1) / 4);

        // We choose points in the perifocal frame then translate them to sketch cartesian.
        // This gives us a better approximation of an ellipse, i.e. more points where the
        // curvature is higher.  If the eccentricity is high, we shift the points a bit towards
        // the semi-minor axis.
        double partitionAngle = (M_PI - atan2(b, ae)) / n;
        double radianShift = 0;
        if (e > 0.8) {
            radianShift = (partitionAngle / 5) * 4;
        }
        for (int i = 0; i < n; i++) {
            theta = i * partitionAngle;
            if (i > 0) {
                theta = theta + radianShift;
            }
            r = num / (1 + (e * cos(theta)));
            // r(pi/2) is semi-latus rectum, if we need it
            pos.x = r * cos(theta + phi);  // phi rotates, sin/cos translate
            pos.y = r * sin(theta + phi);
            pos = pos + f;
            posPrime.x = r * cos(theta + phi + M_PI);
            posPrime.y = r * sin(theta + phi + M_PI);
            posPrime = posPrime + fPrime;
            // over the loop, loads Quadrant I points, by using f as origin
            editCurve[i] = pos;
            // over the loop, loads Quadrant III points, by using fPrime as origin
            editCurve[(2 * n) + i] = posPrime;
            // load points with negative theta angles (i.e. cw)
            if (i > 0) {
                pos.x = r * cos(-1 * theta + phi);
                pos.y = r * sin(-1 * theta + phi);
                pos = pos + f;
                // loads Quadrant IV points
                editCurve[(4 * n) - i] = pos;
                posPrime.x = r * cos(-1 * theta + phi + M_PI);
                posPrime.y = r * sin(-1 * theta + phi + M_PI);
                posPrime = posPrime + fPrime;
                // loads Quadrant II points
                editCurve[(2 * n) - i] = posPrime;
            }
        }
        // load pos & neg b points
        theta = M_PI - atan2(b, ae);  // the angle from f to the positive b point
        r = num / (1 + (e * cos(theta)));
        pos.x = r * cos(theta + phi);
        pos.y = r * sin(theta + phi);
        pos = pos + f;
        editCurve[n] = pos;  // positive
        pos.x = r * cos(-1 * theta + phi);
        pos.y = r * sin(-1 * theta + phi);
        pos = pos + f;
        editCurve[(3 * n)] = pos;  // negative
        // force the curve to be a closed shape
        editCurve[(4 * n)] = editCurve[0];
    }

    /**
     * @brief Prints the ellipse data to STDOUT as an GNU Octave script
     * @param onSketchPos position of the cursor on the sketch
     */
    void ellipseToOctave(Base::Vector2d /*onSketchPos*/)
    {
        int n = static_cast<int>((editCurve.size() - 1) / 4);

        // send a GNU Octave script to stdout to plot points for debugging
        std::ostringstream octave;
        octave << std::fixed << std::setprecision(12);
        octave << "\nclear all;\nclose all;\nclc;\n\n";
        octave << "periapsis = [" << periapsis.x << ", " << periapsis.y << "];\n";
        octave << "apoapsis = [" << apoapsis.x << ", " << apoapsis.y << "];\n";
        octave << "positiveB = [" << editCurve[n].x << ", " << editCurve[n].y << "];\n";
        octave << "apseHat = [" << apseHat.x << ", " << apseHat.y << "];\n";
        octave << "a = " << a << ";\n";
        octave << "b = " << b << ";\n";
        octave << "eccentricity = " << e << ";\n";
        octave << "centroid = [" << centroid.x << ", " << centroid.y << "];\n";
        octave << "f = [" << f.x << ", " << f.y << "];\n";
        octave << "fPrime = [" << fPrime.x << ", " << fPrime.y << "];\n";
        octave << "phi = " << phi << ";\n\n";
        octave << "x = [";
        for (int i = 0; i < 4 * n + 1; i++) {
            octave << editCurve[i].x;
            if (i < 4 * n) {
                octave << ", ";
            }
        }
        octave << "];\n";
        octave << "y = [";
        for (int i = 0; i < 4 * n + 1; i++) {
            octave << editCurve[i].y;
            if (i < 4 * n) {
                octave << ", ";
            }
        }
        octave << "];\n\n";
        octave << "% Draw ellipse points in red;\n";
        octave << "plot (x, y, \"r.\", \"markersize\", 5);\n";
        octave << "axis ([-300, 300, -300, 300], \"square\");grid on;\n";
        octave << "hold on;\n\n";
        octave << "% Draw centroid in blue, f in cyan, and fPrime in magenta;\n";
        octave << "plot(centroid(1), centroid(2), \"b.\", \"markersize\", 5);\n";
        octave << "plot(f(1), f(2), \"c.\", \"markersize\", 5);\n";
        octave << "plot(fPrime(1), fPrime(2), \"m.\", \"markersize\", 5);\n";
        octave << "n = [periapsis(1) - f(1), periapsis(2) - f(2)];\n";
        octave << "h = quiver(f(1),f(2),n(1),n(2), 0);\n";
        octave << "set (h, \"maxheadsize\", 0.1);\n\n";
        octave << "% Draw the three position vectors used for Gui::Command::doCommand(...)\n";
        octave << "periapsisVec = quiver(0,0,periapsis(1),periapsis(2), 0);\n";
        octave << "set (periapsisVec, \"maxheadsize\", 0.01, \"color\", \"black\");\n";
        octave << "centroidVec = quiver(0,0,centroid(1),centroid(2), 0);\n";
        octave << "set (centroidVec, \"maxheadsize\", 0.01, \"color\", \"black\");\n";
        octave << "bVec = quiver(0,0,positiveB(1),positiveB(2), 0);\n";
        octave << "set (bVec, \"maxheadsize\", 0.01, \"color\", \"black\");\n\n";
        octave << "% Draw the local x & y basis vectors, scaled to a and b, in red and blue, "
                  "respectively\n";
        octave << "xLocalVec = "
                  "quiver(centroid(1),centroid(2),periapsis(1)-centroid(1),periapsis(2)-centroid(2)"
                  ", 0);\n";
        octave << "set (xLocalVec, \"maxheadsize\", 0.01, \"color\", \"red\");\n";
        octave << "yLocalVec = quiver(centroid(1),centroid(2), positiveB(1)-centroid(1), "
                  "positiveB(2)-centroid(2), 0);\n";
        octave << "set (yLocalVec, \"maxheadsize\", 0.01, \"color\", \"blue\");\nhold off;\n";
        qDebug() << QString::fromStdString(octave.str());
    }

    /**
     * @brief Finalizes and saves the drawn ellipse
     * @return nothing
     */
    void saveEllipse()
    {
        unsetCursor();
        resetPositionText();

        /* There are a couple of issues with Gui::Command::doCommand(...) and
         * GC_MakeEllipse(...) that cause bugs if not handled properly, even
         * when we give them a mathematically-correct ellipse.
         *
         * GC_MakeEllipse may fail with a gce_InvertAxis error for a small
         * circular ellipse when floating point roundoff or representation
         * errors make the b axis slightly larger than the a axis.
         *
         * A similar, larger, issue arises in Gui::Command::doCommand(...) because
         * we cast our double vector components into strings with a fixed
         * precision of six, and then create new doubles from the strings
         * in EllipsePy::PyInit(...).  Thus, by the time we call GC_MakeEllipse(...)
         * in EllipsePy::PyInit(...), our ellipse may not be valid anymore
         * because b is now greater than a.
         *
         * To handle these issues, we simulate the effects Gui::Command::doCommand(...)
         * has on our ellipse, and we adjust our ellipse parameters until
         * GC_MakeEllipse successfully creates an ellipse with our mangled
         * parameters.
         *
         * In almost all cases, we only have to make our test ellipse one time;
         * it is only in the rare edge cases that require repeated test ellipses
         * until we get a valid one, or fail due to excessive attempts. With a
         * limit of 25 attempts, I have been unable to make it fail.
         */

        // simulate loss of precision in centroid, periapsis, and apoapsis
        char cx[64];
        char cy[64];
        char px[64];
        char py[64];
        char ax[64];
        char ay[64];
        sprintf(cx, "%.6lf\n", centroid.x);
        sprintf(cy, "%.6lf\n", centroid.y);
        sprintf(px, "%.6lf\n", periapsis.x);
        sprintf(py, "%.6lf\n", periapsis.y);
        sprintf(ax, "%.6lf\n", apoapsis.x);
        sprintf(ay, "%.6lf\n", apoapsis.y);
        centroid.x = atof(cx);
        centroid.y = atof(cy);
        periapsis.x = atof(px);
        periapsis.y = atof(py);
        apoapsis.x = atof(ax);
        apoapsis.y = atof(ay);
        double majorLength = (periapsis - apoapsis).Length();
        double minorLength = 0;

        /* GC_MakeEllipse requires a right-handed coordinate system, with +X
         * from centroid to periapsis, +Z out of the page.
         */
        Base::Vector3d k(0, 0, 1);
        Base::Vector3d i(periapsis.x - centroid.x, periapsis.y - centroid.y, 0);
        Base::Vector3d j = k % i;  // j = k cross i
        double beta = 1e-7;
        int count = 0;
        int limit = 25;  // no infinite loops!
        bool success = false;
        double tempB = b;

        // adjust b until our mangled vectors produce a good ellipse in GC_MakeEllipse
        // and the mangled major and minor lines in LinePy::PyInit(...) are such that
        // major is at least slightly larger than minor
        do {
            tempB = b - double(count * beta);
            j = j.Normalize() * tempB;
            positiveB.x = centroid.x + j.x;
            positiveB.y = centroid.y + j.y;
            negativeB.x = centroid.x + (j.x * -1);
            negativeB.y = centroid.y + (j.y * -1);
            char bpx[64];
            char bpy[64];
            char bnx[64];
            char bny[64];
            sprintf(bpx, "%.6lf\n", positiveB.x);
            sprintf(bpy, "%.6lf\n", positiveB.y);
            sprintf(bnx, "%.6lf\n", negativeB.x);
            sprintf(bny, "%.6lf\n", negativeB.y);
            positiveB.x = atof(bpx);
            positiveB.y = atof(bpy);
            negativeB.x = atof(bnx);
            negativeB.y = atof(bny);
            GC_MakeEllipse me(gp_Pnt(periapsis.x, periapsis.y, 0),
                              gp_Pnt(positiveB.x, positiveB.y, 0),
                              gp_Pnt(centroid.x, centroid.y, 0));
            minorLength = (negativeB - positiveB).Length();
            count++;
            success = me.IsDone() && (minorLength + beta < majorLength);
        } while (!success && (count <= limit));
        if (!success) {
            qDebug() << "Failed to create a valid mangled ellipse after" << count << "attempts";
        }

        // save any changes to b, then recalculate ellipse as required due to change in b
        b = tempB;
        e = sqrt(1 - ((b * b) / (a * a)));
        ae = a * e;
        f = apseHat;
        f.Scale(ae);
        f = centroid + f;
        fPrime = apseHat;
        fPrime.Scale(-1 * ae);
        fPrime = centroid + fPrime;

        int currentgeoid = getHighestCurveIndex();  // index of the ellipse we just created

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch ellipse"));
            Gui::cmdAppObjectArgs(
                sketchgui->getObject(),
                "addGeometry(Part.Ellipse"
                "(App.Vector(%f,%f,0),App.Vector(%f,%f,0),App.Vector(%f,%f,0)),%s)",
                periapsis.x,
                periapsis.y,
                positiveB.x,
                positiveB.y,
                centroid.x,
                centroid.y,
                geometryCreationMode == Construction ? "True" : "False");

            currentgeoid++;

            Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                  "exposeInternalGeometry(%d)",
                                  currentgeoid);
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(sketchgui,
                             QT_TRANSLATE_NOOP("Notifications", "Error"),
                             QT_TRANSLATE_NOOP("Notifications", "Failed to add an ellipse"));
            Gui::Command::abortCommand();

            tryAutoRecomputeIfNotSolve(
                static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));

            return;
        }

        Gui::Command::commitCommand();

        if (method == CENTER_PERIAPSIS_B) {
            // add auto constraints for the center point
            if (!sugConstr1.empty()) {
                createAutoConstraints(sugConstr1, currentgeoid, Sketcher::PointPos::mid);
                sugConstr1.clear();
            }
            if (!sugConstr2.empty()) {
                createAutoConstraints(sugConstr2, currentgeoid, Sketcher::PointPos::none);
                sugConstr2.clear();
            }
            if (!sugConstr3.empty()) {
                createAutoConstraints(sugConstr3, currentgeoid, Sketcher::PointPos::none);
                sugConstr3.clear();
            }
        }

        if (method == PERIAPSIS_APOAPSIS_B) {
            if (!sugConstr1.empty()) {
                createAutoConstraints(sugConstr1, currentgeoid, Sketcher::PointPos::none);
                sugConstr1.clear();
            }
            if (!sugConstr2.empty()) {
                createAutoConstraints(sugConstr2, currentgeoid, Sketcher::PointPos::none);
                sugConstr2.clear();
            }
            if (!sugConstr3.empty()) {
                createAutoConstraints(sugConstr3, currentgeoid, Sketcher::PointPos::none);
                sugConstr3.clear();
            }
        }

        tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));

        // This code enables the continuous creation mode.
        if (constrMethod == 0) {
            method = CENTER_PERIAPSIS_B;
            mode = STATUS_SEEK_CENTROID;
        }
        else {
            method = PERIAPSIS_APOAPSIS_B;
            mode = STATUS_SEEK_PERIAPSIS;
        }
        editCurve.clear();
        drawEdit(editCurve);

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);


        if (continuousMode) {
            // This code enables the continuous creation mode.
            editCurve.resize(33);
            applyCursor();
            /* It is ok not to call to purgeHandler
             * in continuous creation mode because the
             * handler is destroyed by the quit() method on pressing the
             * right button of the mouse */
        }
        else {
            sketchgui
                ->purgeHandler();  // no code after this line, Handler get deleted in ViewProvider
        }
    }
};

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerEllipse_H
