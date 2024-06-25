cmake_minimum_required(VERSION 3.23)
project(plugin-hello VERSION 0.0.0)

option(BUILD_FROM_SDK_SRC "Build octo-sdk from source" OFF)
set (OCTO_SDK_VERSION 3.4.2 CACHE STRING "octo sdk version")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(PLUGIN_NAME ${CMAKE_PROJECT_NAME})
set(PLUGIN_VERSION ${PROJECT_VERSION})

# Check and Parse plugin_key
set(PLUGIN_KEY ${CMAKE_SOURCE_DIR}/plugin_credential.json CACHE FILEPATH "plugin credential file")
if(NOT EXISTS ${PLUGIN_KEY})
    message(FATAL_ERROR "can't found ${PLUGIN_KEY}")
endif()
file(READ ${PLUGIN_KEY} PLUGIN_KEY_CONTENT)
message(${PLUGIN_KEY_CONTENT})
string(JSON PLUGIN_APP_GUID GET ${PLUGIN_KEY_CONTENT} app_guid)
string(JSON PLUGIN_ACCESS_KEY GET ${PLUGIN_KEY_CONTENT} access_key)
string(JSON PLUGIN_PLATFORM GET ${PLUGIN_KEY_CONTENT} platform)
string(JSON PLUGIN_ARCH GET ${PLUGIN_KEY_CONTENT} architecture)

