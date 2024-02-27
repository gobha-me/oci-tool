find_package(fmt QUIET)

if (fmt_FOUND)
else ()
    if (NOT FMT_URI)
        set(FMT_URI https://github.com/fmtlib/fmt) 
    endif()

    if (NOT FMT_TAG)
        set(FMT_TAG 10.2.1)
    endif()

    include(FetchContent)
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY ${FMT_URI}
        GIT_TAG ${FMT_TAG}
    )

    FetchContent_MakeAvailable(fmt)
endif()

