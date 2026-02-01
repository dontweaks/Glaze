project(Sandbox LANGUAGES CXX)

add_executable(Sandbox)
target_compile_features(Sandbox PRIVATE cxx_std_23)
target_link_libraries(
        Sandbox)

target_sources(Sandbox
        PRIVATE
        main.cpp
)