// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include <Mod/Material/App/PreCompiled.h>
#ifndef _PreComp_
#endif

#include <QString>

#include <Mod/Material/App/Model.h>
#include <Mod/Material/App/ModelManager.h>

// clang-format off
TEST(Material, TestModelLoad)
{
        // self.ModelManager = Material.ModelManager()
        // self.MaterialManager = Material.MaterialManager()

        //     density = self.ModelManager.getModel("454661e5-265b-4320-8e6f-fcf6223ac3af")
        // self.assertIsNotNone(density)
        // self.assertEqual(density.Name, "Density")
        // self.assertEqual(density.UUID, "454661e5-265b-4320-8e6f-fcf6223ac3af")
        // self.assertIn("Density", density.Properties)
        // prop = density.Properties["Density"]
        // self.assertIn("Description", dir(prop))
        // self.assertIn("Name", dir(prop))
        // self.assertIn("Type", dir(prop))
        // self.assertIn("URL", dir(prop))
        // self.assertIn("Units", dir(prop))
        // self.assertEqual(prop.Name, "Density")

    auto manager = Materials::ModelManager::getManager();
    EXPECT_NE(manager, nullptr);
    auto density = manager->getModel(QString::fromStdString("454661e5-265b-4320-8e6f-fcf6223ac3af"));
    // EXPECT_NE(density, nullptr);
    EXPECT_EQ(density.getName(), QString::fromStdString("density"));
    EXPECT_EQ(density.getUUID(), QString::fromStdString("454661e5-265b-4320-8e6f-fcf6223ac3af"));
}

// clang-format on
