add_library(batch_bench_adapters STATIC
    signature/registry.c
    signature/hypericum_adapter.c
    signature/shipovnik_adapter.c
    signature/kryzhovnik_adapter.c
    utils/timer.c
    utils/message_gen.c
)

target_include_directories(batch_bench_adapters
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/third_party/kryzhovnik
)

target_link_libraries(batch_bench_adapters
    PUBLIC
        hypericum
        shipovnik
        kryzhovnik
)

target_compile_features(batch_bench_adapters PRIVATE c_std_99)

add_library(batch_signing_core STATIC
    merkle/merkle.c
    batch_signing.c
    batch_adapters.c
)

target_include_directories(batch_signing_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(batch_signing_core
    PUBLIC
        batch_bench_adapters
)

target_compile_features(batch_signing_core PRIVATE c_std_99)
