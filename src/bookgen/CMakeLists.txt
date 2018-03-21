add_executable(merge-books merge-books.c)

add_executable(gen-book gen-book.c)
target_compile_options(gen-book PRIVATE -Wno-pointer-sign)
target_link_libraries(gen-book enginecore Judy)
