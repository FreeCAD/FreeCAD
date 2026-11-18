#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""
score_prs.py

Score open pull requests in a GitHub Projects v2 board and write two metrics
into custom number fields so the board can be sorted by priority.

Metrics:
  Merge Meeting Priority  - how urgently a PR needs to be discussed at a
                            maintainer meeting (stalled, complex, no reviews).
  Mergeability Score      - how close a PR is to being merged asynchronously
                            without meeting discussion.

Usage:
    python score_prs.py --project <number> [--dry-run] [--all] [--sort METRIC]
                        [--skip-label LABEL ...]

    --project     GitHub Project v2 number (visible in the project URL)
    --dry-run     Print scores to stdout without updating the project
    --all         Show all individual scoring factors (implies --dry-run output)
    --sort        Sort output by 'meeting' or 'mergeability' (default: meeting)
    --skip-label  Skip PRs with this label (repeatable); 'Status: Stale' always skipped

Environment variables (all required):
    GH_TOKEN    GitHub token with repo read + projects write access
    REPO_OWNER  Organisation or user name (e.g. "FreeCAD")
"""

from __future__ import annotations

import argparse
import csv
import logging
import math
import os
import random
import sys
import time
from datetime import datetime, timezone
from typing import Any, NamedTuple

import requests

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------


def _require_env(name: str) -> str:
    """Return an environment variable or exit with a readable message."""
    value = os.environ.get(name)
    if not value:
        raise SystemExit(f"error: required environment variable {name} is not set")
    return value


GH_TOKEN = _require_env("GH_TOKEN")
REPO_OWNER = _require_env("REPO_OWNER")

GRAPHQL_URL = "https://api.github.com/graphql"

MEETING_FIELD = "Merge Meeting Priority"
MERGEABILITY_FIELD = "Mergeability Score"

# authorAssociation values on reviews/comments that indicate write access.
# Read inline from GraphQL, so no admin token is required.
WRITE_ACCESS_ASSOCIATIONS = {"OWNER", "MEMBER", "COLLABORATOR"}

SIZE_NEUTRAL_LINES = 1000
SIZE_WEIGHT = 30.0

HEADERS = {
    "Authorization": f"Bearer {GH_TOKEN}",
    "Accept": "application/vnd.github+json",
    "X-GitHub-Api-Version": "2022-11-28",
}

logging.basicConfig(level=logging.INFO, format="%(levelname)s %(message)s")
log = logging.getLogger(__name__)


# ===========================================================================
# SCORING LOGIC: adjust these functions freely without touching plumbing code
# ===========================================================================


def compute_meeting_priority(
    *,
    days_since_last_write_activity: float,
    pr_age_days: float,
    additions: int,
    deletions: int,
    requires_label_count: int,
    approved_label_count: int,
    author_association: str,
    is_blocked: bool,
) -> tuple[float, dict[str, float]]:
    """Compute the Merge Meeting Priority score for a pull request.

    *** THIS FUNCTION IS INTENTIONALLY ISOLATED FOR EASY ADJUSTMENT ***

    Higher = more urgent to raise at a maintainer meeting.

    is_blocked short-circuits the score to 0. Callers currently always pass False:
    a PR that is conflicting or has requested changes is still a valid meeting
    topic, so it must not be pushed to the bottom of the board. The hook is kept
    for a future blocked condition that genuinely has nothing to discuss.

    Returns:
        (score, factors) where factors maps each signal name to its numerical contribution.
    """
    if is_blocked:
        return 0.0, {"blocked": 0.0}

    factors: dict[str, float] = {}

    factors["days_since_activity"] = days_since_last_write_activity
    factors["age_bonus"] = math.sqrt(max(pr_age_days, 0.0)) / 10
    factors["size_bonus"] = math.log(max(additions + deletions + 1, 1)) * 2
    factors["untouched_bonus"] = 10.0 if (requires_label_count > 0 and pr_age_days > 14) else 0.0
    factors["approval_penalty"] = -(approved_label_count * 3.0)

    if author_association in ("FIRST_TIME_CONTRIBUTOR", "FIRST_TIMER"):
        factors["contributor_bonus"] = 25.0
    elif author_association == "CONTRIBUTOR":
        factors["contributor_bonus"] = 10.0
    else:
        factors["contributor_bonus"] = 0.0

    return round(max(sum(factors.values()), 0.0), 2), factors


def compute_mergeability_score(
    *,
    approved_label_count: int,
    requires_label_count: int,
    mergeable: str,
    additions: int,
    deletions: int,
    has_change_requested: bool,
    author_association: str,
    pr_age_days: float,
) -> tuple[float, dict[str, float]]:
    """Compute the Mergeability Score for a pull request.

    *** THIS FUNCTION IS INTENTIONALLY ISOLATED FOR EASY ADJUSTMENT ***

    Higher = easier to merge asynchronously without meeting discussion.

    Returns:
        (score, factors) where factors maps each signal name to its numerical contribution.
    """
    factors: dict[str, float] = {}

    factors["approved_labels"] = approved_label_count * 15.0
    factors["requires_labels"] = -(requires_label_count * 10.0)
    factors["size"] = SIZE_WEIGHT * math.log10(SIZE_NEUTRAL_LINES / max(additions + deletions, 1))
    factors["change_requested"] = -25.0 if has_change_requested else 0.0
    # Soft age penalty: long-open PRs tend to be complex or controversial, but
    # sqrt growth keeps well-approved old PRs from being buried entirely.
    factors["age_penalty"] = -(math.sqrt(max(pr_age_days, 0.0)) / 5)

    if mergeable == "MERGEABLE":
        factors["mergeable"] = 10.0
    elif mergeable == "CONFLICTING":
        factors["mergeable"] = -20.0
    else:
        factors["mergeable"] = 0.0

    if author_association in ("FIRST_TIME_CONTRIBUTOR", "FIRST_TIMER"):
        factors["contributor_bonus"] = 25.0
    elif author_association == "CONTRIBUTOR":
        factors["contributor_bonus"] = 10.0
    else:
        factors["contributor_bonus"] = 0.0

    return round(max(sum(factors.values()), 0.0), 2), factors


# ===========================================================================
# API PLUMBING
# ===========================================================================

_RETRYABLE_STATUSES = {500, 502, 503, 504}
_RATE_LIMIT_STATUSES = {403, 429}
_MAX_RETRIES = 5
_RETRY_BACKOFF_BASE = 2.0  # seconds; delay = base * 2^attempt plus jitter
_REQUEST_TIMEOUT = 60  # seconds
_MAX_RATE_LIMIT_WAIT = 300.0  # seconds; never idle a runner longer than this


class TransientGraphQLError(RuntimeError):
    """Raised when a GraphQL request keeps failing with transient server errors.

    Kept distinct from the plain RuntimeError raised for GraphQL 'errors' payloads,
    which signal a real problem (bad token, bad query) that must not be retried.
    """


def _rate_limit_wait(response: requests.Response) -> float | None:
    """Return the seconds GitHub asks us to wait, or None when this is not a rate limit.

    Handles both the Retry-After header used for secondary rate limits and the
    X-RateLimit-Reset header used when the primary hourly budget is exhausted.
    """
    retry_after = response.headers.get("Retry-After")
    if retry_after:
        try:
            return min(max(float(retry_after), 0.0), _MAX_RATE_LIMIT_WAIT)
        except ValueError:
            return None

    if response.headers.get("X-RateLimit-Remaining") == "0":
        reset = response.headers.get("X-RateLimit-Reset")
        if reset:
            try:
                return min(max(float(reset) - time.time(), 0.0), _MAX_RATE_LIMIT_WAIT)
            except ValueError:
                return None

    return None


def graphql(query: str, variables: dict[str, Any] | None = None) -> dict:
    """Execute a GraphQL query or mutation and return the parsed data dict.

    Retries up to _MAX_RETRIES times on transient failures:
      - connection resets, DNS failures and read timeouts
      - HTTP 500 / 502 / 503 / 504 (gateway errors)
      - HTTP 200 with an empty or non-JSON body (occasional GitHub infra hiccup)
      - primary and secondary rate limits, honouring the wait GitHub asks for
    Uses exponential backoff with jitter between attempts. Authentication and
    authorisation failures raise immediately, with the response body included so
    the cause is visible in the workflow log.
    """
    payload: dict[str, Any] = {"query": query}
    if variables:
        payload["variables"] = variables

    last_error: Exception | None = None
    for attempt in range(_MAX_RETRIES + 1):
        if attempt:
            delay = _RETRY_BACKOFF_BASE * (2 ** (attempt - 1)) + random.uniform(0.0, 1.0)
            log.warning(
                "Retrying GraphQL request (attempt %d/%d) after %.1fs: %s",
                attempt,
                _MAX_RETRIES,
                delay,
                last_error,
            )
            time.sleep(delay)

        try:
            response = requests.post(
                GRAPHQL_URL, json=payload, headers=HEADERS, timeout=_REQUEST_TIMEOUT
            )
        except requests.RequestException as exc:
            last_error = exc
            continue

        if response.status_code in _RATE_LIMIT_STATUSES:
            wait = _rate_limit_wait(response)
            if wait is not None:
                log.warning("Rate limited by GitHub, waiting %.0fs before retrying", wait)
                time.sleep(wait)
                last_error = RuntimeError(
                    f"GraphQL request rate limited (HTTP {response.status_code})"
                )
                continue

        if response.status_code in _RETRYABLE_STATUSES or not response.text:
            last_error = RuntimeError(
                f"GraphQL request returned HTTP {response.status_code}"
                + (" with empty body" if not response.text else "")
            )
            continue

        if response.status_code >= 400:
            # 401 bad credentials, 403 without a rate limit header, 404 and friends.
            # None of these get better by waiting, so fail loudly and show the body.
            raise RuntimeError(
                f"GraphQL request failed with HTTP {response.status_code}:\n{response.text[:500]}"
            )

        try:
            body = response.json()
        except ValueError:
            # Non-JSON body on a 200 is also an ingress-level glitch, so retry.
            last_error = RuntimeError(
                f"GraphQL request returned HTTP {response.status_code} with non-JSON body:\n"
                f"{response.text[:200]}"
            )
            continue

        if "errors" in body:
            errors = body["errors"]
            if any(error.get("type") == "RATE_LIMITED" for error in errors):
                last_error = RuntimeError(f"GraphQL errors: {errors}")
                continue
            raise RuntimeError(f"GraphQL errors: {errors}")

        return body["data"]

    raise TransientGraphQLError(
        f"GraphQL request failed after {_MAX_RETRIES} retries"
    ) from last_error


# ---------------------------------------------------------------------------
# Project data: metadata + all PR items in minimum API calls
# ---------------------------------------------------------------------------

# Number of project items fetched per request. GitHub resolves ProjectV2 item
# pages server-side and answers HTTP 504 when the selection is too expensive, so
# this stays well below the API maximum of 100.
_ITEMS_PAGE_SIZE = 50
_MIN_ITEMS_PAGE_SIZE = 5

# Shared selection for every project item: the PR payload plus the score fields
# already stored on the board, so unchanged items can be skipped on write.
#
# reviews/comments use 'last' because find_last_write_activity() needs the most
# recent maintainer activity. 'first' returns the oldest page and silently misses
# recent reviews on long-running PRs. The comment window is wider than the review
# window because comment volume is much higher than review volume.
#
# fieldValues stays at 30 so it still covers every field on the board. A smaller
# page can push the two number fields out of the result, which defeats the
# skip-unchanged-scores optimisation in _scores_unchanged().
_PR_ITEM_FRAGMENT = """
fragment PrItem on ProjectV2Item {
  id
  content {
    __typename
    ... on PullRequest {
      id number title createdAt state isDraft
      authorAssociation additions deletions mergeable
      labels(first: 20) { nodes { name } }
      reviews(last: 20) { nodes { authorAssociation submittedAt } }
      comments(last: 30) { nodes { authorAssociation createdAt } }
    }
  }
  fieldValues(first: 30) {
    nodes {
      ... on ProjectV2ItemFieldNumberValue {
        number
        field { ... on ProjectV2FieldCommon { name } }
      }
    }
  }
}
"""

# First request: fetches project metadata and first items page together.
_INITIAL_QUERY = """
query($org: String!, $number: Int!, $first: Int!) {
  organization(login: $org) {
    projectV2(number: $number) {
      id
      fields(first: 50) {
        nodes {
          ... on ProjectV2Field { id name dataType }
        }
      }
      items(first: $first) {
        pageInfo { hasNextPage endCursor }
        nodes { ...PrItem }
      }
    }
  }
}
""" + _PR_ITEM_FRAGMENT

# Subsequent requests: items only, keyed by project node ID.
_ITEMS_PAGE_QUERY = """
query($projectId: ID!, $cursor: String!, $first: Int!) {
  node(id: $projectId) {
    ... on ProjectV2 {
      items(first: $first, after: $cursor) {
        pageInfo { hasNextPage endCursor }
        nodes { ...PrItem }
      }
    }
  }
}
""" + _PR_ITEM_FRAGMENT


class ProjectItem(NamedTuple):
    """A project board row holding a pull request that needs scoring or clearing."""

    item_id: str
    pr: dict
    stored_scores: dict[str, float]
    clear_only: bool


def _stored_scores(item: dict) -> dict[str, float]:
    """Return the number-field values already on the board, keyed by field name."""
    values: dict[str, float] = {}
    for node in item.get("fieldValues", {}).get("nodes", []):
        name = (node.get("field") or {}).get("name")
        if name is not None and node.get("number") is not None:
            values[name] = float(node["number"])
    return values


def _filter_pr_item(item: dict, skip_labels: set[str], open_only: bool) -> ProjectItem | None:
    """Return the ProjectItem if it passes all filters, else None.

    Always skips non-PR items and drafts. A PR is excluded from scoring when it is
    no longer open or carries a skip label. Excluded PRs are dropped outright in
    report mode (open_only), and in write mode they are kept only while a non-zero
    score is still on the board, so the stale value can be cleared exactly once.
    """
    content = item.get("content") or {}
    if content.get("__typename") != "PullRequest":
        return None
    if content.get("isDraft"):
        return None

    stored = _stored_scores(item)
    pr_labels = {node["name"] for node in content.get("labels", {}).get("nodes", [])}
    excluded = content.get("state") != "OPEN" or bool(pr_labels & skip_labels)

    if excluded:
        if open_only:
            return None
        if not any(stored.get(field) for field in (MEETING_FIELD, MERGEABILITY_FIELD)):
            return None

    return ProjectItem(item_id=item["id"], pr=content, stored_scores=stored, clear_only=excluded)


def _fetch_items_page(project_id: str, cursor: str, page_size: int) -> tuple[dict, int]:
    """Fetch one page of project items, halving the page size on repeated timeouts.

    Returns (page, page_size). The reduced size is handed back to the caller so the
    remaining pages use it too, instead of paying the full timeout cost every time.
    """
    size = page_size
    while True:
        try:
            data = graphql(
                _ITEMS_PAGE_QUERY,
                {"projectId": project_id, "cursor": cursor, "first": size},
            )
            return data["node"]["items"], size
        except TransientGraphQLError:
            if size <= _MIN_ITEMS_PAGE_SIZE:
                raise
            size = max(size // 2, _MIN_ITEMS_PAGE_SIZE)
            log.warning("Reducing items page size to %d after repeated timeouts", size)


def fetch_project_data(
    project_number: int, skip_labels: set[str], open_only: bool
) -> tuple[str, dict[str, str | None], list[ProjectItem]]:
    """Return (project_id, field_ids, pr_items) fetched in the minimum number of calls.

    The first call fetches project metadata (id + field ids) and the first items page
    together. Additional calls fetch only the remaining pages. Field IDs are None for
    fields not found in the project. When open_only is False, closed, merged and
    skip-labelled PRs are included only when they still carry a non-zero score to
    clear; the rest are dropped so no pointless writes are issued.
    """
    data = graphql(
        _INITIAL_QUERY,
        {"org": REPO_OWNER, "number": project_number, "first": _ITEMS_PAGE_SIZE},
    )
    project = data["organization"]["projectV2"]
    if project is None:
        raise RuntimeError(
            f"Project #{project_number} not found in organization '{REPO_OWNER}'. "
            f"Verify the project number and that the token has 'read:project' scope."
        )

    project_id = project["id"]

    field_ids: dict[str, str | None] = {MEETING_FIELD: None, MERGEABILITY_FIELD: None}
    for field in project["fields"]["nodes"]:
        name = field.get("name")
        if name in field_ids:
            if field.get("dataType") != "NUMBER":
                raise RuntimeError(
                    f"Project field '{name}' has data type {field.get('dataType')}, "
                    f"expected NUMBER. Recreate it as a Number field in the project UI."
                )
            field_ids[name] = field["id"]

    pr_items: list[ProjectItem] = []

    def _collect_page(page: dict) -> None:
        for item in page["nodes"]:
            result = _filter_pr_item(item, skip_labels, open_only)
            if result is not None:
                pr_items.append(result)

    page = project["items"]
    _collect_page(page)

    page_size = _ITEMS_PAGE_SIZE
    while page["pageInfo"]["hasNextPage"]:
        cursor = page["pageInfo"]["endCursor"]
        page, page_size = _fetch_items_page(project_id, cursor, page_size)
        _collect_page(page)

    return project_id, field_ids, pr_items


# ---------------------------------------------------------------------------
# Activity and label helpers
# ---------------------------------------------------------------------------


def _parse_dt(timestamp: str) -> datetime:
    return datetime.fromisoformat(timestamp.rstrip("Z")).replace(tzinfo=timezone.utc)


def _has_write_access(association: str | None) -> bool:
    return association in WRITE_ACCESS_ASSOCIATIONS


def find_last_write_activity(pr_data: dict) -> datetime:
    """Return the datetime of the most recent review or comment by a write-access user.

    Determined via authorAssociation (OWNER, MEMBER, COLLABORATOR), so no extra API
    calls are needed. Only the most recent reviews and comments are fetched, so a
    maintainer comment buried under a very long author-only thread is not seen; in
    that case the PR creation date is used, which errs towards flagging the PR.
    """
    candidates: list[datetime] = []

    for review in pr_data["reviews"]["nodes"]:
        if _has_write_access(review.get("authorAssociation")) and review.get("submittedAt"):
            candidates.append(_parse_dt(review["submittedAt"]))

    for comment in pr_data["comments"]["nodes"]:
        if _has_write_access(comment.get("authorAssociation")) and comment.get("createdAt"):
            candidates.append(_parse_dt(comment["createdAt"]))

    return max(candidates) if candidates else _parse_dt(pr_data["createdAt"])


def parse_pr_labels(pr_data: dict) -> tuple[int, int, bool]:
    """Return (approved_count, requires_count, has_change_requested) from PR labels."""
    labels = {node["name"] for node in pr_data.get("labels", {}).get("nodes", [])}
    approved_count = sum(1 for label in labels if label.startswith("Approved: "))
    requires_count = sum(1 for label in labels if label.startswith("Requires: "))
    has_change_requested = any(label.startswith("Requested changes: ") for label in labels)
    return approved_count, requires_count, has_change_requested


# ---------------------------------------------------------------------------
# Field value update
# ---------------------------------------------------------------------------

_UPDATE_ONE_MUTATION = """
mutation($projectId: ID!, $itemId: ID!, $fieldId: ID!, $value: ProjectV2FieldValue!) {
  updateProjectV2ItemFieldValue(
    input: { projectId: $projectId, itemId: $itemId, fieldId: $fieldId, value: $value }
  ) { projectV2Item { id } }
}
"""

_UPDATE_BOTH_MUTATION = """
mutation(
  $projectId: ID!, $itemId: ID!,
  $fieldId1: ID!, $value1: ProjectV2FieldValue!,
  $fieldId2: ID!, $value2: ProjectV2FieldValue!
) {
  meeting: updateProjectV2ItemFieldValue(
    input: { projectId: $projectId, itemId: $itemId, fieldId: $fieldId1, value: $value1 }
  ) { projectV2Item { id } }
  mergeability: updateProjectV2ItemFieldValue(
    input: { projectId: $projectId, itemId: $itemId, fieldId: $fieldId2, value: $value2 }
  ) { projectV2Item { id } }
}
"""


def update_scores(
    project_id: str,
    item_id: str,
    meeting_field_id: str | None,
    mergeability_field_id: str | None,
    meeting_score: float,
    mergeability_score: float,
) -> None:
    """Write both scores in a single API call when both fields exist, one call otherwise."""
    if meeting_field_id and mergeability_field_id:
        graphql(
            _UPDATE_BOTH_MUTATION,
            {
                "projectId": project_id,
                "itemId": item_id,
                "fieldId1": meeting_field_id,
                "value1": {"number": meeting_score},
                "fieldId2": mergeability_field_id,
                "value2": {"number": mergeability_score},
            },
        )
    else:
        if meeting_field_id:
            graphql(
                _UPDATE_ONE_MUTATION,
                {
                    "projectId": project_id,
                    "itemId": item_id,
                    "fieldId": meeting_field_id,
                    "value": {"number": meeting_score},
                },
            )
        if mergeability_field_id:
            graphql(
                _UPDATE_ONE_MUTATION,
                {
                    "projectId": project_id,
                    "itemId": item_id,
                    "fieldId": mergeability_field_id,
                    "value": {"number": mergeability_score},
                },
            )


# ---------------------------------------------------------------------------
# Output helpers
# ---------------------------------------------------------------------------


def _scores_unchanged(result: dict) -> bool:
    """True when the board already holds both freshly computed scores for this item.

    Compared with a tolerance below the two-decimal rounding of the scores, so a
    float round-trip through the API never triggers a pointless write.
    """
    stored = result["stored_scores"]
    for field, value in (
        (MEETING_FIELD, result["meeting_score"]),
        (MERGEABILITY_FIELD, result["mergeability_score"]),
    ):
        current = stored.get(field)
        if current is None or not math.isclose(current, value, abs_tol=0.005):
            return False
    return True


def _print_factors(label: str, score: float, factors: dict[str, float]) -> None:
    print(f"  {label} = {score:.2f}")
    for name, value in factors.items():
        if value != 0.0:
            print(f"    {name:<25} {value:>+8.2f}")


def _factor_columns(results: list[dict], key: str) -> list[str]:
    """Return every factor name seen under key, in first-seen order.

    Cleared PRs carry an empty factor set, so the column list has to be the union
    over all rows rather than whichever row happens to sort first.
    """
    columns: dict[str, None] = {}
    for result in results:
        columns.update(dict.fromkeys(result[key]))
    return list(columns)


def _write_csv(path: str, results: list[dict], show_all: bool) -> None:
    """Write results to a CSV file. Includes factor columns when show_all is True."""
    if not results:
        log.warning("No results to write to %s", path)
        return

    base_columns = ["pr_number", "pr_title", "meeting_score", "mergeability_score"]
    meeting_keys: list[str] = []
    mergeability_keys: list[str] = []
    factor_columns: list[str] = []

    if show_all:
        meeting_keys = _factor_columns(results, "meeting_factors")
        mergeability_keys = _factor_columns(results, "mergeability_factors")
        factor_columns = [f"meeting_{key}" for key in meeting_keys] + [
            f"mergeability_{key}" for key in mergeability_keys
        ]

    with open(path, "w", newline="", encoding="utf-8") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=base_columns + factor_columns)
        writer.writeheader()
        for result in results:
            row: dict = {
                "pr_number": result["pr_number"],
                "pr_title": result["pr_title"],
                "meeting_score": result["meeting_score"],
                "mergeability_score": result["mergeability_score"],
            }
            if show_all:
                for key in meeting_keys:
                    row[f"meeting_{key}"] = result["meeting_factors"].get(key, 0.0)
                for key in mergeability_keys:
                    row[f"mergeability_{key}"] = result["mergeability_factors"].get(key, 0.0)
            writer.writerow(row)

    log.info("Results written to %s (%d rows)", path, len(results))


# ===========================================================================
# Main
# ===========================================================================


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--project", type=int, required=True, help="GitHub Project v2 number")
    parser.add_argument(
        "--dry-run", action="store_true", help="Print scores without updating the project"
    )
    parser.add_argument(
        "--all", action="store_true", dest="show_all", help="Show all scoring factors per PR"
    )
    parser.add_argument(
        "--sort",
        choices=["meeting", "mergeability"],
        default="meeting",
        help="Sort output by this metric (default: meeting)",
    )
    parser.add_argument(
        "--skip-label",
        metavar="LABEL",
        action="append",
        dest="skip_labels",
        default=[],
        help="Skip PRs carrying this label (repeatable). 'Status: Stale' and '✋ On hold' are always skipped.",
    )
    parser.add_argument(
        "-o",
        "--output",
        metavar="FILE",
        help="Write results to a CSV file (compatible with --all for factor columns)",
    )
    args = parser.parse_args()

    # --all prints factors instead of writing, so it is a report mode like --dry-run.
    report_only = args.dry_run or args.show_all

    skip_labels = {"Status: Stale", "✋ On hold"} | set(args.skip_labels)
    now = datetime.now(tz=timezone.utc)

    log.info("Fetching project data for project #%d...", args.project)
    project_id, field_ids, pr_items = fetch_project_data(
        args.project, skip_labels, open_only=report_only
    )

    meeting_field_id = field_ids[MEETING_FIELD]
    mergeability_field_id = field_ids[MERGEABILITY_FIELD]

    if not report_only:
        missing = [name for name, fid in field_ids.items() if fid is None]
        if missing:
            raise RuntimeError(
                f"Missing project fields: {', '.join(missing)}. "
                f"Create Number fields with those exact names in the GitHub Project UI."
            )
    else:
        for name, fid in field_ids.items():
            if fid is None:
                log.warning("Field '%s' not found, scores will be printed but not written.", name)

    results = []

    for item_id, pr_data, stored_scores, clear_only in pr_items:
        if clear_only:
            # PR was merged, closed or skip-labelled after being scored; clear the
            # stale values so it does not keep sitting at the top of the board.
            results.append(
                {
                    "item_id": item_id,
                    "stored_scores": stored_scores,
                    "pr_number": pr_data["number"],
                    "pr_title": pr_data["title"],
                    "meeting_score": 0.0,
                    "meeting_factors": {},
                    "mergeability_score": 0.0,
                    "mergeability_factors": {},
                }
            )
            continue

        created_at = _parse_dt(pr_data["createdAt"])
        last_activity = find_last_write_activity(pr_data)
        days_since = (now - last_activity).total_seconds() / 86400
        age_days = (now - created_at).total_seconds() / 86400

        approved_count, requires_count, has_change_requested = parse_pr_labels(pr_data)
        additions = pr_data.get("additions", 0)
        deletions = pr_data.get("deletions", 0)
        author_assoc = pr_data.get("authorAssociation", "NONE")
        mergeable = pr_data.get("mergeable", "UNKNOWN")

        meeting_score, meeting_factors = compute_meeting_priority(
            days_since_last_write_activity=days_since,
            pr_age_days=age_days,
            additions=additions,
            deletions=deletions,
            requires_label_count=requires_count,
            approved_label_count=approved_count,
            author_association=author_assoc,
            is_blocked=False,
        )

        mergeability_score, mergeability_factors = compute_mergeability_score(
            approved_label_count=approved_count,
            requires_label_count=requires_count,
            mergeable=mergeable,
            additions=additions,
            deletions=deletions,
            has_change_requested=has_change_requested,
            author_association=author_assoc,
            pr_age_days=age_days,
        )

        results.append(
            {
                "item_id": item_id,
                "stored_scores": stored_scores,
                "pr_number": pr_data["number"],
                "pr_title": pr_data["title"],
                "meeting_score": meeting_score,
                "meeting_factors": meeting_factors,
                "mergeability_score": mergeability_score,
                "mergeability_factors": mergeability_factors,
            }
        )

    sort_key = "meeting_score" if args.sort == "meeting" else "mergeability_score"
    results.sort(key=lambda result: result[sort_key], reverse=True)

    if args.output:
        _write_csv(args.output, results, show_all=args.show_all)

    if report_only:
        if args.show_all:
            for result in results:
                print(f"PR #{result['pr_number']}  {result['pr_title']!r}")
                _print_factors("meeting     ", result["meeting_score"], result["meeting_factors"])
                _print_factors(
                    "mergeability", result["mergeability_score"], result["mergeability_factors"]
                )
                print()
        else:
            print(f"{'PR':<10} {'meeting':>8}  {'mergeability':>13}  title")
            print("-" * 70)
            for result in results:
                title = result["pr_title"]
                title_display = title[:48] + "…" if len(title) > 49 else title
                print(
                    f"PR #{result['pr_number']:<7}"
                    f" {result['meeting_score']:>8.2f}"
                    f"  {result['mergeability_score']:>13.2f}"
                    f"  {title_display}"
                )
    else:
        updated = 0
        for result in results:
            log.info(
                "PR #%d  meeting=%.2f  mergeability=%.2f  %r",
                result["pr_number"],
                result["meeting_score"],
                result["mergeability_score"],
                result["pr_title"],
            )
            if _scores_unchanged(result):
                continue
            update_scores(
                project_id,
                result["item_id"],
                meeting_field_id,
                mergeability_field_id,
                result["meeting_score"],
                result["mergeability_score"],
            )
            updated += 1

        log.info("Done. Updated %d of %d PR(s).", updated, len(results))

    return 0


if __name__ == "__main__":
    sys.exit(main())
