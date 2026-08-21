cmake -B build -DCMAKE_BUILD_TYPE=Debug -DWISP_LOG_LEVEL=VERBOSE -DWISP_BUILD_GTK_FRONTEND=ON -DWISP_ENABLE_TESTS=ON && cmake --build build -j$(nproc)
cd build && ctest --test-dir . --output-on-failure
