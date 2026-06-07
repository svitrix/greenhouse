# Legacy monolith — historical reference

The pre-Phase-A single-board layout (`src/`, `lib/`, `test/`, root `platformio.ini`,
root `partitions.csv`) lived here. After the Phase A refactor every file was moved
into `shared/` or `firmware/coordinator/` via `git mv`, so full history is preserved
in the git log rather than as a parallel directory tree.

To inspect the old structure:

```
git checkout a036328 -- .             # snapshot the baseline into the working tree (dangerous if you have uncommitted changes)
git show a036328:src/main.cpp         # read a single file from the baseline without checking out
git log --follow firmware/coordinator/src/main.cpp   # walk the rename history of any post-refactor file
```

Baseline tag: `a036328` ("chore: baseline before Phase A monorepo refactor").
Phase A complete tag: `phase-a-complete`.
