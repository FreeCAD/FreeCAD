import logging
import subprocess


def setup_logger(verbose: bool):
    """Configure logging settings."""
    level = logging.DEBUG if verbose else logging.INFO
    logging.basicConfig(level=level, format="%(levelname)s - %(message)s")


def run_command(cmd, capture_output=True, check=True, text=True) -> subprocess.CompletedProcess:
    """Run a command and return the completed process."""
    result = subprocess.run(cmd, capture_output=capture_output, text=text, check=check)

    if check and result.returncode != 0:
        logging.error(f"Executing: {' '.join(cmd)}")
        logging.error(f"Command failed with exit code {result.returncode}")
    else:
        logging.debug(f"Running command: {' '.join(cmd) if isinstance(cmd, list) else cmd}")

    logging.debug(f"stdout: {result.stdout}")
    if result.stderr:
        logging.debug(f"stderr: {result.stderr}")

    return result.stdout, result.stderr, result.returncode
