add_executable(bench_seq bench_seq.c)

target_link_libraries(bench_seq PRIVATE batch_bench_adapters)

target_compile_features(bench_seq PRIVATE c_std_99)
