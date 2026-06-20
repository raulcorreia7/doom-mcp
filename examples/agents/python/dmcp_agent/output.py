from __future__ import annotations

import json
from typing import Any


def write(value: Any, pretty: bool) -> None:
    if isinstance(value, str):
        print(value)
        return

    kwargs = {"indent": 2, "sort_keys": True} if pretty else {"separators": (",", ":")}
    print(json.dumps(value, **kwargs))
