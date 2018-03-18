Generating puzzles
==================

0. Build binaries

   ~~~
   make -C ..
   ~~~

1. Process a PGN dump into a filtered list of games (1 game per line)

   ~~~
   make INPUT=foo.pgn OUTPUT=games.jz
   ~~~

2. Generate puzzles

   ~~~
   # Analyze all games for all kinds of puzzles
   make puzzles-all INPUT=games.jz OUTPUT=puzzles-all

   # Analyze only games 100-149
   make puzzles INPUT=games.jz OUTPUT=p START=100 END=149
   ~~~

3. Convert to JSON and update manifest

   ~~~
   make json-all
   make -C out js-all
   ~~~
