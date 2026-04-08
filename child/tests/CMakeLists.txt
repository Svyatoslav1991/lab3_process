add_executable(test_child_mainwindow
    test_mainwindow.cpp
    ../app/src/mainwindow.cpp
    ../app/include/mainwindow.h
    ../app/src/mainwindow.ui
)

target_include_directories(test_child_mainwindow PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/app/include
)

target_link_libraries(test_child_mainwindow PRIVATE
    lab3_common
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

if(WIN32 AND MINGW AND QT_VERSION_MAJOR EQUAL 5)
    target_link_libraries(test_child_mainwindow PRIVATE Qt5::WinMain)
endif()

add_test(NAME test_child_mainwindow COMMAND test_child_mainwindow)
