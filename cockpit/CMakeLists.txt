cmake_minimum_required(VERSION 3.30)

project(sinviCockpit VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Gui)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Svg)
find_package(Qt6 REQUIRED COMPONENTS SvgWidgets)


add_custom_target(fgfs_invis
    SOURCES fgfs_invis.xml)

set (${PROJECT_NAME}_SRC
    src/main.cpp
    src/instruments.cpp
)

set (${PROJECT_NAME}_UI
    src/instruments.ui
)

set (${PROJECT_NAME}_INCLUDE
    include/instruments.h
)

qt_add_executable(${PROJECT_NAME}
    ${${PROJECT_NAME}_SRC}
    ${${PROJECT_NAME}_UI}
    ${${PROJECT_NAME}_INCLUDE}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    Qt6::SvgWidgets
    qfi
    Sinvi
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ../lib/include
)

qt_add_library(qfi
    src/qfi/qfi_AI.cpp     include/qfi/qfi_AI.h
    src/qfi/qfi_ALT.cpp    include/qfi/qfi_ALT.h
    src/qfi/qfi_ASI.cpp    include/qfi/qfi_ASI.h
    src/qfi/qfi_Colors.cpp include/qfi/qfi_Colors.h
    src/qfi/qfi_EADI.cpp   include/qfi/qfi_EADI.h
    src/qfi/qfi_EHSI.cpp   include/qfi/qfi_EHSI.h
    src/qfi/qfi_Fonts.cpp  include/qfi/qfi_Fonts.h
    src/qfi/qfi_HI.cpp     include/qfi/qfi_HI.h
    src/qfi/qfi_TC.cpp     include/qfi/qfi_TC.h
    src/qfi/qfi_VSI.cpp    include/qfi/qfi_VSI.h
                           include/qfi/qfi_defs.h
                           include/qfi/qfi_doxygen.h
)

target_link_libraries(qfi PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    Qt6::SvgWidgets
)

target_include_directories(qfi PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ../lib/include
)

qt6_add_resources(qfi "images"
    FILES
    images/vsi/vsi.svg
    images/vsi/vsi_hand.svg
    images/vsi/vsi_face.svg
    images/vsi/vsi_case.svg
    images/ehsi/ehsi_dev_bar.svg
    images/ehsi/ehsi_cdi_from.svg
    images/ehsi/ehsi_crs_arrow.svg
    images/ehsi/ehsi_mask.svg
    images/ehsi/ehsi_hdg_bug.svg
    images/ehsi/ehsi_back.svg
    images/ehsi/ehsi_cdi_to.svg
    images/ehsi/ehsi_hdg_scale.svg
    images/ehsi/ehsi_mark.svg
    images/ehsi/ehsi_brg_arrow.svg
    images/ehsi/ehsi_dev_scale.svg
    images/asi/asi_case.svg
    images/asi/asi_face.svg
    images/asi/asi_hand.svg
    images/asi/asi.svg
    images/alt/alt_face_3.svg
    images/alt/alt_case.svg
    images/alt/alt_hand_1.svg
    images/alt/alt_hand_2.svg
    images/alt/alt_face_1.svg
    images/alt/alt.svg
    images/alt/alt_face_2.svg
    images/eadi/eadi_hsi_back.svg
    images/eadi/eadi_adi_mask.svg
    images/eadi/eadi_asi_bug.svg
    images/eadi/eadi_hsi_face.svg
    images/eadi/eadi_adi_fd.svg
    images/eadi/eadi_hsi_bug.svg
    images/eadi/eadi_alt_ground.svg
    images/eadi/eadi_adi_dotv.svg
    images/eadi/eadi_adi_fpmx.svg
    images/eadi/eadi_adi_slip.svg
    images/eadi/eadi_adi_roll.svg
    images/eadi/eadi_asi_frame.svg
    images/eadi/eadi_asi_scale.svg
    images/eadi/eadi_alt_frame.svg
    images/eadi/eadi_alt_bug.svg
    images/eadi/eadi_hsi_marks.svg
    images/eadi/eadi_vsi_scale.svg
    images/eadi/eadi_adi_stall.svg
    images/eadi/eadi_asi_back.svg
    images/eadi/eadi_alt_scale.svg
    images/eadi/eadi_adi_ladd.svg
    images/eadi/eadi_back.svg
    images/eadi/eadi_asi_vne.svg
    images/eadi/eadi_adi_turn.svg
    images/eadi/eadi_adi_scaleh.svg
    images/eadi/eadi_adi_back.svg
    images/eadi/eadi_adi_doth.svg
    images/eadi/eadi_adi_fpm.svg
    images/eadi/eadi_alt_back.svg
    images/eadi/eadi_mask.svg
    images/eadi/eadi_adi_scalev.svg
    images/tc/tc_case.svg
    images/tc/tc.svg
    images/tc/tc_mark.svg
    images/tc/tc_face_2.svg
    images/tc/tc_ball.svg
    images/tc/tc_back.svg
    images/tc/tc_face_1.svg
    images/hi/hi.svg
    images/hi/hi_case.svg
    images/hi/hi_face.svg
    images/ai/ai.svg
    images/ai/ai_back.svg
    images/ai/ai_face.svg
    images/ai/ai_case.svg
    images/ai/ai_ring.svg
)
