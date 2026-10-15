"""Publish scoped GitHub updates to a configured Discord thread (stdlib only)."""

import json
import os
from pathlib import Path
import re
import sys
import time
from urllib import error, parse, request


class NotificationError(Exception):
    pass


def build_message(event_name, event):
    if event_name == "workflow_dispatch":
        message = str(event.get("inputs", {}).get("message", "")).strip()
        if not message:
            raise NotificationError("Enter an update message before running the workflow.")
        heading, body, link = "Development update", message, ""
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
        body = (pr.get("body") or "See the pull request for the change details.").strip()
        link = pr["html_url"]
    elif event_name == "release":
        release = event.get("release", {})
        if event.get("action") != "published" or not release.get("tag_name", "").startswith("drugtrade-"):
            return None
        heading = f"Release: {release.get('name') or release['tag_name']}"
        body = (release.get("body") or "See the release for details.").strip()
        link = release["html_url"]
    else:
        return None

    # Reserve room for the source link; measure UTF-16 units conservatively for emoji.
    prefix = "**Project Axum | Drug Trade**\n"
    suffix = f"\n\n{link}" if link else ""
    text = f"{heading}\n\n{body}"
    budget = 1900 - len((prefix + suffix).encode("utf-16-le")) // 2
    if budget < 10:
        raise NotificationError("The source link is too long to publish safely.")
    encoded = text.encode("utf-16-le")
    if len(encoded) // 2 > budget:
        text = encoded[: (budget - 1) * 2].decode("utf-16-le", errors="ignore") + "…"
    return prefix + text + suffix


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


def publish(webhook, thread_id, message):
    target = webhook_target(webhook.strip(), thread_id.strip())
    payload = json.dumps({
        "content": message,
        "allowed_mentions": {"parse": []},
    }).encode("utf-8")
    opener = request.build_opener(NoRedirects())
    for attempt in range(3):
        req = request.Request(target, data=payload, headers={
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
                400: "Check that the thread ID is valid and belongs to the webhook's parent channel.",
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
        message = build_message(os.environ.get("GITHUB_EVENT_NAME", ""), event)
        if message is None:
            print("No drug-trade update for this event.")
            return 0
        dry_run = str(event.get("inputs", {}).get("dry_run", "false")).lower() == "true"
        if dry_run:
            print("Preview only; no Discord request was made.\n" + message)
            return 0
        if int(os.environ.get("GITHUB_RUN_ATTEMPT", "1")) > 1:
            raise NotificationError("This run has already been attempted. Check Discord, then start a fresh manual update if needed.")
        message_id = publish(
            os.environ.get("DISCORD_UPDATES_WEBHOOK_URL", ""),
            os.environ.get("DISCORD_DRUGTRADEFRAMEWORK_THREAD_ID", ""),
            message,
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
