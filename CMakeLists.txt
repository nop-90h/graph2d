cmake_minimum_required(VERSION 3.21)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ensurevcpkg.cmake)

project(game)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_BUILD_PARALLEL_LEVEL 8)

# ==============================================================================
# 1. SEARCH AND COLLECT PROJECT SOURCE FILES
# ==============================================================================
file( GLOB_RECURSE INC_HEADERS INC/*.h )
file( GLOB_RECURSE SRC_SOURCES SRC/*.cc )
file( GLOB_RECURSE INC_THIRDPARTY_HEADERS INC/thirdparty/*.h )
file( GLOB_RECURSE SRC_THIRDPARTY_SOURCES SRC/thirdparty/*.cpp )
file( GLOB_RECURSE INC_SPINE_HEADERS INC/thirdparty/spine/*.h )
file( GLOB_RECURSE SRC_SPINE_SOURCES SRC/thirdparty/spine/*.cpp )
file( GLOB_RECURSE INC_G2D_HEADERS INC/g2d/*.h )
file( GLOB_RECURSE SRC_G2D_SOURCES SRC/g2d/*.cc )
file( GLOB_RECURSE INC_G2D_DIALOGS_HEADERS INC/g2d/dialogs/*.h )
file( GLOB_RECURSE SRC_G2D_DIALOGS_SOURCES SRC/g2d/dialogs/*.cc )
file( GLOB_RECURSE INC_G2D_WIDGETS_HEADERS INC/g2d/widgets/*.h )
file( GLOB_RECURSE SRC_G2D_WIDGETS_SOURCES SRC/g2d/widgets/*.cc )

set(GAME_SOURCES
    ${SRC_SOURCES}
    ${INC_HEADERS}
    ${INC_THIRDPARTY_HEADERS}
    ${SRC_THIRDPARTY_SOURCES}
    ${INC_SPINE_HEADERS}
    ${SRC_SPINE_SOURCES}
    ${INC_G2D_HEADERS}
    ${SRC_G2D_SOURCES}
    ${INC_G2D_DIALOGS_HEADERS}
    ${SRC_G2D_DIALOGS_SOURCES}
    ${INC_G2D_WIDGETS_HEADERS}
    ${SRC_G2D_WIDGETS_SOURCES}
)

# ==============================================================================
# 2. PLATFORM-SPECIFIC CONFIGURATION (WEB / DESKTOP)
# ==============================================================================
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten" OR EMSCRIPTEN)
    message(STATUS "Configuring project for Emscripten (WebAssembly)")
    file( GLOB_RECURSE INC_PLATFORM_HEADERS INC/g2d/platforms/web/*.h )
    add_executable(game ${GAME_SOURCES} ${INC_PLATFORM_HEADERS})
    set(CMAKE_EXECUTABLE_SUFFIX .html)
    target_compile_definitions(game PRIVATE TARGET_EMSCRIPTEN)

    set(BOOST_DIR "${CMAKE_BINARY_DIR}/boost")
    set(BOOST_MODULES
        intrusive circular_buffer
        assert config container_hash core static_assert type_traits
        predef describe detail integer io throw_exception
        move utility iterator concept_check conversion function_types
        fusion mpl optional smart_ptr preprocessor tuple typeof
        variant type_index variant2 mp11
    )

    if(NOT EXISTS "${BOOST_DIR}/libs/intrusive/include/boost/intrusive/list.hpp")
        find_package(Git REQUIRED)

        if(NOT EXISTS "${BOOST_DIR}/.git")
            message(STATUS "[boost] Cloning boost superproject (shallow, no submodules)...")
            execute_process(
                COMMAND ${GIT_EXECUTABLE} clone --depth 1 --single-branch
                        --branch boost-1.83.0
                        https://github.com/boostorg/boost.git "${BOOST_DIR}"
                RESULT_VARIABLE _boost_rc
                OUTPUT_QUIET
            )
            if(NOT _boost_rc EQUAL 0)
                message(FATAL_ERROR "[boost] superproject clone failed (rc=${_boost_rc})")
            endif()
        endif()

        foreach(m IN LISTS BOOST_MODULES)
            message(STATUS "[boost] Fetching submodule: ${m}")
            execute_process(
                COMMAND ${GIT_EXECUTABLE} submodule update --init --depth 1 "libs/${m}"
                WORKING_DIRECTORY "${BOOST_DIR}"
                RESULT_VARIABLE _sub_rc
                OUTPUT_QUIET
            )
            if(NOT _sub_rc EQUAL 0)
                message(FATAL_ERROR "[boost] submodule '${m}' failed (rc=${_sub_rc})")
            endif()
            execute_process(
                COMMAND ${GIT_EXECUTABLE} -C "${BOOST_DIR}/libs/${m}"
                        sparse-checkout set include
                OUTPUT_QUIET ERROR_QUIET
            )
        endforeach()
    endif()

    foreach(m IN LISTS BOOST_MODULES)
        if(EXISTS "${BOOST_DIR}/libs/${m}/include")
            target_include_directories(game PRIVATE "${BOOST_DIR}/libs/${m}/include")
        endif()
    endforeach()

    # Emscripten linker flags
    set(EMSCRIPTEN_LINK_FLAGS "")
    string(APPEND EMSCRIPTEN_LINK_FLAGS " -s MIN_WEBGL_VERSION=1")
    string(APPEND EMSCRIPTEN_LINK_FLAGS " -s MAX_WEBGL_VERSION=2")
    string(APPEND EMSCRIPTEN_LINK_FLAGS " -s WEBGL2_BACKWARDS_COMPATIBILITY_EMULATION=1")
    string(APPEND EMSCRIPTEN_LINK_FLAGS " -s DISABLE_EXCEPTION_CATCHING=1")
    string(APPEND EMSCRIPTEN_LINK_FLAGS " -s STACK_SIZE=512KB")
    string(APPEND EMSCRIPTEN_LINK_FLAGS " -s INITIAL_MEMORY=33554432")
    string(APPEND EMSCRIPTEN_LINK_FLAGS " -s ALLOW_MEMORY_GROWTH=1")
    string(APPEND EMSCRIPTEN_LINK_FLAGS " -s ERROR_ON_UNDEFINED_SYMBOLS=1")
    string(APPEND EMSCRIPTEN_LINK_FLAGS " -s WASM=1")
    string(APPEND EMSCRIPTEN_LINK_FLAGS " -s MALLOC='emmalloc'")
    string(APPEND EMSCRIPTEN_LINK_FLAGS " -s EXPORTED_RUNTIME_METHODS=callMain,stringToUTF8")
    string(APPEND EMSCRIPTEN_LINK_FLAGS " -s EXPORTED_FUNCTIONS=_main,_free")
    string(APPEND EMSCRIPTEN_LINK_FLAGS " -s EXIT_RUNTIME=0")
    string(APPEND EMSCRIPTEN_LINK_FLAGS " -s FORCE_FILESYSTEM=0")
    string(APPEND EMSCRIPTEN_LINK_FLAGS " --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/shell.html")
    string(APPEND EMSCRIPTEN_LINK_FLAGS " --embed-file ${CMAKE_CURRENT_SOURCE_DIR}/EMBED@EMBED")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EMSCRIPTEN_LINK_FLAGS}")

    target_include_directories(game PRIVATE ./INC/g2d/platforms/web)
else()
    message(STATUS "Configuring project for Desktop (Windows / Linux / macOS)")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/INC/g2d/platforms/desktop")
        file( GLOB_RECURSE INC_PLATFORM_HEADERS INC/g2d/platforms/desktop/*.h )
        set(PLATFORM_HEADERS ${INC_PLATFORM_HEADERS})
    endif()
    add_executable(game ${GAME_SOURCES} ${PLATFORM_HEADERS})
    target_compile_definitions(game PRIVATE TARGET_WIN)
    if(PLATFORM_HEADERS)
        target_include_directories(game PRIVATE ./INC/g2d/platforms/desktop)
    endif()

    # GLFW3
    find_package(glfw3 CONFIG REQUIRED)
    target_link_libraries(game PRIVATE glfw)

    # GLAD
    find_package(glad CONFIG REQUIRED)
    target_link_libraries(game PRIVATE glad::glad)

    # Boost (desktop uses vcpkg)
    find_package(boost_intrusive CONFIG REQUIRED)
    find_package(boost_circular_buffer CONFIG REQUIRED)
    target_link_libraries(game PRIVATE
        Boost::intrusive
        Boost::circular_buffer
    )

    # Linux threading
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        find_package(Threads REQUIRED)
        target_link_libraries(game PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
    endif()
endif()

# ==============================================================================
# 3. GLOBAL INCLUDE DIRECTORIES AND COMPILER FLAGS
# ==============================================================================
target_include_directories(game PRIVATE ./INC)
target_include_directories(game PRIVATE ./INC/thirdparty)
target_include_directories(game PRIVATE ./INC/thirdparty/spine)
target_include_directories(game PRIVATE ./INC/g2d)
target_include_directories(game PRIVATE ./INC/g2d/dialogs)
target_include_directories(game PRIVATE ./INC/g2d/widgets)

target_compile_features(game PRIVATE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(game PRIVATE -fexperimental-library)
endif()