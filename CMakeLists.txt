cmake_minimum_required(VERSION 3.15)

project(expr)

include(common.cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN_DIR})

set(EXTRADEFS_TARGET generate_extradefs)
set(EXTRADEFS ${CMAKE_CURRENT_BINARY_DIR}/extradefs.h)
file(STRINGS ${ROOT_DIR}/extradefs.depend EXTRADEFS_DEPENDS)
list(TRANSFORM EXTRADEFS_DEPENDS PREPEND ${ROOT_DIR}/)
list(PREPEND EXTRADEFS_DEPENDS ${ROOT_DIR}/extradefs.depend)

add_custom_command(
    OUTPUT ${EXTRADEFS}
    COMMAND bash ${ROOT_DIR}/extradefs.sh ${EXTRADEFS}
    DEPENDS ${EXTRADEFS_DEPENDS}
    BYPRODUCTS ${EXTRADEFS})

add_custom_target(${EXTRADEFS_TARGET} DEPENDS ${EXTRADEFS})

include_directories(expr)

aux_source_directory(expr SOURCES)
add_library(${PROJECT_NAME} STATIC ${SOURCES})
add_dependencies(${PROJECT_NAME} ${EXTRADEFS_TARGET})

add_executable(calc samples/calc.cpp)
target_link_libraries(calc PRIVATE ${PROJECT_NAME})
