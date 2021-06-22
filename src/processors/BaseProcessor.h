#pragma once

#include "JuceProcWrapper.h"

enum ProcessorType
{
    Drive,
    Tone,
    Other,
};

struct ProcessorUIOptions
{
    Colour backgroundColour = Colours::red;
    std::unique_ptr<Drawable> backgroundImage;
    LookAndFeel* lnf = nullptr;
};

class BaseProcessor : private JuceProcWrapper
{
public:
    using Ptr = std::unique_ptr<BaseProcessor>;

    BaseProcessor (const String& name,
                   AudioProcessorValueTreeState::ParameterLayout params,
                   UndoManager* um = nullptr);

    // metadata
    virtual ProcessorType getProcessorType() const = 0;
    const String getName() const override { return JuceProcWrapper::getName(); }

    // audio processing methods
    virtual void prepare (double sampleRate, int samplesPerBlock) = 0;
    virtual void processAudio (AudioBuffer<float>& buffer) = 0;

    // state save/load methods
    virtual std::unique_ptr<XmlElement> toXML();
    virtual void fromXML (XmlElement* xml);

    // interface for processor editors
    AudioProcessorValueTreeState& getVTS() { return vts; }
    const ProcessorUIOptions& getUIOptions() const { return uiOptions; }

protected:
    AudioProcessorValueTreeState vts;
    ProcessorUIOptions uiOptions;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaseProcessor)
};
