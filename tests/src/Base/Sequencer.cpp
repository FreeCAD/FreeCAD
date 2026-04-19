// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include <cstddef>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <Base/Sequencer.h>

namespace
{

class RecordingSequencer: public Base::SequencerBase
{
public:
    std::string text;
    std::vector<std::size_t> progressUpdates;
    int startCount = 0;
    int stopCount = 0;

protected:
    void startStep() override
    {
        ++startCount;
    }

    void stopStep() override
    {
        ++stopCount;
    }

    void setText(const char* value) override
    {
        text = value ? value : "";
    }

    void setProgress(std::size_t value) override
    {
        progressUpdates.push_back(value);
    }
};

}  // namespace

TEST(SequencerLauncher, NestedLaunchersDoNotUpdateOuterProgress)
{
    RecordingSequencer sequencer;

    {
        Base::SequencerLauncher outer("Outer", 100);
        EXPECT_TRUE(Base::Sequencer().isRunning());
        EXPECT_EQ(sequencer.startCount, 1);
        EXPECT_EQ(sequencer.text, "Outer");

        outer.setText("Outer update");
        outer.setProgress(25);
        ASSERT_EQ(sequencer.progressUpdates.size(), 1);
        EXPECT_EQ(sequencer.progressUpdates.back(), 25);
        EXPECT_EQ(sequencer.text, "Outer update");

        {
            Base::SequencerLauncher inner("Inner", 100);
            EXPECT_EQ(sequencer.startCount, 1);

            inner.setText("Inner update");
            inner.setProgress(75);
            EXPECT_EQ(sequencer.progressUpdates.size(), 1);
            EXPECT_EQ(sequencer.text, "Outer update");
        }

        EXPECT_TRUE(Base::Sequencer().isRunning());
        EXPECT_EQ(sequencer.stopCount, 0);

        outer.setProgress(50);
        ASSERT_EQ(sequencer.progressUpdates.size(), 2);
        EXPECT_EQ(sequencer.progressUpdates.back(), 50);
    }

    EXPECT_FALSE(Base::Sequencer().isRunning());
    EXPECT_EQ(sequencer.stopCount, 1);
}
