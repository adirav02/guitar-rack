cmake_minimum_required(VERSION 3.22)
project(GuitarRack VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(GUITARRACK_BUILD_STANDALONE "Build the JUCE standalone application" ON)
option(GUITARRACK_SANITIZERS "Enable ASan/UBSan diagnostic checks" OFF)

find_package(Threads REQUIRED)
add_library(guitarrack_core STATIC src/core/MonitorEngine.cpp src/core/Effects.cpp
    src/core/RackEngine.cpp src/core/ToneState.cpp src/dsp/Gain.cpp src/dsp/Overdrive.cpp src/dsp/EQ.cpp src/dsp/Delay.cpp)
target_link_libraries(guitarrack_core PUBLIC Threads::Threads)
target_include_directories(guitarrack_core PUBLIC src)
target_compile_options(guitarrack_core PRIVATE -Wall -Wextra -Wpedantic)
if(GUITARRACK_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(guitarrack_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(guitarrack_core PUBLIC -fsanitize=address,undefined)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(guitarrack_tests tests/MonitorTests.cpp tests/HeapAudit.cpp)
    target_link_libraries(guitarrack_tests PRIVATE guitarrack_core Threads::Threads)
    target_compile_options(guitarrack_tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME monitoring_and_routing COMMAND guitarrack_tests)
    add_executable(guitarrack_rack_tests tests/RackTests.cpp tests/HeapAudit.cpp)
    target_link_libraries(guitarrack_rack_tests PRIVATE guitarrack_core)
    target_compile_options(guitarrack_rack_tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME serial_rack_lifecycle COMMAND guitarrack_rack_tests)
    add_executable(guitarrack_effect_tests tests/EffectTests.cpp tests/HeapAudit.cpp)
    target_link_libraries(guitarrack_effect_tests PRIVATE guitarrack_core)
    target_compile_options(guitarrack_effect_tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME built_in_effect_signals COMMAND guitarrack_effect_tests)
    set_tests_properties(serial_rack_lifecycle built_in_effect_signals PROPERTIES TIMEOUT 30)
endif()

if(GUITARRACK_BUILD_STANDALONE)
    include(FetchContent)
    # Offline source checkout, still pinned and patched. No global JUCE install.
    if(NOT FETCHCONTENT_SOURCE_DIR_JUCE AND EXISTS "${CMAKE_SOURCE_DIR}/build/vendor/JUCE/CMakeLists.txt")
        set(FETCHCONTENT_SOURCE_DIR_JUCE "${CMAKE_SOURCE_DIR}/build/vendor/JUCE" CACHE PATH "Local pinned JUCE checkout")
    endif()
    if(FETCHCONTENT_SOURCE_DIR_JUCE)
        execute_process(COMMAND git -C "${FETCHCONTENT_SOURCE_DIR_JUCE}" rev-parse HEAD
            OUTPUT_VARIABLE local_juce_commit OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE local_juce_result)
        if(NOT local_juce_result EQUAL 0 OR NOT local_juce_commit STREQUAL "91ad83ae34a81e0833b1a2b0866f54846370ae53")
            message(FATAL_ERROR "Local JUCE must be the exact pinned 8.0.15 checkout")
        endif()
        set(JUCE_SOURCE "${FETCHCONTENT_SOURCE_DIR_JUCE}")
        set(GUITARRACK_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}")
        include(cmake/PatchJuce.cmake)
    endif()
    # JUCE 8.0.15, verified official tag on 2026-09-30. Never follow a branch.
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 91ad83ae34a81e0833b1a2b0866f54846370ae53
        GIT_PROGRESS TRUE
        PATCH_COMMAND ${CMAKE_COMMAND}
            -DJUCE_SOURCE=<SOURCE_DIR>
            -DGUITARRACK_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PatchJuce.cmake)
    FetchContent_MakeAvailable(JUCE)
    add_library(guitarrack_state STATIC src/state/Presets.cpp src/state/PresetSession.cpp src/state/Preferences.cpp)
    target_link_libraries(guitarrack_state PUBLIC guitarrack_core juce::juce_core)
    target_compile_definitions(guitarrack_state PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_compile_options(guitarrack_state PRIVATE -Wall -Wextra -Wpedantic)
    juce_add_gui_app(GuitarRack PRODUCT_NAME "GuitarRack")
    target_sources(GuitarRack PRIVATE
        src/standalone/Main.cpp
        src/standalone/AudioSession.cpp
        src/plugin/MonitorProcessor.cpp
        src/ui/MainComponent.cpp src/ui/RackEditor.cpp src/ui/PresetBar.cpp)
    target_compile_definitions(GuitarRack PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_JACK=1
        JUCE_ALSA=1 JUCE_PLUGINHOST_LADSPA=0
        JUCE_JACK_CLIENT_NAME="GuitarRack")
    target_link_libraries(GuitarRack PRIVATE guitarrack_state
        juce::juce_audio_devices juce::juce_audio_processors_headless
        juce::juce_gui_basics juce::juce_recommended_config_flags)
    target_compile_options(GuitarRack PRIVATE -Wall -Wextra -Wpedantic)
    if(BUILD_TESTING)
        add_executable(guitarrack_state_tests tests/StateTests.cpp tests/HeapAudit.cpp)
        target_link_libraries(guitarrack_state_tests PRIVATE guitarrack_state)
        target_compile_definitions(guitarrack_state_tests PRIVATE GUITARRACK_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
        add_test(NAME presets_and_persistence COMMAND guitarrack_state_tests)
        set_tests_properties(presets_and_persistence PROPERTIES TIMEOUT 30)
        add_executable(guitarrack_adapter_tests tests/AdapterTests.cpp tests/HeapAudit.cpp
            src/standalone/AudioSession.cpp src/plugin/MonitorProcessor.cpp)
        target_compile_definitions(guitarrack_adapter_tests PRIVATE
            JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_JACK=1 JUCE_ALSA=1
            JUCE_PLUGINHOST_LADSPA=0 JUCE_JACK_CLIENT_NAME="GuitarRack")
        target_link_libraries(guitarrack_adapter_tests PRIVATE guitarrack_core
            juce::juce_audio_devices juce::juce_audio_processors_headless
            juce::juce_recommended_config_flags)
        add_test(NAME device_adapter_lifecycle COMMAND guitarrack_adapter_tests)
    endif()
endif()
