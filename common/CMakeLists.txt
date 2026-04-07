add_library(lab3_common STATIC
    src/sharedmemorychannel.cpp
)

target_include_directories(lab3_common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(lab3_common
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
)