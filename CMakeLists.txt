cmake_minimum_required(VERSION 3.21)

project(pigeon-gui VERSION 0.0.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt QML metatype generation requires AUTOMOC on the executable target.
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Quick QuickControls2 Sql Network)

qt_add_executable(pigeon-gui
    src/main.cpp
    src/app/SettingsController.h
    src/app/SettingsController.cpp
    src/app/CaptureController.h
    src/app/CaptureController.cpp
    src/app/AppIconProvider.h
    src/app/AppIconProvider.cpp
    src/app/AppController.h
    src/app/AppController.cpp
    src/data/Snapshot.h
    src/data/DatabaseReader.h
    src/data/DatabaseReader.cpp
    src/data/RateSampler.h
    src/data/RateSampler.cpp
    src/models/Models.h
    src/models/Models.cpp
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

foreach(name Sparkline BrandMark ActionButton ApplicationCell MetricsRow NavTab MetricCard EmptyState CellText DirectionIcon TrafficChart
             ConversationTable ApplicationsTable EndpointsTable FilterBar ScopeCombo ConversationDetails)
    set_source_files_properties(qml/components/${name}.qml PROPERTIES
        QT_RESOURCE_ALIAS components/${name}.qml
    )
endforeach()

foreach(name OverviewPage TrafficPage ApplicationsPage EndpointsPage)
    set_source_files_properties(qml/pages/${name}.qml PROPERTIES
        QT_RESOURCE_ALIAS pages/${name}.qml
    )
endforeach()

set_source_files_properties(qml/theme/Format.js PROPERTIES
    QT_RESOURCE_ALIAS theme/Format.js
)

qt_add_qml_module(pigeon-gui
    URI Pigeon.Gui
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/Main.qml
        qml/theme/Theme.qml
        qml/components/SurfaceCard.qml
        qml/components/Sparkline.qml
        qml/components/BrandMark.qml
        qml/components/ActionButton.qml
        qml/components/ApplicationCell.qml
        qml/components/MetricsRow.qml
        qml/components/NavTab.qml
        qml/components/MetricCard.qml
        qml/components/EmptyState.qml
        qml/components/CellText.qml
        qml/components/DirectionIcon.qml
        qml/components/TrafficChart.qml
        qml/components/ConversationTable.qml
        qml/components/ApplicationsTable.qml
        qml/components/EndpointsTable.qml
        qml/components/FilterBar.qml
        qml/components/ScopeCombo.qml
        qml/components/ConversationDetails.qml
        qml/pages/OverviewPage.qml
        qml/pages/TrafficPage.qml
        qml/pages/ApplicationsPage.qml
        qml/pages/EndpointsPage.qml
    RESOURCES
        assets/pigeon.png
        qml/theme/Format.js
)

target_link_libraries(pigeon-gui PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Sql
    Qt6::Network
)

target_include_directories(pigeon-gui PRIVATE src)

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

# Runtime branding uses the bundled PNG on all platforms. Linux launchers
# also receive the original image through the standard pixmaps fallback.
if(UNIX AND NOT APPLE)
    install(FILES assets/pigeon.png DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps)
    install(FILES packaging/linux/pigeon-gui.desktop
            DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Test)
    add_executable(pigeon-icon-tests
        tests/test_icons.cpp
        src/app/AppIconProvider.h
        src/app/AppIconProvider.cpp
    )
    target_include_directories(pigeon-icon-tests PRIVATE src)
    target_link_libraries(pigeon-icon-tests PRIVATE Qt6::Quick Qt6::Test)
    add_test(NAME pigeon-icons COMMAND pigeon-icon-tests)
    set_tests_properties(pigeon-icons PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    add_executable(pigeon-data-tests
        tests/test_data.cpp
        src/data/Snapshot.h
        src/data/DatabaseReader.h
        src/data/DatabaseReader.cpp
        src/data/RateSampler.h
        src/data/RateSampler.cpp
        src/models/Models.h
        src/models/Models.cpp
    )
    target_include_directories(pigeon-data-tests PRIVATE src)
    target_link_libraries(pigeon-data-tests PRIVATE Qt6::Core Qt6::Sql Qt6::Network Qt6::Test)
    add_test(NAME pigeon-data COMMAND pigeon-data-tests)

    add_executable(pigeon-controller-tests
        tests/test_controller.cpp
        src/app/AppController.h
        src/app/AppController.cpp
        src/data/Snapshot.h
        src/data/DatabaseReader.h
        src/data/DatabaseReader.cpp
        src/data/RateSampler.h
        src/data/RateSampler.cpp
        src/models/Models.h
        src/models/Models.cpp
    )
    target_include_directories(pigeon-controller-tests PRIVATE src)
    target_link_libraries(pigeon-controller-tests PRIVATE Qt6::Core Qt6::Sql Qt6::Network Qt6::Test)
    add_test(NAME pigeon-controller COMMAND pigeon-controller-tests)

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(NAME pigeon-gui-fixture
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/test_demo_db.py)
        add_test(NAME pigeon-qml-startup
            COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/test_gui_startup.py
                $<TARGET_FILE:pigeon-gui>)
        set_tests_properties(pigeon-qml-startup PROPERTIES TIMEOUT 30)
    endif()
endif()

if(BUILD_TESTING)
    add_executable(pigeon-capture-tests
        tests/test_capture_control.cpp
        src/app/CaptureController.h
        src/app/CaptureController.cpp
    )
    target_include_directories(pigeon-capture-tests PRIVATE src)
    target_link_libraries(pigeon-capture-tests PRIVATE Qt6::Core Qt6::Network Qt6::Test)
    add_test(NAME pigeon-capture COMMAND pigeon-capture-tests)
endif()

if(BUILD_TESTING)
    add_executable(pigeon-settings-tests
        tests/test_settings.cpp
        src/app/SettingsController.h
        src/app/SettingsController.cpp
    )
    target_include_directories(pigeon-settings-tests PRIVATE src)
    target_link_libraries(pigeon-settings-tests PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME pigeon-settings COMMAND pigeon-settings-tests)
endif()
