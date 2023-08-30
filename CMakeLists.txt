cmake_minimum_required(VERSION 3.5)

project(BJPrinter VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
list(APPEND CMAKE_AUTOUIC_SEARCH_PATHS src/ui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(GCLIB_INSTALL_DIR "C:/Program Files (x86)/Galil/gclib")

find_package(OpenCV REQUIRED)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Svg PrintSupport SerialPort REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Svg PrintSupport SerialPort REQUIRED)
find_package(ueyeapi REQUIRED)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS SvgWidgets)
endif()

set(HEADERS

    include/mainwindow.h
    include/jetdrive.h
    include/mfjdrv.h
    include/asyncserialdevice.h
    include/lineprintdata.h
    include/outputwindow.h
    include/printer.h
    include/printhread.h
    include/svgview.h
    include/datarecordpoller.h
    include/ginterrupthandler.h
    include/pcd.h
    include/dmc4080.h
    include/mister.h

)

set(SOURCES

    src/jetdrive.cpp
    src/mfjdrv.cpp
    src/asyncserialdevice.cpp
    src/lineprintdata.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/outputwindow.cpp
    src/printer.cpp
    src/printhread.cpp
    src/svgview.cpp
    src/datarecordpoller.cpp
    src/ginterrupthandler.cpp
    src/pcd.cpp
    src/dmc4080.cpp
    src/mister.cpp

)

set(WIDGET_HEADERS

    include/widgets/printerwidget.h
    include/widgets/powdersetupwidget.h
    include/widgets/jettingwidget.h
    include/widgets/lineprintwidget.h
    include/widgets/highspeedlinewidget.h
    include/widgets/dropletobservationwidget.h
    include/widgets/pressurecontrollerwidget.h

)

set(WIDGET_SOURCES

    src/widgets/printerwidget.cpp
    src/widgets/highspeedlinewidget.cpp
    src/widgets/jettingwidget.cpp
    src/widgets/lineprintwidget.cpp
    src/widgets/powdersetupwidget.cpp
    src/widgets/dropletobservationwidget.cpp
    src/widgets/pressurecontrollerwidget.cpp

)

set(FORMS

    src/ui/mainwindow.ui
    src/ui/highspeedlinewidget.ui
    src/ui/jettingwidget.ui
    src/ui/lineprintwidget.ui
    src/ui/outputwindow.ui
    src/ui/powdersetupwidget.ui
    src/ui/dropletobservationwidget.ui
    src/ui/dropletanalyzerwidget.ui
    src/ui/pressurecontrollerwidget.ui

)

set(CAMERA_HEADERS

    include/camera/camera.h
    include/camera/display.h
    include/camera/eventthread.h
    include/camera/graphicsscene.h
    include/camera/graphicsview.h
    include/camera/property_class.h
    include/camera/queyeimage.h
    include/camera/subwindow.h
    include/camera/utils.h
    include/camera/cameralist.h

)

set(CAMERA_SOURCES

    src/camera/camera.cpp
    src/camera/display.cpp
    src/camera/eventthread.cpp
    src/camera/graphicsscene.cpp
    src/camera/graphicsview.cpp
    src/camera/queyeimage.cpp
    src/camera/subwindow.cpp
    src/camera/utils.cpp
    src/camera/cameralist.cpp

)

set(DA_HEADERS

    include/dropletanalyzer/dropletanalyzer.h
    include/dropletanalyzer/dropletanalyzerwidget.h
    include/dropletanalyzer/imageviewer.h
    include/dropletanalyzer/linearanalysis.h
    include/dropletanalyzer/qcustomplot.h

)

set(DA_SOURCES

    src/dropletanalyzer/dropletanalyzer.cpp
    src/dropletanalyzer/imageviewer.cpp
    src/dropletanalyzer/dropletanalyzerwidget.cpp
    src/dropletanalyzer/qcustomplot.cpp
)

set(RESOURCES

    resources.qrc
    bjprinter.rc

)

source_group("1 - Header Files" REGULAR_EXPRESSION "^.*\.(h|hpp)$")
source_group("2 - Source Files" REGULAR_EXPRESSION "^.*\.(cpp)$")
source_group("3 - UI Forms" REGULAR_EXPRESSION ".*\.ui")

source_group("1 - Header Files/Printer Widgets" FILES ${WIDGET_HEADERS})
source_group("2 - Source Files/Printer Widgets" FILES ${WIDGET_SOURCES})

source_group("1 - Header Files/Camera" FILES ${CAMERA_HEADERS})
source_group("2 - Source Files/Camera" FILES ${CAMERA_SOURCES})

source_group("1 - Header Files/Droplet Analyzer" FILES ${DA_HEADERS})
source_group("2 - Source Files/Droplet Analyzer" FILES ${DA_SOURCES})

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
        ${WIDGET_HEADERS}
        ${WIDGET_SOURCES}
        ${CAMERA_HEADERS}
        ${CAMERA_SOURCES}
        ${DA_HEADERS}
        ${DA_SOURCES}
    )
else()
    add_executable(${EXE_NAME} ${GUI_TYPE}

        ${HEADERS}
        ${SOURCES}
        ${FORMS}
        ${RESOURCES}
        ${WIDGET_HEADERS}
        ${WIDGET_SOURCES}
        ${CAMERA_HEADERS}
        ${CAMERA_SOURCES}
        ${DA_HEADERS}
        ${DA_SOURCES}

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
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::SerialPort
    gclib
    gclibo
    ueye_api${PLATFORM_SUFFIX}
    ueye_tools${PLATFORM_SUFFIX}
    ${OpenCV_LIBS}

)

target_include_directories(${EXE_NAME} PUBLIC

    ${GCLIB_INSTALL_DIR}/include
    include
    include/widgets
    include/camera
    include/dropletanalyzer
    ${OpenCV_INCLUDE_DIRS}

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
