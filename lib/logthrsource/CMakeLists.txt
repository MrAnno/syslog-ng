set(LOGTHRSOURCE_HEADERS
    logthrsource/logthrsourcedrv.h
    logthrsource/logthrfetcher.h
    PARENT_SCOPE)

set(LOGTHRSOURCE_SOURCES
    logthrsource/logthrsourcedrv.c
    logthrsource/logthrfetcher.c
    PARENT_SCOPE)

add_test_subdirectory(tests)
