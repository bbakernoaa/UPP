# Agent Instructions

This repository uses layered agent instructions.

## Primary Source

Use `./.github/copilot-instructions.md` as the authoritative, top-level instruction set for this codebase.

## Additional Scoped Rules

Language and domain-specific rules are located in `./.github/instructions/` and apply by file pattern (`applyTo`) and task scope.

## Precedence

When multiple rules apply, use this order:

1. Security and federal compliance
2. EE2 standards
3. Language/domain instructions
4. Task-specific user request

If two rules conflict, choose the option that preserves EE2 operational correctness and document the decision.

## Skills

Canonical reusable skills live in `./.github/skills/`.
Runtime-specific skill links are maintained in:

- `.agent/skills/`
- `.gemini/skills/`

To refresh skill links, run:

```bash
./hooks/sync-skills.sh
```
