# core/ = no ESP-IDF dependency. Same code builds on host and device.
add_library(ftota_core STATIC src/chunk.c)
target_include_directories(ftota_core PUBLIC include)
add_executable(test_chunk tests/test_chunk.c)
target_link_libraries(test_chunk ftota_core unity)
add_test(NAME test_chunk COMMAND test_chunk)
