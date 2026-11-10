# core/ = no ESP-IDF dependency. Same code builds on host and device.
add_library(ftota_core STATIC src/chunk.c src/patch.c)
target_include_directories(ftota_core PUBLIC include)

add_executable(test_chunk tests/test_chunk.c)
target_link_libraries(test_chunk ftota_core unity)
add_test(NAME test_chunk COMMAND test_chunk)

add_executable(test_patch tests/test_patch.c)
target_link_libraries(test_patch ftota_core unity)
add_test(NAME test_patch COMMAND test_patch)
