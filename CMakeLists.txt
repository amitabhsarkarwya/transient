cmake_minimum_required(VERSION 3.23.1)

project(MySynth VERSION 0.0.1)

# General setup
set(PROJECT_NAME "MySynth")
set(CURRENT_VERSION "0.0.1")
set(PRODUCT_NAME "Transient Shaper")
set(COMPANY_NAME "Archie DSP")
set(BUNDLE_ID "com.archiedsp.TransientShaper")
set(FORMATS Standalone VST3)

# JUCE
add_subdirectory(modules/JUCE)

# On Windows, copy to the user's VST3 directory so admin elevation is not required
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_property(GLOBAL PROPERTY JUCE_VST3_COPY_DIR "$ENV{LOCALAPPDATA}/Programs/Common/VST3")
endif()

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows runtime library
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE INTERNAL "")
endif()

# Define the plugin target
juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"
    COPY_PLUGIN_AFTER_BUILD TRUE         # Automatically copies .vst3 to system plugin folder
    PLUGIN_MANUFACTURER_CODE ADSP
    PLUGIN_CODE AExp
    FORMATS ${FORMATS}
    PRODUCT_NAME "${PRODUCT_NAME}"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    IS_SYNTH FALSE
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_target_properties(${PROJECT_NAME} PROPERTIES JUCE_VST3_COPY_DIR "$ENV{LOCALAPPDATA}/Programs/Common/VST3")
endif()

juce_generate_juce_header(${PROJECT_NAME})

# Source files
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
)

# Add sources directly to plugin target
target_sources(${PROJECT_NAME} PRIVATE ${SourceFiles})

# Optional assets
file(GLOB_RECURSE Assets CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/*.*"
)

# Only create binary data if assets exist
if(Assets)
    juce_add_binary_data(AudioPluginData
        SOURCES ${Assets}
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE AudioPluginData)

    target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_BINARY_DIR}/AudioPluginData"
    )
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

if(TARGET juce_vst3_helper)
    target_include_directories(juce_vst3_helper PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/JUCE/modules/juce_audio_processors/format_types/VST3_SDK
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/JUCE/modules/juce_audio_processors_headless/format_types/VST3_SDK
    )
    target_compile_definitions(juce_vst3_helper PRIVATE
        JucePlugin_Name="${PRODUCT_NAME}"
        JucePlugin_Desc="${PRODUCT_NAME}"
        JucePlugin_Manufacturer="${COMPANY_NAME}"
        JucePlugin_ManufacturerWebsite=""
        JucePlugin_ManufacturerEmail=""
        JucePlugin_ManufacturerCode=0x41445350
        JucePlugin_PluginCode=0x41457870
        JucePlugin_VSTUniqueID=0x41457870
        JucePlugin_VersionString="${CURRENT_VERSION}"
        JucePlugin_VersionCode=0x1
        JucePlugin_VSTCategory=kPlugCategEffect
        "JucePlugin_Vst3Category=\"Fx|Dynamics\""
    )
endif()

if(TARGET MySynth_VST3)
    target_include_directories(MySynth_VST3 PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/JUCE/modules/juce_audio_processors_headless/format_types/VST3_SDK
    )
endif()

# Compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_ENABLE_REPAINT_DEBUGGING=0
)

# Link JUCE modules
target_link_libraries(${PROJECT_NAME} PRIVATE
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
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
