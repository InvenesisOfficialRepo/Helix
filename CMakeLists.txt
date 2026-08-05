cmake_minimum_required(VERSION 3.21)

# Ensure compatibility with older submodules (such as c-ares inside gRPC)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Compatibility with older subprojects" FORCE)

project(Helix LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt's automatic MOC, UIC, and RCC processing globally
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)



# Silence private Qt module warning (for GuiPrivate inside QXlsx)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

# Find Qt components
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Network
    Concurrent
    Quick
    QuickControls2
    Graphs
    Multimedia
    Core5Compat
)

# --- gRPC and Protobuf via FetchContent ---
include(FetchContent)

# Workaround for Qt Creator and Windows Git submodule clone failures
# If we already successfully downloaded gRPC in a previous build directory (like build/release),
# we tell CMake to just use that source directory instead of cloning it again.
if(EXISTS "${CMAKE_SOURCE_DIR}/build/release/_deps/grpc-src")
    set(FETCHCONTENT_SOURCE_DIR_GRPC "${CMAKE_SOURCE_DIR}/build/release/_deps/grpc-src" CACHE PATH "Use pre-downloaded gRPC source")
endif()

# We set some options to prevent gRPC from building its tests and unused components
set(OPENSSL_NO_ASM ON CACHE BOOL "Disable assembly optimizations in BoringSSL to avoid NASM dependency" FORCE)
set(gRPC_BUILD_TESTS OFF CACHE BOOL "Disable gRPC tests" FORCE)
set(gRPC_INSTALL OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_CSHARP_EXT OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_CSHARP_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_NODE_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_PHP_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_PYTHON_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_RUBY_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_ZLIB_PROVIDER "module" CACHE STRING "" FORCE)
set(gRPC_CARES_PROVIDER "module" CACHE STRING "" FORCE)
set(gRPC_RE2_PROVIDER "module" CACHE STRING "" FORCE)
set(gRPC_SSL_PROVIDER "module" CACHE STRING "" FORCE)
set(gRPC_PROTOBUF_PROVIDER "module" CACHE STRING "" FORCE)
set(gRPC_ABSL_PROVIDER "module" CACHE STRING "" FORCE)
set(gRPC_WERROR OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_WERROR OFF CACHE BOOL "" FORCE)
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    grpc
    GIT_REPOSITORY https://github.com/grpc/grpc
    GIT_TAG        v1.54.3
)
FetchContent_MakeAvailable(grpc)

qt_standard_project_setup()

# Set up global include directories for legacy Widgets relative includes
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/qml_core)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/_deps/grpc-src/include)

# Build QXlsx (vendored locally in third_party)
add_subdirectory(third_party/QXlsx)

