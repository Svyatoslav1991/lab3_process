set(PARENT_APP_HEADERS
    include/mainwindow.h
)

set(PARENT_APP_SOURCES
    src/main.cpp
    src/mainwindow.cpp
)

set(PARENT_APP_FORMS
    src/mainwindow.ui
)

add_executable(${PARENT_APP_TARGET})

target_sources(${PARENT_APP_TARGET}
    PRIVATE
        ${PARENT_APP_HEADERS}
        ${PARENT_APP_SOURCES}
        ${PARENT_APP_FORMS}
)

target_include_directories(${PARENT_APP_TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PARENT_APP_TARGET}
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
)

if(WIN32)
    set_target_properties(${PARENT_APP_TARGET} PROPERTIES WIN32_EXECUTABLE YES)
endif()
