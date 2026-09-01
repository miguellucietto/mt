# mt

An extensible C editor built with SDL3 and inspired by Emacs architecture:
everything happens in buffers, and actions are named commands provided by the
core or by packages.

> **Transparency:** this is an experimental, *vibe-coded* project developed with
> intensive AI assistance. The repository is not intended as evidence of the
> author's personal technical expertise. Changes are reviewed, tested, and
> documented to make the editor progressively more reliable, but the software
> should still be treated as experimental.

Feature planning, TODOs, and known technical debt are tracked in
[ROADMAP.md](ROADMAP.md). The modularity and extensibility plan is tracked in
[ARCHITECTURE.md](ARCHITECTURE.md). Data-structure choices and their replacement
criteria are recorded in [DATA_STRUCTURES.md](DATA_STRUCTURES.md).

## Building

```sh
make
make test
./mt file.c
```

The project requires a C17 compiler, `pkg-config`, SDL3, and SDL3_ttf.

## Usage

- `Alt+X`: open the `M-x` minibuffer and run a command by name
- `Alt+T`: open the `cmd` prompt directly
- `Ctrl+O`: open a file or directory
- `Ctrl+D`: open a directory in Dired
- `Ctrl+B`: switch to the next buffer
- `Ctrl+S`: save the current buffer
- `Ctrl+A/C/X/V`: select all, copy, cut, and paste
- `Ctrl+Z` / `Ctrl+Shift+Z`: undo and redo changes in the current buffer
- `Ctrl+Left/Right`: move by word
- `Ctrl+Shift+Left/Right`: select by word
- `Home/End`: move to the start or end of the line
- `Ctrl+Home/End`: move to the start or end of the buffer
- `Ctrl+K`: delete from the cursor to the end of the line; at line end, remove
  the newline
- `Ctrl+F`: start incremental search
- Arrow keys, Home, End, Page Up/Down, and Shift extend or move the selection

Important `M-x` commands:

- `cmd`: prompt for a shell command and display stdout/stderr in `*cmd*`
- `find-file`: open a path in another buffer
- `dired`: open the directory browser
- `next-buffer`: switch between buffers
- `save`, `copy`, `paste`, `select-all`, and `quit`
- `isearch` and `query-replace`

Press `Enter` to submit the minibuffer and `Esc` to cancel it. Submitting an
empty `M-x` opens `*commands*`, which lists every registered native and package
command with its description.

When quitting with unsaved changes, or when opening a file that would replace a
modified buffer with the same name, the editor requires the exact confirmation
text `yes`.

## Search and replace

`Ctrl+F` or `M-x isearch` starts incremental search. Results update while text is
entered. Press `Ctrl+F` again for the next occurrence, `Enter` to accept, or
`Esc` to cancel and restore the original cursor position.

`M-x query-replace` asks for the search text and then the replacement. At each
match:

- `y`: replace this match
- `n`: skip this match
- `!`: replace all remaining matches
- `q` or `Esc`: stop

On keyboards where `Fn` is handled by firmware, SDL receives Home, End, Page Up,
or Page Down directly; these events are already mapped.

## Dired

Run `M-x dired`, enter a directory, and press Enter. Inside the buffer:

- Arrow keys move the cursor.
- `Enter` opens a file or enters a directory.
- `g` refreshes the listing.
- `Ctrl+B` switches back to another buffer.

File-management commands available through `M-x` while in Dired:

- `dired-create-file`
- `dired-create-directory`
- `dired-rename` — operates on the entry under the cursor
- `dired-delete` — requires typing `yes` before deletion

## C highlighting

Buffers for `.c` and `.h` files highlight keywords, strings, character literals,
numbers, line comments, and preprocessor directives. The highlighter is
independent from the renderer so other languages can be added later.

## Configuration

On first launch, mt creates:

```text
~/.config/mt/
├── keymap.conf
└── packages/
```

When `XDG_CONFIG_HOME` is set, it is used instead of `~/.config`. The legacy
`MT_KEYMAP` variable is no longer required.

The `keymap.conf` format is:

```text
# key = command
alt+x = execute-command
ctrl+o = find-file
ctrl+d = dired
ctrl+b = next-buffer
alt+t = cmd
```

Supported modifiers are `ctrl`, `shift`, `alt`, and `super`. A binding may refer
to a native command or a command registered by a package.

## Packages

Packages are `.so` shared libraries placed in `~/.config/mt/packages`. Each
package exports:

```c
bool mt_package_init(MtAPI *api);
```

`api->register_command` adds a command's name, description, flags, and function
to the same registry used by native commands. The command then becomes available
to `M-x`, the command listing, and keymaps. See `examples/hello-package.c`:

```sh
make package-example
cp build/hello-package.so ~/.config/mt/packages/
./mt
```

Then run `M-x hello`. The executable uses `-rdynamic`, allowing packages to use
the public API declared under `include/`.

The package API is still experimental and exposes internal structures. ABI
stabilization and opaque handles are tracked in `ARCHITECTURE.md`.

## Architecture

- `buffer`: buffer lifecycle, switching, files, and directories
- `command`: unified registration and lookup for native and package commands
- `document`: mutable text storage, selection, undo/redo, and persistence
- `text`: UTF-8 navigation and text coordinates
- `editor`: event handling and command coordination
- `editing`: editing and navigation command implementations
- `minibuffer`: command and argument input
- `keymap`: configurable mapping from keys to command names
- `package`: dynamic package loading with `dlopen`
- `highlight`: lexical analysis independent from rendering
- `renderer`: SDL presentation for buffers and the minibuffer
- `config`: XDG configuration discovery and creation
