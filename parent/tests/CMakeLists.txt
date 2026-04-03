set(PARENT_TEST_TARGET test_lab3_parent)

set(PARENT_TEST_SOURCES
    # test_mainwindow.cpp
)

if(PARENT_TEST_SOURCES)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test Widgets)

    add_executable(${PARENT_TEST_TARGET})

    target_sources(${PARENT_TEST_TARGET}
        PRIVATE
            ${PARENT_TEST_SOURCES}
    )

    target_include_directories(${PARENT_TEST_TARGET}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../app/include
    )

    target_link_libraries(${PARENT_TEST_TARGET}
        PRIVATE
            Qt${QT_VERSION_MAJOR}::Test
            Qt${QT_VERSION_MAJOR}::Widgets
    )

    add_test(NAME ${PARENT_TEST_TARGET} COMMAND ${PARENT_TEST_TARGET})
else()
    message(STATUS "parent/tests: тестовые файлы пока не добавлены, цель тестов не создается.")
endif()