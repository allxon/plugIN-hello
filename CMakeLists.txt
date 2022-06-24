cmake_minimum_required(VERSION 3.23)
project(plugIN-hello VERSION 1.0.0)
set(PLUGIN_SDK_VERSION 2.0.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(PLUGIN_NAME ${CMAKE_PROJECT_NAME})
set(PLUGIN_VERSION ${PROJECT_VERSION})

# Check and Parse plugin_key
set(PLUGIN_KEY ${CMAKE_SOURCE_DIR}/plugin_key.json CACHE FILEPATH "plugIN key file")
if(NOT EXISTS ${PLUGIN_KEY})
    message(FATAL_ERROR "can't found ${PLUGIN_KEY}")
endif()
file(READ ${PLUGIN_KEY} PLUGIN_KEY_CONTENT)
message(${PLUGIN_KEY_CONTENT})
string(JSON PLUGIN_APP_GUID GET ${PLUGIN_KEY_CONTENT} app_guid)
string(JSON PLUGIN_ACCESS_KEY GET ${PLUGIN_KEY_CONTENT} access_key)
string(JSON PLUGIN_PLATFORM GET ${PLUGIN_KEY_CONTENT} platform)
string(JSON PLUGIN_ARCH GET ${PLUGIN_KEY_CONTENT} architecture)
if(WIN32)
    if(NOT "${PLUGIN_PLATFORM}" STREQUAL "windows")
        message(FATAL_ERROR "plugin_key platform is not matched current platform")
    endif()
elseif(UNIX)
    if(NOT "${PLUGIN_PLATFORM}" STREQUAL "linux")
        message(FATAL_ERROR "plugin_key platform is not matched current platform")
    endif()
endif()

# Check avaliable plugin_sdk binary arch
if("${PLUGIN_ARCH}" STREQUAL "aarch64|jetson") 
    set(PLUGIN_ARCH "aarch64")
elseif("${PLUGIN_ARCH}" STREQUAL "x86")
    set(PLUGIN_ARCH "AMD64")
endif()
if (NOT "${CMAKE_HOST_SYSTEM_PROCESSOR}" STREQUAL "${PLUGIN_ARCH}")
    message(FATAL_ERROR "unsupport architecture on ${CMAKE_HOST_SYSTEM_PROCESSOR}, please build from source")
endif()

# Check plugin package materials
if(WIN32)
    set(PLUGIN_MATERIAL_DIR ${CMAKE_SOURCE_DIR}/resource_dir_windows)
    set(INSTALL_SH_NAME "install_plugIN.bat")
    set(UNINSTALL_SH_NAME "uninstall_plugIN.bat")
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
message("PLUGIN_SDK_VERSION: ${PLUGIN_SDK_VERSION}")
message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message("*** PROJECT_DIR : ${PROJECT_SOURCE_DIR} ")
message("PLUGIN_PLATFORM: ${PLUGIN_PLATFORM}")
message("PLUGIN_ARCH: ${PLUGIN_ARCH}")
message("PLUGIN_APP_GUID: ${PLUGIN_APP_GUID}")
message("PLUGIN_ACCESS_KEY: ${PLUGIN_ACCESS_KEY}")
message("BUILD_FROM_SDK_SRC: ${BUILD_FROM_SDK_SRC}")

include(FetchContent)
if (BUILD_FROM_SDK_SRC)
    FetchContent_Declare(
        plugIN_SDK
        GIT_REPOSITORY git@gitlab.com:allxon.com/plugin/plugin-sdk.git
        GIT_TAG        ${PLUGIN_SDK_VERSION}
    )
    FetchContent_MakeAvailable(plugIN_SDK)
    set(PLUGIN_SDK_TARGET plugIN_SDK)
else()
    if (WIN32)
        set(URL_OS win)
        set(URL_SUFFIX zip)
        set(URL_ARCH ${CMAKE_GENERATOR_PLATFORM})
    elseif (UNIX AND NOT APPLE)
        set(URL_OS linux)
        set(URL_SUFFIX tar.gz)
        set(URL_ARCH ${CMAKE_HOST_SYSTEM_PROCESSOR})
    endif()

    FetchContent_Declare(
    plugIN_SDK
    URL     https://github.com/allxon/plugIN-sdk-v2/releases/download/${PLUGIN_SDK_VERSION}/plugIN_SDK-${PLUGIN_SDK_VERSION}-${URL_OS}-${URL_ARCH}.${URL_SUFFIX}
    )
    FetchContent_MakeAvailable(plugIN_SDK)

    if (WIN32)
        set(PLUGIN_SDK_TARGET ${plugin_sdk_SOURCE_DIR}/lib/plugIN_SDK.lib)
    elseif (UNIX AND NOT APPLE)
        set(PLUGIN_SDK_TARGET ${plugin_sdk_SOURCE_DIR}/lib/libplugIN_SDK.a)
    endif()
endif()

configure_file(build_info.h.in build_info.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
aux_source_directory(${PROJECT_SOURCE_DIR}/src SOURCE_FILE)
add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILE} ${PROJECT_SOURCE_DIR}/dep/cJSON/cJSON.c )

