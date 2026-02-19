#!/bin/bash
# Start Doom with proper environment and output handling

export SDL_VIDEODRIVER=dummy
export SDL_AUDIODRIVER=dummy
export SDL_NOMOUSE=1

exec "$@"
