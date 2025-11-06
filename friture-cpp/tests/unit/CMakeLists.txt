# Unit tests for Friture C++

# Create ringbuffer test executable
add_executable(ringbuffer_test ringbuffer_test.cpp)

# Link against GoogleTest
target_link_libraries(ringbuffer_test
    GTest::gtest
    GTest::gtest_main
    pthread
)

# Include directories
target_include_directories(ringbuffer_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Enable sanitizers for tests (best practice for catching bugs early)
# AddressSanitizer detects: memory leaks, use-after-free, buffer overflows
# UndefinedBehaviorSanitizer detects: undefined behavior like integer overflow
if(CMAKE_BUILD_TYPE MATCHES "Debug" OR NOT CMAKE_BUILD_TYPE)
    # Sanitizer flags
    set(SANITIZER_FLAGS
        -fsanitize=address
        -fsanitize=undefined
        -fno-omit-frame-pointer
        -g
    )

    # Apply to compilation
    target_compile_options(ringbuffer_test PRIVATE ${SANITIZER_FLAGS})

    # Apply to linking (required for sanitizers to work)
    target_link_options(ringbuffer_test PRIVATE ${SANITIZER_FLAGS})

    message(STATUS "Enabled sanitizers for ringbuffer_test (ASan + UBSan)")
endif()

# Register test with CTest
add_test(NAME ringbuffer_test COMMAND ringbuffer_test)

# Set test properties
set_tests_properties(ringbuffer_test PROPERTIES
    TIMEOUT 30
    ENVIRONMENT "ASAN_OPTIONS=detect_leaks=1:check_initialization_order=1"
)

# ============================================================================
# Settings Test
# ============================================================================

# Create settings test executable
add_executable(settings_test settings_test.cpp)

# Link against GoogleTest
target_link_libraries(settings_test
    GTest::gtest
    GTest::gtest_main
    pthread
)

# Include directories
target_include_directories(settings_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Enable sanitizers for tests
if(CMAKE_BUILD_TYPE MATCHES "Debug" OR NOT CMAKE_BUILD_TYPE)
    # Apply sanitizer flags
    target_compile_options(settings_test PRIVATE ${SANITIZER_FLAGS})
    target_link_options(settings_test PRIVATE ${SANITIZER_FLAGS})
    message(STATUS "Enabled sanitizers for settings_test (ASan + UBSan)")
endif()

# Register test with CTest
add_test(NAME settings_test COMMAND settings_test)

# Set test properties
set_tests_properties(settings_test PROPERTIES
    TIMEOUT 10
    ENVIRONMENT "ASAN_OPTIONS=detect_leaks=1:check_initialization_order=1"
)
