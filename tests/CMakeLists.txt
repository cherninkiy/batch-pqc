add_executable(adapters_smoke adapters_smoke.c)

target_link_libraries(adapters_smoke PRIVATE batch_bench_adapters)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	target_link_options(adapters_smoke PRIVATE -Wl,--allow-multiple-definition)
endif()

target_compile_features(adapters_smoke PRIVATE c_std_99)

add_test(NAME adapters_smoke COMMAND adapters_smoke)
