#include "ProcessorEditor.h"
#include "../BoardComponent.h"
#include "processors/chain/ProcessorChainActionHelper.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

namespace
{
constexpr float cornerSize = 5.0f;
}

ProcessorEditor::ProcessorEditor (BaseProcessor& baseProc,
                                  ProcessorChain& procs,
                                  chowdsp::HostContextProvider& hostContextProvider) : proc (baseProc),
                                                                                       procChain (procs),
                                                                                       procUI (proc.getUIOptions()),
                                                                                       contrastColour (procUI.backgroundColour.contrasting()),
                                                                                       knobs (baseProc,
                                                                                              proc.getVTS(),
                                                                                              contrastColour,
                                                                                              procUI.powerColour,
                                                                                              hostContextProvider),
                                                                                       powerButton (procUI.powerColour)
{
    addAndMakeVisible (knobs);
    setBroughtToFrontOnMouseClick (true);

    addAndMakeVisible (powerButton);
    powerButton.setEnableDisableComps ({ &knobs });
    powerButton.attachButton (proc.getVTS(), "on_off");

    auto xSvg = Drawable::createFromImageData (BinaryData::xsolid_svg, BinaryData::xsolid_svgSize);
    xSvg->replaceColour (Colours::white, contrastColour);
    xButton.setImages (xSvg.get());
    addAndMakeVisible (xButton);

    if (&proc != &procs.getInputProcessor() && &proc != &procs.getOutputProcessor())
    {
        auto cog = Drawable::createFromImageData (BinaryData::cogsolid_svg, BinaryData::cogsolid_svgSize);
        cog->replaceColour (Colours::white, contrastColour);
        settingsButton.setImages (cog.get());
        addAndMakeVisible (settingsButton);

        settingsButton.onClick = [&]
        {
            PopupMenu menu;
            PopupMenu::Options options;
            processorSettingsCallback (menu, options);
            menu.showMenuAsync (options);
        };
    }

    if (auto* lnf = proc.getCustomLookAndFeel())
        setLookAndFeel (lnf);
    else
        setLookAndFeel (lnfAllocator->getLookAndFeel<ProcessorLNF>());

    for (int i = 0; i < baseProc.getNumInputs(); ++i)
    {
        const auto portType = baseProc.getInputPortType (i);
        auto newPort = inputPorts.add (std::make_unique<Port> (procUI.backgroundColour, portType));
        newPort->setInputOutput (true);
        newPort->setTooltip (baseProc.getTooltipForPort (i, true));
        addAndMakeVisible (newPort);
    }

    for (int i = 0; i < baseProc.getNumOutputs(); ++i)
    {
        const auto portType = baseProc.getOutputPortType (i);
        auto newPort = outputPorts.add (std::make_unique<Port> (procUI.backgroundColour, portType));
        newPort->setInputOutput (false);
        newPort->setTooltip (baseProc.getTooltipForPort (i, false));
        addAndMakeVisible (newPort);
    }

    for (int i = 0; i < proc.getNumInputs(); ++i)
        toggleParamsEnabledOnInputConnectionChange (i, false);

    uiOptionsChangedCallback = proc.uiOptionsChanged.connect (
        [this]
        {
            contrastColour = procUI.backgroundColour.contrasting();
            knobs.setColours (contrastColour, procUI.powerColour);
            powerButton.setupPowerButton (procUI.powerColour);
        });

    baseProc.setEditor (this);
}

ProcessorEditor::~ProcessorEditor() = default;

void ProcessorEditor::addToBoard (BoardComponent* boardComp)
{
    broadcasterCallbacks += {
        showInfoCompBroadcaster.connect<&BoardComponent::showInfoComp> (boardComp),
        editorDraggedBroadcaster.connect<&BoardComponent::editorDragged> (boardComp),
        duplicateProcessorBroadcaster.connect<&BoardComponent::duplicateProcessor> (boardComp),
    };

    xButton.onClick = [this, boardComp]
    { boardComp->editorDeleteRequested (*this); };
}

void ProcessorEditor::processorSettingsCallback (PopupMenu& menu, PopupMenu::Options& options)
{
    proc.addToPopupMenu (menu);

    menu.addItem ("Reset", [&]
                  { resetProcParameters(); });

    PopupMenu replaceProcMenu;
    createReplaceProcMenu (replaceProcMenu);
    if (replaceProcMenu.containsAnyActiveItems())
        menu.addSubMenu ("Replace", replaceProcMenu);

    menu.addItem ("Duplicate", [&]
                  { duplicateProcessorBroadcaster (*this); });

    menu.addItem ("Info", [this]
                  { showInfoCompBroadcaster (proc); });

    menu.setLookAndFeel (lnfAllocator->getLookAndFeel<ProcessorLNF>());
    options = options
                  .withParentComponent (getParentComponent())
                  .withStandardItemHeight (27);
}

void ProcessorEditor::resetProcParameters()
{
    auto& vts = proc.getVTS();
    if (auto* um = vts.undoManager)
        um->beginNewTransaction();

    for (auto* param : proc.getVTS().processor.getParameters())
    {
        if (auto* rangedParam = dynamic_cast<RangedAudioParameter*> (param))
        {
            if (rangedParam->paramID == "on_off")
                continue;

            rangedParam->setValueNotifyingHost (rangedParam->getDefaultValue());
        }
    }

    if (auto* netlistQuantities = proc.getNetlistCircuitQuantities())
    {
        for (auto& element : *netlistQuantities)
        {
            element.value = element.defaultValue;
            element.needsUpdate = true;
        }
    }
}

