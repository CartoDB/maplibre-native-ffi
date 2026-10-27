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
history. When a code change makes text wrong or changes what a reader would do,
fix that text in place. Notes about what changed ("now", "no longer", migration
steps) belong in the PR description.

Each layer has one reader and one job. Material for another layer moves there or
is deleted.

| Layer                                                                           | Reader                                                      | Holds                                                                                | Excludes                                                  |
| ------------------------------------------------------------------------------- | ----------------------------------------------------------- | ------------------------------------------------------------------------------------ | --------------------------------------------------------- |
| Site, in `docs/src/content/docs/`                                               | someone integrating the library                             | fit, installation, platform setup, guides for the common path, a few concepts        | edge cases, per-binding caveats, exact contracts, history |
| C reference: header comments, rendered by Doxygen                               | C users, binding authors, anyone checking an exact contract | each declaration's behavior and ABI                                                  | engine teaching, implementation                           |
| Binding reference: each binding's doc comments, rendered by its language's tool | users of one binding                                        | the package overview, and each declaration's behavior in the binding's names         | ABI, other bindings                                       |
| READMEs, root and per package                                                   | someone deciding what this is, often on a registry page     | one paragraph, one install line, links                                               | everything else                                           |
| `CONTRIBUTING.md`                                                               | contributors, human or agent                                | where to discuss and start, and steps that the code and task descriptions cannot say | rules, history                                            |
| `AGENTS.md`                                                                     | agents and reviewers                                        | priorities and rules                                                                 | how-to steps                                              |
| Code comments                                                                   | the next editor of that code                                | a non-obvious choice, where it is made                                               | narration, inventories                                    |

No layer narrates how the code is implemented or lists what the code already
holds.

A C declaration's contract mixes three kinds of content:

- Behavior: what it does, ordering, engine surprises, conditions a caller can
  trigger, host-resource lifetimes, which threads run a callback, and which
  targets have it. State it once, in the C header. A binding restates it in its
  own names and types.
- ABI, the calling convention and memory ownership: `size` and `fields`,
  out-parameters, buffer views, borrowed or copied, `user_data`, and null or
  undersized pointers. It belongs only in the C reference.
- Binding mechanism: how a binding surfaces failures and completions, closes
  handles, adds its own errors, and runs callbacks under a delivery queue. State
  it once, in that binding's package overview.

Readers know their language but often not MapLibre, and many read English as a
second language, so say what the API does in literal terms. Define a term before
you use it, or don't use it.

Make behavior unsurprising rather than document a surprise. A contract that is
awkward to state is a design smell: state the shortest complete contract of the
current behavior, and name the smell in the PR description, never in the docs.

The guides embed C examples from `docs/snippets/c/`, which
`//docs:check-snippets` syntax-checks against the headers, so examples stay
correct as the API changes. Add or update a snippet file for each C example
rather than write C in a page.

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
