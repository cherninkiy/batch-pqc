add_executable(test_adapters_smoke test_adapters_smoke.c)

target_link_libraries(test_adapters_smoke PRIVATE batch_bench_adapters)

target_compile_features(test_adapters_smoke PRIVATE c_std_99)

add_test(NAME test_adapters_smoke COMMAND test_adapters_smoke)

add_executable(test_adapters_batch test_adapters_batch.c)

target_link_libraries(test_adapters_batch PRIVATE batch_bench_adapters)

target_compile_features(test_adapters_batch PRIVATE c_std_99)

add_test(NAME test_adapters_batch COMMAND test_adapters_batch)

add_executable(test_merkle test_merkle.c)

target_link_libraries(test_merkle PRIVATE batch_signing_core)

target_compile_features(test_merkle PRIVATE c_std_99)

add_test(NAME test_merkle COMMAND test_merkle)

add_executable(test_batch_signing test_batch_signing.c)

target_link_libraries(test_batch_signing PRIVATE batch_adapters)

target_compile_features(test_batch_signing PRIVATE c_std_99)

add_test(NAME test_batch_signing COMMAND test_batch_signing)

add_executable(test_streebog test_streebog.c)

target_link_libraries(test_streebog PRIVATE streebog)

target_compile_features(test_streebog PRIVATE c_std_99)

add_test(NAME test_streebog COMMAND test_streebog)
