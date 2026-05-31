# cb

download opening book [here](https://github.com/official-stockfish/books/blob/master/8moves_v3.pgn.zip) and put it in [`testing/book`](./testing/book)

suggested improvements:
- hardbound = `time_left - 10ms` move overhead. alloc_time = `min(soft_target, hardbound)`. (suggested by a member of the stockfish discord server) ✓ done
- softbound for elo gain (must be smaller than hardbound)

softbound is the budget per move you _want_ to spend; checked once at each
iterative_deepening iteration boundary, to stop launching a new depth if you
can't realistically finish it.

hardbound is the time you _must not_ exceed; checked every N nodes (2048 in this
engine) via the `canceled` flag, to interrupt the current iteration if it runs
past budget.

softbound < hardbound. They give different signals: soft says "we got a decent
result, don't push further"; hard says "stop right now or we lose on time".