# Check avaliable octo_sdk
if(WIN32 AND MSVC)
    if(NOT "${PLUGIN_PLATFORM}" STREQUAL "windows")
        message(FATAL_ERROR "plugin_key platform is not matched current platform")
    endif()
    if ((NOT "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "Win32") AND (NOT "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64")) 
            message(FATAL_ERROR "unsupport architecture on ${CMAKE_GENERATOR_PLATFORM}, please build from source")
    endif()
    if(("${PLUGIN_ARCH}" STREQUAL "x86_64") AND (NOT "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64"))
            message(FATAL_ERROR "plugin_key architecture is not matched current build")
    endif()
    if(("${PLUGIN_ARCH}" STREQUAL "x86") AND (NOT "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "Win32"))
            message(FATAL_ERROR "plugin_key architecture is not matched current build")
    endif()
elseif(UNIX)
    if(NOT "${PLUGIN_PLATFORM}" STREQUAL "linux")
        message(FATAL_ERROR "plugin_key platform is not matched current platform")
    endif()
    if (NOT "${CMAKE_HOST_SYSTEM_PROCESSOR}" STREQUAL "${PLUGIN_ARCH}")
        message(FATAL_ERROR "unsupport architecture on ${CMAKE_HOST_SYSTEM_PROCESSOR}, please build from source")
    endif()
else()
    message(FATAL_ERROR "current os is not support")
endif()

# Check plugin package materials
if(WIN32)
    set(PLUGIN_MATERIAL_DIR ${CMAKE_SOURCE_DIR}/resource_dir_windows)
    set(INSTALL_SH_NAME "install_plugIN.ps1")
    set(UNINSTALL_SH_NAME "uninstall_plugIN.ps1")
elseif(UNIX)
    set(PLUGIN_MATERIAL_DIR ${CMAKE_SOURCE_DIR}/resource_dir_linux)
    set(INSTALL_SH_NAME "install_plugIN.sh")
    set(UNINSTALL_SH_NAME "uninstall_plugIN.sh")
else()
    message(FATAL_ERROR "unsupport platform")
endif()
set(PLUGIN_MATERIAL_INSTALL_SH ${PLUGIN_MATERIAL_DIR}/${INSTALL_SH_NAME}) 
message(CHECK_START "Finding ${PLUGIN_MATERIAL_INSTALL_SH}")
if(NOT EXISTS ${PLUGIN_MATERIAL_INSTALL_SH})
    message(CHECK_FAIL "not found")
else()
    message(CHECK_PASS "found")
endif()
set(PLUGIN_MATERIAL_UNINSTALL_SH ${PLUGIN_MATERIAL_DIR}/${UNINSTALL_SH_NAME}) 
message(CHECK_START "Finding ${PLUGIN_MATERIAL_UNINSTALL_SH}")
if(NOT EXISTS ${PLUGIN_MATERIAL_UNINSTALL_SH})
    message(CHECK_FAIL "not found")
else()
    message(CHECK_PASS "found")
endif()

message("=================== CMAKE_DIR : ${CMAKE_SOURCE_DIR} ======================")
message("PLUGIN_NAME: ${PLUGIN_NAME}")
message("PLUGIN_VERSION: ${PLUGIN_VERSION}")
message("OCTO_SDK_VERSION: ${OCTO_SDK_VERSION}")
message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message("*** PROJECT_DIR : ${PROJECT_SOURCE_DIR} ")
message("PLUGIN_PLATFORM: ${PLUGIN_PLATFORM}")
message("PLUGIN_ARCH: ${PLUGIN_ARCH}")
message("PLUGIN_APP_GUID: ${PLUGIN_APP_GUID}")
message("PLUGIN_ACCESS_KEY: ${PLUGIN_ACCESS_KEY}")
message("BUILD_FROM_SDK_SRC: ${BUILD_FROM_SDK_SRC}")

### OpenSSL library, linux(use system), windows(.lib binary)
if(WIN32 AND MSVC)
    set(OPENSSL_DIR ${PROJECT_SOURCE_DIR}/dep/openssl)
    if("${PLUGIN_ARCH}" STREQUAL "x86")
        set(OPENSSL_ARCH x86)
    else()
        set(OPENSSL_ARCH x64)
        set(OPENSSL_DLL_SUFFIX -x64)
    endif()
    list(APPEND LINK_LIBS 
            Crypt32.lib
            ${PROJECT_SOURCE_DIR}/dep/openssl/win_${OPENSSL_ARCH}/lib/libcrypto.lib
            ${PROJECT_SOURCE_DIR}/dep/openssl/win_${OPENSSL_ARCH}/lib/libssl.lib)
    list(APPEND EXTRA_INCLUDES ${PROJECT_SOURCE_DIR}/dep/openssl/include)
elseif(UNIX AND NOT APPLE)
    set(OPENSSL_USE_STATIC_LIBS true)
    find_package(OpenSSL REQUIRED COMPONENTS Crypto SSL)
    list (APPEND LINK_LIBS OpenSSL::SSL OpenSSL::Crypto)
endif()

include(FetchContent)

### websocketpp library
FetchContent_Declare(
    websocketpp
    URL https://github.com/zaphoyd/websocketpp/archive/refs/tags/0.8.2.tar.gz
)
FetchContent_MakeAvailable(websocketpp)
add_library(websocketpp INTERFACE)
target_include_directories(websocketpp INTERFACE ${websocketpp_SOURCE_DIR})
list(APPEND LINK_LIBS websocketpp)
message(STATUS "websocketpp library fetched")

### ASIO library: set websocket to use C++ STL and non-Boost 
FetchContent_Declare(
    asio
    URL https://dev.allxon.net/source_code/asio-1.28.1.tar.gz
)
FetchContent_MakeAvailable(asio)
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/include)
list(APPEND LINK_LIBS asio)
message(STATUS "ASIO library fetched")

### octo-sdk library
if (BUILD_FROM_SDK_SRC)
    FetchContent_Declare(
        octo_sdk
        GIT_REPOSITORY git@gitlab.com:allxon.com/plugin/octo-sdk.git
        GIT_TAG        master
    )
    FetchContent_MakeAvailable(octo_sdk)
    set(OCTO_SDK_TARGET octo_sdk)
    message("octo-sdk library (source) fetched")
else()
    if (WIN32)
        set(URL_OS win)
        set(URL_SUFFIX zip)
        set(URL_ARCH ${PLUGIN_ARCH})
    elseif (UNIX AND NOT APPLE)
        set(URL_OS linux)
        set(URL_SUFFIX tar.gz)
        set(URL_ARCH ${CMAKE_HOST_SYSTEM_PROCESSOR})
    endif()
    FetchContent_Declare(
    octo_sdk
    URL     https://github.com/allxon/octo-sdk/releases/download/${OCTO_SDK_VERSION}/octo_sdk-${OCTO_SDK_VERSION}-${URL_OS}-${URL_ARCH}.${URL_SUFFIX}
    )
    FetchContent_MakeAvailable(octo_sdk)
    if (WIN32)
        set(OCTO_SDK_TARGET ${octo_sdk_SOURCE_DIR}/lib/octo_sdk.lib)
    elseif (UNIX AND NOT APPLE)
        set(OCTO_SDK_TARGET ${octo_sdk_SOURCE_DIR}/lib/libocto_sdk.a)
    endif()
    message("octo-sdk library (binary) fetched")

    #nlohmann_json library
    message(STATUS "Fetching nlohmann_json library")
    FetchContent_Declare(
        nl_json
        URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz
    )
    FetchContent_MakeAvailable(nl_json)
    add_library(nl_json INTERFACE)
    # set_target_properties(nl_json PROPERTIES LINKER_LANGUAGE CXX)
    target_include_directories(nl_json INTERFACE ${nl_json_SOURCE_DIR}/single_include)
    list(APPEND LINK_LIBS nl_json)
    message(STATUS "nlohmann_json library fetched")
endif()

list(APPEND LINK_LIBS ${OCTO_SDK_TARGET})

### build executable
aux_source_directory(${PROJECT_SOURCE_DIR}/src SOURCE_FILE)
add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILE})
target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC
    ASIO_STANDALONE
    _WEBSOCKETPP_CPP11_STL_
    PLUGIN_NAME="${PLUGIN_NAME}"
    PLUGIN_VERSION="${PLUGIN_VERSION}"
    PLUGIN_APP_GUID="${PLUGIN_APP_GUID}"
    PLUGIN_ACCESS_KEY="${PLUGIN_ACCESS_KEY}"
)
if(WIN32 AND MSVC)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC UNICODE _UNICODE)
endif()

if (UNIX AND NOT APPLE)
    list(APPEND LINK_LIBS stdc++fs)
endif()
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${LINK_LIBS})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE 
    ${PROJECT_SOURCE_DIR}/dep/openssl/include
    ${octo_sdk_SOURCE_DIR}/include
)

# install stage
install(DIRECTORY ${PLUGIN_MATERIAL_DIR}/ DESTINATION ./${PLUGIN_APP_GUID}
        FILE_PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE
        PATTERN ${INSTALL_SH_NAME} EXCLUDE)
install(PROGRAMS ${PLUGIN_MATERIAL_INSTALL_SH} DESTINATION .)
if(WIN32 AND MSVC)
    install(PROGRAMS ${PROJECT_BINARY_DIR}/$<CONFIG>/${CMAKE_PROJECT_NAME}.exe
            DESTINATION ./${PLUGIN_APP_GUID})
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/dep/msvc/ DESTINATION ./${PLUGIN_APP_GUID})
else()
    install(PROGRAMS ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME} DESTINATION ./${PLUGIN_APP_GUID})
endif()

# package stage
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILE_NAME "plugin-hello-${PLUGIN_VERSION}-win-${PLUGIN_ARCH}")
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ")
    set(CPACK_PACKAGE_FILE_NAME "plugin-hello-${PLUGIN_VERSION}-linux-${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack)