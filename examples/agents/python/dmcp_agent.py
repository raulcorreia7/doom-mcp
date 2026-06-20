#!/usr/bin/env python3
"""Compatibility wrapper for running the packaged DMCP agent helper."""

from dmcp_agent.cli import main


if __name__ == "__main__":
    raise SystemExit(main())
