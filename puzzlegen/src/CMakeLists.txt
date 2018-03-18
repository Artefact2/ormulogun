add_definitions("--std=c11 -D_POSIX_C_SOURCE=200809")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/gumble/include)

add_library(ormuloguncore STATIC eval.c gumbleextras.c load.c puzzle.c tags.c uci.c)
target_link_libraries(ormuloguncore enginecore)

add_executable(gen-puzzles gen-puzzles.c)
target_link_libraries(gen-puzzles ormuloguncore)

add_executable(retag-puzzles retag-puzzles.c)
target_link_libraries(retag-puzzles ormuloguncore)

add_executable(regen-puzzles regen-puzzles.c)
target_link_libraries(regen-puzzles ormuloguncore)
