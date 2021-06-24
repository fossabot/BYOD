#pragma once

#include "../processors/ProcessorChain.h"
#include "KnobsComponent.h"
#include "utils/PowerButton.h"

class ProcessorEditor : public Component
{
public:
    ProcessorEditor (BaseProcessor& baseProc, ProcessorChain& procs);
    ~ProcessorEditor();

    void paint (Graphics& g) override;
    void resized() override;
    void mouseDrag (const MouseEvent& e) override;

    const BaseProcessor* getProcPtr() const { return &proc; }

private:
    BaseProcessor& proc;
    ProcessorChain& procChain;
    
    const ProcessorUIOptions& procUI;
    Colour contrastColour;

    KnobsComponent knobs;
    PowerButton powerButton;
    TextButton xButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorEditor)
};
