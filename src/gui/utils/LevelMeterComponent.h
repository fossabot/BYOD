#pragma once

#include <pch.h>

class LevelMeterComponent : public Component,
                            private Timer
{
public:
    using LevelDataType = std::array<std::atomic<float>, 2>;

    explicit LevelMeterComponent (const LevelDataType& levelData);

    void paint (Graphics& g) override;
    void timerCallback() final;

private:
    Rectangle<int> getMeterBounds() const;

    const LevelDataType& rmsLevels;
    std::array<float, 2> dbLevels;
    std::array<float, 2> dbLevelsPrev;
    chowdsp::LevelDetector<float> levelDetector[2];
};
