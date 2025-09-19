cmake_minimum_required(VERSION 3.25)

project(HyperPrismReimagined VERSION 1.0.0)

set_property(GLOBAL PROPERTY USE_FOLDERS YES)

option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Enable Module Source Groups" ON)

# Adding the JUCE subfolder
add_subdirectory(JUCE)

# Define plugin formats to build
if(APPLE)
    set(PLUGIN_FORMATS AU VST3 Standalone)
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
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
    )
endfunction()

# HyperPrism Delay Plugin
juce_add_plugin(HyperPrismDelay
    COMPANY_NAME "ZQ SFX"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hdly
    BUNDLE_ID "com.RevivalProject.HyperPrismDelay"
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

target_sources(HyperPrismDelay
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hrvb
    BUNDLE_ID "com.RevivalProject.HyperPrismReverb"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Reverb"
    VST3_CATEGORIES "Fx" "Reverb"
)

target_sources(HyperPrismReverb
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hchr
    BUNDLE_ID "com.RevivalProject.HyperPrismChorus"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Chorus"
    VST3_CATEGORIES "Fx" "Modulation"
)

target_sources(HyperPrismChorus
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hflg
    BUNDLE_ID "com.RevivalProject.HyperPrismFlanger"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Flanger"
    VST3_CATEGORIES "Fx" "Modulation"
)

target_sources(HyperPrismFlanger
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hhpf
    BUNDLE_ID "com.RevivalProject.HyperPrismHighPass"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined High-Pass Filter"
    VST3_CATEGORIES "Fx" "Filter"
)

target_sources(HyperPrismHighPass
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hlpf
    BUNDLE_ID "com.RevivalProject.HyperPrismLowPass"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Low-Pass Filter"
    VST3_CATEGORIES "Fx" "Filter"
)

target_sources(HyperPrismLowPass
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hbpf
    BUNDLE_ID "com.RevivalProject.HyperPrismBandPass"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Band-Pass Filter"
    VST3_CATEGORIES "Fx" "Filter"
)

target_sources(HyperPrismBandPass
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hbrf
    BUNDLE_ID "com.RevivalProject.HyperPrismBandReject"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Band-Reject Filter"
    VST3_CATEGORIES "Fx" "Filter"
)

target_sources(HyperPrismBandReject
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Htrm
    BUNDLE_ID "com.RevivalProject.HyperPrismTremolo"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Tremolo"
    VST3_CATEGORIES "Fx" "Modulation"
)

target_sources(HyperPrismTremolo
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hvib
    BUNDLE_ID "com.RevivalProject.HyperPrismVibrato"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Vibrato"
    VST3_CATEGORIES "Fx" "Modulation"
)

target_sources(HyperPrismVibrato
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hphs
    BUNDLE_ID "com.RevivalProject.HyperPrismPhaser"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Phaser"
    VST3_CATEGORIES "Fx" "Modulation"
)

target_sources(HyperPrismPhaser
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hngt
    BUNDLE_ID "com.RevivalProject.HyperPrismNoiseGate"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Noise Gate"
    VST3_CATEGORIES "Fx" "Dynamics"
)

target_sources(HyperPrismNoiseGate
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hcmp
    BUNDLE_ID "com.RevivalProject.HyperPrismCompressor"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Compressor"
    VST3_CATEGORIES "Fx" "Dynamics"
)

target_sources(HyperPrismCompressor
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hlmt
    BUNDLE_ID "com.RevivalProject.HyperPrismLimiter"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Limiter"
    VST3_CATEGORIES "Fx" "Dynamics" "Limiter"
)

target_sources(HyperPrismLimiter
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hsat
    BUNDLE_ID "com.RevivalProject.HyperPrismTubeTapeSaturation"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Tube-Tape Saturation"
    VST3_CATEGORIES "Fx" "Distortion" "Saturation"
)

target_sources(HyperPrismTubeTapeSaturation
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hhex
    BUNDLE_ID "com.RevivalProject.HyperPrismHarmonicExciter"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Harmonic Exciter"
    VST3_CATEGORIES "Fx" "Distortion"
)

target_sources(HyperPrismHarmonicExciter
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hbmx
    BUNDLE_ID "com.RevivalProject.HyperPrismBassMaximiser"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Bass Maximiser"
    VST3_CATEGORIES "Fx" "Distortion"
)

target_sources(HyperPrismBassMaximiser
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hsdl
    BUNDLE_ID "com.RevivalProject.HyperPrismSingleDelay"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Single Delay"
    VST3_CATEGORIES "Fx" "Delay"
)

target_sources(HyperPrismSingleDelay
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hmdl
    BUNDLE_ID "com.RevivalProject.HyperPrismMultiDelay"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Multi Delay"
    VST3_CATEGORIES "Fx" "Delay"
)

target_sources(HyperPrismMultiDelay
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hpan
    BUNDLE_ID "com.RevivalProject.HyperPrismPan"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Pan"
    VST3_CATEGORIES "Fx" "Spatial"
)

target_sources(HyperPrismPan
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hapn
    BUNDLE_ID "com.RevivalProject.HyperPrismAutoPan"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Auto Pan"
    VST3_CATEGORIES "Fx" "Spatial" "Modulation"
)

target_sources(HyperPrismAutoPan
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hqst
    BUNDLE_ID "com.RevivalProject.HyperPrismQuasiStereo"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Quasi Stereo"
    VST3_CATEGORIES "Fx" "Spatial"
)

target_sources(HyperPrismQuasiStereo
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hmst
    BUNDLE_ID "com.RevivalProject.HyperPrismMoreStereo"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined More Stereo"
    VST3_CATEGORIES "Fx" "Spatial"
)

target_sources(HyperPrismMoreStereo
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hmsm
    BUNDLE_ID "com.RevivalProject.HyperPrismMSMatrix"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined M+S Matrix"
    VST3_CATEGORIES "Fx" "Spatial"
)

target_sources(HyperPrismMSMatrix
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hsdy
    BUNDLE_ID "com.RevivalProject.HyperPrismStereoDynamics"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Stereo Dynamics"
    VST3_CATEGORIES "Fx" "Dynamics" "Spatial"
)

target_sources(HyperPrismStereoDynamics
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hfrs
    BUNDLE_ID "com.RevivalProject.HyperPrismFrequencyShifter"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Frequency Shifter"
    VST3_CATEGORIES "Fx" "Pitch Shift" "Modulation"
)

target_sources(HyperPrismFrequencyShifter
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hvoc
    BUNDLE_ID "com.RevivalProject.HyperPrismVocoder"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Vocoder"
    VST3_CATEGORIES "Fx" "Modulation"
)

target_sources(HyperPrismVocoder
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hpch
    BUNDLE_ID "com.RevivalProject.HyperPrismPitchChanger"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Pitch Changer"
    VST3_CATEGORIES "Fx" "Pitch Shift"
)

target_sources(HyperPrismPitchChanger
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hsdc
    BUNDLE_ID "com.RevivalProject.HyperPrismSonicDecimator"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Sonic Decimator"
    VST3_CATEGORIES "Fx" "Distortion"
)

target_sources(HyperPrismSonicDecimator
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hrmd
    BUNDLE_ID "com.RevivalProject.HyperPrismRingModulator"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Ring Modulator"
    VST3_CATEGORIES "Fx" "Modulation"
)

target_sources(HyperPrismRingModulator
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hhph
    BUNDLE_ID "com.RevivalProject.HyperPrismHyperPhaser"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined HyperPhaser"
    VST3_CATEGORIES "Fx" "Modulation"
)

target_sources(HyperPrismHyperPhaser
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
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
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ZQFX
    PLUGIN_CODE Hech
    BUNDLE_ID "com.RevivalProject.HyperPrismEcho"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HyperPrism Reimagined Echo"
    VST3_CATEGORIES "Fx" "Delay"
)

target_sources(HyperPrismEcho
    PRIVATE
        Source/Shared/HyperPrismLookAndFeel.cpp
        Source/Shared/HyperPrismLookAndFeel.h
        Source/Shared/XYPadComponent.cpp
        Source/Shared/XYPadComponent.h
        Source/Shared/StandardLayout.cpp
        Source/Shared/StandardLayout.h
        Source/Echo/EchoProcessor.cpp
        Source/Echo/EchoProcessor.h
        Source/Echo/EchoEditor.cpp
        Source/Echo/EchoEditor.h
        Source/Echo/EchoPlugin.cpp
)

configure_hyperprism_plugin(HyperPrismEcho)

# All 32 HyperPrism plugins are now implemented!