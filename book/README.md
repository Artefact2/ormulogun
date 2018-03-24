Generating an opening book
==========================

1. Generate a master book from a PGN list of games.

   ~~~
   make book.tsv.zst INPUT=foo.pgn
   ~~~

2. Prune positions from the book that were reached, for example, less than 100 times:

   ~~~
   ../build/src/bookgen/merge-books 100 <(zstdcat book.tsv.zst) | ../tools/bookgen/prune-book > book100.tsv
   ~~~

3. Test the generated book.

   ~~~
   ../tools/bookgen/test-book book100.tsv
   ~~~

Generating an ECO code book
===========================

Requires a PGN list of games, annonated with the appropriate tags
(ECO, Opening and Variation).

~~~
make eco.tsv INPUT=foo.pgn
~~~

Test with `../tools/bookgen/test-book book.tsv eco.tsv`.
