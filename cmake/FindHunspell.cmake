# Hunspell's vcpkg port provides headers and libraries but no CMake package.
# This module also supports a manually installed Hunspell library.

find_path(Hunspell_INCLUDE_DIR
    NAMES hunspell/hunspell.hxx
)

find_library(Hunspell_LIBRARY_RELEASE
    NAMES hunspell hunspell-1.7
    PATH_SUFFIXES lib
)
find_library(Hunspell_LIBRARY_DEBUG
    NAMES hunspell hunspell-1.7
    PATH_SUFFIXES debug/lib lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Hunspell
    REQUIRED_VARS Hunspell_INCLUDE_DIR Hunspell_LIBRARY_RELEASE
)

if(Hunspell_FOUND AND NOT TARGET Hunspell::Hunspell)
    add_library(Hunspell::Hunspell UNKNOWN IMPORTED)
    set_target_properties(Hunspell::Hunspell PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Hunspell_INCLUDE_DIR}"
        IMPORTED_LOCATION_RELEASE "${Hunspell_LIBRARY_RELEASE}"
        IMPORTED_LOCATION_RELWITHDEBINFO "${Hunspell_LIBRARY_RELEASE}"
        IMPORTED_LOCATION_MINSIZEREL "${Hunspell_LIBRARY_RELEASE}"
    )
    if(Hunspell_LIBRARY_DEBUG)
        set_property(TARGET Hunspell::Hunspell PROPERTY IMPORTED_LOCATION_DEBUG "${Hunspell_LIBRARY_DEBUG}")
    else()
        set_property(TARGET Hunspell::Hunspell PROPERTY IMPORTED_LOCATION_DEBUG "${Hunspell_LIBRARY_RELEASE}")
    endif()
endif()
