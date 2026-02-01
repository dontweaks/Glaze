project(ECS VERSION 0.1.0 LANGUAGES CXX)

add_library(ECS STATIC)
add_library(Glaze::ECS ALIAS ECS)

target_compile_features(ECS PRIVATE cxx_std_23)

add_subdirectory(include/ECS)

target_sources(ECS
        PRIVATE
        src/World.cpp
)

target_include_directories(ECS
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
        $<INSTALL_INTERFACE:include/>
        PRIVATE
        src
)

target_link_libraries(ECS
        PUBLIC
        Glaze::Utils
)

add_executable(ECSSandbox)

target_compile_features(ECSSandbox PRIVATE cxx_std_23)

target_sources(ECSSandbox
        PRIVATE
        main.cpp
)

target_link_libraries(ECSSandbox
        PRIVATE
        Glaze::ECS
)