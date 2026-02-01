project(Engine LANGUAGES CXX)

add_subdirectory(Source/Utils)
add_subdirectory(Source/ECS)

add_library(Glaze INTERFACE)
add_library(Glaze::Engine ALIAS Glaze)
target_link_libraries(Glaze INTERFACE Glaze::ECS)