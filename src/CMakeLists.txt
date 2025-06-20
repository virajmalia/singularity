add_library(singularity_lib
    language_stats.cpp
    repo_analyzer.cpp
    analyzers.cpp
    application.cpp
    security_recommender.cpp
)

target_include_directories(singularity_lib 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Find dependencies
find_package(CURL REQUIRED)
find_package(libgit2 REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)

target_link_libraries(singularity_lib 
    PUBLIC
        CURL::libcurl
        libgit2::libgit2
        fmt::fmt
        nlohmann_json::nlohmann_json
)

# Main executable
add_executable(singularity main.cpp)
target_link_libraries(singularity PRIVATE singularity_lib)
target_include_directories(singularity PRIVATE ${CMAKE_SOURCE_DIR}/include)

install(TARGETS singularity_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
