1. The CI is failing on `test_nscolour` with an ASAN odr-violation for `nscolours`.
2. Looking at memory and CMakeLists.txt, we shouldn't compile `src/utils/nscolour.c` in the test target itself if it's already in `libwisp.so`. The `add_wisp_test` should only contain the test file and not the implementation files `nscolour.c`, `system_colour.c`, `nsoption.c`.
3. I'll modify `src/test/CMakeLists.txt` to remove these implementation files from the `test_nscolour` target.
4. Run `cmake --build build -j$(nproc) && export ASAN_OPTIONS=detect_odr_violation=1 && ctest --test-dir build` to verify.
5. Complete pre-commit steps.
6. Submit changes.
