# mt roadmap

This document tracks the work required to turn `mt` into a small, stable, and
genuinely extensible editor. The goal is not to reproduce all of Emacs, but to
preserve its strongest ideas: buffers as the central unit, named commands,
transparent configuration, and extensions decoupled from the core.

The structural work required to keep these features modular is detailed in
[ARCHITECTURE.md](ARCHITECTURE.md).

## Principles

- Keep the core small and independent from the graphical interface.
- Every relevant interactive action must be a named command.
- Native and package commands must behave the same way.
- Special buffers must not require scattered exceptions throughout the editor.
- Operations that can lose data require confirmation or recovery.
- Text features must handle UTF-8 correctly.
- Every change must compile without warnings and include risk-appropriate tests.

## Priority 0 — Reliability and safety

- [x] Warn about modified buffers before quitting.
- [x] Warn before closing or replacing a modified buffer.
- [x] Implement atomic saving through a temporary file and `rename`.
- [x] Preserve file permissions while saving.
- [ ] Detect external changes to an open file.
- [ ] Report complete open, read, and write errors in `*messages*`.
- [ ] Prevent Dired from deleting non-empty directories without explicit
      confirmation.
- [ ] Run `cmd` asynchronously so it cannot freeze the interface.
- [ ] Add a configurable limit for very large process output.
- [ ] Handle binary files and invalid UTF-8 bytes without corrupting the buffer.

Completion criterion: normal workflows cannot silently lose changes, and
external processes cannot block the SDL main loop.

## Priority 1 — Editing foundations

- [x] Implement per-buffer undo/redo history.
- [ ] Add an Emacs-style kill ring with `kill-region`, `kill-line`, and `yank-pop`.
- [ ] Delete the previous or next word.
- [ ] Transpose characters, words, and lines.
- [ ] Duplicate, move, and select lines.
- [ ] Indent and unindent regions.
- [ ] Detect and preserve LF and CRLF line endings.
- [ ] Configure Tab width and behavior.
- [ ] Add basic automatic indentation on Enter.
- [ ] Add reverse search and search history.
- [ ] Make `query-replace` optionally case-sensitive.
- [ ] Add regular-expression search and replacement.
- [ ] Preserve visual columns across tabs, wide Unicode, and combining marks.

Completion criterion: long editing sessions are safe, predictable, and
reversible.

## Priority 2 — Buffers and windows

- [ ] Implement `list-buffers` with selection, closing, and modified indicators.
- [ ] Implement `kill-buffer` and `rename-buffer`.
- [ ] Allow duplicate buffer names through automatic unique naming.
- [ ] Separate `Buffer`, `View`, and `Window` so one buffer can have multiple views.
- [ ] Split windows horizontally and vertically.
- [ ] Resize, switch, and close splits.
- [ ] Store cursor and scrolling per view instead of globally.
- [ ] Create special buffers through a mode interface without direct `BufferType`
      checks in the event loop.
- [ ] Add a persistent and configurable `*scratch*` buffer.
- [ ] Persist sessions: open files, positions, splits, and active buffer.

Completion criterion: users can organize several files without losing cursor or
scrolling context.

## Priority 3 — Minibuffer and commands

- [ ] Add command completion to `M-x`.
- [ ] Show the selected command's description and keybinding.
- [ ] Keep separate histories for commands, paths, searches, and shell input.
- [ ] Navigate history with arrow keys.
- [ ] Complete paths and file names.
- [ ] Support prefix arguments analogous to Emacs `C-u`.
- [ ] Add `describe-command`, `describe-key`, and `where-is`.
- [ ] Reload keymaps and packages without restarting.
- [ ] Validate the complete keymap file and show all errors.
- [ ] Explicitly remove bindings.
- [ ] Support mode-local and transient keymaps.
- [ ] Support key sequences such as `Ctrl+X Ctrl+S`.

Completion criterion: every feature is discoverable and executable without
reading source code.

## Priority 4 — Dired and files

- [ ] Display size, permissions, and modification time.
- [ ] Sort by name, size, date, and type.
- [ ] Toggle hidden files.
- [ ] Mark multiple entries for batch operations.
- [ ] Copy and move files between directories.
- [ ] Use the system trash when available.
- [ ] Show a confirmation containing the exact target list.
- [ ] Rename multiple files.
- [ ] Search and filter entries.
- [ ] Refresh while preserving selection.
- [ ] Watch filesystem changes.
- [ ] Open files in external applications through an explicit command.

Completion criterion: common file-management tasks are available without leaving
the editor or risking accidental destructive operations.

