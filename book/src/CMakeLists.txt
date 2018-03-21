add_definitions(--std=c11 -D_POSIX_C_SOURCE=200809)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../ext/gumble/include)

add_executable(merge-books merge-books.c)

add_executable(gen-book gen-book.c)
target_compile_options(gen-book PRIVATE -Wno-pointer-sign)
target_link_libraries(gen-book enginecore Judy)
