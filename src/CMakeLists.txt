add_library(singularity_lib
    language_stats.cpp
    repo_analyzer.cpp
    analyzers.cpp
    formatter.cpp
    language_detector.cpp
    application.cpp
)

target_include_directories(singularity_lib 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Find dependencies
find_package(CURL REQUIRED)
find_package(libgit2 REQUIRED)

target_link_libraries(singularity_lib 
    PUBLIC
        CURL::libcurl
        libgit2::libgit2
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
