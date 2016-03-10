add_executable(loggen loggen.c)
target_link_libraries(loggen PRIVATE
    ${GLIB_GTHREAD_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)
install(TARGETS loggen RUNTIME DESTINATION bin)
