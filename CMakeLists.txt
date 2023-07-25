cmake_minimum_required(VERSION 3.5)

project(mothman-qt VERSION 0.1)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

if(MOTHMAN_QT_CAIRO)
    message("Building mothman with the CDraw Cairo port")

    set(CDRAW_USE_CAIRO 1)
    add_subdirectory(cdraw)
    add_definitions(${CDRAW_DEFINITIONS})
else()
    message("Building mothman with the CDraw Qt port")

    set(CDRAW_USE_QT 1)
    add_subdirectory(cdraw)
    add_definitions(${CDRAW_DEFINITIONS})
endif()

set(PROJECT_SOURCES
        mothman/mothman.c
        mothman/tagparser.c
        mothman/mothman.h
        mothman/tagparser.h
        mothman/protohandle.c
        mothman/protohandle.h

        frontend/qt/main.cpp
        frontend/qt/mothmanwindow.cpp
        frontend/qt/mothmanwindow.h
        frontend/qt/mothmanview.h
        frontend/qt/mothmanview.cpp
        network/include/request-curl.h
        network/lib/request-curl.c
)

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(mothman-qt
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET mothman-qt APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(mothman-qt SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(mothman-qt
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(mothman-qt PRIVATE Qt${QT_VERSION_MAJOR}::Widgets cdraw ${CURL_LIBRARY})
target_include_directories(mothman-qt PUBLIC ${CDRAW_INCLUDE_DIR})

set_target_properties(mothman-qt PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER com.bravotic.mothman
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS mothman-qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(mothman-qt)
endif()
