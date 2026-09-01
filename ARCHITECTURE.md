# mt architecture evolution plan

This document turns the modularity audit into an executable backlog. Its goal is
to make appearance, behavior, modes, and extensions changeable without modifying
unrelated parts of the editor.

It complements [ROADMAP.md](ROADMAP.md): the roadmap describes features, while
this plan describes the low-coupling foundation required to implement them.

## Execution rules

- Complete one step at a time in its own branch.
- Do not mix structural refactoring with unrelated features.
- Preserve observable behavior unless an acceptance criterion explicitly changes it.
- Keep every commit warning-free, buildable, and covered by risk-appropriate tests.
- Prefer incremental migrations over complete rewrites.
- Do not add dependencies without documenting the need, cost, and alternatives.
- Do not expose new internal structures through the public API.
- Review affected entries in `DATA_STRUCTURES.md` before changing storage or
  ownership, and replace a structure only when its documented trigger applies.
- Review `README.md` in every delivery and update it whenever project state, usage,
  architecture, configuration, or limitations change.
- Mark checklist items complete only after all acceptance criteria pass.

## Current state

### Strengths to preserve

- `Document` already owns content, selection, persistence, and undo/redo.
- Renderer, highlighter, keymap, and minibuffer already have separate modules.
- Relevant interactive actions already have command names.
- Configuration follows `XDG_CONFIG_HOME`.
- Dynamic packages have an initial working implementation.
- Tests cover documents, UTF-8, buffers, keymaps, and safe saving.

### Confirmed limitations

- `editor.c` still combines events, editing, search, Dired, shell, and UI coordination.
- The command registry is unified, but still has fixed capacity.
- Packages receive `Editor *` and depend on internal structures.
- There is no formal major-mode interface.
- Colors, metrics, and most font configuration are compile-time constants.
- Keymaps have fixed capacity, depend on SDL, and lack scopes and sequences.
- Cursor, selection, scrolling, buffers, and windows are not independent models.
- Buffers, commands, packages, and bindings use fixed-size storage.
- Every minibuffer flow requires enum and submission-handler changes.
- Core-facing interfaces still expose SDL types.

## Implementation order

### A1 — Unified command registry

Suggested branch: `refactor/command-registry`

- [x] Define `CommandSpec` with name, function, description, and flags.
- [x] Use one `CommandRegistry` for native and package commands.
- [x] Replace the closed command enum with registry lookup.
- [x] Migrate every native command without changing names or shortcuts.
- [x] Make `M-x`, keymaps, and packages consult the same registry.
- [x] Reject empty, duplicate, and overlong names.
- [x] Test registration, lookup, duplication, capacity, and execution.

Acceptance criteria:

- Adding a native command does not require editing an enum or central switch.
- Native and external commands use the same resolution and execution path.
- Existing shortcuts and command names continue to work.

### A2 — Editor controller decomposition

Suggested branch: `refactor/editor-controller`

- [x] Extract editing commands into their own module.
- [ ] Extract search and replacement.
- [ ] Extract file operations and confirmations.
- [ ] Extract Dired from the general event loop.
- [ ] Separate external-process execution from presentation.
- [ ] Keep `editor.c` responsible only for lifecycle and coordination.
- [ ] Add direct tests for extracted controllers.

Acceptance criteria:

- The SDL loop contains no domain-feature implementation.
- Each controller has explicit dependencies and one primary responsibility.
- No existing behavior is removed.

### A3 — Typed configuration

Suggested branch: `feat/settings`

- [ ] Define typed settings with centralized defaults.
- [ ] Separate path discovery, parsing, and validation.
- [ ] Load the entire configuration before applying it.
- [ ] Report every error with file and line.
- [ ] Reload safely without restarting.
- [ ] Add settings for Tab, search, processes, and visual preferences.
- [ ] Test defaults, valid values, errors, and transactional reload.

Acceptance criteria:

- Invalid configuration never leaves partially applied state.
- New options do not spread parsing logic across the codebase.
- The editor always has valid values, even without configuration files.

### A4 — Theme system

Suggested branch: `feat/theme-system`

- [ ] Define colors without exposing `SDL_Color` in the public model.
- [ ] Add semantic roles for background, panels, text, secondary text, selection,
      cursor, line numbers, and diagnostics.
- [ ] Add semantic roles for syntax highlighting.
- [ ] Move all hard-coded colors into the default theme.
- [ ] Load and fully validate `~/.config/mt/theme.conf`.
- [ ] Reload themes at runtime.
- [ ] Test parsing, fallback, and invalid or incomplete themes.

Acceptance criteria:

- Renderer and highlighters contain no presentation color literals.
- Changing themes requires no rebuild.
- An invalid theme cannot destroy the active theme.

### A5 — Configurable fonts and metrics

Suggested branch: `feat/font-settings`

- [ ] Configure font family or path.
- [ ] Configure size, line height, padding, and gutter width.
- [ ] Preserve `MT_FONT` as a compatible override or document its replacement.
- [ ] Reopen fonts transactionally during reload.
- [ ] Recalculate metrics, viewport, and cursor after changes.
- [ ] Prepare HiDPI scaling without assuming fixed physical pixels.
- [ ] Test validation and fallback with minimal SDL coupling.

Acceptance criteria:

- No configurable visual metric remains a macro in `editor.h`.
- A failed font change keeps the previous font active.
- Visual changes do not affect the document model.

### A6 — Formal major-mode interface

Suggested branch: `refactor/mode-interface`

