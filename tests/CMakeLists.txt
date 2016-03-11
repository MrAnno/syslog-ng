add_subdirectory(loggen)

find_package(Criterion)
if (CRITERION_FOUND)
    add_subdirectory(unit)
endif()
