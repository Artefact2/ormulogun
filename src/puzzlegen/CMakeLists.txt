add_library(ormpuzzlegencore STATIC eval.c gumbleextras.c load.c puzzle.c tags.c uci.c)
target_link_libraries(ormpuzzlegencore enginecore)

add_executable(gen-puzzles gen-puzzles.c)
target_link_libraries(gen-puzzles ormpuzzlegencore)

add_executable(retag-puzzles retag-puzzles.c)
target_link_libraries(retag-puzzles ormpuzzlegencore)

add_executable(regen-puzzles regen-puzzles.c)
target_link_libraries(regen-puzzles ormpuzzlegencore)
