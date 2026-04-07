add_executable(adapters_smoke adapters_smoke.c)

target_link_libraries(adapters_smoke PRIVATE batch_bench_adapters)

target_compile_features(adapters_smoke PRIVATE c_std_99)

add_test(NAME adapters_smoke COMMAND adapters_smoke)

add_executable(test_adapters_batch test_adapters_batch.c)

target_link_libraries(test_adapters_batch PRIVATE batch_bench_adapters)

target_compile_features(test_adapters_batch PRIVATE c_std_99)

add_test(NAME test_adapters_batch COMMAND test_adapters_batch)
