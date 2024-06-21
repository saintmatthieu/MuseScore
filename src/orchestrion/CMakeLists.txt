set(MODULE orchestrion)

set(MODULE_SRC
    internal/VoiceSequencer.cpp
    internal/VoiceSequencer.h
    OrchestrionSequencer.cpp
    OrchestrionSequencer.h
    OrchestrionTypes.h
    VoiceBlank.cpp
    VoiceBlank.h
    IChord.h
)

set(MODULE_NOT_LINK_GLOBAL ON)
set(MODULE_USE_PCH_NONE ON)
set(MODULE_USE_UNITY_NONE ON)

include(${PROJECT_SOURCE_DIR}/build/module.cmake)
