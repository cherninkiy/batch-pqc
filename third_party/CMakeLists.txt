
# third_party/CMakeLists.txt
# Collects all submodules: shipovnik, hypericum, kryzhovnik, iaik_merkle_tree

cmake_minimum_required(VERSION 3.14)
project(ThirdPartyModules LANGUAGES C CXX)

# -------------------------------------------------------------------
# 1. shipovnik (shipovnik/CMakeLists.txt)
# -------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/shipovnik/CMakeLists.txt")
    add_subdirectory(shipovnik)
else()
    message(WARNING "shipovnik submodule not initialized or missing CMakeLists.txt")
endif()

# -------------------------------------------------------------------
# 2. hypericum (hypericum/CMakeLists.txt)
# -------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/hypericum/CMakeLists.txt")
    add_subdirectory(hypericum)
else()
    message(WARNING "hypericum submodule not initialized or missing CMakeLists.txt")
endif()

# -------------------------------------------------------------------
# 3. kryzhovnik (no CMakeLists.txt – build manually)
# -------------------------------------------------------------------
set(KRYZHOVNIK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kryzhovnik)
if(EXISTS "${KRYZHOVNIK_DIR}/pqs_test.c")   # marker: submodule is loaded
    # Find OpenSSL
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(OPENSSL REQUIRED libcrypto)
    else()
        find_package(OpenSSL REQUIRED)
    endif()
    if(NOT OPENSSL_FOUND)
        message(FATAL_ERROR "OpenSSL not found (required for kryzhovnik)")
    endif()

    # All sources except the main test file
    file(GLOB_RECURSE KRYZHOVNIK_SOURCES
        "${KRYZHOVNIK_DIR}/*.c"
    )
    list(REMOVE_ITEM KRYZHOVNIK_SOURCES "${KRYZHOVNIK_DIR}/pqs_test.c")

    # Build all sources as C++
    set_source_files_properties(${KRYZHOVNIK_SOURCES} PROPERTIES LANGUAGE CXX)
    add_library(kryzhovnik STATIC ${KRYZHOVNIK_SOURCES})
    target_include_directories(kryzhovnik PRIVATE
        ${KRYZHOVNIK_DIR}
        ${OPENSSL_INCLUDE_DIRS}
    )
    target_link_libraries(kryzhovnik PRIVATE ${OPENSSL_LIBRARIES})
    target_compile_options(kryzhovnik PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>>:-Wall -Wextra -Wshadow -Wconversion -O3 -march=native>
        $<$<C_COMPILER_ID:MSVC>:/W4 /O2>
    )

    # Test for kryzhovnik (if testing is enabled)
    if(BUILD_TESTING)
        set_source_files_properties(${KRYZHOVNIK_DIR}/pqs_test.c PROPERTIES LANGUAGE CXX)
        add_executable(kryzhovnik_test ${KRYZHOVNIK_DIR}/pqs_test.c)
        target_link_libraries(kryzhovnik_test PRIVATE kryzhovnik)
        target_compile_definitions(kryzhovnik_test PRIVATE ${CRYPTO_PQPQC_CONFIG_DEFINITIONS})
        add_test(NAME kryzhovnik_test COMMAND kryzhovnik_test)
    endif()
else()
    message(WARNING "kryzhovnik submodule not initialized (run 'git submodule update --init')")
endif()

# -------------------------------------------------------------------
# 4. iaik_merkle_tree (no CMakeLists.txt – build manually)
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
            add_executable(merkletree_test ${MERKLE_DIR}/tests/MerkleTreeTest.cpp)
            target_include_directories(merkletree_test PRIVATE ${CPPUNIT_INCLUDE_DIRS})
            target_link_libraries(merkletree_test PRIVATE merkletree ${CPPUNIT_LIBRARIES})
            add_test(NAME merkletree_test COMMAND merkletree_test)
        else()
            message(WARNING "CppUnit not found, skipping merkletree tests")
        endif()
    endif()
else()
    message(WARNING "iaik_merkle_tree submodule not initialized (run 'git submodule update --init')")
endif()