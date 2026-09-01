# Contributing

## Function documentation

Every function must have an accurate English comment that explains its contract
or intent. Public functions are documented next to their declaration in the
public header; internal functions, callbacks, tests, examples, and entry points
are documented immediately before their definition.

A useful function comment records the non-obvious parts of the contract: return
meaning, ownership, mutation, preconditions, important side effects, or why the
function exists. Do not merely restate the function name or narrate its body.

Whenever a function changes, review its comment in the same commit. Update the
comment when behavior, ownership, inputs, outputs, failure handling, or side
effects change. A change is incomplete if its implementation and documentation
disagree.

## Change checklist

- Keep each branch focused on one roadmap item or maintenance concern.
- Add or update tests for behavior changes.
- Review `README.md`, `ROADMAP.md`, `ARCHITECTURE.md`, and `DATA_STRUCTURES.md` and
  update whichever documents are affected.
- Run `make`, `make test`, `make package-example`, `make format-check`, and
  `git diff --check` before committing.
- Use a descriptive commit message and push the branch checkpoint.
