#!/bin/bash
# Default values
BUILD_TYPE="debug"
BUILD_DIR="buildDir"
SANITIZER=""
CLEAN=0
RUN_GO_TESTS=0
RUN_PROG=0

# Help message
show_help() {
    echo "Usage: ./build.sh [options]"
    echo "Options:"
    echo "  -h, --help            Show this help message"
    echo "  -c, --clean           Clean build directory before building"
    echo "  -r, --release         Build in Release mode"
    echo "  -a, --asan          Build with Address Sanitizer"
    echo "  -m, --msan          Build with Memory Sanitizer"
    echo "  -t, --tsan          Build with Thread Sanitizer"
    echo "  -u, --ubsan           Build with Undefined Behavior Sanitizer"
    echo "  --run-go-tests        Build and run Go integration tests"
    echo "  --run-with-asan       Run the built program with ASan preloaded (only with -a)"
    echo ""
    echo "Note: MSAN only works with Clang"
    echo "      TSAN can't be combined with ASAN"
}

# Process command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--clean)
            CLEAN=1
            shift
            ;;
        -r|--release)
            BUILD_TYPE="release"
            shift
            ;;
        -a|--asan)
            SANITIZER="address"
            shift
            ;;
        -m|--msan)
            SANITIZER="memory"
            shift
            ;;
        -t|--tsan)
            SANITIZER="thread"
            shift
            ;;
        -u|--ubsan)
            SANITIZER="undefined"
            shift
            ;;
        --run-go-tests)
            RUN_GO_TESTS=1
            shift
            ;;
        --run-with-asan)
            RUN_PROG=1
            shift
            ;;
        *)
            echo "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Clean build directory if requested
if [[ $CLEAN -eq 1 ]]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Meson setup command
MESON_OPTS="-Dbuildtype=$BUILD_TYPE"

# Add sanitizer if specified
if [[ -n "$SANITIZER" ]]; then
    MESON_OPTS="$MESON_OPTS -Db_sanitize=$SANITIZER"
fi

# Setup build directory
echo "Setting up build with: meson setup $BUILD_DIR $MESON_OPTS"
LANG=C meson setup "$BUILD_DIR" $MESON_OPTS

# Check setup status
if [ $? -ne 0 ]; then
    echo "Meson setup failed!"
    exit 1
fi

# Build the project
echo "Building..."
ninja -C "$BUILD_DIR"

# Check build status
if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Binary location: $BUILD_DIR/oshell"
    
    # Run Go tests if requested
    if [[ $RUN_GO_TESTS -eq 1 ]]; then
        echo ""
        echo "Running Go integration tests..."
        
        # Check if integration_tests directory exists
        if [[ -d "integration_tests" ]]; then
            cd integration_tests
            
            # Check if go.mod exists
            if [[ -f "go.mod" ]]; then
                go test -v ./...
                GO_TEST_STATUS=$?
                cd ..
                
                # Check test status
                if [ $GO_TEST_STATUS -eq 0 ]; then
                    echo "All Go integration tests passed!"
                else
                    echo "Some Go integration tests failed!"
                    exit 1
                fi
            else
                echo "Error: go.mod not found in integration_tests directory"
                echo "Run 'cd integration_tests && go mod init shell-tests' to initialize"
                exit 1
            fi
        else
            echo "Error: integration_tests directory not found"
            echo "Create the directory and add your Go test files"
            exit 1
        fi
    fi

    # Run the program with ASan preloaded if requested
    if [[ $RUN_PROG -eq 1 ]]; then
        if [[ "$SANITIZER" == "address" ]]; then
            echo ""
            echo "Running the program with ASan preloaded..."

            # Find the libasan.so path
            ASAN_LIB_PATH=$(find /usr/lib -name 'libasan.so*' 2>/dev/null | head -n 1)

            if [[ -z "$ASAN_LIB_PATH" ]]; then
                echo "Error: libasan.so not found. Please install the ASan library (e.g., libasan-dev or libasan-x.x)."
                exit 1
            else
                LD_PRELOAD=$ASAN_LIB_PATH "./$BUILD_DIR/oshell"
            fi
        else
            echo "Warning: The --run-with-asan option is only valid when using --asan."
        fi
    fi

else
    echo "Build failed!"
    exit 1
fi
