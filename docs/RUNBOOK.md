# Documentation Maintenance Runbook

This runbook keeps product-facing documentation fresh, linked, and easy to use
for both humans and LLM agents.

## Scope

In scope:
- `README.md` - Project entry point
- `docs/QUICKSTART.md` - Quick start guide
- `docs/USAGE.md` - Usage patterns
- `docs/FEATURES.md` - Feature overview
- `docs/README.md` - API reference
- `docs/INTEGRATION.md` - MCP client setup
- Adapter READMEs under `adapters/`

Out of scope (escalate to maintainers before editing):
- Specifications/design docs (for example `docs/ARCHITECTURE.md`)
- ADRs
- Contracts and protocol compatibility commitments

## Documentation Map

| Document | Purpose | Audience |
|----------|---------|----------|
| `README.md` | Entry point, what/why/how | New users |
| `docs/QUICKSTART.md` | Get running in 5 minutes | New users |
| `docs/USAGE.md` | Common patterns | Developers |
| `docs/FEATURES.md` | Complete feature list | All users |
| `docs/README.md` | Full API reference | Integrators |
| `docs/INTEGRATION.md` | MCP client setup | AI tool users |
| `docs/ARCHITECTURE.md` | System design | Contributors |
| `docs/MCP_COMPLIANCE.md` | Protocol compliance | Contributors |
| `docs/CHANGELOG.md` | Version history | All users |

## Maintenance Loop

1. Survey
   - Inventory docs in scope.
   - Mark freshness: current, at risk, stale.
   - Note drift against code paths, commands, and exposed APIs.
2. Prioritize
   - Rank updates by user impact:
     - P0: setup/build/run steps broken or misleading.
     - P1: API/tool naming drift.
     - P2: style/clarity improvements.
3. Revise
   - Update in-place where possible.
   - Link to a single source of truth instead of duplicating content.
   - Remove stale or redundant sections.
4. Plan
   - Record next review date and owner in the cadence table.

## Freshness Signals

- Build commands match `Makefile` and active tests under `tests/`.
- Tool names/examples match `src/doom/handlers/tools/` behavior.
- Endpoint examples match registered routes in `src/doom/context.cpp`.
- Cross-links resolve and avoid duplicate long-form API content.

## Versioning

- The SDK release version is maintained in top-level `CMakeLists.txt` through
  `project(dmcp VERSION X.Y.Z)`.
- CMake generates `mcp/core/version_config.h` from that version at configure time.
  Doom and Generic MCP public headers derive their version macros from it.
- Release tags use `dmcp-vX.Y.Z`; `scripts/ci/verify_release_tag.sh` validates
  tags against the CMake project version.
- README and architecture documents do not carry separate version banners; use
  `docs/CHANGELOG.md` for release history.

## Cadence

| Cadence | Owner | Trigger | Expected Output |
|---------|-------|---------|-----------------|
| Weekly | Docs DRI | Any merged PR touching CLI, API tools, or routes | Small corrections and link cleanup |
| Release (pre-tag) | Release manager | Version bump/changelog prep | Full sweep of setup, compatibility, and examples |
| Monthly | Docs DRI | No release activity | Drift audit and stale-content removal |

## Update Checklist

- Verify `make check`, `make validate`, and SDK integration commands still match docs.
- Keep engine-backed runtime checks out of SDK CI/docs unless they are clearly
  owned by an adapter document.
- Verify MCP examples still work for `initialize`, `tools/list`, and `tools/call`.
- Remove sections that duplicate another maintained document.
- Capture owner and due date for the next review cycle.
