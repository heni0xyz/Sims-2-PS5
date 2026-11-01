# Tool target definitions follow the MIT SDK's cmake/XbeRecomp.cmake, without
# configuring its Windows desktop runtime or fetching SDL/ImGui for this stage.
# Upstream attribution and license: sdk/LICENSE, Copyright 2026 TGP482.
include(FetchContent)
set(ZYDIS_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_DOXYGEN OFF CACHE BOOL "" FORCE)
FetchContent_Declare(zydis GIT_REPOSITORY https://github.com/zyantific/zydis.git GIT_TAG a2278f1d254e492f6a6b39f6cb5d1f5d515659dc GIT_SHALLOW FALSE)
FetchContent_Declare(tomlplusplus GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git GIT_TAG 30172438cee64926dc41fdd9c11fb3ba5b2ba9de GIT_SHALLOW FALSE)
FetchContent_MakeAvailable(zydis tomlplusplus)
set(SDK ${PROJECT_SOURCE_DIR}/sdk)
file(GLOB RECOMP_SOURCES CONFIGURE_DEPENDS ${SDK}/tools/recomp/*.cpp)
add_executable(xbe_recomp ${RECOMP_SOURCES})
target_include_directories(xbe_recomp PRIVATE ${SDK}/runtime ${SDK}/tools/recomp)
target_compile_definitions(xbe_recomp PRIVATE _CRT_SECURE_NO_WARNINGS)
target_link_libraries(xbe_recomp PRIVATE Zydis tomlplusplus::tomlplusplus)
add_executable(xbe_symbols ${SDK}/tools/symbols/main.cpp)
target_include_directories(xbe_symbols PRIVATE ${SDK}/runtime)
target_compile_definitions(xbe_symbols PRIVATE _CRT_SECURE_NO_WARNINGS)
target_link_libraries(xbe_symbols PRIVATE Zydis tomlplusplus::tomlplusplus)
add_executable(iso_xbe iso_xbe.cpp ${SDK}/runtime/kernel/disc.cpp)
target_include_directories(iso_xbe PRIVATE ${SDK}/runtime)
add_executable(iso_assets iso_assets.cpp ${SDK}/runtime/kernel/disc.cpp)
target_include_directories(iso_assets PRIVATE ${SDK}/runtime)
add_executable(iso_verify iso_verify.cpp ${PROJECT_SOURCE_DIR}/src/game/disc_boot.cpp
    ${PROJECT_SOURCE_DIR}/runtime-sdk/runtime/kernel/disc.cpp ${PROJECT_SOURCE_DIR}/runtime-sdk/runtime/kernel/crypto.cpp)
target_include_directories(iso_verify PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/runtime-sdk/runtime)
add_executable(iso_install iso_install.cpp ${PROJECT_SOURCE_DIR}/src/game/install.cpp
    ${PROJECT_SOURCE_DIR}/src/game/install_notices.cpp ${PROJECT_SOURCE_DIR}/src/game/disc_boot.cpp
    ${PROJECT_SOURCE_DIR}/runtime-sdk/runtime/kernel/disc.cpp ${PROJECT_SOURCE_DIR}/runtime-sdk/runtime/kernel/crypto.cpp)
target_include_directories(iso_install PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/runtime-sdk/runtime)
foreach(tool xbe_recomp xbe_symbols iso_xbe iso_assets iso_verify iso_install)
    set_target_properties(${tool} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endforeach()
