
# third_party/CMakeLists.txt
# Collects all submodules: shipovnik, hypericum, kryzhovnik, iaik_merkle_tree

cmake_minimum_required(VERSION 3.14)
project(ThirdPartyModules LANGUAGES C CXX)

# -------------------------------------------------------------------
# 1. streebog (from hypericum/streebog sources — shared by hypericum and shipovnik)
# -------------------------------------------------------------------
set(STREEBOG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hypericum/streebog)
if(EXISTS "${STREEBOG_DIR}/gost3411-2012-core.c")
    set(GOST_OPTIMIZATION "0" CACHE STRING "Set GOST optimization level (0=none, 2=SSE2, 3=SSE4.1)")
    set_property(CACHE GOST_OPTIMIZATION PROPERTY STRINGS "0" "1" "2" "3")

    add_library(streebog STATIC
        ${STREEBOG_DIR}/gost3411-2012-core.c
    )
    target_include_directories(streebog PUBLIC
        $<BUILD_INTERFACE:${STREEBOG_DIR}>
    )
    set(INSTRUCTION_SET_SSE2  2)
    set(INSTRUCTION_SET_SSE41 3)
    if(GOST_OPTIMIZATION GREATER_EQUAL INSTRUCTION_SET_SSE41)
        message(STATUS "GOST 34.11-2012 SSE4.1 optimization enabled")
        target_compile_options(streebog PRIVATE -msse4.1 -msse2)
        target_compile_definitions(streebog PRIVATE __GOST3411_HAS_SSE41__ __GOST3411_HAS_SSE2__)
    elseif(GOST_OPTIMIZATION GREATER_EQUAL INSTRUCTION_SET_SSE2)
        message(STATUS "GOST 34.11-2012 SSE2 optimization enabled")
        target_compile_options(streebog PRIVATE -msse2)
        target_compile_definitions(streebog PRIVATE __GOST3411_HAS_SSE2__)
    else()
        message(STATUS "GOST 34.11-2012 no optimization enabled")
    endif()
    set_property(TARGET streebog PROPERTY POSITION_INDEPENDENT_CODE ON)
else()
    message(WARNING "hypericum/streebog not found — hypericum and shipovnik will not be built")
endif()

