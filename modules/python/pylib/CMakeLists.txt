add_custom_target(BuildPylib
    COMMAND python setup.py build --build-base=${CMAKE_CURRENT_BINARY_DIR}/build
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build/ DESTINATION lib/python2.7/site-packages/
    PATTERN "*.py"
)
