include(FindPackageHandleStandardArgs)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(PLATFORM "x86")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM "x64")
    set(VERSION_SUFFIX "_64")
else()
    message(FATAL_ERROR "Unsupported architecture")
endif()

function (FindInstalledApi)
    message(STATUS "Searching installed API ")

    if (NOT UEYE_API_INCLUDE_DIR OR NOT UEYE_API_LIBRARY_DIR)
        if(WIN32)
            unset(INSTALL_PATH CACHE)
            set(INSTALL_PATH "C:/Program Files/IDS/uEye")
            set(UEYE_INSTALLED_INCLUDE_PATH "${INSTALL_PATH}/develop/include")
            if(NOT EXISTS ${UEYE_INSTALLED_INCLUDE_PATH}/ueye.h)
                message(SEND_ERROR "ueye header not found, please provide UEYE_API_INCLUDE_DIR and UEYE_API_LIBRARY_DIR")
            else()
                message(STATUS "found ueye development files at ${INSTALL_PATH}/develop")
            endif()
        elseif(UNIX)
            set(UEYE_INSTALLED_INCLUDE_PATH "/opt/ids/ueye/dev")
        else()
            message(FATAL_ERROR "Unsupported platform")
        endif()

        if (WIN32)
            set(UEYE_LIBRARY_PATH ${UEYE_INSTALLED_INCLUDE_PATH}/../Lib) # big L
        elseif(UNIX)
            set(UEYE_LIBRARY_PATH ${UEYE_INSTALLED_INCLUDE_PATH}/../lib) # low l
        endif()
    endif()

    if (UEYE_API_INCLUDE_DIR)
        message(INFO "UEYE_API_INCLUDE_DIR set, looking for header in ${UEYE_API_INCLUDE_DIR}")
        set(UEYE_INSTALLED_INCLUDE_PATH ${UEYE_API_INCLUDE_DIR})
    endif()

    if (UEYE_API_LIBRARY_DIR)
        message(INFO "UEYE_API_LIBRARY_DIR set, looking for library in ${UEYE_API_LIBRARY_DIR}")
        set(UEYE_LIBRARY_PATH ${UEYE_API_LIBRARY_DIR})
    endif()

    if (NOT UEYE_HEADER_DIR)
        find_path(UEYE_HEADER_DIR
            "ueye.h"
            HINTS
            ${UEYE_INSTALLED_INCLUDE_PATH}
            )
    endif()

    if (NOT UEYE_LIBRARY)
        find_library(UEYE_LIBRARY
            ${LIBUEYEAPI}
            HINTS
            ${UEYE_LIBRARY_PATH}
            )
    endif()
endfunction()

function(FindAPI)
    set(Product_LOWER ueye)
    if(WIN32)
        set(LIBUEYEAPI "ueye_api${VERSION_SUFFIX}.lib")
    elseif(UNIX)
        set(LIBUEYEAPI "libueye_api.so")
    endif()
    if (NOT UEYE_LIBRARY OR NOT UEYE_HEADER_DIR)
        FindInstalledApi()
    endif()


    get_filename_component(${UEYE_LIBRARY}_DIR ${UEYE_LIBRARY} DIRECTORY)
    message(STATUS "UEYE_HEADER_DIR: ${UEYE_HEADER_DIR}")
    message(STATUS "UEYE_LIBRARY: ${UEYE_LIBRARY}")
    message(STATUS "UEYE_LIBRARY_DIR: ${${UEYE_LIBRARY}_DIR}")

    include_directories(${UEYE_HEADER_DIR})
    link_directories(${${UEYE_LIBRARY}_DIR})
endfunction()

FindAPI()
find_package_handle_standard_args(ueyeapi REQUIRED_VARS UEYE_HEADER_DIR UEYE_LIBRARY
        FAIL_MESSAGE "ERROR ${UEYE_LIBRARY} not found!")
