Generating puzzles
==================

If you don't have `pv` installed, replace it by `cat`.

1. Process a PGN dump into a filtered list of games (1 game per line)

   ~~~
   pv ~/pgn_dump.pgn.bz2 | bzcat | ./chunk-pgn | xargs -d '\n' -P 16 ./preprocess-pgn | zstd -4 - -o pgnlist.jz
   ~~~

2. Generate puzzles

   ~~~
   pv pgnlist.jz | zstdcat | xargs -d '\n' -P 16 ./probe-puzzles | zstd -4 - -o puzzles.jz

   # games 2000-2999 only
   zstdcat pgnlist.gz | tail -n +2000 | head -n 1000 | pv -ls 1000 | xargs...
   ~~~

3. Categorize puzzles (change `root-` into any prefix you want)

   ~~~
   pv puzzles.jz | zstdcat | ./dispatch-puzzles root-
   ~~~

4. Merge puzzle sets

   ~~~
   find . -maxdepth 1 -type f -name '*.jl' -print0 | xargs -0 -n 1 -P 16 ./merge-puzzles ../frontend/puzzles

   # Without categorizing
   zstdcat puzzles.jz | ./merge-puzzles ../frontend/puzzles/foo.json
   ~~~

5. Update manifest

   ~~~
   ./update-manifest ../frontend/puzzles/manifest.json
   ~~~
