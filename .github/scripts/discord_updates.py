"""Publish scoped GitHub updates to a configured Discord thread (stdlib only)."""

import json
from datetime import datetime, timezone
import os
from pathlib import Path
import re
import sys
import time
from urllib import error, parse, request


class NotificationError(Exception):
    pass


def clip_text(text, limit):
    """Respect Discord text limits without splitting a supplementary character."""
    encoded = text.encode("utf-16-le")
    if len(encoded) // 2 <= limit:
        return text
    return encoded[: (limit - 1) * 2].decode("utf-16-le", errors="ignore") + "…"


def build_payload(event_name, event):
    timestamp = None
    if event_name == "workflow_dispatch":
        message = str(event.get("inputs", {}).get("message", "")).strip()
        if not message:
            raise NotificationError("Enter an update message before running the workflow.")
        heading, body, link = "Development update", message, ""
        color, category = 0xC7A34D, "Progress update"
        timestamp = datetime.now(timezone.utc).isoformat()
    elif event_name == "pull_request":
        pr = event.get("pull_request", {})
        labels = {item.get("name") for item in pr.get("labels", [])}
        head = pr.get("head", {})
        if not (
            event.get("action") == "closed"
            and pr.get("merged") is True
            and pr.get("base", {}).get("ref") == "main"
            and head.get("repo", {}).get("full_name") == event.get("repository", {}).get("full_name")
            and ("area:drug-trade" in labels or head.get("ref", "").startswith("codex/drug-trade-"))
        ):
            return None
        heading = f"Merged #{pr['number']}: {pr['title']}"
        body = (pr.get("body") or "").strip() or "See the pull request for the change details."
        link = pr["html_url"]
        color, category = 0x2E8B57, "Merged pull request"
        timestamp = pr.get("merged_at")
    elif event_name == "release":
        release = event.get("release", {})
        if event.get("action") != "published" or not release.get("tag_name", "").startswith("drugtrade-"):
            return None
        heading = f"Release: {release.get('name') or release['tag_name']}"
        body = (release.get("body") or "").strip() or "See the release for details."
        link = release["html_url"]
        color, category = 0x3498DB, "Published release"
        timestamp = release.get("published_at")
    else:
        return None

    # One embed stays below the 6000-character aggregate limit even at both caps.
    embed = {
        "author": {"name": "Project Axum | Drug Trade"},
        "title": clip_text(heading, 256),
        "description": clip_text(body, 4096),
        "color": color,
        "footer": {"text": f"Axum Development • {category}"},
    }
    if link:
        embed["url"] = link
    if timestamp:
        embed["timestamp"] = timestamp
    return {"embeds": [embed], "allowed_mentions": {"parse": []}}


def webhook_target(webhook, thread_id):
    if not re.fullmatch(r"[0-9]{15,22}", thread_id):
        raise NotificationError("Set repository variable DISCORD_DRUGTRADEFRAMEWORK_THREAD_ID to the numeric thread ID.")
    if not webhook:
        raise NotificationError("Set repository secret DISCORD_UPDATES_WEBHOOK_URL to the parent-channel webhook URL.")
    try:
        url = parse.urlsplit(webhook)
        valid = (
            url.scheme == "https"
            and url.hostname in {"discord.com", "discordapp.com"}
            and url.port in {None, 443}
            and not url.username
            and not url.password
            and not url.fragment
            and re.fullmatch(r"/api(?:/v[0-9]+)?/webhooks/[0-9]+/[A-Za-z0-9_-]+", url.path)
        )
    except ValueError:
        valid = False
    if not valid:
        raise NotificationError("The webhook secret must contain a standard Discord webhook URL, without /github.")
    query = parse.urlencode({"thread_id": thread_id, "wait": "true"})
    return parse.urlunsplit((url.scheme, url.netloc, url.path, query, ""))


class NoRedirects(request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


def publish(webhook, thread_id, payload):
    target = webhook_target(webhook.strip(), thread_id.strip())
    encoded_payload = json.dumps(payload).encode("utf-8")
    opener = request.build_opener(NoRedirects())
    for attempt in range(3):
        req = request.Request(target, data=encoded_payload, headers={
            "Content-Type": "application/json",
            "User-Agent": "Project-Axum-Updates/1.0",
        }, method="POST")
        try:
            with opener.open(req, timeout=20) as response:
                result = json.load(response)
            if not result.get("id") or str(result.get("channel_id")) != thread_id.strip():
                raise NotificationError("Discord did not confirm a message in the configured thread. Check the thread before retrying.")
            return str(result["id"])
        except error.HTTPError as exc:
            if exc.code == 429 and attempt < 2:
                try:
                    delay = float(json.loads(exc.read()).get("retry_after", 1))
                except (ValueError, TypeError, AttributeError):
                    delay = 1
                if 0 <= delay <= 60:
                    time.sleep(delay)
                    continue
            hint = {
                400: "Check the thread/parent-channel match and Discord embed limits.",
                403: "Check channel/thread permissions and whether the thread is locked.",
                404: "Check the webhook URL and thread ID, including their parent-channel match.",
                429: "Discord is rate limiting updates. Wait before retrying.",
            }.get(exc.code, "Check Discord and the destination thread before retrying; delivery may be uncertain.")
            raise NotificationError(f"Discord returned HTTP {exc.code}. {hint}") from None
        except (error.URLError, TimeoutError, OSError, ValueError):
            # Never print request URLs, tokens, or remote response bodies.
            raise NotificationError("Could not confirm delivery. Check the Discord thread before retrying to avoid duplicate posts.") from None


def main():
    try:
        event = json.loads(Path(os.environ["GITHUB_EVENT_PATH"]).read_text(encoding="utf-8"))
        payload = build_payload(os.environ.get("GITHUB_EVENT_NAME", ""), event)
        if payload is None:
            print("No drug-trade update for this event.")
            return 0
        dry_run = str(event.get("inputs", {}).get("dry_run", "false")).lower() == "true"
        if dry_run:
            print("Preview only; no Discord request was made.\n" + json.dumps(payload, indent=2))
            return 0
        if int(os.environ.get("GITHUB_RUN_ATTEMPT", "1")) > 1:
            raise NotificationError("This run has already been attempted. Check Discord, then start a fresh manual update if needed.")
        message_id = publish(
            os.environ.get("DISCORD_UPDATES_WEBHOOK_URL", ""),
            os.environ.get("DISCORD_DRUGTRADEFRAMEWORK_THREAD_ID", ""),
            payload,
        )
        print(f"Discord confirmed message {message_id} in the configured thread.")
        return 0
    except (NotificationError, KeyError, OSError, ValueError) as exc:
        if isinstance(exc, NotificationError):
            print(str(exc), file=sys.stderr)
        else:
            print("The workflow event could not be read. Check its configuration.", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
