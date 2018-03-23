add_executable(merge-books merge-books.c)

add_executable(gen-book gen-book.c common.c)
target_compile_options(gen-book PRIVATE -Wno-pointer-sign)
target_link_libraries(gen-book enginecore Judy)

add_executable(gen-eco gen-eco.c common.c)
target_compile_options(gen-eco PRIVATE -Wno-pointer-sign)
target_link_libraries(gen-eco enginecore Judy)
