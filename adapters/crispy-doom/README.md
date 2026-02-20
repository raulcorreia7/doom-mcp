# Crispy Doom DMCP Integration

This directory tracks the DMCP integration patch used for the Crispy Doom
submodule.

## Files

- `patches/dmcp_integration.patch` - Unified diff applied to `crispy-doom/`
  to enable DMCP hooks and build wiring.

## Build Workflow

From the repository root:

```bash
make submodules
make crispy-doom
```

`make crispy-doom` automatically runs
`tests/integration/apply_crispy_dmcp_patch.sh` before configuring/building.

## Reapply Patch Manually

```bash
./tests/integration/apply_crispy_dmcp_patch.sh
```

The script is idempotent:

- applies the patch if missing,
- reports "already applied" if present,
- fails if the submodule tree diverged from the expected base.
