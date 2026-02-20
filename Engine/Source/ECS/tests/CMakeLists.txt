add_executable(ECS.Tests
        test_SparseArray.cpp
        test_SparseSet.cpp
)

target_compile_features(ECS.Tests PRIVATE cxx_std_23)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(ECS.Tests
        PRIVATE
        Glaze::ECS
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(ECS.Tests)