- [ ] Define `MajorMode` with name, detection, highlighting, and indentation.
- [ ] Detect modes by extension, file name, and content.
- [ ] Associate each buffer with a mode instance.
- [ ] Move C mode into a registered module.
- [ ] Add fundamental/plain-text fallback mode.
- [ ] Support mode-local commands and keymaps.
- [ ] Remove `.c` and `.h` detection from the renderer.
- [ ] Remove direct buffer-type checks from general editing paths.

Acceptance criteria:

- Adding a mode does not require renderer or SDL-loop changes.
- The stable API can later expose mode registration to packages.
- C and plain-text behavior remain compatible.

### A7 — Separate Document, Buffer, View, and Window

Suggested branch: `refactor/view-model`

- [ ] Keep text, files, and undo/redo in `Document`.
- [ ] Keep names, modes, and local variables in `Buffer`.
- [ ] Move cursor, selection, scrolling, and desired column into `View`.
- [ ] Introduce `Window` as owner of geometry and displayed View.
- [ ] Allow two Views of one Buffer with independent positions.
- [ ] Prepare window splitting without implementing it in this refactor.
- [ ] Test independent cursor, selection, and scrolling state.

Acceptance criteria:

- Switching or duplicating Views does not modify other Views.
- `Document` contains no presentation-specific state.
- Renderer receives an explicit View.

### A8 — Opaque public API and versioned ABI

Suggested branch: `refactor/public-api`

- [ ] Prevent packages from accessing `Editor` fields directly.
- [ ] Expose opaque handles for editor, buffer, document, and registry.
- [ ] Provide small APIs for messages, buffers, text, and commands.
- [ ] Add ABI version negotiation during `mt_package_init`.
- [ ] Define ownership and pointer-lifetime rules.
- [ ] Add minimum package metadata.
- [ ] Migrate the example package to public API only.
- [ ] Test compatibility and incompatible-ABI rejection.

Acceptance criteria:

- Internal `Editor` field changes do not require rebuilding compatible packages.
- The SDK includes no internal headers.
- Loading failures report package, reason, and expected version.

### A9 — Extensible, SDL-independent keymaps

Suggested branch: `refactor/keymap-system`

- [ ] Define editor-owned key and modifier types.
- [ ] Isolate SDL conversion in the platform layer.
- [ ] Replace fixed storage with dynamic storage.
- [ ] Explicitly remove bindings.
- [ ] Apply keymap files transactionally.
- [ ] Support global, mode-local, and transient keymaps.
- [ ] Support key sequences.
- [ ] Reload through a command.
- [ ] Test precedence, removal, sequences, errors, and reload.

Acceptance criteria:

- Keymaps are testable without SDL initialization.
- Local bindings do not modify the global keymap.
- Invalid files cannot modify the active keymap.

### A10 — Session-oriented minibuffer

Suggested branch: `refactor/minibuffer-session`

- [ ] Replace enum-coded flows with callback-based sessions.
- [ ] Define update, confirmation, and cancellation callbacks.
- [ ] Add an optional completion provider.
- [ ] Separate history by category.
- [ ] Validate input before closing a prompt.
- [ ] Migrate search, `M-x`, file opening, and confirmations.
- [ ] Test sessions without the SDL loop.

Acceptance criteria:

- A new prompt does not require changing `submit_minibuffer`.
- Confirmation and cancellation are consistent.
- Completion and history are independent from a specific minibuffer mode.

### A11 — Dynamic collections and configurable limits

Suggested branch: `refactor/dynamic-collections`

- [ ] Replace fixed buffer storage with a dynamic vector.
- [ ] Replace fixed command and package storage.
- [ ] Define configurable safety limits where appropriate.
- [ ] Handle allocation failure without losing prior state.
- [ ] Test growth, limits, and predictable failures.

Acceptance criteria:

- Buffer and command counts do not depend on capacity macros.
- Growth preserves pointers or clearly documents invalidation.
- Safety limits produce actionable messages.

### A12 — Core testable without video

Suggested branch: `refactor/core-library`

- [ ] Separate the core library from the SDL executable.
- [ ] Remove SDL types from document, command, mode, and keymap interfaces.
- [ ] Add a platform layer for clipboard, events, clocks, and processes.
- [ ] Test every editing command without initializing video.
- [ ] Add official ASan and UBSan targets.
- [ ] Add CI for builds, tests, formatting, and sanitizers.

Acceptance criteria:

- Most tests run without SDL video or installed fonts.
- The executable composes the core with the SDL frontend.
- Platform dependencies live in explicitly identified modules.

## Dependencies

```text
A1 Command Registry
 ├── A2 Editor Controller
 ├── A6 Major Modes
 └── A8 Public API

A3 Settings
 ├── A4 Themes
 ├── A5 Font Settings
 └── A9 Keymaps

A6 Major Modes ── A9 Keymaps
A7 View Model ─── future window splitting
A8 Public API ─── external modes, hooks, and renderers
A10 Minibuffer ── completion, histories, and command discovery
A11 Collections ─ buffer, command, and package scalability
A12 Core Library ─ broad tests, CI, and alternative frontends
```

## General definition of done

A step is complete only when:

- [ ] Every step-specific acceptance criterion passes.
- [ ] `make`, `make test`, and `make format-check` pass without warnings.
- [ ] `git diff --check` passes.
- [ ] Relevant sanitizers pass, or an environment limitation is documented.
- [ ] Tests cover migrated behavior and at least one failure case.
- [ ] README, roadmap, and public documentation are updated when necessary.
- [ ] `README.md` was explicitly reviewed even when no edit was required.
- [ ] The branch was published and integrated into `main` without another step.
