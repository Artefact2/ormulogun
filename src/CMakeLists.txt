add_definitions(--std=c11 -D_POSIX_C_SOURCE=200809)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ext/gumble/include ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_subdirectory(bookgen)
add_subdirectory(puzzlegen)
