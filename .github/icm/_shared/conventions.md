---
verified_at: 2026-10-15
verified_commit: b28ff129d49b7a79cae81e69b205c1bf3821234b
---

# Conventions — how code here is written

## Language and style

- **C++14**, required and without extensions (`CMakeLists.txt:16`); C++17 library types do not compile. Optional values are `boost::optional` (`include/simulator/cli_options.hpp`, `include/simulator/config_loader.hpp`).
- **Naming**: classes `PascalCase`, functions `camelCase`, variables `snake_case`, constants `UPPER_SNAKE_CASE`, private members with a trailing underscore (`CONTRIBUTING.md` "Naming Conventions"; `include/simulator/virtual_node.hpp` is a faithful example).
- **Layout**: two-space indent, opening brace on the same line, includes ordered system, external, project (`CONTRIBUTING.md` "Code Formatting"). There is no `.clang-format`; the `lint` job's clang-format and cppcheck are advisory (`ci.yml`).
- **Headers**: every file opens with a Doxygen block (`@file`, `@brief`, `@copyright`, `@license MIT`), and public APIs carry `@brief`/`@param`/`@throws` (`CONTRIBUTING.md` "Documentation"; `include/simulator/node_manager.hpp`). A file that includes painlessMesh includes `simulator/platform_compat.hpp` first (`src/core/virtual_node.cpp`).
- **Ownership**: smart pointers and RAII, no bare `new`/`delete`; exceptions for errors, with a message that names what was wrong (`CONTRIBUTING.md` "Best Practices").
- **Boost.Asio**: one `io_context` per run, created in `src/main.cpp` and passed by reference to `NodeManager` and each `VirtualNode`; nothing blocks it.
- **Comments say why**, with what was observed: the house style of `CMakeLists.txt`, `ci.yml` and `scripts/ci_integration_check.sh`, where each workaround names the failure it prevents.

## Tests

- Catch2 v3 (`FetchContent` in `CMakeLists.txt`), `TEST_CASE`/`SECTION`, arrange-act-assert (`CONTRIBUTING.md` "Testing Guidelines"). A new `test/test_*.cpp` is added to the `simulator_tests` list in `CMakeLists.txt` or it never runs.
- A change to what a run does gets an assertion in `scripts/ci_integration_check.sh`, not only a unit test ([build-and-test.md](build-and-test.md) "Evidence").
- A scenario under `examples/scenarios/` is a test: it must pass `--validate-only` in CI.

## Changes and pull requests

- One change per pull request, anchored to its issue: the issue number goes in the title or body (`.github/PULL_REQUEST_TEMPLATE.md` "Related Issues"), and a capability the issue does not need gets its own issue ([review.md](review.md)).
- Pull requests go to `main`, the default and only long-lived branch. Subjects say what the change does in plain words; `CONTRIBUTING.md` "Commit Message Format" (conventional commits) is accepted too.
- Never commit build output: `build*/`, `bin/`, `out/`, `lib/` are ignored (`.gitignore`). If `git status` shows a Makefile or object files, the build folder is misnamed; rename it rather than add an exception.
