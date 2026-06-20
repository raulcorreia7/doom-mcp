# Crispy Doom DMCP Adapter

## Files

- `include/` - C headers used by the Crispy engine hook points.
- `src/` - Adapter lifecycle, state extraction, command execution, and input
  handling.

The pinned `crispy-doom/` submodule already contains the minimal engine hook
points and CMake options needed to build these adapter sources. This adapter
directory is the reusable implementation; there is no tracked patch file to
apply.

## Build Workflow

From the repository root:

```bash
make submodules
make crispy-doom
```

`make crispy-doom` automatically runs
`tests/integration/build_crispy_doom.sh`, which validates the checked-in hooks,
builds DMCP, and configures Crispy with `DMCP_ENABLE=ON`.

This is a build/integration check only: it does not apply patches, download a
WAD, or launch the game. Runtime/headless Crispy checks are optional engine e2e
validation and should stay separate from the default no-game test path.
