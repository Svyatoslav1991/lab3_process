set(PARENT_APP_TARGET lab3_parent)

add_subdirectory(app)

if(BUILD_PARENT_TESTS)
    add_subdirectory(tests)
endif()