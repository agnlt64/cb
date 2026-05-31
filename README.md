# cb

download opening book [here](https://github.com/official-stockfish/books/blob/master/8moves_v3.pgn.zip) and put it in [`testing/book`](./testing/book)

suggested improvements:
- use max(alloc_time, time_left - 10ms) for hardbound. suggested by a member of the stockfish discord server
- softbound for a good elo gain? (must be smaller than hardbound)

softbound must be checked each iteration of iterative deepening while hardbound is checked every x nodes (2048 in this engine)