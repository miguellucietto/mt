# Data structure audit

This document records why each core data structure is currently used, where it
will stop scaling, and what evidence should justify replacing it. It prevents
premature complexity while keeping known limits explicit.

## Decision policy

A structure should change only when at least one of these conditions is met:

- measured latency or memory use violates an explicit target;
- a required feature cannot preserve correct ownership or pointer lifetime;
- a fixed limit is reachable in normal daily use;
- failure handling cannot preserve the previous valid state;
- the structure forces unrelated modules to depend on internal representation.

Before replacement, record the workload, benchmark or failing use case, ownership
model, pointer-invalidation rules, and migration tests. Prefer the simplest
structure that meets the measured requirement.

## Audit summary

| Area | Current structure | Decision | Planned trigger |
|---|---|---|---|
| Document text | growable contiguous byte array | keep for now | benchmark large edits/files |
| Undo/redo | two growable arrays of reversible edits | keep | add memory policy when needed |
| Buffers | fixed array of 32 inline `Buffer` values | keep temporarily | A7 before A11 migration |
| Commands | fixed array of 128 `CommandSpec` values | keep temporarily | A11 dynamic collections |
| Keybindings | fixed array of 64 bindings | keep until A9 | scopes/sequences require redesign |
| Packages | fixed array of 32 handles | keep temporarily | ABI lifecycle work in A8/A11 |
| Minibuffer text | fixed bounded arrays | keep | session model in A10 |
| Search state | fixed bounded arrays in `Editor` | move, then reassess | A2 extraction |
| Dired entries | rendered text parsed as records | replace later | formal mode/file model |
| Highlight spans | fixed per-line stack array | keep for current lexer | incremental mode/highlighting work |
| Messages | one fixed 256-byte field | replace later | `*messages*` log buffer |
| Configuration paths | fixed 4096-byte arrays | keep | platform/path abstraction |

## Detailed findings

### Document text

Current representation: a heap-allocated contiguous `char` array with geometric
capacity growth.

Why it fits today:

- UTF-8 byte offsets match the existing document and undo APIs.
- Sequential rendering and saving are simple and cache-friendly.
- Small files and ordinary insertions have low implementation overhead.
- Ownership is clear: one allocation belongs to one `Document`.

Known limit: insertion and deletion move the suffix with `memmove`, making edits
near the start of large documents O(n). Reallocation also invalidates pointers
into text, although current callers use offsets instead of retaining pointers.

Decision: keep it until representative benchmarks exist. Before choosing a gap
buffer, piece table, or rope, benchmark typing, large paste, undo, line lookup,
render iteration, and saving at several file sizes. The replacement must not be
selected by reputation alone.

### Undo and redo

Current representation: two growable arrays of `DocumentEdit`, each edit owning
copies of removed and inserted bytes.

Strengths:

- stack operations are amortized O(1);
- edits remain in chronological order;
- ownership and cleanup are explicit;
- redo invalidation is straightforward.

Known limit: history memory is unbounded and duplicated text can become large.

Decision: keep the representation. Add a configurable byte or transaction budget
only when long-session measurements justify it. Any eviction policy must preserve
the saved revision semantics.

### Buffer storage

Current representation: 32 inline `Buffer` objects in `BufferManager`.

Strengths:

- no allocation failure after manager initialization;
- `Buffer *` values remain stable;
- destruction is simple and deterministic.

Limits:

- 32 buffers is reachable in daily use;
- inline values make later growth dangerous because `realloc` would invalidate
  every retained `Buffer *`;
- name lookup is linear and duplicate names are not yet supported.

Decision: do not mechanically replace this with `realloc` now. A7 should first
define stable Buffer/View ownership. A11 can then use individually allocated
buffers plus a dynamic pointer vector, or stable handles, without introducing
pointer invalidation.

### Command registry

Current representation: 128 inline `CommandSpec` objects with linear lookup.

Strengths:

- simple unified behavior for native and package commands;
- stable `CommandSpec *` addresses;
- linear lookup is negligible at the current scale.

