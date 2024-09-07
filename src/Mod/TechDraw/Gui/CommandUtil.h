
#include "PreCompiled.h"
#ifndef _PreComp_
#include <vector>
#endif


namespace TechDraw {
    class DrawViewPart;
}

namespace TechDrawGui::CommandUtil {

TechDraw::DrawViewPart* getDrawViewPart(Gui::Command* cmd);
std::vector<TechDraw::DrawViewPart*> getDrawViewParts(Gui::Command* cmd);

}