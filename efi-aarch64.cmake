set(CMAKE_SYSTEM_NAME Generic)

find_program(CLANG clang REQUIRED True)
set(CMAKE_C_COMPILER clang)

find_program(CLANGXX clang++ REQUIRED False)
set(CMAKE_CXX_COMPILER ${CLANGXX})

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_C_COMPILER_TARGET aarch64-unknown-windows)
set(CMAKE_CXX_COMPILER_TARGET aarch64-unknown-windows)
add_compile_options(-target ${CMAKE_C_COMPILER_TARGET})
add_link_options(-target ${CMAKE_C_COMPILER_TARGET})

set(EFI_ARCH_INCLUDES include/UEFI/AArch64)
set(EFI_TOOLCHAIN TRUE)
