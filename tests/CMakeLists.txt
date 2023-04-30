include_directories(../src)

# Reliable broadcast
add_executable(test_RB test_RB.cpp)
target_link_libraries(test_RB ${PROJECT_NAME} GTest::gtest_main)
add_test(NAME test_RB COMMAND test_RB)


# Binary Consensus
add_executable(test_BinCon test_BinCon.cpp)
target_link_libraries(test_BinCon ${PROJECT_NAME} GTest::gtest_main)
add_test(NAME test_BinCon COMMAND test_BinCon)

# DBFT
add_executable(test_DBFT test_DBFT.cpp)
target_link_libraries(test_DBFT ${PROJECT_NAME} GTest::gtest_main)
add_test(NAME test_DBFT COMMAND test_DBFT)