if(WIN32 AND MSVC)
    add_definitions (-D_WEBSOCKETPP_CPP11_STL_)
    set(OPENSSL_DIR ${PROJECT_SOURCE_DIR}/dep/openssl)
    list(APPEND LINK_LIBS ${PLUGIN_SDK_TARGET}
            ${PROJECT_SOURCE_DIR}/dep/openssl/win_x86/lib/libcrypto.lib
            ${PROJECT_SOURCE_DIR}/dep/openssl/win_x86/lib/libssl.lib)
    list(APPEND EXTRA_INCLUDES ${PROJECT_SOURCE_DIR}/dep/openssl/include)

    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/dep/openssl/win_x86/bin/libssl-1_1.dll ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
    )
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/dep/openssl/win_x86/bin/libcrypto-1_1.dll ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/ 
    )
elseif(UNIX AND NOT APPLE)
    list(APPEND LINK_LIBS ${PLUGIN_SDK_TARGET} -pthread -lcrypto -lssl)
endif()

list(APPEND EXTRA_INCLUDES ${plugin_sdk_SOURCE_DIR}/include
                           ${PROJECT_SOURCE_DIR}/dep/asio/asio/include
                           ${PROJECT_SOURCE_DIR}/dep/websocketpp
                           ${PROJECT_SOURCE_DIR}/dep/cJSON
    )

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${LINK_LIBS})
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC 
                            ${EXTRA_INCLUDES}
                            )

# install stage
if(WIN32 AND MSVC)
    install(DIRECTORY ${PLUGIN_MATERIAL_DIR}/ DESTINATION ./${PLUGIN_APP_GUID}
            FILE_PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE)
    install(PROGRAMS ${PROJECT_BINARY_DIR}/$<CONFIG>/${CMAKE_PROJECT_NAME}.exe
                     ${PROJECT_BINARY_DIR}/$<CONFIG>/libcrypto-1_1.dll
                     ${PROJECT_BINARY_DIR}/$<CONFIG>/libssl-1_1.dll
            DESTINATION ./${PLUGIN_APP_GUID})
else()
    install(DIRECTORY ${PLUGIN_MATERIAL_DIR}/ DESTINATION ./${PLUGIN_APP_GUID}
            FILE_PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE
            PATTERN ${INSTALL_SH_NAME} EXCLUDE)
    install(PROGRAMS ${PLUGIN_MATERIAL_INSTALL_SH} DESTINATION .)
    install(PROGRAMS ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME} DESTINATION ./${PLUGIN_APP_GUID})
endif()

# package stage
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILE_NAME "plugIN-hello-${PLUGIN_SDK_VERSION}-win-${CMAKE_GENERATOR_PLATFORM}")
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ")
    set(CPACK_PACKAGE_FILE_NAME "plugIN-hello-${PLUGIN_SDK_VERSION}-linux-${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack)