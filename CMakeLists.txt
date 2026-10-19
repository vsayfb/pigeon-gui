cmake_minimum_required(VERSION 3.21)

project(pigeon-gui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt QML metatype generation requires AUTOMOC on the executable target.
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Quick QuickControls2 Sql Network)

qt_add_executable(pigeon-gui
    src/main.cpp
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

foreach(name NavTab MetricCard EmptyState CellText DirectionIcon TrafficChart
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

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Test)
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
    endif()
endif()
