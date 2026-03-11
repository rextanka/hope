# ---------------------------------------------------------------------------
# hope_lib — the interpreter as a static library.
# main.cpp is excluded so unit tests can link against the library without
# pulling in main(). The hope executable adds main.cpp on top.
# ---------------------------------------------------------------------------

file(GLOB_RECURSE HOPE_LIB_SOURCES CONFIGURE_DEPENDS
    lexer/*.cpp
    # parser/*.cpp        — added in Phase 1 when parser files exist
    # types/*.cpp         — added in Phase 2
    # compiler/*.cpp      — added in Phase 3
    # runtime/*.cpp       — added in Phase 3
    # builtins/*.cpp      — added in Phase 3
    # modules/*.cpp       — added in Phase 4
    # printer/*.cpp       — added in Phase 4
    # repl/*.cpp          — added in Phase 4
    # errors/*.cpp        — added in Phase 4
)

add_library(hope_lib STATIC ${HOPE_LIB_SOURCES})

target_include_directories(hope_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(hope_lib PUBLIC
    HOPELIB="${HOPELIB}"
)

target_compile_options(hope_lib PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
        -Wall -Wextra -Wpedantic -Wno-unused-parameter>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# ---------------------------------------------------------------------------
# hope — the interpreter executable
# ---------------------------------------------------------------------------
add_executable(hope main.cpp)
target_link_libraries(hope PRIVATE hope_lib)
