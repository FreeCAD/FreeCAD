// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

#include <Inventor/draggers/SoDragger.h>

namespace Gui
{

enum class DraggerInteraction
{
    Start,
    Motion,
    Finish,
};

template<typename Owner, void (Owner::*Handler)(DraggerInteraction, SoDragger*)>
void installDraggerInteractionCallbacks(SoDragger* dragger, Owner* owner)
{
    struct Callbacks
    {
        static void start(void* data, SoDragger* dragger)
        {
            invoke(data, dragger, DraggerInteraction::Start);
        }

        static void motion(void* data, SoDragger* dragger)
        {
            invoke(data, dragger, DraggerInteraction::Motion);
        }

        static void finish(void* data, SoDragger* dragger)
        {
            invoke(data, dragger, DraggerInteraction::Finish);
        }

        static void invoke(void* data, SoDragger* dragger, DraggerInteraction interaction)
        {
            (static_cast<Owner*>(data)->*Handler)(interaction, dragger);
        }
    };

    dragger->addStartCallback(&Callbacks::start, owner);
    dragger->addFinishCallback(&Callbacks::finish, owner);
    dragger->addMotionCallback(&Callbacks::motion, owner);
}

}  // namespace Gui
