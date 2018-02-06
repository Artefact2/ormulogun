Generating puzzles
==================

If you don't have `pv` installed, replace it by `cat`. Change `root-` into any prefix you want for the puzzles.

1. Process a PGN dump into a filtered list of games (1 game per line)

   ~~~
   pv ~/pgn_dump.pgn.bz2 | bzcat | ./chunk-pgn | xargs -d '\n' -P 16 ./preprocess-pgn | zstd -4 - -o pgnlist.jz
   ~~~

2. Generate puzzles

   ~~~
   pv pgnlist.jz | zstdcat | xargs -d '\n' -P 16 ./probe-puzzles | zstd -4 - -o puzzles.jz

   # games 1000-1999 only
   pv pgnlist.gz | zstdcat | tail -n +1000 | head -n 1000 | xargs...
   ~~~

3. Categorize puzzles

   ~~~
   pv puzzles.jz | zstdcat & ./dispatch-puzzles root-
   ~~~

4. Merge puzzle sets

   ~~~
   find . -maxdepth 1 -type f -name "root-*.json" -print0 | xargs -0 -n 1 ./merge-puzzle ../frontend/puzzles
   ~~~

5. Update manifest

   ~~~
   ./update-manifest ../frontend/puzzles/manifest.json
   ~~~
