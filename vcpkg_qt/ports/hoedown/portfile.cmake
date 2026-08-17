vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO hoedown/hoedown
    REF 3.0.7
    SHA512 b8b6652350693084d70fd4db4e48a4dbdc1d02164152b1337eee981c9268bc3913ce6b70d56d955204549f650b0bce3d53d40d09c18f6fb21e88937597c76241
    HEAD_REF master
)

file(COPY
    "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt"
    "${CMAKE_CURRENT_LIST_DIR}/hoedownConfig.cmake.in"
    DESTINATION "${SOURCE_PATH}"
)

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/hoedown)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
