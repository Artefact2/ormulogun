Ormulogun
=========

A very simple serverless, privacy aware chess tactics
trainer. Released under the Apache License 2.0.

Test it live at: <https://artefact2.gitlab.io/ormulogun/>

Dependencies
============

* PHP (CLI)
* Any UCI engine (eg stockfish)
* CMake
* LLVM, clang
* sassc

To-Do
=====

* More automatic tagging (sacrifices, coercion, Zugzwang, Zwischenzug, checkmate patterns, avoid stalemate...)
* Better automatic tagging (not a trivial task)
* Progress tracking
* Better activity log, activity streak
* Progress saving, loading, merging
* Touch-friendly drag and drop
* Find some way of rating puzzles by difficulty without relying on a central server and user tracking (if at all possible)
* localStorage space limit may become an issue with large puzzlesets, maybe use lz-string

Credits
=======

* [Lichess](https://lichess.org/), for making their game dumps available
* [Colin M.L. Burnett](https://en.wikipedia.org/wiki/User:Cburnett) for releasing his chess pieces as SVG (full license in `COPYING.pieces`)
* Pablo Impallari for releasing the [Kaushan Script](https://fontlibrary.org/en/font/kaushan-script) under the SIL Open Font License
* [GitHub Octicons](https://octicons.github.com/) for the "fork" icon, released under the SIL Open Font License, version 1.1
* [stockfish.js](https://github.com/niklasf/stockfish.js) and all the contributors of Stockfish
