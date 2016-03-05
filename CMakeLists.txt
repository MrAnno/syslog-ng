cmake_minimum_required (VERSION 2.8.12)

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

project (syslog-ng)
set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")
set (CMAKE_POSITION_INDEPENDENT_CODE 1)
set (BISON_FLAGS "-Wno-other")

### Submodules
include (UpdateGitSubmodules)

add_subdirectory(libtest/Criterion)
set (CRITERION_LIBRARIES "${PROJECT_BINARY_DIR}/libtest/Criterion/libcriterion.so")
set (CRITERION_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/libtest/Criterion/include")
### Submodules

set (SYSLOG_NG_VERSION_MAJOR 3)
set (SYSLOG_NG_VERSION_MINOR 8)
set (SYSLOG_NG_VERSION_PATCH alpha0)
set (SYSLOG_NG_VERSION "${SYSLOG_NG_VERSION_MAJOR}.${SYSLOG_NG_VERSION_MINOR}.${SYSLOG_NG_VERSION_PATCH}")
set (SYSLOG_NG_SOURCE_REVISION ${SYSLOG_NG_VERSION})

set (SYSLOG_NG_PATH_PREFIX ${CMAKE_INSTALL_PREFIX})
set (SYSLOG_NG_PATH_SYSCONFDIR "\${prefix}/etc")
set (SYSLOG_NG_PATH_DATADIR "\${prefix}/etc")
set (SYSLOG_NG_PATH_PIDFILEDIR "\${localstatedir}")
set (SYSLOG_NG_PATH_LOCALSTATEDIR "\${prefix}/var")
set (SYSLOG_NG_MODULE_PATH "\${exec_prefix}/lib/syslog-ng")
set (SYSLOG_NG_PATH_EXECPREFIX "\${prefix}")
set (SYSLOG_NG_PATH_CONFIG_INCLUDEDIR "\${prefix}")
set (SYSLOG_NG_PATH_SCLDIR "\${prefix}")
set (SYSLOG_NG_PATH_LIBEXECDIR "\${exec_prefix}/libexec")
set (SYSLOG_NG_PATH_DATAROOTDIR "\${prefix}/share")
set (SYSLOG_NG_ENABLE_LINUX_CAPS 0)
set (SYSLOG_NG_ENABLE_TCP_WRAPPER 0)
set (SYSLOG_NG_ENABLE_DEBUG 0)
set (SYSLOG_NG_ENABLE_GPROF 0)
set (SYSLOG_NG_ENABLE_IPV6 0)
set (SYSLOG_NG_ENABLE_MEMTRACE 0)
set (SYSLOG_NG_PATH_MODULEDIR "modulestuff")
set (SYSLOG_NG_PACKAGE_NAME "packagestuff")
set (SYSLOG_NG_PATH_XSDDIR "\${datadir}/syslog-ng/xsd")

# CMake will generate tests only if the enable_testing() command has been invoked.
# The CTest module invokes the command automatically when the BUILD_TESTING option is ON.
enable_testing()

include (CheckTypeSize)
include (CheckStructMember)
include (CheckSymbolExists)

add_definitions(-D_GNU_SOURCE=1)

set (CMAKE_EXTRA_INCLUDE_FILES sys/socket.h)
check_type_size ("struct sockaddr_storage" STRUCT_SOCKADDR_STORAGE)
set (SYSLOG_NG_HAVE_STRUCT_SOCKADDR_STORAGE ${HAVE_STRUCT_SOCKADDR_STORAGE})
set (CMAKE_EXTRA_INCLUDE_FILES)

set (CMAKE_EXTRA_INCLUDE_FILES sys/types.h sys/socket.h)
set (CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE=1)
check_type_size ("struct ucred" STRUCT_UCRED_SIZE)
check_type_size ("struct cmsgcred" STRUCT_CMSGCRED_SIZE)
if (HAVE_STRUCT_UCRED_SIZE OR HAVE_STRUCT_CMSGCRED_SIZE)
    set (SYSLOG_NG_HAVE_STRUCT_UCRED 1)
else()
    set (SYSLOG_NG_HAVE_STRUCT_UCRED 0)
endif()
unset (CMAKE_EXTRA_INCLUDE_FILES)
unset (CMAKE_REQUIRED_DEFINITIONS)

include(CheckIPv6)

option(ENABLE_IPV6 "Enable IPv6" ON)
if (ENABLE_IPV6)
    set(SYSLOG_NG_ENABLE_IPV6 ${HAVE_IPV6})
endif()

check_symbol_exists (strtoll stdlib.h SYSLOG_NG_HAVE_STRTOLL)
check_symbol_exists (strtoimax inttypes.h SYSLOG_NG_HAVE_STRTOIMAX)
check_symbol_exists (inet_aton "sys/socket.h;netinet/in.h;arpa/inet.h" SYSLOG_NG_HAVE_INET_ATON)
check_symbol_exists (getutent utmp.h SYSLOG_NG_HAVE_GETUTENT)
check_symbol_exists (getutxent utmpx.h SYSLOG_NG_HAVE_GETUTXENT)

set (WITH_GETTEXT "" CACHE STRING "Set the prefix where gettext is installed (e.g. /usr)")

find_package(BISON 2.4 REQUIRED)
find_package(GLIB 2.10.1 REQUIRED)
find_package(Ivykis REQUIRED)
find_package(Eventlog REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(FLEX REQUIRED)
find_package(LibNet REQUIRED)
find_package(Resolv REQUIRED)
find_package(Wrap)

pkg_check_modules(LIBPCRE REQUIRED libpcre)

if (WRAP_FOUND)
  set(SYSLOG_NG_ENABLE_TCP_WRAPPER 1)
endif()

if (LIBNET_FOUND)
  set(SYSLOG_NG_ENABLE_SPOOF_SOURCE 1)
endif()

if (WITH_GETTEXT)
    set (CMAKE_PREFIX_PATH ${WITH_GETTEXT})
    find_package(Gettext REQUIRED QUIET)
    set (CMAKE_PREFIX_PATH "")
else()
    find_package(Gettext REQUIRED QUIET)
endif()

set(CORE_INCLUDE_DIRS
    ${Eventlog_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${Gettext_INCLUDE_DIR}
    ${Ivykis_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${LIBPCRE_INCLUDE_DIRS}
)

set(CORE_LIBRARIES
    ${BISON_LIBRARIES}
    ${Eventlog_LIBRARIES}
    ${FLEX_LIBRARIES}
    ${GLIB_GMODULE_LIBRARIES}
    ${GLIB_GTHREAD_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${Gettext_LIBRARIES}
    ${Ivykis_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${RESOLV_LIBRARIES}
    ${LIBPCRE_LIBRARIES}
    )

include_directories(SYSTEM ${CORE_INCLUDE_DIRS})

function(generate_y_from_ym FileWoExt)
    add_custom_command (OUTPUT ${PROJECT_BINARY_DIR}/${FileWoExt}.y
        COMMAND ${PROJECT_SOURCE_DIR}/lib/merge-grammar.pl ${PROJECT_SOURCE_DIR}/${FileWoExt}.ym > ${PROJECT_BINARY_DIR}/${FileWoExt}.y
        DEPENDS ${PROJECT_SOURCE_DIR}/lib/cfg-grammar.y
                ${PROJECT_SOURCE_DIR}/${FileWoExt}.ym)
endfunction()

include_directories (${PROJECT_BINARY_DIR}/lib)
include_directories (${PROJECT_BINARY_DIR})
include_directories (${PROJECT_SOURCE_DIR}/lib)
include_directories (${PROJECT_SOURCE_DIR})
add_subdirectory(lib)
add_subdirectory(modules)
add_subdirectory(syslog-ng)

include (CheckIncludeFiles)

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/syslog-ng-config.h.in ${CMAKE_CURRENT_BINARY_DIR}/syslog-ng-config.h)
