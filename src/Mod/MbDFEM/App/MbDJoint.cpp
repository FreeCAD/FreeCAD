// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDJoint.h"

#include <string>

PROPERTY_SOURCE(MbDFEM::MbDJoint, MbDFEM::MbDItemIJ)

namespace
{

bool startsWith(const std::string& value, const std::string& prefix)
{
    return value.rfind(prefix, 0) == 0;
}

std::string makeJointLabel(const char* jointTypeName, const std::string& currentLabel, const char* name)
{
    const std::string base = "MbDJoint";
    const std::string typedBase = std::string(jointTypeName) + " " + base;

    if (currentLabel.empty() || currentLabel == name) {
        return typedBase;
    }

    if (startsWith(currentLabel, base)) {
        return typedBase;
    }

    for (const char** type = MbDFEM::MbDJoint::JointTypeEnums; *type != nullptr; ++type) {
        const std::string oldTypedBase = std::string(*type) + " " + base;
        if (startsWith(currentLabel, oldTypedBase)) {
            return typedBase;
        }
    }

    return {};
}

}  // namespace

const char* MbDFEM::MbDJoint::JointTypeEnums[] = {"Fixed",
                                                  "Revolute",
                                                  "Prismatic",
                                                  "Cylindrical",
                                                  "Spherical",
                                                  "Universal",
                                                  "Planar",
                                                  "Distance",
                                                  "Gear",
                                                  "RackPinion",
                                                  nullptr};

MbDFEM::MbDJoint::MbDJoint()
{
    jointType.setEnums(JointTypeEnums);
    ADD_PROPERTY_TYPE(jointType,
                      ((long)0),
                      "MbDFEM",
                      App::Prop_None,
                      "Type of multibody joint");
    ADD_PROPERTY_TYPE(gearRatio,
                      (1.0),
                      "MbDFEM",
                      App::Prop_None,
                      "Gear joint ratio");
    ADD_PROPERTY_TYPE(pitchRadius,
                      (1.0),
                      "MbDFEM",
                      App::Prop_None,
                      "Rack-pinion pitch radius");
}

void MbDFEM::MbDJoint::onChanged(const App::Property* prop)
{
    if (prop == &jointType) {
        const auto label = makeJointLabel(jointType.getValueAsString(),
                                          Label.getStrValue(),
                                          getNameInDocument());
        if (!label.empty() && label != Label.getStrValue()) {
            Label.setValue(label.c_str());
        }
    }

    MbDItemIJ::onChanged(prop);
}
