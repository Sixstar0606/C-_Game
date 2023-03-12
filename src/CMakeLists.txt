project(server LANGUAGES CXX)

file(GLOB INCLUDE_FILES
    *.h
    **/*.h
    **/**/*.h
    **/**/**/*.h
    **/**/**/**/*.h)
file(GLOB SOURCE_FILES
    *.cpp
    **/*.cpp
    **/**/*.cpp
    **/**/**/*.cpp
    **/**/**/**/*.cpp)
add_executable(${PROJECT_NAME}
    ${INCLUDE_FILES}
    ${SOURCE_FILES}
)

if (MSVC)
    add_definitions(/EHsc)
else ()
    add_definitions(-fexceptions)
endif ()

set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 11
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    CPPHTTPLIB_OPENSSL_SUPPORT
)
set(CMAKE_BUILD_TYPE Debug)

if (CONAN_EXPORTED)
    include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
else ()
    include(${CMAKE_CURRENT_SOURCE_DIR}/conan.cmake)
    conan_cmake_configure(REQUIRES
            cpp-httplib/0.10.8
            fmt/9.0.0
            openssl/3.0.5
            libcurl/7.80.0
            mariadb-connector-c/3.1.12
            sqlpp11/0.61
            magic_enum/0.8.1
            nlohmann_json/3.10.5
        GENERATORS cmake_find_package
        IMPORTS "bin, *.dll -> ./bin"
        OPTIONS
            fmt:shared=True
            openssl:shared=True
            libcurl:shared=True
            mariadb-connector-c:shared=True)

    conan_cmake_autodetect(settings)
    conan_cmake_install(PATH_OR_REFERENCE .
        BUILD missing
        REMOTE conancenter
        SETTINGS ${settings})
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}")

find_package(fmt REQUIRED)
find_package(httplib REQUIRED)
find_package(mariadb-connector-c REQUIRED)
find_package(Sqlpp11 REQUIRED)
find_package(magic_enum REQUIRED)
find_package(nlohmann_json REQUIRED)

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/enet/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/dpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/SFML-2.5.1/include
    ${sqlpp11_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} 
    enet
    dpp
    sfml-graphics
    fmt::fmt
    httplib::httplib
    mariadb-connector-c::mariadb-connector-c
    magic_enum::magic_enum
    nlohmann_json::nlohmann_json
    ${sqlpp11_LIBRARIES}
)