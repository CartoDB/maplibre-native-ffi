# Repository guidance

MapLibre Native FFI is a C API for MapLibre Native. A generator writes low-level
bindings for nine languages from its headers, over a handwritten runtime in each
binding. Mise provides every tool and task, and `mise tasks --all` lists them.

## Priorities

The project is prerelease. Make breaking changes rather than keep compatibility
shims.

Bindings stay low level and analogous to the C API and to each other: safety
first, then similarity, then idioms. A binding adds a convenience API only when
its language's safety or ergonomics strongly require one. It never repeats
validation that native already does.

MapLibre behavior is whatever the pinned `third_party/maplibre-native` submodule
does with the patches in `patches/maplibre-native/`. Answer questions about
engine behavior from that source; `mise deps` checks it out and applies the
patches if it is empty. After a rebase, run `mise deps` before you stage
anything, since plain `git submodule update` skips this submodule, and read
`git diff origin/main...HEAD --stat` before you push: a stale checkout staged by
`git add -A` reverts the engine pin.

## Tests

Native behavior is tested once, in C, under `tests/native`. A binding suite
tests only what its binding adds: its runtime, its generated shapes, and its
platform integration. Before you delete a binding test, check that a C test
asserts what it checked.

A Linux host can run the Windows and FreeBSD suites locally, under Wine and in a
virtual machine, as the
[Overview](docs/src/content/docs/development/overview.md) describes.

GPU objects that a test creates as the host come from `tests/graphics`. A test
that only renders and reads back may attach a graphics device texture instead.
The target or the build decides a skip, never the environment, so fix the CI
environment rather than skip a test.

## Documentation

Documentation describes the library as it is, for a reader who does not know its
history. Change it when a code change makes existing text wrong or changes what
a reader would do, and fix that text in place. Notes about what changed ("now",
"no longer", migration steps) belong in the PR description.

Each layer carries a different level of detail:

- Site pages in `docs/src/content/docs/` help a user integrate the library,
  complete a task, or understand a concept. They cover the common path. Edge
  cases, per-binding caveats, and exact contracts belong in the reference.
- C header comments are the contract: behavior, plus the calling convention and
  memory ownership that C callers rely on.
- Each binding's doc comments restate that behavior in the binding's names,
  without the C calling convention. Its package overview says once how the
  binding reports errors, delivers completions, and closes handles.
- `CONTRIBUTING.md` explains project-specific decisions to contributors.

No doc narrates how the code is implemented; a comment inside the code explains
a non-obvious choice where it is made.

Readers know their language but often not MapLibre, and many read English as a
second language, so say what the API does in literal terms. A contract that is
awkward to state is a design smell: document the current behavior plainly and
name the smell in the PR description.

The guides embed C examples from `docs/snippets/c/`, which
`//docs:check-snippets` compiles against the headers. Add or update a snippet
file for each C example rather than write C in a page.

The site's Development section holds only what maintainers need and the code
cannot say:

- [Overview](docs/src/content/docs/development/overview.md): machine setup,
  cross-compilation, and debugging on a CI runner.
- [C API guidelines](docs/src/content/docs/development/c-api-guidelines.md):
  read before you change a C declaration.
- [Releases and snapshots](docs/src/content/docs/development/versioning.md):
  versions, tags, snapshot publishing, and registry setup.

## Issues and pull requests

[Rubberstamp](https://github.com/apps/rubberstamp-bot) labels each new issue and
pull request from `.config/rubberstamp.yml`. Don't label by hand; correct a
wrong label instead, and leave GitHub's native issue type unset. The one
exception is `upstream`, which Rubberstamp gives only to pull requests that
change MapLibre Native's code: add it to an issue yourself when the engine
causes the bug or lacks the capability.

PR titles follow Conventional Commits, since they become the squash-merge commit
message. Mark a title breaking only for a change to a user-facing contract,
never for tooling, tests, or CI. Write a short title and description for a
reviewer who will read the diff. Say what the diff, its doc comments, and CI
can't:

- Open with the problem and what users can now do, or what maintainers gain.
- Show one small example of new API, or a bug's trigger with before and after.
- Add these only if they apply: what a host must adopt, measured numbers with a
  one-line method, and validation that CI does not run.
- Rewrite both when review changes the scope.

When you post AI-contributed GitHub content, including a new Discussion or
Issue, a reply, or a PR description or comment, end it with the
[disclosure line](AI_POLICY.md#disclosure).

If Rubberstamp's CI plan skips coverage that a change needs, run every unit with
`gh workflow run CI --ref <branch>`.
