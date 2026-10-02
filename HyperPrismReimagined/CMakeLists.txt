cmake_minimum_required(VERSION 3.22)

# Universal Binary for macOS — MUST be set BEFORE project()
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version (universal binaries cannot go below 11.0)")
endif()

project(HyperPrismReimagined VERSION 1.3.0)

set_property(GLOBAL PROPERTY USE_FOLDERS YES)

option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Enable Module Source Groups" ON)

# Adding the JUCE subfolder
add_subdirectory(JUCE)

# The ZQ SFX house UI (tokens, LookAndFeel, bound controls, embedded OFL fonts). Must come AFTER
# JUCE (its CMakeLists detects the juce::juce_gui_basics target add_subdirectory(JUCE) above
# just created and skips fetching its own copy). Pinned by tag: bump deliberately, then re-run
# the hyperprism_ui_snapshot_* tools and compare before/after.
include(FetchContent)
FetchContent_Declare(zqsfx_ui
    GIT_REPOSITORY https://github.com/themightyzq/zqsfx_ui.git
    GIT_TAG v0.5.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(zqsfx_ui)

# Define plugin formats to build (VST3 + AU on macOS; Logic Pro hosts AU, not VST3)
if(APPLE)
    set(PLUGIN_FORMATS VST3 AU Standalone)
elseif(WIN32)
    set(PLUGIN_FORMATS VST3 Standalone)
else()
    set(PLUGIN_FORMATS VST3 Standalone)
endif()

# Shared plugin configuration function
function(configure_hyperprism_plugin target_name)
    # Plugin characteristics
    juce_generate_juce_header(${target_name})
    
    target_compile_definitions(${target_name}
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            JUCE_DISPLAY_SPLASH_SCREEN=0
            JUCE_REPORT_APP_USAGE=0
    )

    # v1.0.0 and the old nightly shipped with manufacturer code ZQFX; the plugin codes are
    # unchanged. Declaring the old VST3 class as compatible lets hosts that support
    # IPluginCompatibility (e.g. Cubase/Nuendo) swap this build into old sessions. PUBLIC
    # because the VST3 wrapper is compiled in the _VST3 format target.
    get_target_property(hp_plugin_code ${target_name} JUCE_PLUGIN_CODE)
    target_compile_definitions(${target_name}
        PUBLIC
            "JUCE_VST3_COMPATIBLE_CLASSES=VST3Interface::jucePluginId('ZQFX','${hp_plugin_code}')"
    )

    # Platform-specific audio system definitions
    # Disable JACK/ALSA on CI environments or Windows
    if(DEFINED ENV{CI} OR WIN32)
        target_compile_definitions(${target_name} PUBLIC JUCE_JACK=0 JUCE_ALSA=0)
    elseif(APPLE)
        # On local macOS builds, JACK is optional but ALSA is not used
        target_compile_definitions(${target_name} PUBLIC JUCE_JACK=0 JUCE_ALSA=0)
    elseif(UNIX)
        # On Linux, enable both JACK and ALSA
        target_compile_definitions(${target_name} PUBLIC JUCE_JACK=1 JUCE_ALSA=1)
    else()
        target_compile_definitions(${target_name} PUBLIC JUCE_JACK=0 JUCE_ALSA=0)
    endif()

    target_link_libraries(${target_name}
        PRIVATE
            juce::juce_audio_basics
            juce::juce_audio_devices
            juce::juce_audio_formats
            juce::juce_audio_plugin_client
            juce::juce_audio_processors
            juce::juce_audio_utils
            juce::juce_core
            juce::juce_data_structures
            juce::juce_dsp
            juce::juce_events
            juce::juce_graphics
            juce::juce_gui_basics
            juce::juce_gui_extra
            zqsfx::ui
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
    )
endfunction()

# HyperPrism Delay Plugin
juce_add_plugin(HyperPrismDelay
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hdly
    BUNDLE_ID "com.zqsfx.hyperprism.delay"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Delay"
    VST3_CATEGORIES "Fx" "Delay"
)

# Binary data will be added later when we have assets
# juce_add_binary_data(HyperPrismDelay_Assets
#     SOURCES
#         # We'll add assets later (fonts, images, etc.)
#     HEADER_NAME BinaryData.h
#     NAMESPACE BinaryData
# )

file(GLOB HyperPrismDelay_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Delay/Presets/*.hppreset)
if(HyperPrismDelay_PresetFiles)
    juce_add_binary_data(HyperPrismDelayPresets SOURCES ${HyperPrismDelay_PresetFiles})
    target_link_libraries(HyperPrismDelay PRIVATE HyperPrismDelayPresets)
endif()

target_sources(HyperPrismDelay
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/Delay/DelayProcessor.cpp
        Source/Delay/DelayProcessor.h
        Source/Delay/DelayEditor.cpp
        Source/Delay/DelayEditor.h
        Source/Delay/DelayPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismDelay)
# target_link_libraries(HyperPrismDelay PRIVATE HyperPrismDelay_Assets)

# HyperPrism Reverb Plugin
juce_add_plugin(HyperPrismReverb
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hrvb
    BUNDLE_ID "com.zqsfx.hyperprism.reverb"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Reverb"
    VST3_CATEGORIES "Fx" "Reverb"
)

file(GLOB HyperPrismReverb_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Reverb/Presets/*.hppreset)
if(HyperPrismReverb_PresetFiles)
    juce_add_binary_data(HyperPrismReverbPresets SOURCES ${HyperPrismReverb_PresetFiles})
    target_link_libraries(HyperPrismReverb PRIVATE HyperPrismReverbPresets)
endif()

target_sources(HyperPrismReverb
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/Reverb/ReverbProcessor.cpp
        Source/Reverb/ReverbProcessor.h
        Source/Reverb/ReverbEditor.cpp
        Source/Reverb/ReverbEditor.h
        Source/Reverb/ReverbPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismReverb)

# HyperPrism Chorus Plugin
juce_add_plugin(HyperPrismChorus
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hchr
    BUNDLE_ID "com.zqsfx.hyperprism.chorus"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Chorus"
    VST3_CATEGORIES "Fx" "Modulation"
)

file(GLOB HyperPrismChorus_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Chorus/Presets/*.hppreset)
if(HyperPrismChorus_PresetFiles)
    juce_add_binary_data(HyperPrismChorusPresets SOURCES ${HyperPrismChorus_PresetFiles})
    target_link_libraries(HyperPrismChorus PRIVATE HyperPrismChorusPresets)
endif()

target_sources(HyperPrismChorus
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/Chorus/ChorusProcessor.cpp
        Source/Chorus/ChorusProcessor.h
        Source/Chorus/ChorusEditor.cpp
        Source/Chorus/ChorusEditor.h
        Source/Chorus/ChorusPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismChorus)

# HyperPrism Flanger Plugin
juce_add_plugin(HyperPrismFlanger
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hflg
    BUNDLE_ID "com.zqsfx.hyperprism.flanger"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Flanger"
    VST3_CATEGORIES "Fx" "Modulation"
)

file(GLOB HyperPrismFlanger_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Flanger/Presets/*.hppreset)
if(HyperPrismFlanger_PresetFiles)
    juce_add_binary_data(HyperPrismFlangerPresets SOURCES ${HyperPrismFlanger_PresetFiles})
    target_link_libraries(HyperPrismFlanger PRIVATE HyperPrismFlangerPresets)
endif()

target_sources(HyperPrismFlanger
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/Flanger/FlangerProcessor.cpp
        Source/Flanger/FlangerProcessor.h
        Source/Flanger/FlangerEditor.cpp
        Source/Flanger/FlangerEditor.h
        Source/Flanger/FlangerPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismFlanger)

# HyperPrism High-Pass Filter Plugin
juce_add_plugin(HyperPrismHighPass
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hhpf
    BUNDLE_ID "com.zqsfx.hyperprism.highpass"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined High-Pass Filter"
    VST3_CATEGORIES "Fx" "Filter"
)

file(GLOB HyperPrismHighPass_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/HighPass/Presets/*.hppreset)
if(HyperPrismHighPass_PresetFiles)
    juce_add_binary_data(HyperPrismHighPassPresets SOURCES ${HyperPrismHighPass_PresetFiles})
    target_link_libraries(HyperPrismHighPass PRIVATE HyperPrismHighPassPresets)
endif()

target_sources(HyperPrismHighPass
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/HighPass/HighPassProcessor.cpp
        Source/HighPass/HighPassProcessor.h
        Source/HighPass/HighPassEditor.cpp
        Source/HighPass/HighPassEditor.h
        Source/HighPass/HighPassPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismHighPass)

# HyperPrism Low-Pass Filter Plugin
juce_add_plugin(HyperPrismLowPass
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hlpf
    BUNDLE_ID "com.zqsfx.hyperprism.lowpass"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Low-Pass Filter"
    VST3_CATEGORIES "Fx" "Filter"
)

file(GLOB HyperPrismLowPass_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/LowPass/Presets/*.hppreset)
if(HyperPrismLowPass_PresetFiles)
    juce_add_binary_data(HyperPrismLowPassPresets SOURCES ${HyperPrismLowPass_PresetFiles})
    target_link_libraries(HyperPrismLowPass PRIVATE HyperPrismLowPassPresets)
endif()

target_sources(HyperPrismLowPass
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/LowPass/LowPassProcessor.cpp
        Source/LowPass/LowPassProcessor.h
        Source/LowPass/LowPassEditor.cpp
        Source/LowPass/LowPassEditor.h
        Source/LowPass/LowPassPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismLowPass)

# HyperPrism Band-Pass Filter Plugin
juce_add_plugin(HyperPrismBandPass
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hbpf
    BUNDLE_ID "com.zqsfx.hyperprism.bandpass"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Band-Pass Filter"
    VST3_CATEGORIES "Fx" "Filter"
)

file(GLOB HyperPrismBandPass_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/BandPass/Presets/*.hppreset)
if(HyperPrismBandPass_PresetFiles)
    juce_add_binary_data(HyperPrismBandPassPresets SOURCES ${HyperPrismBandPass_PresetFiles})
    target_link_libraries(HyperPrismBandPass PRIVATE HyperPrismBandPassPresets)
endif()

target_sources(HyperPrismBandPass
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/BandPass/BandPassProcessor.cpp
        Source/BandPass/BandPassProcessor.h
        Source/BandPass/BandPassEditor.cpp
        Source/BandPass/BandPassEditor.h
        Source/BandPass/BandPassPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismBandPass)

# HyperPrism Band-Reject (Notch) Filter Plugin
juce_add_plugin(HyperPrismBandReject
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hbrf
    BUNDLE_ID "com.zqsfx.hyperprism.bandreject"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Band-Reject Filter"
    VST3_CATEGORIES "Fx" "Filter"
)

file(GLOB HyperPrismBandReject_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/BandReject/Presets/*.hppreset)
if(HyperPrismBandReject_PresetFiles)
    juce_add_binary_data(HyperPrismBandRejectPresets SOURCES ${HyperPrismBandReject_PresetFiles})
    target_link_libraries(HyperPrismBandReject PRIVATE HyperPrismBandRejectPresets)
endif()

target_sources(HyperPrismBandReject
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/BandReject/BandRejectProcessor.cpp
        Source/BandReject/BandRejectProcessor.h
        Source/BandReject/BandRejectEditor.cpp
        Source/BandReject/BandRejectEditor.h
        Source/BandReject/BandRejectPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismBandReject)

# HyperPrism Tremolo Plugin
juce_add_plugin(HyperPrismTremolo
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Htrm
    BUNDLE_ID "com.zqsfx.hyperprism.tremolo"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Tremolo"
    VST3_CATEGORIES "Fx" "Modulation"
)

file(GLOB HyperPrismTremolo_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Tremolo/Presets/*.hppreset)
if(HyperPrismTremolo_PresetFiles)
    juce_add_binary_data(HyperPrismTremoloPresets SOURCES ${HyperPrismTremolo_PresetFiles})
    target_link_libraries(HyperPrismTremolo PRIVATE HyperPrismTremoloPresets)
endif()

target_sources(HyperPrismTremolo
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/Tremolo/TremoloProcessor.cpp
        Source/Tremolo/TremoloProcessor.h
        Source/Tremolo/TremoloEditor.cpp
        Source/Tremolo/TremoloEditor.h
        Source/Tremolo/TremoloPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismTremolo)

# HyperPrism Vibrato Plugin
juce_add_plugin(HyperPrismVibrato
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hvib
    BUNDLE_ID "com.zqsfx.hyperprism.vibrato"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Vibrato"
    VST3_CATEGORIES "Fx" "Modulation"
)

file(GLOB HyperPrismVibrato_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Vibrato/Presets/*.hppreset)
if(HyperPrismVibrato_PresetFiles)
    juce_add_binary_data(HyperPrismVibratoPresets SOURCES ${HyperPrismVibrato_PresetFiles})
    target_link_libraries(HyperPrismVibrato PRIVATE HyperPrismVibratoPresets)
endif()

target_sources(HyperPrismVibrato
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/Vibrato/VibratoProcessor.cpp
        Source/Vibrato/VibratoProcessor.h
        Source/Vibrato/VibratoEditor.cpp
        Source/Vibrato/VibratoEditor.h
        Source/Vibrato/VibratoPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismVibrato)

# HyperPrism Phaser Plugin
juce_add_plugin(HyperPrismPhaser
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hphs
    BUNDLE_ID "com.zqsfx.hyperprism.phaser"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Phaser"
    VST3_CATEGORIES "Fx" "Modulation"
)

file(GLOB HyperPrismPhaser_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Phaser/Presets/*.hppreset)
if(HyperPrismPhaser_PresetFiles)
    juce_add_binary_data(HyperPrismPhaserPresets SOURCES ${HyperPrismPhaser_PresetFiles})
    target_link_libraries(HyperPrismPhaser PRIVATE HyperPrismPhaserPresets)
endif()

target_sources(HyperPrismPhaser
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/Phaser/PhaserProcessor.cpp
        Source/Phaser/PhaserProcessor.h
        Source/Phaser/PhaserEditor.cpp
        Source/Phaser/PhaserEditor.h
        Source/Phaser/PhaserPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismPhaser)

# HyperPrism Noise Gate Plugin
juce_add_plugin(HyperPrismNoiseGate
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hngt
    BUNDLE_ID "com.zqsfx.hyperprism.noisegate"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Noise Gate"
    VST3_CATEGORIES "Fx" "Dynamics"
)

file(GLOB HyperPrismNoiseGate_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/NoiseGate/Presets/*.hppreset)
if(HyperPrismNoiseGate_PresetFiles)
    juce_add_binary_data(HyperPrismNoiseGatePresets SOURCES ${HyperPrismNoiseGate_PresetFiles})
    target_link_libraries(HyperPrismNoiseGate PRIVATE HyperPrismNoiseGatePresets)
endif()

target_sources(HyperPrismNoiseGate
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/NoiseGate/NoiseGateProcessor.cpp
        Source/NoiseGate/NoiseGateProcessor.h
        Source/NoiseGate/NoiseGateEditor.cpp
        Source/NoiseGate/NoiseGateEditor.h
        Source/NoiseGate/NoiseGatePlugin.cpp
)

configure_hyperprism_plugin(HyperPrismNoiseGate)

# HyperPrism Compressor Plugin
juce_add_plugin(HyperPrismCompressor
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hcmp
    BUNDLE_ID "com.zqsfx.hyperprism.compressor"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Compressor"
    VST3_CATEGORIES "Fx" "Dynamics"
)

file(GLOB HyperPrismCompressor_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Compressor/Presets/*.hppreset)
if(HyperPrismCompressor_PresetFiles)
    juce_add_binary_data(HyperPrismCompressorPresets SOURCES ${HyperPrismCompressor_PresetFiles})
    target_link_libraries(HyperPrismCompressor PRIVATE HyperPrismCompressorPresets)
endif()

target_sources(HyperPrismCompressor
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/Compressor/CompressorProcessor.cpp
        Source/Compressor/CompressorProcessor.h
        Source/Compressor/CompressorEditor.cpp
        Source/Compressor/CompressorEditor.h
        Source/Compressor/CompressorPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismCompressor)

# HyperPrism Limiter Plugin
juce_add_plugin(HyperPrismLimiter
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hlmt
    BUNDLE_ID "com.zqsfx.hyperprism.limiter"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Limiter"
    VST3_CATEGORIES "Fx" "Dynamics" "Limiter"
)

file(GLOB HyperPrismLimiter_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Limiter/Presets/*.hppreset)
if(HyperPrismLimiter_PresetFiles)
    juce_add_binary_data(HyperPrismLimiterPresets SOURCES ${HyperPrismLimiter_PresetFiles})
    target_link_libraries(HyperPrismLimiter PRIVATE HyperPrismLimiterPresets)
endif()

target_sources(HyperPrismLimiter
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/Limiter/LimiterProcessor.cpp
        Source/Limiter/LimiterProcessor.h
        Source/Limiter/LimiterEditor.cpp
        Source/Limiter/LimiterEditor.h
        Source/Limiter/LimiterPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismLimiter)

# HyperPrism Tube/Tape Saturation Plugin
juce_add_plugin(HyperPrismTubeTapeSaturation
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hsat
    BUNDLE_ID "com.zqsfx.hyperprism.tubetapesaturation"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Tube-Tape Saturation"
    VST3_CATEGORIES "Fx" "Distortion" "Saturation"
)

file(GLOB HyperPrismTubeTapeSaturation_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/TubeTapeSaturation/Presets/*.hppreset)
if(HyperPrismTubeTapeSaturation_PresetFiles)
    juce_add_binary_data(HyperPrismTubeTapeSaturationPresets SOURCES ${HyperPrismTubeTapeSaturation_PresetFiles})
    target_link_libraries(HyperPrismTubeTapeSaturation PRIVATE HyperPrismTubeTapeSaturationPresets)
endif()

target_sources(HyperPrismTubeTapeSaturation
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/TubeTapeSaturation/TubeTapeSaturationProcessor.cpp
        Source/TubeTapeSaturation/TubeTapeSaturationProcessor.h
        Source/TubeTapeSaturation/TubeTapeSaturationEditor.cpp
        Source/TubeTapeSaturation/TubeTapeSaturationEditor.h
        Source/TubeTapeSaturation/TubeTapeSaturationPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismTubeTapeSaturation)

# HyperPrism Harmonic Exciter Plugin
juce_add_plugin(HyperPrismHarmonicExciter
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hhex
    BUNDLE_ID "com.zqsfx.hyperprism.harmonicexciter"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Harmonic Exciter"
    VST3_CATEGORIES "Fx" "Distortion"
)

file(GLOB HyperPrismHarmonicExciter_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/HarmonicExciter/Presets/*.hppreset)
if(HyperPrismHarmonicExciter_PresetFiles)
    juce_add_binary_data(HyperPrismHarmonicExciterPresets SOURCES ${HyperPrismHarmonicExciter_PresetFiles})
    target_link_libraries(HyperPrismHarmonicExciter PRIVATE HyperPrismHarmonicExciterPresets)
endif()

target_sources(HyperPrismHarmonicExciter
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/HarmonicExciter/HarmonicExciterProcessor.cpp
        Source/HarmonicExciter/HarmonicExciterProcessor.h
        Source/HarmonicExciter/HarmonicExciterEditor.cpp
        Source/HarmonicExciter/HarmonicExciterEditor.h
        Source/HarmonicExciter/HarmonicExciterPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismHarmonicExciter)

# HyperPrism Bass Maximiser Plugin
juce_add_plugin(HyperPrismBassMaximiser
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hbmx
    BUNDLE_ID "com.zqsfx.hyperprism.bassmaximiser"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Bass Maximiser"
    VST3_CATEGORIES "Fx" "Distortion"
)

file(GLOB HyperPrismBassMaximiser_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/BassMaximiser/Presets/*.hppreset)
if(HyperPrismBassMaximiser_PresetFiles)
    juce_add_binary_data(HyperPrismBassMaximiserPresets SOURCES ${HyperPrismBassMaximiser_PresetFiles})
    target_link_libraries(HyperPrismBassMaximiser PRIVATE HyperPrismBassMaximiserPresets)
endif()

target_sources(HyperPrismBassMaximiser
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/BassMaximiser/BassMaximiserProcessor.cpp
        Source/BassMaximiser/BassMaximiserProcessor.h
        Source/BassMaximiser/BassMaximiserEditor.cpp
        Source/BassMaximiser/BassMaximiserEditor.h
        Source/BassMaximiser/BassMaximiserPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismBassMaximiser)


# HyperPrism Single Delay Plugin
juce_add_plugin(HyperPrismSingleDelay
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hsdl
    BUNDLE_ID "com.zqsfx.hyperprism.singledelay"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Single Delay"
    VST3_CATEGORIES "Fx" "Delay"
)

file(GLOB HyperPrismSingleDelay_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/SingleDelay/Presets/*.hppreset)
if(HyperPrismSingleDelay_PresetFiles)
    juce_add_binary_data(HyperPrismSingleDelayPresets SOURCES ${HyperPrismSingleDelay_PresetFiles})
    target_link_libraries(HyperPrismSingleDelay PRIVATE HyperPrismSingleDelayPresets)
endif()

target_sources(HyperPrismSingleDelay
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/SingleDelay/SingleDelayProcessor.cpp
        Source/SingleDelay/SingleDelayProcessor.h
        Source/SingleDelay/SingleDelayEditor.cpp
        Source/SingleDelay/SingleDelayEditor.h
        Source/SingleDelay/SingleDelayPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismSingleDelay)

# HyperPrism Multi Delay Plugin
juce_add_plugin(HyperPrismMultiDelay
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hmdl
    BUNDLE_ID "com.zqsfx.hyperprism.multidelay"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Multi Delay"
    VST3_CATEGORIES "Fx" "Delay"
)

file(GLOB HyperPrismMultiDelay_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/MultiDelay/Presets/*.hppreset)
if(HyperPrismMultiDelay_PresetFiles)
    juce_add_binary_data(HyperPrismMultiDelayPresets SOURCES ${HyperPrismMultiDelay_PresetFiles})
    target_link_libraries(HyperPrismMultiDelay PRIVATE HyperPrismMultiDelayPresets)
endif()

target_sources(HyperPrismMultiDelay
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/MultiDelay/MultiDelayProcessor.cpp
        Source/MultiDelay/MultiDelayProcessor.h
        Source/MultiDelay/MultiDelayEditor.cpp
        Source/MultiDelay/MultiDelayEditor.h
        Source/MultiDelay/MultiDelayPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismMultiDelay)

# HyperPrism Pan Plugin
juce_add_plugin(HyperPrismPan
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hpan
    BUNDLE_ID "com.zqsfx.hyperprism.pan"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Pan"
    VST3_CATEGORIES "Fx" "Spatial"
)

file(GLOB HyperPrismPan_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Pan/Presets/*.hppreset)
if(HyperPrismPan_PresetFiles)
    juce_add_binary_data(HyperPrismPanPresets SOURCES ${HyperPrismPan_PresetFiles})
    target_link_libraries(HyperPrismPan PRIVATE HyperPrismPanPresets)
endif()

target_sources(HyperPrismPan
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/Pan/PanProcessor.cpp
        Source/Pan/PanProcessor.h
        Source/Pan/PanEditor.cpp
        Source/Pan/PanEditor.h
        Source/Pan/PanPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismPan)

# HyperPrism Auto Pan Plugin
juce_add_plugin(HyperPrismAutoPan
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hapn
    BUNDLE_ID "com.zqsfx.hyperprism.autopan"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Auto Pan"
    VST3_CATEGORIES "Fx" "Spatial" "Modulation"
)

file(GLOB HyperPrismAutoPan_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/AutoPan/Presets/*.hppreset)
if(HyperPrismAutoPan_PresetFiles)
    juce_add_binary_data(HyperPrismAutoPanPresets SOURCES ${HyperPrismAutoPan_PresetFiles})
    target_link_libraries(HyperPrismAutoPan PRIVATE HyperPrismAutoPanPresets)
endif()

target_sources(HyperPrismAutoPan
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/AutoPan/AutoPanProcessor.cpp
        Source/AutoPan/AutoPanProcessor.h
        Source/AutoPan/AutoPanEditor.cpp
        Source/AutoPan/AutoPanEditor.h
        Source/AutoPan/AutoPanPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismAutoPan)

# HyperPrism Quasi Stereo Plugin
juce_add_plugin(HyperPrismQuasiStereo
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hqst
    BUNDLE_ID "com.zqsfx.hyperprism.quasistereo"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Quasi Stereo"
    VST3_CATEGORIES "Fx" "Spatial"
)

file(GLOB HyperPrismQuasiStereo_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/QuasiStereo/Presets/*.hppreset)
if(HyperPrismQuasiStereo_PresetFiles)
    juce_add_binary_data(HyperPrismQuasiStereoPresets SOURCES ${HyperPrismQuasiStereo_PresetFiles})
    target_link_libraries(HyperPrismQuasiStereo PRIVATE HyperPrismQuasiStereoPresets)
endif()

target_sources(HyperPrismQuasiStereo
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/QuasiStereo/QuasiStereoProcessor.cpp
        Source/QuasiStereo/QuasiStereoProcessor.h
        Source/QuasiStereo/QuasiStereoEditor.cpp
        Source/QuasiStereo/QuasiStereoEditor.h
        Source/QuasiStereo/QuasiStereoPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismQuasiStereo)

# HyperPrism More Stereo Plugin
juce_add_plugin(HyperPrismMoreStereo
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hmst
    BUNDLE_ID "com.zqsfx.hyperprism.morestereo"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined More Stereo"
    VST3_CATEGORIES "Fx" "Spatial"
)

file(GLOB HyperPrismMoreStereo_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/MoreStereo/Presets/*.hppreset)
if(HyperPrismMoreStereo_PresetFiles)
    juce_add_binary_data(HyperPrismMoreStereoPresets SOURCES ${HyperPrismMoreStereo_PresetFiles})
    target_link_libraries(HyperPrismMoreStereo PRIVATE HyperPrismMoreStereoPresets)
endif()

target_sources(HyperPrismMoreStereo
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/MoreStereo/MoreStereoProcessor.cpp
        Source/MoreStereo/MoreStereoProcessor.h
        Source/MoreStereo/MoreStereoEditor.cpp
        Source/MoreStereo/MoreStereoEditor.h
        Source/MoreStereo/MoreStereoPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismMoreStereo)

# HyperPrism M+S Matrix Plugin
juce_add_plugin(HyperPrismMSMatrix
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hmsm
    BUNDLE_ID "com.zqsfx.hyperprism.msmatrix"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined M+S Matrix"
    VST3_CATEGORIES "Fx" "Spatial"
)

file(GLOB HyperPrismMSMatrix_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/MSMatrix/Presets/*.hppreset)
if(HyperPrismMSMatrix_PresetFiles)
    juce_add_binary_data(HyperPrismMSMatrixPresets SOURCES ${HyperPrismMSMatrix_PresetFiles})
    target_link_libraries(HyperPrismMSMatrix PRIVATE HyperPrismMSMatrixPresets)
endif()

target_sources(HyperPrismMSMatrix
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/MSMatrix/MSMatrixProcessor.cpp
        Source/MSMatrix/MSMatrixProcessor.h
        Source/MSMatrix/MSMatrixEditor.cpp
        Source/MSMatrix/MSMatrixEditor.h
        Source/MSMatrix/MSMatrixPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismMSMatrix)

# HyperPrism Stereo Dynamics Plugin
juce_add_plugin(HyperPrismStereoDynamics
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hsdy
    BUNDLE_ID "com.zqsfx.hyperprism.stereodynamics"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Stereo Dynamics"
    VST3_CATEGORIES "Fx" "Dynamics" "Spatial"
)

file(GLOB HyperPrismStereoDynamics_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/StereoDynamics/Presets/*.hppreset)
if(HyperPrismStereoDynamics_PresetFiles)
    juce_add_binary_data(HyperPrismStereoDynamicsPresets SOURCES ${HyperPrismStereoDynamics_PresetFiles})
    target_link_libraries(HyperPrismStereoDynamics PRIVATE HyperPrismStereoDynamicsPresets)
endif()

target_sources(HyperPrismStereoDynamics
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/StereoDynamics/StereoDynamicsProcessor.cpp
        Source/StereoDynamics/StereoDynamicsProcessor.h
        Source/StereoDynamics/StereoDynamicsEditor.cpp
        Source/StereoDynamics/StereoDynamicsEditor.h
        Source/StereoDynamics/StereoDynamicsPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismStereoDynamics)

# HyperPrism Frequency Shifter Plugin
juce_add_plugin(HyperPrismFrequencyShifter
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hfrs
    BUNDLE_ID "com.zqsfx.hyperprism.frequencyshifter"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Frequency Shifter"
    VST3_CATEGORIES "Fx" "Pitch Shift" "Modulation"
)

file(GLOB HyperPrismFrequencyShifter_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/FrequencyShifter/Presets/*.hppreset)
if(HyperPrismFrequencyShifter_PresetFiles)
    juce_add_binary_data(HyperPrismFrequencyShifterPresets SOURCES ${HyperPrismFrequencyShifter_PresetFiles})
    target_link_libraries(HyperPrismFrequencyShifter PRIVATE HyperPrismFrequencyShifterPresets)
endif()

target_sources(HyperPrismFrequencyShifter
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/FrequencyShifter/FrequencyShifterProcessor.cpp
        Source/FrequencyShifter/FrequencyShifterProcessor.h
        Source/FrequencyShifter/FrequencyShifterEditor.cpp
        Source/FrequencyShifter/FrequencyShifterEditor.h
        Source/FrequencyShifter/FrequencyShifterPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismFrequencyShifter)

# HyperPrism Vocoder Plugin
juce_add_plugin(HyperPrismVocoder
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hvoc
    BUNDLE_ID "com.zqsfx.hyperprism.vocoder"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Vocoder"
    VST3_CATEGORIES "Fx" "Modulation"
)

file(GLOB HyperPrismVocoder_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Vocoder/Presets/*.hppreset)
if(HyperPrismVocoder_PresetFiles)
    juce_add_binary_data(HyperPrismVocoderPresets SOURCES ${HyperPrismVocoder_PresetFiles})
    target_link_libraries(HyperPrismVocoder PRIVATE HyperPrismVocoderPresets)
endif()

target_sources(HyperPrismVocoder
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/Vocoder/VocoderProcessor.cpp
        Source/Vocoder/VocoderProcessor.h
        Source/Vocoder/VocoderEditor.cpp
        Source/Vocoder/VocoderEditor.h
        Source/Vocoder/VocoderPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismVocoder)

# HyperPrism Pitch Changer Plugin
juce_add_plugin(HyperPrismPitchChanger
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hpch
    BUNDLE_ID "com.zqsfx.hyperprism.pitchchanger"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Pitch Changer"
    VST3_CATEGORIES "Fx" "Pitch Shift"
)

file(GLOB HyperPrismPitchChanger_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/PitchChanger/Presets/*.hppreset)
if(HyperPrismPitchChanger_PresetFiles)
    juce_add_binary_data(HyperPrismPitchChangerPresets SOURCES ${HyperPrismPitchChanger_PresetFiles})
    target_link_libraries(HyperPrismPitchChanger PRIVATE HyperPrismPitchChangerPresets)
endif()

target_sources(HyperPrismPitchChanger
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/PitchChanger/PitchChangerProcessor.cpp
        Source/PitchChanger/PitchChangerProcessor.h
        Source/PitchChanger/PitchChangerEditor.cpp
        Source/PitchChanger/PitchChangerEditor.h
        Source/PitchChanger/PitchChangerPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismPitchChanger)

# HyperPrism Sonic Decimator Plugin
juce_add_plugin(HyperPrismSonicDecimator
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hsdc
    BUNDLE_ID "com.zqsfx.hyperprism.sonicdecimator"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Sonic Decimator"
    VST3_CATEGORIES "Fx" "Distortion"
)

file(GLOB HyperPrismSonicDecimator_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/SonicDecimator/Presets/*.hppreset)
if(HyperPrismSonicDecimator_PresetFiles)
    juce_add_binary_data(HyperPrismSonicDecimatorPresets SOURCES ${HyperPrismSonicDecimator_PresetFiles})
    target_link_libraries(HyperPrismSonicDecimator PRIVATE HyperPrismSonicDecimatorPresets)
endif()

target_sources(HyperPrismSonicDecimator
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/SonicDecimator/SonicDecimatorProcessor.cpp
        Source/SonicDecimator/SonicDecimatorProcessor.h
        Source/SonicDecimator/SonicDecimatorEditor.cpp
        Source/SonicDecimator/SonicDecimatorEditor.h
        Source/SonicDecimator/SonicDecimatorPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismSonicDecimator)

# HyperPrism Ring Modulator Plugin
juce_add_plugin(HyperPrismRingModulator
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hrmd
    BUNDLE_ID "com.zqsfx.hyperprism.ringmodulator"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Ring Modulator"
    VST3_CATEGORIES "Fx" "Modulation"
)

file(GLOB HyperPrismRingModulator_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/RingModulator/Presets/*.hppreset)
if(HyperPrismRingModulator_PresetFiles)
    juce_add_binary_data(HyperPrismRingModulatorPresets SOURCES ${HyperPrismRingModulator_PresetFiles})
    target_link_libraries(HyperPrismRingModulator PRIVATE HyperPrismRingModulatorPresets)
endif()

target_sources(HyperPrismRingModulator
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/RingModulator/RingModulatorProcessor.cpp
        Source/RingModulator/RingModulatorProcessor.h
        Source/RingModulator/RingModulatorEditor.cpp
        Source/RingModulator/RingModulatorEditor.h
        Source/RingModulator/RingModulatorPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismRingModulator)

# HyperPrism HyperPhaser Plugin
juce_add_plugin(HyperPrismHyperPhaser
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hhph
    BUNDLE_ID "com.zqsfx.hyperprism.hyperphaser"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined HyperPhaser"
    VST3_CATEGORIES "Fx" "Modulation"
)

file(GLOB HyperPrismHyperPhaser_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/HyperPhaser/Presets/*.hppreset)
if(HyperPrismHyperPhaser_PresetFiles)
    juce_add_binary_data(HyperPrismHyperPhaserPresets SOURCES ${HyperPrismHyperPhaser_PresetFiles})
    target_link_libraries(HyperPrismHyperPhaser PRIVATE HyperPrismHyperPhaserPresets)
endif()

target_sources(HyperPrismHyperPhaser
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/HyperPhaser/HyperPhaserProcessor.cpp
        Source/HyperPhaser/HyperPhaserProcessor.h
        Source/HyperPhaser/HyperPhaserEditor.cpp
        Source/HyperPhaser/HyperPhaserEditor.h
        Source/HyperPhaser/HyperPhaserPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismHyperPhaser)

# HyperPrism Echo Plugin
juce_add_plugin(HyperPrismEcho
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    IS_SYNTH FALSE
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE Hech
    BUNDLE_ID "com.zqsfx.hyperprism.echo"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Echo"
    VST3_CATEGORIES "Fx" "Delay"
)

file(GLOB HyperPrismEcho_PresetFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Echo/Presets/*.hppreset)
if(HyperPrismEcho_PresetFiles)
    juce_add_binary_data(HyperPrismEchoPresets SOURCES ${HyperPrismEcho_PresetFiles})
    target_link_libraries(HyperPrismEcho PRIVATE HyperPrismEchoPresets)
endif()

target_sources(HyperPrismEcho
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/PresetManager.cpp
        Source/Shared/PresetManager.h
        Source/Shared/PresetBar.cpp
        Source/Shared/PresetBar.h
        Source/Echo/EchoProcessor.cpp
        Source/Echo/EchoProcessor.h
        Source/Echo/EchoEditor.cpp
        Source/Echo/EchoEditor.h
        Source/Echo/EchoPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismEcho)

# All 32 HyperPrism plugins are now implemented!

# Check tools (UI snapshots, state/preset/DSP checks) and their CTest registrations. ON by
# default. Windows CI turns it off: the ~70 check executables pushed the Windows job past its
# time limit, and the checks exercise platform-independent DSP that macOS and Linux CI run.
option(HP_BUILD_CHECKS "Build the HyperPrism check tools and register them with CTest" ON)
if(HP_BUILD_CHECKS)

# ---- UI snapshot tool (headless before/after render gate for the zqsfx_ui house-UI migration;
# see tools/ui_snapshot/main.cpp). Piloted for three plugins:
# Compressor (XY pad + dynamics), Chorus (modulation), BandPass (frequency). See
# tools/ui_snapshot/main.cpp's header comment for why this is one function per plugin rather
# than a single "hyperprism_ui_snapshot" binary.
function(add_hyperprism_ui_snapshot plugin_target effect_dir processor_header processor_class)
    set(snapshot_target "hyperprism_ui_snapshot_${plugin_target}")
    juce_add_console_app(${snapshot_target} PRODUCT_NAME "${snapshot_target}")
    target_sources(${snapshot_target} PRIVATE tools/ui_snapshot/main.cpp)
    target_compile_definitions(${snapshot_target} PRIVATE
        "HP_SNAPSHOT_PROCESSOR_HEADER=\"${processor_header}\""
        "HP_SNAPSHOT_PROCESSOR_CLASS=${processor_class}")
    # juce_generate_juce_header(${plugin_target})'s generated JuceHeader.h is only added to the
    # plugin target's OWN include path as PRIVATE (see JUCEUtils.cmake), so it does not
    # propagate through target_link_libraries; added back explicitly here (the LFlOw migration's
    # "JUCE_GENERATED_SOURCES_DIRECTORY propagation trick", documented internally).
    target_include_directories(${snapshot_target} PRIVATE
        Source/${effect_dir}
        Source/Shared
        $<TARGET_PROPERTY:${plugin_target},JUCE_GENERATED_SOURCES_DIRECTORY>)
    target_link_libraries(${snapshot_target} PRIVATE
        ${plugin_target}
        zqsfx::ui
        juce::juce_audio_utils
        juce::juce_dsp)
    # ${plugin_target}'s own JuceHeader.h #includes "BinaryData.h" once any target links its
    # juce_add_binary_data() library (JUCE_TARGET_HAS_BINARY_DATA), and this tool compiles that
    # same generated JuceHeader.h itself -- but a PRIVATE link only reaches the direct linker's
    # own compilation, so this tool needs the binary-data library linked here too, not just on
    # ${plugin_target}, whenever that plugin has any Presets/*.hppreset to compile in.
    if(TARGET ${plugin_target}Presets)
        target_link_libraries(${snapshot_target} PRIVATE ${plugin_target}Presets)
    endif()
endfunction()

add_hyperprism_ui_snapshot(HyperPrismCompressor    Compressor     CompressorProcessor.h     CompressorProcessor)
add_hyperprism_ui_snapshot(HyperPrismChorus        Chorus         ChorusProcessor.h         ChorusProcessor)
add_hyperprism_ui_snapshot(HyperPrismBandPass      BandPass       BandPassProcessor.h       BandPassProcessor)
add_hyperprism_ui_snapshot(HyperPrismMultiDelay    MultiDelay     MultiDelayProcessor.h     MultiDelayProcessor)
# Added with the SonicDecimator Anti-Alias/Dither ParameterLabel visibility fix (see
# hp_label_visibility_SonicDecimator below and CHANGELOG) as the before/after render gate.
add_hyperprism_ui_snapshot(HyperPrismSonicDecimator SonicDecimator SonicDecimatorProcessor.h SonicDecimatorProcessor)

# ---- State-migration checks (HarmonicExciter and NoiseGate moved from addParameter to APVTS
# on 2026-09-23; see tools/state_check/main.cpp). Registered with CTest: HyperPrism's first
# tests. Same link pattern as add_hyperprism_ui_snapshot().
enable_testing()
function(add_hyperprism_state_check plugin_target effect_dir processor_header processor_class flag)
    string(REPLACE "HyperPrism" "" effect_short "${plugin_target}")
    set(check_target "hp_state_${effect_short}")   # short: Windows MAX_PATH on the generated .rc path
    juce_add_console_app(${check_target} PRODUCT_NAME "${check_target}")
    target_sources(${check_target} PRIVATE tools/state_check/main.cpp)
    target_compile_definitions(${check_target} PRIVATE
        "HP_CHECK_PROCESSOR_HEADER=\"${processor_header}\""
        "HP_CHECK_PROCESSOR_CLASS=${processor_class}"
        "${flag}=1")
    target_include_directories(${check_target} PRIVATE
        Source/${effect_dir}
        Source/Shared
        $<TARGET_PROPERTY:${plugin_target},JUCE_GENERATED_SOURCES_DIRECTORY>)
    target_link_libraries(${check_target} PRIVATE
        ${plugin_target}
        zqsfx::ui
        juce::juce_audio_utils
        juce::juce_dsp)
    add_test(NAME ${check_target} COMMAND ${check_target})
endfunction()

add_hyperprism_state_check(HyperPrismHarmonicExciter HarmonicExciter HarmonicExciterProcessor.h HarmonicExciterProcessor HP_CHECK_HARMONIC_EXCITER)
add_hyperprism_state_check(HyperPrismNoiseGate       NoiseGate       NoiseGateProcessor.h       NoiseGateProcessor       HP_CHECK_NOISE_GATE)

# ---- Preset round-trip checks (hp::PresetManager, Source/Shared/PresetManager.h/.cpp; see
# tools/preset_roundtrip/main.cpp). Piloted for the same three plugins as the UI snapshot tool
# above. Same link pattern as add_hyperprism_ui_snapshot()/add_hyperprism_state_check(); the
# extra apvts_kind argument is "member" for a plugin whose APVTS is a public `apvts` member
# (Compressor) or "accessor" for one exposing `getValueTreeState()` (Chorus, BandPass).
function(add_hyperprism_preset_roundtrip plugin_target effect_dir processor_header processor_class apvts_kind param_a param_b)
    string(REPLACE "HyperPrism" "" effect_short "${plugin_target}")
    set(check_target "hp_preset_${effect_short}")  # short: Windows MAX_PATH on the generated .rc path
    if(apvts_kind STREQUAL "member")
        set(apvts_is_member 1)
    else()
        set(apvts_is_member 0)
    endif()
    juce_add_console_app(${check_target} PRODUCT_NAME "${check_target}")
    target_sources(${check_target} PRIVATE tools/preset_roundtrip/main.cpp)
    target_compile_definitions(${check_target} PRIVATE
        "HP_ROUNDTRIP_PROCESSOR_HEADER=\"${processor_header}\""
        "HP_ROUNDTRIP_PROCESSOR_CLASS=${processor_class}"
        "HP_ROUNDTRIP_APVTS_IS_MEMBER=${apvts_is_member}"
        "HP_ROUNDTRIP_PARAM_A=\"${param_a}\""
        "HP_ROUNDTRIP_PARAM_B=\"${param_b}\"")
    target_include_directories(${check_target} PRIVATE
        Source/${effect_dir}
        Source/Shared
        $<TARGET_PROPERTY:${plugin_target},JUCE_GENERATED_SOURCES_DIRECTORY>)
    target_link_libraries(${check_target} PRIVATE
        ${plugin_target}
        zqsfx::ui
        juce::juce_audio_utils
        juce::juce_dsp)
    # See add_hyperprism_ui_snapshot()'s comment above: this tool needs ${plugin_target}'s
    # binary-data library linked here too, not just on ${plugin_target}, whenever that plugin
    # has any Presets/*.hppreset compiled in.
    if(TARGET ${plugin_target}Presets)
        target_link_libraries(${check_target} PRIVATE ${plugin_target}Presets)
    endif()
    add_test(NAME ${check_target} COMMAND ${check_target})
endfunction()

add_hyperprism_preset_roundtrip(HyperPrismCompressor Compressor CompressorProcessor.h CompressorProcessor member   threshold  ratio)
add_hyperprism_preset_roundtrip(HyperPrismChorus     Chorus     ChorusProcessor.h     ChorusProcessor     accessor rate       depth)
add_hyperprism_preset_roundtrip(HyperPrismBandPass   BandPass   BandPassProcessor.h   BandPassProcessor   accessor centerFreq bandwidth)


# ---- Stereo null check (identical mono input must give identical L and R; added with the
# SonicDecimator per-channel state fix, 2026-09-23) and the oversampling alias/latency check
# (tools/oversampling_check/main.cpp). Same link pattern as add_hyperprism_state_check().
function(add_hyperprism_console_check kind plugin_target effect_dir processor_header processor_class flag)
    string(REPLACE "HyperPrism" "" effect_short "${plugin_target}")
    string(REPLACE "_check" "" kind_short "${kind}")
    set(check_target "hp_${kind_short}_${effect_short}")  # short: Windows MAX_PATH on the generated .rc path
    juce_add_console_app(${check_target} PRODUCT_NAME "${check_target}")
    target_sources(${check_target} PRIVATE tools/${kind}/main.cpp)
    target_compile_definitions(${check_target} PRIVATE
        "HP_CHECK_PROCESSOR_HEADER=\"${processor_header}\""
        "HP_CHECK_PROCESSOR_CLASS=${processor_class}"
        "${flag}=1")
    target_include_directories(${check_target} PRIVATE
        Source/${effect_dir}
        Source/Shared
        $<TARGET_PROPERTY:${plugin_target},JUCE_GENERATED_SOURCES_DIRECTORY>)
    target_link_libraries(${check_target} PRIVATE
        ${plugin_target}
        zqsfx::ui
        juce::juce_audio_utils
        juce::juce_dsp)
    if(TARGET ${plugin_target}Presets)
        target_link_libraries(${check_target} PRIVATE ${plugin_target}Presets)
    endif()
    add_test(NAME ${check_target} COMMAND ${check_target})
endfunction()

add_hyperprism_console_check(stereo_null_check HyperPrismSonicDecimator SonicDecimator SonicDecimatorProcessor.h SonicDecimatorProcessor HP_CHECK_SONICDECIMATOR)
add_hyperprism_console_check(oversampling_check HyperPrismSonicDecimator     SonicDecimator     SonicDecimatorProcessor.h     SonicDecimatorProcessor     HP_CHECK_SONICDECIMATOR)
add_hyperprism_console_check(oversampling_check HyperPrismTubeTapeSaturation TubeTapeSaturation TubeTapeSaturationProcessor.h TubeTapeSaturationProcessor HP_CHECK_TUBETAPE)
add_hyperprism_console_check(oversampling_check HyperPrismHarmonicExciter    HarmonicExciter    HarmonicExciterProcessor.h    HarmonicExciterProcessor    HP_CHECK_HARMONICEXCITER)

# ---- Defect-fix regression checks (this pass): hp_release_time_Limiter proves the Release
# parameter now actually controls release time (tools/release_time_check/main.cpp);
# hp_frequency_sweep_BassMaximiser proves the Frequency crossover is audible, i.e. that removing
# processBassCompression()'s unused `frequency` argument was a true no-op refactor
# (tools/frequency_sweep_check/main.cpp); hp_label_visibility_SonicDecimator proves the
# Anti-Alias/Dither ParameterLabels are now visible with a real hit target
# (tools/label_visibility_check/main.cpp). Same link pattern as add_hyperprism_state_check().
add_hyperprism_console_check(release_time_check     HyperPrismLimiter        Limiter        LimiterProcessor.h        LimiterProcessor        HP_CHECK_LIMITER)
add_hyperprism_console_check(frequency_sweep_check   HyperPrismBassMaximiser  BassMaximiser  BassMaximiserProcessor.h  BassMaximiserProcessor  HP_CHECK_BASSMAXIMISER)
add_hyperprism_console_check(label_visibility_check  HyperPrismSonicDecimator SonicDecimator SonicDecimatorProcessor.h SonicDecimatorProcessor HP_CHECK_SONICDECIMATOR)

# ---- MultiDelay echo timing (tools/echo_timing_check/main.cpp): every tap is read once per
# sample, so each echo lands at its configured delay and renders are block-size invariant.
add_hyperprism_console_check(echo_timing_check HyperPrismMultiDelay MultiDelay MultiDelayProcessor.h MultiDelayProcessor HP_CHECK_MULTIDELAY)

# ---- 2026-10-01 backlog checks. These link only ${plugin_target}'s shared-code library, which
# already contains every JUCE module compiled for that plugin, and take its compile settings
# from it, instead of compiling the JUCE modules again per tool (about 160 s of CPU each, times
# 70 tools). Same naming as add_hyperprism_console_check() (short: Windows MAX_PATH).
function(add_hyperprism_light_check kind plugin_target effect_dir processor_header processor_class flag)
    string(REPLACE "HyperPrism" "" effect_short "${plugin_target}")
    string(REPLACE "_check" "" kind_short "${kind}")
    set(check_target "hp_${kind_short}_${effect_short}")
    add_executable(${check_target} tools/${kind}/main.cpp)
    target_compile_features(${check_target} PRIVATE cxx_std_17)
    target_compile_definitions(${check_target} PRIVATE
        $<TARGET_PROPERTY:${plugin_target},COMPILE_DEFINITIONS>
        "HP_CHECK_PROCESSOR_HEADER=\"${processor_header}\""
        "HP_CHECK_PROCESSOR_CLASS=${processor_class}"
        "${flag}=1"
        ${ARGN})
    target_include_directories(${check_target} PRIVATE
        Source/${effect_dir}
        Source/Shared
        $<TARGET_PROPERTY:${plugin_target},INCLUDE_DIRECTORIES>)
    target_link_libraries(${check_target} PRIVATE ${plugin_target})
    if(TARGET ${plugin_target}Presets)
        target_link_libraries(${check_target} PRIVATE ${plugin_target}Presets)
    endif()
    add_test(NAME ${check_target} COMMAND ${check_target})
endfunction()

# hp_rt_alloc_<Effect> (tools/rt_alloc_check/main.cpp): no heap allocation inside processBlock,
# with parameters, block sizes and an oversized host block all changing; every plugin.
set(HYPERPRISM_EFFECTS
    AutoPan BandPass BandReject BassMaximiser Chorus Compressor Delay Echo Flanger
    FrequencyShifter HarmonicExciter HighPass HyperPhaser Limiter LowPass MSMatrix MoreStereo
    MultiDelay NoiseGate Pan Phaser PitchChanger QuasiStereo Reverb RingModulator SingleDelay
    SonicDecimator StereoDynamics Tremolo TubeTapeSaturation Vibrato Vocoder)
foreach(effect IN LISTS HYPERPRISM_EFFECTS)
    add_hyperprism_light_check(rt_alloc_check HyperPrism${effect} ${effect} ${effect}Processor.h ${effect}Processor HP_CHECK_GENERIC)
endforeach()

# hp_smoothing_<Effect> (tools/smoothing_check/main.cpp): the listed continuous parameters that
# feed a gain, mix, frequency, delay time or depth are smoothed. "id:r" = ramp check on noise,
# "id:c:delta" = click check on a sine for delay times, "id:h:delta" = high-frequency check on a
# sine for a delay time whose click a later stage smears (Reverb Pre-Delay),
# "id:r:delta:ms" = ramp check measured
# that many ms after the change (a feedback path's delay, or the Limiter's 20 ms latency). NoiseGate has none (threshold, times and
# range only; its gate gain is already smoothed by attack/release).
# Optional third argument: "id=value,..." settings applied first (HP_SMOOTH_SETUP).
function(add_hyperprism_smoothing_check effect params)
    set(setup_def "")
    if(ARGC GREATER 2)
        set(setup_def "HP_SMOOTH_SETUP=\"${ARGV2}\"")
    endif()
    add_hyperprism_light_check(smoothing_check HyperPrism${effect} ${effect} ${effect}Processor.h ${effect}Processor
        HP_CHECK_GENERIC "HP_SMOOTH_PARAMS=\"${params}\"" ${setup_def})
endfunction()
add_hyperprism_smoothing_check(AutoPan            "depth:r,phase:r,outputLevel:r")
add_hyperprism_smoothing_check(BandPass           "centerFreq:r,bandwidth:r,gain:r,mix:r")
add_hyperprism_smoothing_check(BandReject         "centerFreq:r,q:r,gain:r,mix:r")
add_hyperprism_smoothing_check(BassMaximiser      "frequency:r,boost:r,harmonics:r,outputGain:r")
add_hyperprism_smoothing_check(Chorus             "mix:r,depth:r,feedback:r:0.3:15,delay:c:0.2,lowCut:r,highCut:r")
add_hyperprism_smoothing_check(Compressor         "makeupGain:r,mix:r")
add_hyperprism_smoothing_check(Delay              "mix:r,delayTime:c:0.02,feedback:r:0.3:125,lowCut:r,highCut:r,stereoOffset:c:0.02")
add_hyperprism_smoothing_check(Echo               "delay:c:0.02,feedback:r:0.3:250,mix:r")
add_hyperprism_smoothing_check(Flanger            "mix:r,depth:r,feedback:r:0.3:5,delay:c:0.2,lowCut:r,highCut:r")
add_hyperprism_smoothing_check(FrequencyShifter   "mix:r,outputLevel:r" "frequencyShift=300")
add_hyperprism_smoothing_check(HarmonicExciter    "drive:r,frequency:r,harmonics:r,mix:r")
add_hyperprism_smoothing_check(HighPass           "frequency:r,resonance:r,gain:r,mix:r")
add_hyperprism_smoothing_check(HyperPhaser        "baseFreq:r,peakNotchDepth:r,bandwidth:r,feedback:r,mix:r")
add_hyperprism_smoothing_check(Limiter            "inputgain:r:0.3:20,ceiling:r:0.5:20")
add_hyperprism_smoothing_check(LowPass            "frequency:r,resonance:r,gain:r,mix:r")
add_hyperprism_smoothing_check(MSMatrix           "midLevel:r,sideLevel:r,stereoBalance:r,outputLevel:r")
add_hyperprism_smoothing_check(MoreStereo         "width:r,bassMono:r,crossoverFreq:r,stereoEnhance:r,ambience:r,outputLevel:r")
add_hyperprism_smoothing_check(MultiDelay         "masterMix:r,globalFeedback:r:0.3:125,delay1Time:c:0.02,delay1Level:r,delay1Pan:r,delay1Feedback:r:0.3:125,delay2Time:c:0.02,delay2Level:r,delay2Pan:r,delay2Feedback:r:0.3:250,delay3Time:c:0.02,delay3Level:r,delay3Pan:r,delay4Time:c:0.02,delay4Level:r,delay4Pan:r")
add_hyperprism_smoothing_check(Pan                "panPosition:r,width:r,balance:r,outputLevel:r")
add_hyperprism_smoothing_check(Phaser             "depth:r,feedback:r,mix:r")
add_hyperprism_smoothing_check(PitchChanger       "mix:r,outputLevel:r" "pitchShift=7")
add_hyperprism_smoothing_check(QuasiStereo        "width:r,delayTime:c:0.02,phaseShift:r,highFreqEnhance:r,outputLevel:r")
add_hyperprism_smoothing_check(Reverb             "mix:r,width:r,lowCut:r,highCut:r,preDelay:h:0.02" "mix=100")
add_hyperprism_smoothing_check(RingModulator      "mix:r")
add_hyperprism_smoothing_check(SingleDelay        "delayTime:c:0.02,feedback:r:0.3:250,wetDryMix:r,highCut:r,lowCut:r,stereoSpread:c:0.6")
add_hyperprism_smoothing_check(SonicDecimator     "mix:r,outputLevel:r")
add_hyperprism_smoothing_check(StereoDynamics     "outputLevel:r")
add_hyperprism_smoothing_check(Tremolo            "depth:r,mix:r")
add_hyperprism_smoothing_check(TubeTapeSaturation "drive:r,warmth:r,brightness:r,outputLevel:r")
add_hyperprism_smoothing_check(Vibrato            "mix:r,depth:r,delay:c:0.2,feedback:r:0.3:5")
add_hyperprism_smoothing_check(Vocoder            "outputLevel:r")

# Plugin-specific behaviour checks for the same pass (each tool's header comment says what it
# proves): shared/static DSP state, flat crossover sums and BassMaximiser Harmonics, the
# Vocoder filter bank, and lookahead latency reporting.
add_hyperprism_light_check(isolation_check HyperPrismPhaser             Phaser             PhaserProcessor.h             PhaserProcessor             HP_CHECK_PHASER)
add_hyperprism_light_check(isolation_check HyperPrismTubeTapeSaturation TubeTapeSaturation TubeTapeSaturationProcessor.h TubeTapeSaturationProcessor HP_CHECK_TUBETAPE)
add_hyperprism_light_check(crossover_check HyperPrismBassMaximiser      BassMaximiser      BassMaximiserProcessor.h      BassMaximiserProcessor      HP_CHECK_BASSMAXIMISER)
add_hyperprism_light_check(crossover_check HyperPrismMoreStereo         MoreStereo         MoreStereoProcessor.h         MoreStereoProcessor         HP_CHECK_MORESTEREO)
add_hyperprism_light_check(vocoder_check   HyperPrismVocoder            Vocoder            VocoderProcessor.h            VocoderProcessor            HP_CHECK_VOCODER)
add_hyperprism_light_check(lookahead_check HyperPrismNoiseGate          NoiseGate          NoiseGateProcessor.h          NoiseGateProcessor          HP_CHECK_NOISEGATE)
add_hyperprism_light_check(lookahead_check HyperPrismLimiter            Limiter            LimiterProcessor.h            LimiterProcessor            HP_CHECK_LIMITER)

# Follow-up checks (2026-10-01, second pass): block-size invariance (Tremolo's stereo LFO phase),
# fixed internal delays in ms at every sample rate (More Stereo ambience, Quasi Stereo's 50 ms
# line), smoothers starting at the current values (Echo), and Pitch Changer's reported latency.
add_hyperprism_light_check(blocksize_check     HyperPrismTremolo      Tremolo      TremoloProcessor.h      TremoloProcessor      HP_CHECK_TREMOLO)
add_hyperprism_light_check(delay_time_check    HyperPrismMoreStereo   MoreStereo   MoreStereoProcessor.h   MoreStereoProcessor   HP_CHECK_MORESTEREO)
add_hyperprism_light_check(delay_time_check    HyperPrismQuasiStereo  QuasiStereo  QuasiStereoProcessor.h  QuasiStereoProcessor  HP_CHECK_QUASISTEREO)
add_hyperprism_light_check(start_check         HyperPrismEcho         Echo         EchoProcessor.h         EchoProcessor         HP_CHECK_ECHO)
add_hyperprism_light_check(pitch_latency_check HyperPrismPitchChanger PitchChanger PitchChangerProcessor.h PitchChangerProcessor HP_CHECK_PITCHCHANGER)

# hp_dial_<Effect> (tools/dial_check/main.cpp): every knob bound to a parameter is a
# zqsfx::ui::Dial (Tab focus, focus ring, arrow and Shift+arrow steps) and double-clicks back to
# that parameter's default; every plugin. Opens the editor, so Linux CI skips it like
# hp_label_visibility_ (.github/workflows/build.yml).
foreach(effect IN LISTS HYPERPRISM_EFFECTS)
    add_hyperprism_light_check(dial_check HyperPrism${effect} ${effect} ${effect}Processor.h ${effect}Processor HP_CHECK_GENERIC)
endforeach()

endif() # HP_BUILD_CHECKS
