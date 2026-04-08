add_executable(test_sharedmemorychannel
    test_sharedmemorychannel.cpp
)

target_link_libraries(test_sharedmemorychannel PRIVATE
    lab3_common
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)

target_include_directories(test_sharedmemorychannel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/include
)

add_test(NAME test_sharedmemorychannel COMMAND test_sharedmemorychannel)