void ProcessorEditor::createReplaceProcMenu (PopupMenu& menu)
{
    int menuID = 100;
    procChain.getProcStore().createProcReplaceList (menu, menuID, &proc);
}

void ProcessorEditor::paint (Graphics& g)
{
    TRACE_COMPONENT();

    const auto& procColour = procUI.backgroundColour;
    ColourGradient grad { procColour,
                          0.0f,
                          0.0f,
                          procColour.darker (0.25f),
                          (float) getWidth(),
                          (float) getWidth(),
                          false };
    g.setGradientFill (grad);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), cornerSize);

    if (procUI.backgroundImage != nullptr)
    {
        auto backgroundBounds = getLocalBounds().reduced ((int) cornerSize);
        procUI.backgroundImage->drawWithin (g, backgroundBounds.toFloat(), RectanglePlacement::stretchToFit, 1.0f);
    }

    auto fontHeight = proportionOfHeight (0.139f);
    auto nameHeight = proportionOfHeight (0.167f);

    g.setColour (contrastColour);
    g.setFont (Font ((float) fontHeight).boldened());
    g.drawFittedText (proc.getName(), 5, 0, jmax (getWidth() - 50, 100), nameHeight, Justification::centredLeft, 1);
}

void ProcessorEditor::resized()
{
    TRACE_COMPONENT();

    const auto width = getWidth();
    const auto height = getHeight();

    const auto knobsPad = proportionOfWidth (0.015f);
    auto nameHeight = proportionOfHeight (0.167f);
    knobs.setBounds (knobsPad, nameHeight, width - 2 * knobsPad, height - (nameHeight + knobsPad));

    bool isIOProcessor = typeid (proc) == typeid (InputProcessor) || typeid (proc) == typeid (OutputProcessor);
    if (! isIOProcessor)
    {
        const auto xButtonSize = proportionOfWidth (0.1f);
        settingsButton.setBounds (Rectangle { width - 3 * xButtonSize, 0, xButtonSize, xButtonSize }.reduced (proportionOfWidth (0.01f)));
        powerButton.setBounds (width - 2 * xButtonSize, 0, xButtonSize, xButtonSize);
        xButton.setBounds (Rectangle { width - xButtonSize, 0, xButtonSize, xButtonSize }.reduced (proportionOfWidth (0.015f)));
    }

    const int portDim = proportionOfHeight (0.17f);
    auto placePorts = [=] (int x, auto& ports)
    {
        const auto nPorts = ports.size();
        if (nPorts == 0)
            return;

        const auto yPad = height / nPorts;
        int y = yPad / 2;
        for (auto* port : ports)
        {
            port->setBounds (x, y, portDim, portDim);
            y += yPad;
        }
    };

    placePorts (-portDim / 2, inputPorts);
    placePorts (width - portDim / 2, outputPorts);
}

void ProcessorEditor::mouseDown (const MouseEvent& e)
{
    mouseDownOffset = e.getEventRelativeTo (this).getPosition();
}

void ProcessorEditor::mouseDrag (const MouseEvent& e)
{
    editorDraggedBroadcaster (*this, e, mouseDownOffset, false);
}

void ProcessorEditor::mouseUp (const MouseEvent& e)
{
    editorDraggedBroadcaster (*this, e, mouseDownOffset, true);
}

Port* ProcessorEditor::getPortPrivate (int portIndex, bool isInput) const
{
    if (isInput)
    {
        jassert (portIndex < inputPorts.size());
        return inputPorts[portIndex];
    }

    jassert (portIndex < outputPorts.size());
    return outputPorts[portIndex];
}

Port* ProcessorEditor::getPort (int portIndex, bool isInput)
{
    return getPortPrivate (portIndex, isInput);
}

const Port* ProcessorEditor::getPort (int portIndex, bool isInput) const
{
    return getPortPrivate (portIndex, isInput);
}

juce::Point<int> ProcessorEditor::getPortLocation (int portIndex, bool isInput) const
{
    return getPortPrivate (portIndex, isInput)->getBounds().getCentre();
}

void ProcessorEditor::setConnectionStatus (bool isConnected, int portIndex, bool isInput)
{
    if (isInput)
    {
        inputPorts[portIndex]->setConnectionStatus (isConnected);
        toggleParamsEnabledOnInputConnectionChange (portIndex, isConnected);
    }
    else
    {
        outputPorts[portIndex]->setConnectionStatus (isConnected);
    }
}

void ProcessorEditor::toggleParamsEnabledOnInputConnectionChange (int inputPortIndex, bool isConnected)
{
    if (auto* toggleParamIDs = proc.getParametersToDisableWhenInputIsConnected (inputPortIndex))
        knobs.toggleParamsEnabled (*toggleParamIDs, ! isConnected);

    if (auto* toggleParamIDs = proc.getParametersToEnableWhenInputIsConnected (inputPortIndex))
        knobs.toggleParamsEnabled (*toggleParamIDs, isConnected);
}
