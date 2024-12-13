set(MODULE orchestrionsequencer)

set(MODULE_SRC
    internal/ComputerKeyboardMidiController.cpp
    internal/ComputerKeyboardMidiController.h
    internal/MuseChord.cpp
    internal/MuseChord.h
    internal/Orchestrion.cpp
    internal/Orchestrion.h
    internal/OrchestrionSequencer.cpp
    internal/OrchestrionSequencer.h
    internal/OrchestrionSequencer.cpp
    internal/OrchestrionSequencer.h
    internal/OrchestrionSequencerFactory.cpp
    internal/OrchestrionSequencerFactory.h
    internal/VoiceBlank.cpp
    internal/VoiceBlank.h
    internal/VoiceSequencer.cpp
    internal/VoiceSequencer.h
    OrchestrionTypes.h
    IChord.h
    IOrchestrion.h
    IOrchestrionSequencer.h
    OrchestrionModule.cpp
    OrchestrionModule.h
)

set(MODULE_NOT_LINK_GLOBAL ON)
set(MODULE_USE_PCH_NONE ON)
set(MODULE_USE_UNITY_NONE ON)

setup_module()
