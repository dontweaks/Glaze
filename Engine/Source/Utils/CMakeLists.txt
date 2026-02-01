project(Utils LANGUAGES CXX)

add_library(Utils INTERFACE)
add_library(Glaze::Utils ALIAS Utils)

target_include_directories(Utils
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
        $<INSTALL_INTERFACE:/>
)