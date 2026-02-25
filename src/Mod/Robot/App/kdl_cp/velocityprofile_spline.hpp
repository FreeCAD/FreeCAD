// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "velocityprofile.hpp"

namespace KDL
{
	/**
	 * \brief A spline VelocityProfile trajectory interpolation.
	 * @ingroup Motion
	 */
class VelocityProfile_Spline : public VelocityProfile
{
public:
    VelocityProfile_Spline();
    VelocityProfile_Spline(const VelocityProfile_Spline &p);

	virtual ~VelocityProfile_Spline();
	
    virtual void SetProfile(double pos1, double pos2);
    /**
     * Generate linear interpolation coefficients.
     *
     * @param pos1 begin position.
     * @param pos2 end position.
     * @param duration duration of the profile.
     */
    virtual void SetProfileDuration(
      double pos1, double pos2, double duration);

    /**
     * Generate cubic spline interpolation coefficients.
     *
     * @param pos1 begin position.
     * @param vel1 begin velocity.
     * @param pos2 end position.
     * @param vel2 end velocity.
     * @param duration duration of the profile.
     */
    virtual void SetProfileDuration(
      double pos1, double vel1, double pos2, double vel2, double duration);

    /**
     * Generate quintic spline interpolation coefficients.
     *
     * @param pos1 begin position.
     * @param vel1 begin velocity.
     * @param acc1 begin acceleration
     * @param pos2 end position.
     * @param vel2 end velocity.
     * @param acc2 end acceleration.
     * @param duration duration of the profile.
     */
    virtual void SetProfileDuration(double pos1, double vel1, double acc1, double pos2, double vel2, double acc2, double duration);
    virtual double Duration() const;
    virtual double Pos(double time) const;
    virtual double Vel(double time) const;
    virtual double Acc(double time) const;
    virtual void Write(std::ostream& os) const;
    virtual VelocityProfile* Clone() const;
private:

    double coeff_[6];
    double duration_;
};
}