## Priority 5 — Modes and highlighting

- [ ] Define a formal major-mode interface.
- [ ] Associate modes by extension, file name, and content.
- [ ] Move C mode into a separate module.
- [ ] Preserve lexical state across lines for multiline comments and strings.
- [ ] Highlight incrementally, only in changed regions.
- [ ] Add Markdown, JSON, Makefile, and plain-text modes.
- [ ] Highlight delimiter pairs.
- [ ] Match parentheses.
- [ ] Add optional relative line numbers.
- [ ] Add whitespace mode and trailing-whitespace indicators.
- [ ] Add an optional Tree-sitter interface.
- [ ] Prepare diagnostics and future LSP integration.

Completion criterion: modes control highlighting, indentation, commands, and
local keymaps without modifying the core.

## Priority 6 — Packages

- [ ] Formally version the public package ABI.
- [ ] Hide internal structures and expose stable functions only.
- [ ] Report detailed `dlopen` and `mt_package_init` failures.
- [ ] Safely unload and reload packages.
- [ ] Add metadata: name, version, author, description, and minimum mt version.
- [ ] Resolve package dependencies.
- [ ] Add hooks for opening, saving, editing, and switching buffers.
- [ ] Let packages register modes, renderers, and local keymaps.
- [ ] Provide a small SDK with an example, documentation, and template.
- [ ] Add ABI compatibility tests.
- [ ] Evaluate a safe configuration language such as Lua for extensions that do
      not require native C.

Completion criterion: editor updates do not silently break packages compatible
with the same ABI version.

## Priority 7 — Interface and performance

- [ ] Cache textures or use a glyph atlas; rendering currently creates textures
      for every segment.
- [ ] Render only visible lines that changed.
- [ ] Replace the linear buffer with a piece table, gap buffer, or rope after
      benchmarking.
- [ ] Add an incremental line index.
- [ ] Support large files without loading or redrawing everything.
- [ ] Correct Unicode display width with an appropriate library.
- [ ] Add horizontal scrolling.
- [ ] Load themes from `~/.config/mt/theme.conf`.
- [ ] Configure font, size, line height, and margins.
- [ ] Add a scrollbar, configurable cursor, and progress indicator.
- [ ] Improve accessibility, contrast, and HiDPI scaling.

Completion criterion: large files remain responsive and rendering does not
recreate unchanged graphical resources every frame.

## Priority 8 — Tests and development tools

- [ ] Separate the core library from the executable.
- [ ] Test all editing commands without initializing SDL video.
- [ ] Test search, replacement, undo, and selection with UTF-8.
- [ ] Test Dired in a controlled temporary tree.
- [ ] Test successful and failed package loading.
- [ ] Add official AddressSanitizer and UndefinedBehaviorSanitizer targets.
- [ ] Run `clang-tidy` or equivalent static analysis.
- [ ] Fuzz document, UTF-8, keymap, and configuration parsing.
- [ ] Test SDL events for shortcuts and the minibuffer.
- [ ] Add Linux CI, followed by macOS and Windows.
- [ ] Measure coverage without treating coverage as an isolated goal.

## Known technical debt

- `Document` uses a contiguous array and moves memory during large insertions.
- Column calculations do not fully model visual Unicode width.
- The C highlighter is intentionally simple and has no cross-line state.
- `cmd` uses synchronous `popen` behind the isolated `process` module.
- Cursor and scrolling state belong to the editor instead of individual views.
- The unified command registry still has a fixed maximum capacity.
- The package API exposes internal structures and has no ABI version.
- Buffers use a fixed-size array.
- Messages use one short field instead of a log buffer.
- Dired encodes one entry per line and parses its rendered text.

## Suggested milestones

### Milestone 1 — Safe editor

Undo/redo, modified-buffer confirmation, atomic saving, and asynchronous `cmd`.

### Milestone 2 — Working environment

Buffer list and closing, window splits, history, and minibuffer completion.

### Milestone 3 — Real modes

Major-mode interface, incremental C mode, Markdown, JSON, and local keymaps.

### Milestone 4 — Stable extensibility

Versioned ABI, hooks, package SDK, reload support, and compatibility tests.

### Milestone 5 — Scale

A benchmark-selected text structure, line index, glyph cache, and reliable large
file support.

## Current recommended work

The command registry step in `ARCHITECTURE.md` is complete. The next structural
step is A2, decomposing the editor controller before more behavior is added to
`editor.c`. Reliability work from Priority 0 should continue after each safe,
isolated architectural migration.
