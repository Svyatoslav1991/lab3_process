add_executable(lab3_child
    src/main.cpp
    src/mainwindow.cpp
    include/mainwindow.h
    src/mainwindow.ui
)

target_include_directories(lab3_child PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/app/include
)

target_link_libraries(lab3_child PRIVATE
    lab3_common
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)
