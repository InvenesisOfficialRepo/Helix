# Common utilities module

# Create common library
add_library(common INTERFACE)

# Link dependencies
target_link_libraries(common
    INTERFACE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
)

# Include directories
target_include_directories(common
    INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)