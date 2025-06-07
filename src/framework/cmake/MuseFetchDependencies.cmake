
if (OS_IS_WIN)
    include(FetchContent)

    if (DONOT_FETCH_MUSESCORE_DEPS)
        set(DEPENDENCIES_DIR ${CMAKE_SOURCE_DIR}/out/build/debug/_deps/musescore_prebuild_win_deps-src)
    else()
        FetchContent_Declare(
          musescore_prebuild_win_deps
          GIT_REPOSITORY https://github.com/musescore/musescore_prebuild_win_deps.git
          GIT_TAG        HEAD
        )
        FetchContent_MakeAvailable(musescore_prebuild_win_deps)
        set(DEPENDENCIES_DIR ${musescore_prebuild_win_deps_SOURCE_DIR})
    endif()

    set(DEPENDENCIES_LIB_DIR ${DEPENDENCIES_DIR}/libx64)
    set(DEPENDENCIES_INC ${DEPENDENCIES_DIR}/include)
endif(OS_IS_WIN)
