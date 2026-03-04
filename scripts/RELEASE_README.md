# Crispy Doom with MCP

Play Doom with Model Context Protocol support.

## Quick Start

1. Unzip this folder
2. Run `./go.sh` (Linux/macOS) or `go.bat` (Windows)
3. Play

## Use Your Own WAD

Copy your `doom.wad` (or any IWAD) into this folder, then run `go.sh` / `go.bat`.

Or set the `DOOM_WAD` environment variable:

```bash
# Linux/macOS
DOOM_WAD=/path/to/doom2.wad ./go.sh

# Windows (PowerShell)
$env:DOOM_WAD="C:\path\to\doom2.wad"
.\go.bat
```

## Included Files

- `crispy-doom[.exe]` - Game binary
- `doom1.wad` - Shareware episode (free)
- `libdmcp.so` / `libdmcp.dylib` / `dmcp.dll` - MCP runtime library
- `go.sh` / `go.bat` - Launcher script

## MCP Server

This build includes an MCP server on `http://localhost:6060` for AI integration.
