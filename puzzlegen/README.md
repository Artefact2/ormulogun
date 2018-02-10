Generating puzzles
==================

1. Process a PGN dump into a filtered list of games (1 game per line)

   ~~~
   make pgnlist.jz INPUT=foo.pgn
   ~~~

2. Generate puzzles

   ~~~
   make puzzles-all # all games
   make puzzles-partial OFFSET=100 COUNT=50 # games 100-149 only
   ~~~

3. Convert to JSON and update manifest

   ~~~
   zstdcat master.jz | ./jl_to_json > out/master.json
   ./update-manifest out/manifest.json
   ~~~
