set(LIBTEST_SOURCES
    config_parse_lib.c
    config_parse_lib.h
    libtest.c
    mock-transport.c
    mock-transport.h
    msg_parse_lib.c
    msg_parse_lib.h
    persist_lib.c
    persist_lib.h
    proto_lib.c
    proto_lib.h
    queue_utils_lib.c
    queue_utils_lib.h
    template_lib.c
    template_lib.h
    testutils.c
    testutils.h
)

add_library(syslog-ng-test ${LIBTEST_SOURCES})
