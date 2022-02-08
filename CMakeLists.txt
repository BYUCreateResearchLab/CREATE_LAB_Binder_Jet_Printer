cmake_minimum_required(VERSION 3.5)

project(BJPrinter VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
list(APPEND CMAKE_AUTOUIC_SEARCH_PATHS src/ui)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(GCLIB_INSTALL_DIR "C:/Program Files (x86)/Galil/gclib")
#set(UEYE_API_INCLUDE_DIR "C:/Program Files/IDS/uEye/develop/include")
#set(UEYE_API_LIBRARY_DIR "C:/Program Files/IDS/uEye/develop/lib")

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Svg REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Svg REQUIRED)
find_package(ueyeapi REQUIRED)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS SvgWidgets)
endif()

#add_library(gclib SHARED IMPORTED GLOBAL)
#set_property(TARGET gclib PROPERTY IMPORTED_LOCATION ${GCLIB_INSTALL_DIR}/dll/x64/gclib.dll)
#set_property(TARGET gclib PROPERTY IMPORTED_IMPLIB ${GCLIB_INSTALL_DIR}/lib/dynamic/x64/gclib.lib)

#add_library(gclibo SHARED IMPORTED GLOBAL)
#set_property(TARGET gclibo PROPERTY IMPORTED_LOCATION ${GCLIB_INSTALL_DIR}/dll/x64/gclibo.dll)
#set_property(TARGET gclibo PROPERTY IMPORTED_IMPLIB ${GCLIB_INSTALL_DIR}/lib/dynamic/x64/gclibo.lib)

set(HEADERS

    src/mainwindow.h
    src/JetServer.h
    src/lineprintdata.h
    src/outputwindow.h
    src/printer.h
    src/printhread.h
    src/printobject.h
    src/svgview.h
    src/vec2.h
    src/widgets/printerwidget.h
    src/widgets/powdersetupwidget.h
    src/widgets/jettingwidget.h
    src/widgets/highspeedlinewidget.h
    src/widgets/dropletobservationwidget.h
)

set(SOURCES

    src/lineprintdata.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/outputwindow.cpp
    src/printer.cpp
    src/printhread.cpp
    src/printobject.cpp
    src/svgview.cpp
    src/widgets/printerwidget.cpp
    src/widgets/highspeedlinewidget.cpp
    src/widgets/jettingwidget.cpp
    src/widgets/lineprintwidget.cpp
    src/widgets/powdersetupwidget.cpp
    src/widgets/dropletobservationwidget.cpp
)

set(FORMS

    src/ui/mainwindow.ui
    src/ui/highspeedlinewidget.ui
    src/ui/jettingwidget.ui
    src/ui/lineprintwidget.ui
    src/ui/outputwindow.ui
    src/ui/powdersetupwidget.ui
    src/ui/dropletobservationwidget.ui
)

set(RESOURCES
    resources.qrc
    bjprinter.rc
)

set(CAMERA_SOURCES
    src/camera/camera.cpp
    src/camera/camera.h
    src/camera/display.cpp
    src/camera/display.h
    src/camera/eventthread.cpp
    src/camera/eventthread.h
    src/camera/graphicsscene.cpp
    src/camera/graphicsscene.h
    src/camera/graphicsview.cpp
    src/camera/graphicsview.h
    src/camera/property_class.h
    src/camera/queyeimage.cpp
    src/camera/queyeimage.h
    src/camera/subwindow.cpp
    src/camera/subwindow.h
    src/camera/utils.cpp
    src/camera/utils.h
    src/camera/cameralist.cpp
    src/camera/cameralist.h
)

source_group("1 - Header Files" REGULAR_EXPRESSION "^.*\.(h|hpp)$")
source_group("2 - Source Files" REGULAR_EXPRESSION "^.*\.(cpp)$")
source_group("3 - UI Forms" REGULAR_EXPRESSION ".*\.ui")
source_group("2 - Source Files/Printer Widgets" REGULAR_EXPRESSION "widget.*\.cpp")
source_group("1 - Header Files/Printer Widgets" REGULAR_EXPRESSION "widget.*\.h")
source_group("3 - UI Forms/Printer Widgets" REGULAR_EXPRESSION "widget.*\.ui")
source_group("4 - Resources" REGULAR_EXPRESSION "^.*\.(rc|qrc)$")

set(EXE_NAME ${PROJECT_NAME})

if(WIN32)
    set(GUI_TYPE WIN32)
endif(WIN32)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${EXE_NAME} ${GUI_TYPE}
        MANUAL_FINALIZATION
        ${HEADERS}
        ${SOURCES}
        ${FORMS}
        ${RESOURCES}
        ${CAMERA_SOURCES}
    )
else()
    add_executable(${EXE_NAME} ${GUI_TYPE}
        ${HEADERS}
        ${SOURCES}
        ${FORMS}
        ${RESOURCES}
        ${CAMERA_SOURCES}
    )
endif()

if (WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8) # if 64 bit
        set(PLATFORM_SUFFIX _64)
        set(GCLIB_LIBRARY_DIR ${GCLIB_INSTALL_DIR}/lib/dynamic/x64)
    else()
        set(GCLIB_LIBRARY_DIR ${GCLIB_INSTALL_DIR}/lib/dynamic/x32)
    endif(CMAKE_SIZEOF_VOID_P EQUAL 8)
endif()

target_link_directories(${EXE_NAME} PUBLIC
    ${GCLIB_LIBRARY_DIR}
)

target_link_libraries(${EXE_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    gclib
    gclibo
    ueye_api${PLATFORM_SUFFIX}
    )

target_include_directories(${EXE_NAME} PUBLIC
    ${GCLIB_INSTALL_DIR}/include
    src
    src/widgets
    src/ui
    src/camera
    )

set_target_properties(${EXE_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(${EXE_NAME} PUBLIC
        Qt${QT_VERSION_MAJOR}::SvgWidgets
    )
    qt_finalize_executable(${EXE_NAME})
endif()

# copy dlls for now, or can add to path. OR figure out how ueye does it...
#add_custom_command (TARGET ${EXE_NAME} POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_directory
#            "${GCLIB_INSTALL_DIR}/dll/x64/"
#            $<TARGET_FILE_DIR:${EXE_NAME}>
#)
