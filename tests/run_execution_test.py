"""Run the C API tests with disposable executable fixtures."""
from pathlib import Path
import subprocess
import sys
import tempfile

executable = str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix="cop4610-api-") as temporary:
    root = Path(temporary)
    for name, contents in {
        "bad-executable": "invalid executable\n",
        "exit127": "#!/bin/sh\nexit 127\n",
    }.items():
        fixture = root / name
        fixture.write_text(contents)
        fixture.chmod(0o700)
    subprocess.run([executable], cwd=root, check=True, timeout=10)