Limits: capacity is fixed, lookup becomes less attractive with large package
sets, and inline growth would invalidate returned pointers.

Decision: keep through A2. In A11, prefer individually owned command specs with a
dynamic ordered collection. Add a hash index only if measured lookup or completion
workloads justify maintaining two structures.

### Keymaps

Current representation: 64 inline bindings containing SDL key types and command
names.

Strengths: deterministic order, trivial override lookup, and no heap ownership.

Limits: normal custom configurations may reach 64 entries; mode-local maps,
transient maps, unbinding, and key sequences cannot be represented cleanly.

Decision: keep until A9, where the semantic model must be designed before storage.
A dynamic flat vector is likely sufficient for individual maps; a trie becomes
useful only for multi-key sequences. SDL conversion must remain outside that model.

### Package handles

Current representation: 32 `dlopen` handles in load order.

Strengths: unloading in reverse-independent order is currently simple, and the
small fixed list matches the experimental API.

Limits: package metadata, dependencies, command ownership, reload, and partial
rollback need a record per package rather than a bare handle.

Decision: keep until A8 defines package identity and ABI lifecycle. Replace with
dynamic `Package` records only after ownership of registered resources is known.

### Minibuffer

Current representation: one enum plus fixed prompt and input arrays.

Strengths: bounded writes, simple ownership, and no allocation in event handling.

Limits: input is capped at 1023 bytes, prompts can truncate, and the enum cannot
represent independent completion/history sessions.

Decision: keep bounded storage during A2. A10 should first introduce session
ownership and callbacks; input can then become a reusable growable string if real
path or shell workloads require it.

### Search and replace state

Current representation: fixed arrays and scalar fields embedded in `Editor`.

Problem: the storage itself is adequate for current limits, but its ownership is
wrong. Search behavior and state should move together into a search controller in
A2. Capacity changes should wait until that boundary is established.

### Dired entries

Current representation: a rendered line per entry, later parsed back into a path
and type.

Problem: presentation is acting as the data model. Sorting, metadata, marking,
batch operations, and safe refresh all become fragile.

Decision: replace when Dired receives a formal mode/controller. Use structured
entry records with owned name/path metadata and render them one-way into the view.
Do not add more parsing conventions to the displayed text.

### Highlight spans

Current representation: up to 128 spans in a stack array for one visible line.

Strengths: no allocation in the render loop and bounded transient memory.

Limit: pathological lines can silently omit spans, and lexical state cannot cross
lines.

Decision: keep for the current deliberately simple C lexer. A6 and incremental
highlighting should define cached per-buffer tokens and explicit truncation/error
behavior before changing storage.

### Messages

Current representation: one 256-byte `Editor.message` array.

Problem: later messages overwrite earlier diagnostics, long errors truncate, and
packages write directly into internal state.

Decision: replace with an append-only bounded `*messages*` log when Priority 0
error reporting is implemented. Use a configurable total byte budget rather than
an unbounded list.

### Configuration paths

Current representation: three inline 4096-byte arrays.

Strengths: simple lifetime, no allocation failures during use, and sufficient
space on the current Linux target.

Limit: the constant is platform-specific and several modules duplicate path
capacities.

Decision: keep until configuration and platform abstractions are introduced. A3
should centralize path construction and reject truncation; portability work can
then select an owned dynamic path type if necessary.

### Editor aggregate

Current representation: one large `Editor` struct owning subsystems and transient
state.

Using an aggregate root is appropriate, but feature-specific state is embedded in
the wrong owner. A2 should extract controllers while keeping `Editor` as the
lifecycle/composition root. A7 should later move per-view state out of it. This is
an ownership refactor, not a reason to introduce inheritance-style abstractions.

The A2 editing extraction reviewed these structures and intentionally retained
them: editing commands operate on stable `Buffer *` and byte offsets, so changing
buffer or document storage during the controller move would add pointer-lifetime
risk without improving the new boundary.

## Review rule for future changes

Every structural change must revisit the affected section above. If the current
choice remains appropriate, document that it was reviewed and leave it alone. If
it changes, update the decision, ownership model, complexity expectations, and
tests in the same branch.
