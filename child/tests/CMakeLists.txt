set(CHILD_TEST_TARGET test_lab3_child)

set(CHILD_TEST_SOURCES
    # test_childwindow.cpp
)

if(CHILD_TEST_SOURCES)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test Widgets)

    add_executable(${CHILD_TEST_TARGET})

    target_sources(${CHILD_TEST_TARGET}
        PRIVATE
            ${CHILD_TEST_SOURCES}
    )

    target_include_directories(${CHILD_TEST_TARGET}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../app/include
    )

    target_link_libraries(${CHILD_TEST_TARGET}
        PRIVATE
            Qt${QT_VERSION_MAJOR}::Test
            Qt${QT_VERSION_MAJOR}::Widgets
    )

    add_test(NAME ${CHILD_TEST_TARGET} COMMAND ${CHILD_TEST_TARGET})
else()
    message(STATUS "child/tests: тестовые файлы пока не добавлены, цель тестов не создается.")
endif()