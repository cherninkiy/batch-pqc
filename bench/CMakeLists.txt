add_executable(bench_seq bench_seq.c)

target_link_libraries(bench_seq PRIVATE batch_bench_adapters)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	target_link_options(bench_seq PRIVATE -Wl,--allow-multiple-definition)
endif()

target_compile_features(bench_seq PRIVATE c_std_99)
