// SPDX-License-Identifier: LGPL-2.1-or-later
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/Application.h>
#include <functional>
#include <Mod/PartDesign/App/Body.h>
#include "Utils.h"

// Existing profile-based command utilities, shared without changing their behavior.
void prepareProfileBased(
    PartDesign::Body*,
    Gui::Command*,
    const std::string&,
    std::function<void(Part::Feature*, App::DocumentObject*)>
);
void finishProfileBased(const Gui::Command*, const Part::Feature*, App::DocumentObject*);

DEF_STD_CMD_A(CmdPartDesignRib)

CmdPartDesignRib::CmdPartDesignRib()
    : Command("PartDesign_Rib")
{
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Rib");
    sToolTipText = QT_TR_NOOP("Creates reinforcing ribs from profile edges");
    sWhatsThis = "PartDesign_Rib";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Rib";
}

void CmdPartDesignRib::activated(int)
{
    auto body = PartDesignGui::getBody(true);
    if (!body) {
        return;
    }
    prepareProfileBased(body, this, "Rib", [this](Part::Feature* profile, App::DocumentObject* feature) {
        if (feature) {
            Gui::Command::updateActive();
            finishProfileBased(this, profile, feature);
        }
    });
}

bool CmdPartDesignRib::isActive()
{
    return hasActiveDocument();
}

void CreateRibCommands()
{
    Gui::Application::Instance->commandManager().addCommand(new CmdPartDesignRib);
}
