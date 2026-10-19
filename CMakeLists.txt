cmake_minimum_required(VERSION 3.21)

project(pigeon-gui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt QML metatype generation requires AUTOMOC on the executable target.
set(CMAKE_AUTOMOC ON)

# Qt 6.4 is the intended minimum for this standalone shell. A Qt toolchain
# was not available in the authoring environment to validate this baseline.
find_package(Qt6 6.4 REQUIRED COMPONENTS Quick QuickControls2)

qt_add_executable(pigeon-gui
    src/main.cpp
)

# Strip source-directory prefixes from the compiled QML resource URLs.
# Theme is a single instance shared across the QML module.
set_source_files_properties(qml/Main.qml PROPERTIES
    QT_RESOURCE_ALIAS Main.qml
)
set_source_files_properties(qml/theme/Theme.qml PROPERTIES
    QT_RESOURCE_ALIAS Theme.qml
    QT_QML_SINGLETON_TYPE TRUE
)
set_source_files_properties(qml/components/SurfaceCard.qml PROPERTIES
    QT_RESOURCE_ALIAS SurfaceCard.qml
)

qt_add_qml_module(pigeon-gui
    URI Pigeon.Gui
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/Main.qml
        qml/theme/Theme.qml
        qml/components/SurfaceCard.qml
)

target_link_libraries(pigeon-gui PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
)

set_target_properties(pigeon-gui PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Pigeon"
)

include(GNUInstallDirs)

install(TARGETS pigeon-gui
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
