cmake_minimum_required(VERSION 3.16)
project(fcitx5-jqwerty VERSION 0.1.0 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Compiler settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clangd / IDEs.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
find_package(ECM REQUIRED NO_MODULE)          # Extra CMake Modules (KDE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

find_package(Fcitx5Core REQUIRED)             # provides Fcitx5::Core
find_package(Fcitx5Utils REQUIRED)            # provides Fcitx5::Utils
find_package(PkgConfig REQUIRED)
pkg_check_modules(XkbCommon REQUIRED xkbcommon)

include(FeatureSummary)
include(GNUInstallDirs)
include(ECMSetupVersion)
include(ECMUninstallTarget)

# ---------------------------------------------------------------------------
# Plugin shared library
# ---------------------------------------------------------------------------
add_library(fcitx5-jqwerty MODULE
    src/jqwerty.cpp
)

target_include_directories(fcitx5-jqwerty PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(fcitx5-jqwerty PRIVATE
    Fcitx5::Core
    Fcitx5::Utils
    ${XkbCommon_LIBRARIES}
)

# fcitx5 plugins must not have a "lib" prefix.
set_target_properties(fcitx5-jqwerty PROPERTIES
    PREFIX ""
    SUFFIX ".so"
)

# ---------------------------------------------------------------------------
# Install targets
# ---------------------------------------------------------------------------

# Shared library → fcitx5 addon directory.
install(TARGETS fcitx5-jqwerty
    LIBRARY DESTINATION "${FCITX_INSTALL_ADDONDIR}"
)

# Addon descriptor.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jqwerty-addon.conf.in"
    jqwerty-addon.conf
    @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/jqwerty-addon.conf"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/fcitx5/addon"
    RENAME "jqwerty.conf"
)

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
