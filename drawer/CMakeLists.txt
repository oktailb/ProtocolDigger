cmake_minimum_required(VERSION 3.30)

project(sinviDrawer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Gui)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Charts)
find_package(Qt6 REQUIRED COMPONENTS Multimedia)
find_package(Qt6 REQUIRED COMPONENTS MultimediaWidgets)

set (${PROJECT_NAME}_SRC
    src/main.cpp
    src/mainwindow.cpp
    src/chartview.cpp
    src/chart.cpp
    src/callout.cpp
    src/videoDialog.cpp
)

set (${PROJECT_NAME}_UI
    src/mainwindow.ui
)

set (${PROJECT_NAME}_INCLUDE
    include/mainwindow.h
    include/chartview.h
    include/chart.h
    include/checklist.h
    include/callout.h
    include/videoDialog.h
)

qt_add_executable(${PROJECT_NAME}
    ${${PROJECT_NAME}_SRC}
    ${${PROJECT_NAME}_UI}
    ${${PROJECT_NAME}_INCLUDE}
)

qt6_add_resources(${PROJECT_NAME} "images"
    PREFIX "drawer"
    FILES
    play.png
    pause.png
    stop.png
    fforward.png
    power.png
    plus.png
    minus.png
    license.txt
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Charts
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::MultimediaWidgets
    Sinvi
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ../lib/include
)
