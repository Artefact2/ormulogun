Generating puzzles
==================

~~~
# Process a PGN dump into a filtered list of games (1 game per line)
pv ~/pgn_dump.pgn.bz2 | bzcat | ./chunk-pgn | xargs -d '\n' -P 16 ./preprocess-pgn | zstd -4 - -o pgnlist.jz



# Generate puzzles (1 puzzle per line)
pv pgnlist.jz | zstdcat | xargs -d '\n' -P 16 ./probe-puzzles | zstd -4 - -o puzzles.jz

# Generate puzzles, games 1000-1999 only
pv pgnlist.gz | zstdcat | tail -n +1000 | head -n 1000 | xargs...



# Generate puzzle sets
# XXX
~~~
