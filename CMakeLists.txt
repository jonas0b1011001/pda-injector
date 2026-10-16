cmake_minimum_required(VERSION 3.20)
project(PDAInjector LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W3 /EHsc)
endif()

add_definitions(-DWIN32 -D_WINDOWS -D_USRDLL -DPDAINJECTOR_EXPORTS -DDX11PROXY)

set(SOURCES
    PDA-Injector.cpp
    PDA-Injector.h
    dllmain.cpp
    dll_proxy.cpp
    inih/ini.c
    inih/ini.h
    inih/cpp/INIReader.cpp
    inih/cpp/INIReader.h
)

add_library(d3dx11_43 SHARED ${SOURCES})

target_include_directories(d3dx11_43 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inih
    ${CMAKE_CURRENT_SOURCE_DIR}/inih/cpp
)

target_link_libraries(d3dx11_43 PRIVATE
    gdiplus
    gdi32
    user32
    kernel32
    ole32
)

set_target_properties(d3dx11_43 PROPERTIES
    PREFIX ""
    OUTPUT_NAME "d3dx11_43"
)
