cmake_minimum_required(VERSION 3.16)
project(ErikBoot VERSION 0.1)

# You can pass -DTARGET_ARCH=<arch> to select an architecture
if(NOT DEFINED TARGET_ARCH)
    set(TARGET_ARCH "x86_64") # Default architecture
endif()

include(${TARGET_ARCH}.cmake OPTIONAL)

if(NOT EFI_TOOLCHAIN)
message(FATAL_ERROR "Please specify a compatible toolchain file. 
For example: \"cmake -DCMAKE_TOOLCHAIN_FILE=../x86_64.cmake ..\"")
endif()

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

file(GLOB SOURCES src/*.c)

add_executable(${PROJECT_NAME}.efi
    ${SOURCES}
    ${ARCH_SOURCES}
)

target_include_directories(${PROJECT_NAME}.efi PUBLIC 
    include include/UEFI ${EFI_ARCH_INCLUDES})

install(TARGETS ${PROJECT_NAME}.efi)