# Build openwall_crypt static library
file(GLOB OPENWALL_CRYPT_SOURCES
    "third_party/openwall_crypt/*.c"
)
add_library(openwall_crypt STATIC ${OPENWALL_CRYPT_SOURCES})
target_include_directories(openwall_crypt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/openwall_crypt")
if(MSVC)
    target_compile_definitions(openwall_crypt PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Unified Source Files (QML Dashboard C++ + traditional Widgets submodules C++)
set(APP_SOURCES
    # Master entry
    src/master_hub/main.cpp

    # Shared Sources
    src/shared/ReferenceDataRepository.h
    src/shared/ReferenceDataRepository.cpp

    # QML Dashboard App C++ Sources
    src/qml_core/app/AppContext.h
    src/qml_core/app/AppContext.cpp
    src/qml_core/services/AnalysisPipeline.h
    src/qml_core/services/AnalysisPipeline.cpp
    src/qml_core/services/AnalysisModules.h
    src/qml_core/services/AnalysisModules.cpp
    src/qml_core/services/FeedingAnalysisProcessor.h
    src/qml_core/services/FeedingAnalysisProcessor.cpp
    src/qml_core/services/PlateCalculator.h
    src/qml_core/services/PlateCalculator.cpp
    src/qml_core/domain/Status.h
    src/qml_core/domain/Status.cpp
    src/qml_core/models/ProjectModel.h
    src/qml_core/models/ProjectModel.cpp
    src/qml_core/models/TestCardModel.h
    src/qml_core/models/TestCardModel.cpp
    src/qml_core/models/CompoundListModel.h
    src/qml_core/models/CompoundListModel.cpp
    src/qml_core/models/CalendarEventsModel.h
    src/qml_core/models/CalendarEventsModel.cpp
    src/qml_core/viewmodels/SessionViewModel.h
    src/qml_core/viewmodels/SessionViewModel.cpp
    src/qml_core/viewmodels/DashboardViewModel.h
    src/qml_core/viewmodels/DashboardViewModel.cpp
    src/qml_core/viewmodels/ProjectViewModel.h
    src/qml_core/viewmodels/ProjectViewModel.cpp
    src/qml_core/viewmodels/DatapointManagerViewModel.h
    src/qml_core/viewmodels/DatapointManagerViewModel.cpp
    src/qml_core/viewmodels/CalendarViewModel.h
    src/qml_core/viewmodels/CalendarViewModel.cpp
    src/qml_core/viewmodels/TecanPlannerViewModel.h
    src/qml_core/viewmodels/TecanPlannerViewModel.cpp
    src/qml_core/db/DatabaseManager.h
    src/qml_core/db/DatabaseManager.cpp
    src/qml_core/db/DbWorker.h
    src/qml_core/db/DbWorker.cpp
    src/qml_core/auth/PasswordHasher.h
    src/qml_core/auth/PasswordHasher.cpp
    src/qml_core/auth/AuthRepository.h
    src/qml_core/auth/AuthRepository.cpp
    src/qml_core/domain/DashboardDtos.h
    src/qml_core/domain/TecanPlannerDtos.h
    src/qml_core/repositories/DashboardRepository.h
    src/qml_core/repositories/DashboardRepository.cpp
    src/qml_core/repositories/TecanPlannerRepository.h
    src/qml_core/repositories/TecanPlannerRepository.cpp
    src/qml_core/domain/ClientDtos.h
    src/qml_core/repositories/ClientRepository.h
    src/qml_core/repositories/ClientRepository.cpp
    src/qml_core/domain/BatchDetailDtos.h
    src/qml_core/repositories/BatchRepository.h
    src/qml_core/repositories/BatchRepository.cpp
    src/qml_core/repositories/CalendarRepository.h
    src/qml_core/repositories/CalendarRepository.cpp
    src/qml_core/repositories/DoneSyncRepository.h
    src/qml_core/repositories/DoneSyncRepository.cpp
    src/qml_core/repositories/ExcelImportRepository.h
    src/qml_core/repositories/ExcelImportRepository.cpp

    # Widgets App C++ Sources
    src/widgets_core/common/ClickableLabel.h

    src/widgets_core/authentication/logindialog.cpp
    src/widgets_core/authentication/logindialog.h
    src/widgets_core/authentication/logindialog.ui
    src/widgets_core/authentication/resetpassworddialog.cpp
    src/widgets_core/authentication/resetpassworddialog.h
    src/widgets_core/authentication/resetpassworddialog.ui
    src/widgets_core/authentication/qtbcrypt.cpp
    src/widgets_core/authentication/qtbcrypt.h
    src/widgets_core/authentication/adminresetpassworddialog.cpp
    src/widgets_core/authentication/adminresetpassworddialog.h
    src/widgets_core/authentication/adminresetpassworddialog.ui

    src/widgets_core/database/customproxymodel.cpp
    src/widgets_core/database/customproxymodel.h
    src/widgets_core/database/draggabletableview.cpp
    src/widgets_core/database/draggabletableview.h
    src/widgets_core/database/Database.h
    src/widgets_core/database/DbConfig.h
    src/widgets_core/database/DbConfig.cpp

    src/widgets_core/data_access/ExperimentDao.cpp
    src/widgets_core/data_access/ExperimentDao.h

    # Hardware Integration - SiLA 2
    src/widgets_core/hardware/FluentSilaClient.h
    src/widgets_core/hardware/FluentSilaClient.cpp
    src/widgets_core/data_access/AuthenticationDao.cpp
    src/widgets_core/data_access/AuthenticationDao.h
    src/widgets_core/data_access/ItemDao.cpp
    src/widgets_core/data_access/ItemDao.h

    src/widgets_core/services/ExperimentManager.cpp
    src/widgets_core/services/ExperimentManager.h
    src/widgets_core/services/DilutionEngine.h
    src/widgets_core/services/DilutionEngine.cpp
    src/widgets_core/services/gwl_helpers.h
    src/widgets_core/services/gwl_helpers.cpp
    src/widgets_core/services/Logger.h
    src/widgets_core/services/Logger.cpp
    src/widgets_core/services/JsonHelpers.h
    src/widgets_core/services/JsonHelpers.cpp
    src/widgets_core/services/ExperimentJsonSerializer.h
    src/widgets_core/services/ExperimentJsonSerializer.cpp
    src/widgets_core/services/ThemeManager.h
    src/widgets_core/services/ThemeManager.cpp

    src/widgets_core/hardware/ILiquidHandlerDriver.h
    src/widgets_core/hardware/EvoDriver.h
    src/widgets_core/hardware/EvoDriver.cpp
    src/widgets_core/hardware/FluentDriver.h
    src/widgets_core/hardware/FluentDriver.cpp

    src/widgets_core/plate_management/PlateWidget.cpp
    src/widgets_core/plate_management/PlateWidget.h
    src/widgets_core/plate_management/PlateMapDialog.cpp
    src/widgets_core/plate_management/PlateMapDialog.h
    src/widgets_core/plate_management/PlateMapDialog.ui
    src/widgets_core/plate_management/Plate1536Dialog.cpp
    src/widgets_core/plate_management/Plate1536Dialog.h
    src/widgets_core/plate_management/matrixplatecontainer.cpp
    src/widgets_core/plate_management/matrixplatecontainer.h
    src/widgets_core/plate_management/matrixplatewidget.cpp
    src/widgets_core/plate_management/matrixplatewidget.h
    src/widgets_core/plate_management/daughterplatewidget.cpp
    src/widgets_core/plate_management/daughterplatewidget.h

    src/widgets_core/tecan_integration/tecanwindow.cpp
    src/widgets_core/tecan_integration/tecanwindow.h
    src/widgets_core/tecan_integration/tecanwindow.ui
    src/widgets_core/tecan_integration/TecanViewModel.cpp
    src/widgets_core/tecan_integration/TecanViewModel.h
    src/widgets_core/tecan_integration/gwlgenerator.cpp
    src/widgets_core/tecan_integration/gwlgenerator.h
    src/widgets_core/tecan_integration/generategwldialog.cpp
    src/widgets_core/tecan_integration/generategwldialog.h
    src/widgets_core/tecan_integration/standardlibrary.cpp
    src/widgets_core/tecan_integration/standardlibrary.h
    src/widgets_core/tecan_integration/standardselectiondialog.cpp
    src/widgets_core/tecan_integration/standardselectiondialog.h
    src/widgets_core/tecan_integration/standardselectiondialog.ui

    src/widgets_core/import_module/ImportSchema.h
    src/widgets_core/import_module/SmartImporter.h
    src/widgets_core/import_module/SmartImporter.cpp
    src/widgets_core/import_module/ImportDataAccess.h
    src/widgets_core/import_module/ImportDataAccess.cpp
    src/widgets_core/import_module/SmartImportDialog.h
    src/widgets_core/import_module/SmartImportDialog.cpp

    src/widgets_core/ui/databaseviewwindow.cpp
    src/widgets_core/ui/databaseviewwindow.h
    src/widgets_core/ui/databaseviewwindow.ui
    src/widgets_core/ui/additemdialog.cpp
    src/widgets_core/ui/additemdialog.h
    src/widgets_core/ui/additemdialog.ui
    src/widgets_core/ui/loadexperimentdialog.cpp
    src/widgets_core/ui/loadexperimentdialog.h
    src/widgets_core/ui/loadexperimentdialog.ui
    src/widgets_core/ui/MainWindowViewModel.cpp
    src/widgets_core/ui/MainWindowViewModel.h
    src/widgets_core/ui/registercompounddialog.cpp
    src/widgets_core/ui/registercompounddialog.h
    src/widgets_core/ui/dissolvebottledialog.cpp
    src/widgets_core/ui/dissolvebottledialog.h

    src/widgets_core/update_mangement/UpdateManager.cpp
    src/widgets_core/update_mangement/UpdateManager.h
    
    # LayoutMaker Integration
    src/layout_maker/CsvParser.h
    src/layout_maker/CsvParser.cpp
    src/layout_maker/LayoutMakerWindow.h
    src/layout_maker/LayoutMakerWindow.cpp
    src/layout_maker/PlateModel.h
    src/layout_maker/PlateModel.cpp
    src/layout_maker/PlateViewWidget.h
    src/layout_maker/PlateViewWidget.cpp
    src/layout_maker/QuadrantDropWidget.h
    src/layout_maker/QuadrantDropWidget.cpp
    src/layout_maker/QuadrantMerger.h
    src/layout_maker/QuadrantMerger.cpp
    src/layout_maker/StyleHelper.h

    resources.rc
)

# Unify both QML Dashboard resources and traditional QWidget resource.qrc
qt_add_executable(Helix ${APP_SOURCES} resources.qrc)

target_include_directories(Helix PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/qml_core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core/database
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core/authentication
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core/data_access
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core/services
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core/plate_management
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core/tecan_integration
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core/update_mangement
    ${CMAKE_CURRENT_SOURCE_DIR}/src/layout_maker
    ${CMAKE_CURRENT_BINARY_DIR}/generated_proto
)

# Generate gRPC and Protobuf files
set(SILA_PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core/hardware/proto/SiLABinaryTransfer.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core/hardware/proto/SiLAFramework.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core/hardware/proto/SilaFluentController.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core/hardware/proto/SilaFluentStatusProvider.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core/hardware/proto/SimulationController.proto
)

# Setup proto generation directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated_proto)
set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated_proto)

foreach(proto ${SILA_PROTO_FILES})
    get_filename_component(file_name ${proto} NAME_WE)
    
    set(generated_srcs
        ${PROTO_GEN_DIR}/${file_name}.pb.cc
        ${PROTO_GEN_DIR}/${file_name}.grpc.pb.cc
    )
    set(generated_hdrs
        ${PROTO_GEN_DIR}/${file_name}.pb.h
        ${PROTO_GEN_DIR}/${file_name}.grpc.pb.h
    )

    add_custom_command(
        OUTPUT ${generated_srcs} ${generated_hdrs}
        COMMAND protobuf::protoc
        ARGS --grpc_out=${PROTO_GEN_DIR}
             --cpp_out=${PROTO_GEN_DIR}
             -I${CMAKE_CURRENT_SOURCE_DIR}/src/widgets_core/hardware/proto
             --plugin=protoc-gen-grpc=$<TARGET_FILE:grpc_cpp_plugin>
             ${proto}
        DEPENDS ${proto}
    )

    target_sources(Helix PRIVATE ${generated_srcs} ${generated_hdrs})
endforeach()

# Set Qt policy to suppress QTP0004 warnings in QML modules
if (COMMAND qt_policy)
    qt_policy(SET QTP0004 OLD)
endif()

# Define QML Singletons
set_source_files_properties(qml/Style.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(Helix
    URI TestRequests
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml/

    QML_FILES
        qml/Main.qml
        qml/Style.qml

        qml/pages/LoginPage.qml
        qml/pages/MasterLauncherHub.qml
        qml/pages/DailyTecanPlannerPage.qml
        qml/pages/DashboardPage.qml
        qml/pages/ProjectDetailPage.qml
        qml/pages/CalendarPage.qml
        qml/pages/PublishResultsPage.qml
        qml/pages/PublishDetailPage.qml
        qml/pages/DatapointManagerPage.qml
        qml/pages/AnalyticsDashboard.qml
        qml/pages/StandaloneAnalysisPage.qml
        qml/pages/WallboardPage.qml

        qml/components/TestCard.qml
        qml/components/PlateHeatmap.qml
        qml/components/PieChart.qml
        qml/components/StatusPill.qml
        qml/components/CompoundRow.qml
        qml/components/PlanningCalendar.qml
        qml/components/DbStatusPill.qml
        qml/components/DbConfigBanner.qml

        qml/components/DashboardCardsList.qml
        qml/components/DashboardHeader.qml
        qml/components/DashboardTopBar.qml
        qml/components/ErrorBanner.qml

        qml/controls/Button.qml
        qml/controls/BusyIndicator.qml
        qml/controls/CheckBox.qml
        qml/controls/TextField.qml
        qml/controls/ToolBar.qml

        qml/components/ProjectHeader.qml
        qml/components/ProjectInfoCard.qml
        qml/components/ProjectActionsBar.qml

        qml/components/CalendarEventRow.qml
        qml/components/CalendarHeader.qml

        qml/dialogs/DatePickerDialog.qml
        qml/dialogs/ImportRequestDialog.qml
        qml/dialogs/ThemeCustomizerDialog.qml
        qml/dialogs/FeedingAnalysisWindow.qml

    RESOURCES
        qml/fonts/MaterialSymbolsOutlined.ttf
        qml/fonts/Sen-Regular.ttf
        qml/fonts/Sen-Bold.ttf
        qml/fonts/MavenPro-Regular.ttf
        qml/fonts/MavenPro-Bold.ttf
        qml/fonts/Inter-Italic-VariableFont.ttf
        qml/fonts/Inter-VariableFont.ttf
        qml/fonts/JetBrainsMono-VariableFont.ttf
        qml/fonts/JetBrainsMono-Italic-VariableFont.ttf
        qml/images/sphere.png
)

# Link libraries
target_link_libraries(Helix PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::Concurrent
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Graphs
    Qt6::Multimedia
    Qt6::Core5Compat
    openwall_crypt
    QXlsx
    grpc++
    grpc++_reflection
    libprotobuf
)

# Compile flags
target_compile_definitions(Helix PRIVATE USE_QXLSX=1)

# Compile as pure Windows GUI application (no console popup window) and Mac OS X App Bundle
set_target_properties(Helix PROPERTIES 
    WIN32_EXECUTABLE TRUE 
    MACOSX_BUNDLE TRUE
)

# Auto-copy PostgreSQL DLLs to the output directory on Windows so the executable can load the database driver
if(WIN32)
    add_custom_command(TARGET Helix POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources/dlls"
        "$<TARGET_FILE_DIR:Helix>"
        COMMENT "Copying PostgreSQL database driver DLLs to output directory..."
    )
endif()
