# FindLibGit2 - CMake module to find libgit2
#
# This module provides the following variables:
# LIBGIT2_FOUND - True if libgit2 was found
# LIBGIT2_INCLUDE_DIRS - libgit2 include directories
# LIBGIT2_LIBRARIES - libgit2 libraries to link against

# Use pkg-config if available
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_LIBGIT2 QUIET libgit2)
endif()

# Find the include directories
find_path(LIBGIT2_INCLUDE_DIR
    NAMES git2.h git2/common.h
    PATHS
        ${PC_LIBGIT2_INCLUDE_DIRS}
        /usr/include
        /usr/local/include
)

# Find the library
find_library(LIBGIT2_LIBRARY
    NAMES git2
    PATHS
        ${PC_LIBGIT2_LIBRARY_DIRS}
        /usr/lib
        /usr/local/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibGit2
    DEFAULT_MSG
    LIBGIT2_LIBRARY LIBGIT2_INCLUDE_DIR)

mark_as_advanced(LIBGIT2_INCLUDE_DIR LIBGIT2_LIBRARY)

if(LIBGIT2_FOUND)
    set(LIBGIT2_LIBRARIES ${LIBGIT2_LIBRARY})
    set(LIBGIT2_INCLUDE_DIRS ${LIBGIT2_INCLUDE_DIR})
endif()
