cmake_minimum_required(VERSION 3.16)
project(wallpaper_carousel VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Activation d'options Release/Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

find_package(Qt6 6.5 COMPONENTS Quick Core Gui DBus Multimedia Widgets REQUIRED)
if(COMMAND qt_policy)
    qt_policy(SET QTP0001 NEW)
    qt_policy(SET QTP0004 NEW)
endif()

# tomlplusplus (header-only) — try the system package first (Fedora: tomlplusplus-devel),
# and fall back to fetching it automatically so the build works even when the
# distro package isn't installed or ships a different header layout.
find_package(tomlplusplus QUIET)
if(NOT tomlplusplus_FOUND)
    message(STATUS "tomlplusplus not found on system, fetching it instead")
    include(FetchContent)
    FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(tomlplusplus)
endif()

# Executable principal
qt_add_executable(wallpaper-carousel
    src/main.cpp
    src/wallpapercontroller.cpp
    src/wallpapercontroller.h
    src/imageloader.cpp
    src/imageloader.h
    src/infinitelistmodel.cpp
    src/infinitelistmodel.h
    src/screenhelper.cpp
    src/screenhelper.h
    src/screenorder.h
    src/appletsrcparse.h
)

# Module QML
qt_add_qml_module(wallpaper-carousel
    URI CarouselUI
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/CarouselItem.qml
        qml/RevealOverlay.qml
)

# Linking : Ajoute Qt6::Widgets et Qt6::Multimedia
target_link_libraries(wallpaper-carousel
    PRIVATE Qt6::Quick Qt6::Core Qt6::Gui Qt6::DBus Qt6::Multimedia Qt6::Widgets tomlplusplus::tomlplusplus
)

# Options de compilation selon le type de build
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Configuring Release build with optimizations")
    target_compile_definitions(wallpaper-carousel PRIVATE NDEBUG)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Configuring Debug build with symbols")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
endif()

# Install
install(TARGETS wallpaper-carousel RUNTIME DESTINATION bin)

# Launcher, not autostart: the picker grabs the screen and minimizes everything as
# soon as it runs, so starting it at login is not what anyone wants. It also has to
# live in an applications dir for the Meta+W service shortcut in kglobalshortcutsrc
# to resolve "wallpaper-carousel.desktop" at all.
install(FILES packaging/wallpaper-carousel.desktop DESTINATION share/applications)
# AppStream requires the file name to equal the <id> inside it, hence the reverse-DNS
# name here while the .desktop above keeps the short one.
install(FILES packaging/io.github.deadindian.WallpaperCarousel.metainfo.xml
        DESTINATION share/metainfo)

# The companion wallpaper plugin. Pure QML, so it is a data install — no ECM or
# Plasma CMake dependency needed for a directory copy.
install(DIRECTORY plugin/org.wallpapercarousel.slideshow
        DESTINATION share/plasma/wallpapers)

# Check for the appletsrc parsing (multi-screen slide lookup): ./wc-tests
add_executable(wc-tests tests/test_appletsrcparse.cpp)
target_link_libraries(wc-tests PRIVATE Qt6::Core)

# Check for the Tab screen-hop ordering: ./wc-tests-screenorder
add_executable(wc-tests-screenorder tests/test_screenorder.cpp)
target_link_libraries(wc-tests-screenorder PRIVATE Qt6::Core)

# The asserts *are* the tests, and CMake's default Release flags carry -DNDEBUG,
# which compiles every one of them out and makes both binaries print "ok"
# unconditionally. Target options land after CMAKE_CXX_FLAGS_RELEASE, so this wins.
foreach(test_target wc-tests wc-tests-screenorder)
    target_compile_options(${test_target} PRIVATE -UNDEBUG)
endforeach()

# Register both self-checks so `ctest` runs them; they still work as bare binaries.
enable_testing()
add_test(NAME wc-tests COMMAND wc-tests)
add_test(NAME wc-tests-screenorder COMMAND wc-tests-screenorder)

