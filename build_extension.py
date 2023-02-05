from subprocess import call
from typing import Any, Dict, Optional


def build(setup_kwargs: Optional[Dict[str, Any]] = None):
    cmd = ["./scripts/install.sh", "release"]
    call(cmd)


if __name__ == "__main__":
    build()
