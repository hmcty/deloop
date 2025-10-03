set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

find_program(CMAKE_C_COMPILER "arm-none-eabi-gcc" REQUIRED)
find_program(CMAKE_CXX_COMPILER "arm-none-eabi-g++" REQUIRED)
find_program(CMAKE_ASM_COMPILER "arm-none-eabi-gcc" REQUIRED)
find_program(CMAKE_OBJCOPY "arm-none-eabi-objcopy" REQUIRED)
find_program(CMAKE_SIZE_UTIL "arm-none-eabi-size" REQUIRED)

# Reuse toolchain provided by LibDaisy (requires compiler to be specified):
include(${CMAKE_CURRENT_LIST_DIR}/../external/libDaisy/cmake/toolchains/ArmGNUToolchain.cmake)
