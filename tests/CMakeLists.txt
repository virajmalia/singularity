add_executable(singularity_tests
    test_language_stats.cpp
    test_language_detector.cpp
    test_repo_analyzer.cpp
    test_formatter.cpp
    test_main.cpp
    security_recommender_test.cpp
)

# Find GTest
find_package(GTest REQUIRED)

target_include_directories(singularity_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(singularity_tests PRIVATE singularity_lib GTest::gtest GTest::gtest_main)

add_test(NAME singularity_tests COMMAND singularity_tests)
