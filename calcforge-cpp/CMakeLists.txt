cmake_minimum_required(VERSION 3.16)

project(CalcForge 
    VERSION 4.0.0
    DESCRIPTION "Advanced Calculator with Mathematical Operations and Unit Conversion"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Performance optimizations
if(MSVC)
    # MSVC optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")  # Whole program optimization
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")  # Link time code generation
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF")  # Remove unused code
else()
    # GCC/Clang optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")  # Link time optimization
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ffast-math")  # Fast math operations
endif()

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Network
)

# Enable Qt6 features
qt6_standard_project_setup()

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/WorksheetWidget.cpp
    src/ExpressionEditor.cpp
    src/ResultsDisplay.cpp
    src/LineNumberArea.cpp
    src/CalculationEngine.cpp
    src/DependencyTracker.cpp
    src/LineChangeDetector.cpp
    src/ReferenceUpdateEngine.cpp
    src/LNReferenceAutoUpdater.cpp
    src/Logger.cpp
    src/UnitConverter.cpp
    src/TimecodeCalculator.cpp
    src/AspectRatioCalculator.cpp
    src/DateCalculator.cpp
    src/CurrencyConverter.cpp
    src/PercentageCalculator.cpp
    src/SyntaxHighlighter.cpp
)

# Header files
set(HEADERS
    include/MainWindow.h
    include/WorksheetWidget.h
    include/ExpressionEditor.h
    include/ResultsDisplay.h
    include/LineNumberArea.h
    include/CalculationEngine.h
    include/DependencyTracker.h
    include/LineChangeDetector.h
    include/ReferenceUpdateEngine.h
    include/LNReferenceAutoUpdater.h
    include/Logger.h
    include/UnitConverter.h
    include/TimecodeCalculator.h
    include/AspectRatioCalculator.h
    include/DateCalculator.h
    include/CurrencyConverter.h
    include/PercentageCalculator.h
    include/SyntaxHighlighter.h
)

# Create executable
qt6_add_executable(CalcForge ${SOURCES} ${HEADERS})

# Link Qt6 libraries (minimal for hello world)
target_link_libraries(CalcForge PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

# Set application properties
set_target_properties(CalcForge PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    OUTPUT_NAME "CalcForge"
)

# Qt performance optimizations
target_compile_definitions(CalcForge PRIVATE
    QT_NO_DEBUG_OUTPUT          # Remove debug output in release
    QT_NO_WARNING_OUTPUT        # Remove warning output in release
    QT_NO_INFO_OUTPUT           # Remove info output in release
    QT_DISABLE_DEPRECATED_BEFORE=0x060000  # Disable deprecated Qt features
)

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    set_target_properties(CalcForge PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
elseif(APPLE)
    # macOS-specific settings
    set_target_properties(CalcForge PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    )
endif()

# Install rules
install(TARGETS CalcForge
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
