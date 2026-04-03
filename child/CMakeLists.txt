set(CHILD_APP_TARGET lab3_child)

add_subdirectory(app)

if(BUILD_CHILD_TESTS)
    add_subdirectory(tests)
endif()