cmake_minimum_required(VERSION 3.10)
project(OMPTSyntaxHighlight)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
        Source.cpp
)

set(HEADERS
        clipboardxx.hpp
        detail/exception.hpp
        detail/interface.hpp
        detail/linux.hpp
        detail/windows.hpp
        detail/linux/provider.hpp
        detail/linux/x11_event_handler.hpp
        detail/linux/x11_provider.hpp
        detail/linux/xcb/xcb.hpp
        detail/linux/xcb/xcb_event.hpp
)

add_executable(${PROJECT_NAME} ${SOURCES})


if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
            /W4
            /WX
            /permissive-
            /wd4388  # Disable signed/unsigned mismatch
            /wd4389  # Disable signed/unsigned mismatch for '==' operator
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Werror
            -Wcast-align
            -Wcast-qual
            -Wconversion
            -Wdouble-promotion
            -Wformat=2
            -Wformat-security
            -Wnull-dereference
            -Wold-style-cast
            -Woverloaded-virtual
            -Wshadow
            -Wunused
            -Wno-sign-compare
            -Wno-sign-conversion
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(${PROJECT_NAME} PRIVATE
                -Wmisleading-indentation
                -Wduplicated-cond
                -Wduplicated-branches
                -Wlogical-op
                -Wuseless-cast
                -Wnrvo
                -Wpessimizing-move
        )
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${PROJECT_NAME} PRIVATE
                -Weverything
                -Wno-padded
                -Wno-documentation-unknown-command
        )
    endif()
endif()


target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    find_package(XCB REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE
            X11::X11
            XCB::XCB
    )
elseif(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            _WIN32_WINNT=0x0601
    )
endif()