# -------------------------------------------------------------------
# 2. hypericum (built directly, without hypericum/CMakeLists.txt)
# -------------------------------------------------------------------
set(HYPERICUM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hypericum)
if(TARGET streebog AND EXISTS "${HYPERICUM_DIR}/hypericum.c")
    set(HYPERICUM_PARAMSET "b_256_64" CACHE STRING "Select a hypericum parameter set")
    set_property(CACHE HYPERICUM_PARAMSET PROPERTY STRINGS
        "b_256_64" "m_256_64" "b_256_20" "m_256_20" "b_128_20" "m_128_20" "debug")
    if(NOT HYPERICUM_PARAMSET)
        set(HYPERICUM_PARAMSET "b_256_64")
    endif()
    message(STATUS "Using Hypericum parameter set ${HYPERICUM_PARAMSET}")

    if(HYPERICUM_PARAMSET STREQUAL "debug")
        set(HYPERICUM_PARAMSET_INCLUDE "paramsets/hypericum/params_debug.h")
    else()
        set(HYPERICUM_PARAMSET_INCLUDE "paramsets/params_${HYPERICUM_PARAMSET}.h")
    endif()

    # Generate current-paramset.h in build tree from superproject template.
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/hypericum-current-paramset.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/hypericum/current-paramset.h
    )

    add_library(hypericum STATIC
        ${HYPERICUM_DIR}/sei.c
        ${HYPERICUM_DIR}/sei_cryptoapi.c
        ${HYPERICUM_DIR}/sei_urandom.c
        ${HYPERICUM_DIR}/drbg.c
        ${HYPERICUM_DIR}/adrs.c
        ${HYPERICUM_DIR}/wotsc.c
        ${HYPERICUM_DIR}/sign.c
        ${HYPERICUM_DIR}/hash.c
        ${HYPERICUM_DIR}/utils.c
        ${HYPERICUM_DIR}/pack.c
        ${HYPERICUM_DIR}/stack.c
        ${HYPERICUM_DIR}/streebog.c
        ${HYPERICUM_DIR}/xmss.c
        ${HYPERICUM_DIR}/xmssmt.c
        ${HYPERICUM_DIR}/fors.c
        ${HYPERICUM_DIR}/hypericum.c
    )
    target_include_directories(hypericum
        PUBLIC
            $<BUILD_INTERFACE:${HYPERICUM_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/hypericum>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        PRIVATE
            ${HYPERICUM_DIR}
            ${STREEBOG_DIR}
    )
    target_link_libraries(hypericum PUBLIC streebog)
    target_compile_features(hypericum PRIVATE c_std_99)

    add_executable(hypericum_example ${HYPERICUM_DIR}/example.c)
    target_link_libraries(hypericum_example PRIVATE hypericum)
    target_compile_definitions(hypericum_example PRIVATE PARAMSET_NAME="${HYPERICUM_PARAMSET}")
else()
    message(WARNING "hypericum sources not found — skipping hypericum target")
endif()

# -------------------------------------------------------------------
# 3. shipovnik (built directly, without shipovnik/CMakeLists.txt)
# -------------------------------------------------------------------
set(SHIPOVNIK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shipovnik)
if(TARGET streebog AND EXISTS "${SHIPOVNIK_DIR}/src/shipovnik.c")
    set(ENTROPY_SOURCE "/dev/urandom" CACHE STRING "Entropy source path for shipovnik")

    file(GLOB SHIPOVNIK_SOURCES "${SHIPOVNIK_DIR}/src/*.c")

    add_library(shipovnik STATIC ${SHIPOVNIK_SOURCES})
    target_include_directories(shipovnik
        PUBLIC
            $<BUILD_INTERFACE:${SHIPOVNIK_DIR}/include/shipovnik>
        PRIVATE
            ${SHIPOVNIK_DIR}/src
            ${STREEBOG_DIR}
    )
    target_compile_definitions(shipovnik PRIVATE
        ENTROPY_SOURCE="${ENTROPY_SOURCE}"
        randombytes=shipovnik_randombytes
        secure_erase=shipovnik_secure_erase
    )
    target_link_libraries(shipovnik PRIVATE streebog)

    add_executable(shipovnik_example ${SHIPOVNIK_DIR}/shipovnik_example.c)
    target_link_libraries(shipovnik_example PRIVATE shipovnik)
else()
    message(WARNING "shipovnik sources not found — skipping shipovnik target")
endif()

# -------------------------------------------------------------------
# 4. kryzhovnik wrapper fork (built directly, without kryzhovnik/CMakeLists.txt)
# -------------------------------------------------------------------
set(KRYZHOVNIK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kryzhovnik)
if(EXISTS "${KRYZHOVNIK_DIR}/sign.c")
    set(KRYZHOVNIK_PARAMSET "medium" CACHE STRING "Select a kryzhovnik parameter set")
    set_property(CACHE KRYZHOVNIK_PARAMSET PROPERTY STRINGS "small" "medium" "large" "debug")
    if(NOT KRYZHOVNIK_PARAMSET)
        set(KRYZHOVNIK_PARAMSET "medium")
    endif()
    if(NOT KRYZHOVNIK_PARAMSET STREQUAL "small" AND
       NOT KRYZHOVNIK_PARAMSET STREQUAL "medium" AND
       NOT KRYZHOVNIK_PARAMSET STREQUAL "large" AND
       NOT KRYZHOVNIK_PARAMSET STREQUAL "debug")
        message(FATAL_ERROR "Unsupported KRYZHOVNIK_PARAMSET='${KRYZHOVNIK_PARAMSET}'. Use: small, medium, large, debug")
    endif()
    message(STATUS "Using Kryzhovnik parameter set ${KRYZHOVNIK_PARAMSET}")

    configure_file(
        ${KRYZHOVNIK_DIR}/current-params.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/kryzhovnik/current-params.h
    )

    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(OPENSSL QUIET IMPORTED_TARGET libcrypto)
    endif()
    if(TARGET PkgConfig::OPENSSL)
        set(KRYZHOVNIK_OPENSSL_TARGET PkgConfig::OPENSSL)
    else()
        find_package(OpenSSL REQUIRED)
        set(KRYZHOVNIK_OPENSSL_TARGET OpenSSL::Crypto)
    endif()

    set(KRYZHOVNIK_SOURCES
        ${KRYZHOVNIK_DIR}/cpucycles.c
        ${KRYZHOVNIK_DIR}/fips202.c
        ${KRYZHOVNIK_DIR}/getCPUTime.c
        ${KRYZHOVNIK_DIR}/poly_mul.c
        ${KRYZHOVNIK_DIR}/randombytes.c
        ${KRYZHOVNIK_DIR}/rng.c
        ${KRYZHOVNIK_DIR}/sign.c
    )
    if(EXISTS "${KRYZHOVNIK_DIR}/kryzhovnik_wrapper.c")
        list(APPEND KRYZHOVNIK_SOURCES ${KRYZHOVNIK_DIR}/kryzhovnik_wrapper.c)
    endif()

    add_library(kryzhovnik STATIC ${KRYZHOVNIK_SOURCES})
    target_include_directories(kryzhovnik PUBLIC
        $<BUILD_INTERFACE:${KRYZHOVNIK_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/kryzhovnik>
    )
    target_link_libraries(kryzhovnik PRIVATE ${KRYZHOVNIK_OPENSSL_TARGET})
    target_compile_options(kryzhovnik PRIVATE
        $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>>:-Wall -Wextra -Wshadow -Wconversion -O3 -march=native>
        $<$<C_COMPILER_ID:MSVC>:/W4 /O2>
    )
    target_compile_features(kryzhovnik PRIVATE c_std_99)

    if(BUILD_TESTING AND EXISTS "${KRYZHOVNIK_DIR}/pqs_test.c")
        add_executable(test_kryzhovnik ${KRYZHOVNIK_DIR}/pqs_test.c)
        target_link_libraries(test_kryzhovnik PRIVATE kryzhovnik)
        add_test(NAME test_kryzhovnik COMMAND test_kryzhovnik)
    endif()

    if(BUILD_TESTING AND EXISTS "${KRYZHOVNIK_DIR}/test_wrapper.c")
        add_executable(test_kryzhovnik_wrapper ${KRYZHOVNIK_DIR}/test_wrapper.c)
        target_link_libraries(test_kryzhovnik_wrapper PRIVATE kryzhovnik)
        add_test(NAME test_kryzhovnik_wrapper COMMAND test_kryzhovnik_wrapper)
    endif()

    if(BUILD_TESTING AND EXISTS "${KRYZHOVNIK_DIR}/test_pqs.c")
        add_executable(test_kryzhovnik_api ${KRYZHOVNIK_DIR}/test_pqs.c)
        target_link_libraries(test_kryzhovnik_api PRIVATE kryzhovnik)
        add_test(NAME test_kryzhovnik_api COMMAND test_kryzhovnik_api)
    endif()
else()
    message(WARNING "kryzhovnik submodule not initialized (run 'git submodule update --init')")
endif()

# -------------------------------------------------------------------
# 5. iaik_merkle_tree (no CMakeLists.txt – build manually)
# -------------------------------------------------------------------
set(MERKLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/iaik_merkle_tree)
if(EXISTS "${MERKLE_DIR}/src/merkletree.h")
    # Build sources from src
    file(GLOB MERKLE_SOURCES "${MERKLE_DIR}/src/*.c")
    add_library(merkletree STATIC ${MERKLE_SOURCES})
    target_include_directories(merkletree PUBLIC
        $<BUILD_INTERFACE:${MERKLE_DIR}/src>
    )
    # Install (optional)
    install(TARGETS merkletree ARCHIVE DESTINATION lib)
    install(FILES ${MERKLE_DIR}/src/merkletree.h DESTINATION include)

    # Merkle Tree tests (require CppUnit)
    if(BUILD_TESTING AND EXISTS "${MERKLE_DIR}/tests/MerkleTreeTest.cpp")
        find_package(PkgConfig)
        if(PkgConfig_FOUND)
            pkg_check_modules(CPPUNIT REQUIRED cppunit)
        else()
            find_package(CppUnit REQUIRED)
        endif()
        if(CPPUNIT_FOUND)
            add_executable(test_merkletree ${MERKLE_DIR}/tests/MerkleTreeTest.cpp)
            target_include_directories(test_merkletree PRIVATE ${CPPUNIT_INCLUDE_DIRS})
            target_link_libraries(test_merkletree PRIVATE merkletree ${CPPUNIT_LIBRARIES})
            add_test(NAME test_merkletree COMMAND test_merkletree)
        else()
            message(WARNING "CppUnit not found, skipping merkletree tests")
        endif()
    endif()
else()
    message(WARNING "iaik_merkle_tree submodule not initialized (run 'git submodule update --init')")
endif()