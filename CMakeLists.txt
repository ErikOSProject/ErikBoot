cmake_minimum_required(VERSION 3.16)
project(ErikBoot VERSION 0.1)

if(NOT EFI_TOOLCHAIN)
message(FATAL_ERROR "Please specify a compatible toolchain file. 
For example: \"cmake -DCMAKE_TOOLCHAIN_FILE=../efi-x64.cmake ..\"")
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

add_compile_options(-Wall -Wextra -pedantic -Werror)
add_compile_options(-ffreestanding)
add_compile_options(-fshort-wchar)
add_compile_options(-mno-red-zone)
add_compile_options(-Wno-unused-variable)
add_compile_options(-Wno-unused-command-line-argument)
add_compile_options(-Wno-c11-extensions)
add_compile_options(-Wno-c99-extensions)
add_compile_options(-Wno-c2x-extensions)
add_compile_options(-Wno-language-extension-token)
add_compile_options(-Wno-microsoft-enum-value)
add_compile_options(-Wno-microsoft-static-assert)
add_compile_options(-Wno-writable-strings)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld-link")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-entry:efi_main")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-subsystem:efi_application")

add_executable(${PROJECT_NAME}.efi
    src/file.c
    src/main.c
)

target_include_directories(${PROJECT_NAME}.efi PUBLIC 
    include include/UEFI ${EFI_ARCH_INCLUDES})
