cmake_minimum_required(VERSION 3.14)

project(gravador)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

# Obtém CLI11
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.3.2
)

# Obtém FTXUI
FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG v4.1.1
)

# Obtém LIBSERIALPORT
FetchContent_Declare(
    serialport
    GIT_REPOSITORY git://sigrok.org/libserialport
)
FetchContent_GetProperties(serialport)
if(NOT serialport_POPULATED)
    FetchContent_Populate(serialport)

    set(
        SOURCE_FILES
        ${serialport_SOURCE_DIR}/serialport.c
        ${serialport_SOURCE_DIR}/timing.c 
        ${serialport_SOURCE_DIR}/libserialport_internal.h 
        ${serialport_SOURCE_DIR}/libserialport.h
    )

    if(WIN32)
        if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
            add_compile_definitions(LIBSERIALPORT_MSBUILD)
        endif()
        set(SOURCE_FILES ${SOURCE_FILES} ${serialport_SOURCE_DIR}/windows.c)
    else()
        execute_process(COMMAND ${serialport_SOURCE_DIR}/autogen.sh)
        execute_process(COMMAND ${serialport_SOURCE_DIR}/configure)
        add_compile_definitions(LIBSERIALPORT_ATBUILD)
    endif()

    if(UNIX)
        set(
            SOURCE_FILES
            ${SOURCE_FILES}
            ${serialport_SOURCE_DIR}/linux.c
            ${serialport_SOURCE_DIR}/linux_termios.c
            ${serialport_SOURCE_DIR}/linux_termios.h
        )
    endif()

    if(APPLE)
        set(SOURCE_FILES ${SOURCE_FILES} ${serialport_SOURCE_DIR}/macosx.c)
    endif()

    if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
        set(SOURCE_FILES ${SOURCE_FILES} ${serialport_SOURCE_DIR}/freebsd.c)
    endif()

    add_library(serialport STATIC ${SOURCE_FILES})
    target_include_directories(
        serialport
        INTERFACE ${serialport_SOURCE_DIR}
        PUBLIC ${CMAKE_BINARY_DIR}
    )
    if(WIN32)
        target_link_libraries(serialport SetupAPI)
    endif()

endif()

FetchContent_MakeAvailable(CLI11 ftxui serialport)

add_executable(gravador src/main.cpp src/SerialPort.cpp src/SerialPort.h src/Wav.h src/Wav.cpp)
target_link_libraries(
    gravador
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component
    CLI11::CLI11
    serialport
)