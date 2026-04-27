add_executable(bench_seq bench_seq.c)

target_link_libraries(bench_seq PRIVATE batch_bench_adapters)

target_compile_features(bench_seq PRIVATE c_std_99)

add_executable(bench_batch bench_batch.c)

target_link_libraries(bench_batch PRIVATE batch_adapters batch_bench_adapters streebog)

target_compile_features(bench_batch PRIVATE c_std_99)
