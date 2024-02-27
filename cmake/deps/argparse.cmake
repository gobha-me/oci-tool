find_package(argparse QUIET)

if (argparse_FOUND)
else ()
    if (NOT ARGPARSE_URI)
        set(ARGPARSE_URI https://github.com/p-ranav/argparse.git) 
    endif()

    if (NOT ARGPARSE_TAG)
        set(ARGPARSE_TAG v3.0)
    endif()

    include(FetchContent)
    FetchContent_Declare(
        argparse
        GIT_REPOSITORY ${ARGPARSE_URI}
        GIT_TAG ${ARGPARSE_TAG}
    )

    FetchContent_MakeAvailable(argparse)
endif()
