set(CHILD_APP_HEADERS
    # include/childwindow.h
)

set(CHILD_APP_SOURCES
    # src/main.cpp
    # src/childwindow.cpp
)

set(CHILD_APP_FORMS
    # src/childwindow.ui
)

add_executable(${CHILD_APP_TARGET})

target_sources(${CHILD_APP_TARGET}
    PRIVATE
        ${CHILD_APP_HEADERS}
        ${CHILD_APP_SOURCES}
        ${CHILD_APP_FORMS}
)

target_include_directories(${CHILD_APP_TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${CHILD_APP_TARGET}
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
)

if(WIN32)
    set_target_properties(${CHILD_APP_TARGET} PROPERTIES WIN32_EXECUTABLE YES)
endif()