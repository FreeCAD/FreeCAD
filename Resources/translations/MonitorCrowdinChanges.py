import json
import os
from pathlib import Path

from crowdin_api import CrowdinClient


# ------------------------------------------------------------
# Configuration
# ------------------------------------------------------------

PROJECT_ID = 392231
FILE_IDS = [32182, 32184]
# FILE_ID = 32184

CROWDIN_TOKEN = "3df4091bbee7b41494ff0ead24e0c52e75fd98efdae2282aa0c6a041bea827e0520be77f1de1eb52"
STATE_FILE = Path("crowdin_state.json")


# ------------------------------------------------------------
# Crowdin client
# ------------------------------------------------------------

client = CrowdinClient(
    token=CROWDIN_TOKEN,
    project_id=PROJECT_ID,
)

# ------------------------------------------------------------
# Load previous state
# ------------------------------------------------------------

if STATE_FILE.exists():
    with STATE_FILE.open("r", encoding="utf-8") as f:
        old_state = json.load(f)
else:
    old_state = {}

new_state = {}


for file_id in FILE_IDS:
    # ------------------------------------------------------------
    # Get current translation status
    # ------------------------------------------------------------
    print(f"Processing file: {file_id}")
    result = client.translation_status.get_file_progress(
        fileId=file_id
    )

    data = result["data"]
    new_state[file_id] = {}

    # ------------------------------------------------------------
    # Compare eTags
    # ------------------------------------------------------------


    for lang_info in data:
        item = lang_info["data"]
        language_id = item["languageId"]
        etag = item["eTag"]

        new_state[file_id][language_id] = etag

        old_etag = None
        if old_state.get(file_id):
            old_etag = old_state[file_id].get(language_id)

        if old_etag is None:
            # First time seeing this language.
            print(f"Initial state: {language_id}")

        elif old_etag != etag:
            print(
                f"Translation changed: "
                f"file ID {file_id}, language {language_id}"
            )


# ------------------------------------------------------------
# Save current state
# ------------------------------------------------------------

with STATE_FILE.open("w", encoding="utf-8") as f:
    json.dump(new_state, f, indent=2)

print("State saved.")
