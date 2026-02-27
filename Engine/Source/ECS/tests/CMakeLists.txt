add_executable(ECS.Tests
        test_Bundle.cpp
        test_ComponentManager.cpp
        test_SparseArray.cpp
        test_SparseSet.cpp
        test_TypeErasedArray.cpp
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