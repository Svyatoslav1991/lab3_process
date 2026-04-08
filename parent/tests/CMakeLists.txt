set(PARENT_TEST_TARGET test_lab3_parent)

set(PARENT_TEST_SOURCES
    test_mainwindow.cpp
    ../app/src/mainwindow.cpp
    ../app/include/mainwindow.h
    ../app/src/mainwindow.ui
)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test Widgets)

add_executable(${PARENT_TEST_TARGET})

target_sources(${PARENT_TEST_TARGET}
    PRIVATE
        ${PARENT_TEST_SOURCES}
)

target_include_directories(${PARENT_TEST_TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../app/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common/app/include
)

target_link_libraries(${PARENT_TEST_TARGET}
    PRIVATE
        lab3_common
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
)

if(WIN32 AND MINGW AND QT_VERSION_MAJOR EQUAL 5)
    target_link_libraries(${PARENT_TEST_TARGET} PRIVATE Qt5::WinMain)
endif()

add_test(NAME ${PARENT_TEST_TARGET} COMMAND ${PARENT_TEST_TARGET})
