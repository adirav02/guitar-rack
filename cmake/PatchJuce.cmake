find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${GUITARRACK_SOURCE}/cmake/patch_juce.py"
            "${JUCE_SOURCE}" "${GUITARRACK_SOURCE}"
    RESULT_VARIABLE patch_result)
if(NOT patch_result EQUAL 0)
    message(FATAL_ERROR "JUCE real-time patch failed; refusing an unverified backend")
endif()
