import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock
from urllib import error, parse

SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "discord_updates.py"
SPEC = importlib.util.spec_from_file_location("discord_updates", SCRIPT)
sender = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(sender)

WEBHOOK = "https://discord.com/api/webhooks/123456789012345678/TEST_TOKEN"
THREAD = "987654321012345678"
TEST_PAYLOAD = {"embeds": [{"title": "Test", "description": "@everyone test"}], "allowed_mentions": {"parse": []}}


def merged_pr(**changes):
    pr = {
        "number": 7, "title": "Improve buyer meetings", "body": "Validated with two players.",
        "html_url": "https://github.com/BlakeStiner/Project-Axum/pull/7",
        "merged": True, "base": {"ref": "main"}, "labels": [],
        "head": {"ref": "codex/drug-trade-buyers", "repo": {"full_name": "BlakeStiner/Project-Axum"}},
    }
    pr.update(changes)
    return {"action": "closed", "repository": {"full_name": "BlakeStiner/Project-Axum"}, "pull_request": pr}


def branch_push(**changes):
    event = {
        "repository": {"full_name": "BlakeStiner/Project-Axum", "html_url": "https://github.com/BlakeStiner/Project-Axum"},
        "ref": "refs/heads/codex/drug-trade-foundation", "deleted": False,
        "before": "a" * 40, "after": "b" * 40,
        "commits": [{"message": "Add buyer appointments\n\nInternal implementation details."}],
        "head_commit": {"message": "Add buyer appointments\n\nInternal implementation details."},
    }
    event.update(changes)
    return event


class DiscordUpdatesTests(unittest.TestCase):
    def test_push_embed_links_exact_revision_and_does_not_claim_validation(self):
        payload = sender.build_payload("push", branch_push())
        embed = payload["embeds"][0]
        self.assertEqual(embed["title"], "Development Update • Add buyer appointments")
        self.assertIn("• Add buyer appointments", embed["description"])
        self.assertNotIn("Internal implementation", embed["description"])
        self.assertIn("Branch `codex/drug-trade-foundation`", embed["description"])
        self.assertIn("Commit [bbbbbbb]", embed["description"])
        self.assertTrue(embed["url"].endswith("/compare/" + "a" * 40 + "..." + "b" * 40))
        self.assertIn("In development", embed["description"])
        self.assertIn("Tests and deployment are not confirmed", embed["description"])
        self.assertNotIn("**Validation**", embed["description"])
        self.assertEqual(payload["allowed_mentions"], {"parse": []})

    def test_push_scope_excludes_deletions_unrelated_branches_and_tags(self):
        for changes in ({"deleted": True}, {"ref": "refs/heads/main"}, {"ref": "refs/heads/codex/terrain"}, {"ref": "refs/tags/codex/drug-trade-tag"}):
            with self.subTest(changes=changes):
                self.assertIsNone(sender.build_payload("push", branch_push(**changes)))
        for sha in ("0" * 40, "bad", ""):
            with self.subTest(sha=sha), self.assertRaises(sender.NotificationError):
                sender.build_payload("push", branch_push(after=sha))

    def test_push_is_one_bounded_embed_per_push_with_latest_five_subjects(self):
        commits = [{"message": f"Change {n} " + "😀" * 600} for n in range(20)]
        payload = sender.build_payload("push", branch_push(commits=commits, head_commit=commits[-1]))
        self.assertEqual(len(payload["embeds"]), 1)
        embed = payload["embeds"][0]
        self.assertIn("Latest five commits", embed["description"])
        self.assertNotIn("Change 14", embed["description"])
        self.assertIn("Change 19", embed["description"])
        self.assertLessEqual(len(embed["title"].encode("utf-16-le")) // 2, 256)
        self.assertLessEqual(len(embed["description"].encode("utf-16-le")) // 2, 4096)

    def test_creation_and_empty_or_forced_pushes_still_have_revision_links(self):
        for changes in ({"before": "0" * 40}, {"commits": [], "head_commit": None}, {"forced": True, "commits": []}):
            with self.subTest(changes=changes):
                embed = sender.build_payload("push", branch_push(**changes))["embeds"][0]
                self.assertIn("Commit [bbbbbbb]", embed["description"])
                self.assertIn("**GitHub**", embed["description"])
                if changes.get("forced"):
                    self.assertIn("force push", embed["description"])
                if changes.get("before") == "0" * 40:
                    self.assertTrue(embed["url"].endswith("/commit/" + "b" * 40))

    def test_only_merged_scoped_same_repository_prs_post(self):
        self.assertIn("Development Update • Improve buyer meetings", sender.build_payload("pull_request", merged_pr())["embeds"][0]["title"])
        self.assertIsNone(sender.build_payload("pull_request", merged_pr(merged=False)))
        unrelated = merged_pr()
        unrelated["pull_request"]["head"]["ref"] = "codex/terrain"
        self.assertIsNone(sender.build_payload("pull_request", unrelated))
        unrelated["pull_request"]["labels"] = [{"name": "area:drug-trade"}]
        self.assertIsNotNone(sender.build_payload("pull_request", unrelated))
        unrelated["pull_request"]["head"]["repo"]["full_name"] = "someone/fork"
        self.assertIsNone(sender.build_payload("pull_request", unrelated))

    def test_release_scope(self):
        event = {"action": "published", "release": {"tag_name": "v1.0", "html_url": "https://github.com/example/release"}}
        self.assertIsNone(sender.build_payload("release", event))
        event["release"]["tag_name"] = "drugtrade-v0.1"
        self.assertIn("drugtrade-v0.1", sender.build_payload("release", event)["embeds"][0]["title"])

    def test_long_unicode_embed_keeps_source_link_and_all_discord_limits(self):
        event = merged_pr(title="😀" * 1200, body="details" * 1000)
        payload = sender.build_payload("pull_request", event)
        self.assertNotIn("content", payload)
        self.assertEqual(len(payload["embeds"]), 1)
        embed = payload["embeds"][0]
        self.assertLessEqual(len(embed["title"].encode("utf-16-le")) // 2, 256)
        self.assertLessEqual(len(embed["description"].encode("utf-16-le")) // 2, 4096)
        self.assertEqual(embed["url"], event["pull_request"]["html_url"])
        combined = embed["title"] + embed["description"]
        self.assertLessEqual(len(combined.encode("utf-16-le")) // 2, 6000)

    def test_manual_update_has_branded_embed_and_suppresses_mentions(self):
        payload = sender.build_payload("workflow_dispatch", {"inputs": {"message": "@everyone **Update**"}})
        embed = payload["embeds"][0]
        self.assertTrue(embed["description"].startswith("@everyone **Update**"))
        self.assertIn("**Status** — In development", embed["description"])
        self.assertEqual(embed["title"], "Development Update • Drug Trade Framework")
        self.assertEqual(embed["color"], 0x5865F2)
        self.assertNotIn("author", embed)
        self.assertNotIn("footer", embed)
        self.assertEqual(payload["allowed_mentions"], {"parse": []})

    def test_pr_embed_has_truthful_status_and_nonempty_body(self):
        event = merged_pr(body="   ")
        embed = sender.build_payload("pull_request", event)["embeds"][0]
        self.assertEqual(embed["color"], 0x2E8B57)
        self.assertIn("See the pull request", embed["description"])
        self.assertIn("Merged into `main`", embed["description"])

    def test_cad_style_card_separates_summary_github_and_validation(self):
        event = merged_pr(body="## Summary\nPlayers can arrange buyer meetings.\n\n## Validation\nTwo-client test passed.\n\n## Internal notes\nDo not dump this paragraph.")
        event["pull_request"]["head"]["sha"] = "a" * 40
        embed = sender.build_payload("pull_request", event)["embeds"][0]
        description = embed["description"]
        self.assertTrue(description.startswith("Players can arrange buyer meetings."))
        self.assertIn("Branch `codex/drug-trade-buyers`", description)
        self.assertIn("Commit [aaaaaaa]", description)
        self.assertIn("[Open pull request]", description)
        self.assertIn("**Validation** — Two-client test passed.", description)
        self.assertNotIn("Do not dump", description)

    def test_manual_feature_and_status_are_explicit_and_commit_is_validated(self):
        inputs = {"title": "Property Processing", "message": "Machine placement is ready.", "status": "Ready for testing", "validation": "Placement boundary checks passed.", "branch": "feature/processing", "commit": "abc1234"}
        embed = sender.build_payload("workflow_dispatch", {"inputs": inputs})["embeds"][0]
        self.assertEqual(embed["title"], "Development Update • Property Processing")
        self.assertIn("Commit [abc1234]", embed["description"])
        self.assertIn("Ready for testing", embed["description"])
        self.assertIn("**Validation**", embed["description"])
        inputs["commit"] = "not-a-sha"
        with self.assertRaises(sender.NotificationError):
            sender.build_payload("workflow_dispatch", {"inputs": inputs})

    def test_plain_validation_paragraph_and_first_summary_paragraph(self):
        event = merged_pr(body="First summary paragraph.\n\nImplementation details.\n\nValidation: Ten checks passed.")
        description = sender.build_payload("pull_request", event)["embeds"][0]["description"]
        self.assertTrue(description.startswith("First summary paragraph."))
        self.assertNotIn("Implementation details.", description)
        self.assertIn("**Validation** — Ten checks passed.", description)

    def test_url_requires_discord_and_sets_exact_thread_with_confirmation(self):
        result = parse.urlsplit(sender.webhook_target(WEBHOOK + "?thread_id=wrong", THREAD))
        self.assertEqual(parse.parse_qs(result.query), {"thread_id": [THREAD], "wait": ["true"]})
        for bad in ("https://example.com/api/webhooks/123/token", WEBHOOK + "/github", "http://discord.com/api/webhooks/123/token", ""):
            with self.subTest(url=bad), self.assertRaises(sender.NotificationError):
                sender.webhook_target(bad, THREAD)
        with self.assertRaises(sender.NotificationError):
            sender.webhook_target(WEBHOOK, "not-a-thread-id")

    @mock.patch.object(sender.request, "build_opener")
    def test_post_disables_mentions_and_checks_thread_confirmation(self, build_opener):
        response = io.BytesIO(json.dumps({"id": "555", "channel_id": THREAD}).encode())
        build_opener.return_value.open.return_value = response
        self.assertEqual(sender.publish(WEBHOOK, THREAD, TEST_PAYLOAD), "555")
        req = build_opener.return_value.open.call_args.args[0]
        self.assertEqual(json.loads(req.data)["allowed_mentions"], {"parse": []})
        self.assertEqual(json.loads(req.data)["embeds"], TEST_PAYLOAD["embeds"])
        self.assertNotIn("content", json.loads(req.data))
        self.assertIn("thread_id=" + THREAD, req.full_url)
        build_opener.return_value.open.return_value = io.BytesIO(b'{"id":"555","channel_id":"wrong"}')
        with self.assertRaises(sender.NotificationError):
            sender.publish(WEBHOOK, THREAD, TEST_PAYLOAD)

    @mock.patch.object(sender.time, "sleep")
    @mock.patch.object(sender.request, "build_opener")
    def test_rate_limit_retries_but_ambiguous_network_failure_does_not(self, build_opener, sleep):
        limited = error.HTTPError(WEBHOOK, 429, "rate limit", {}, io.BytesIO(b'{"retry_after":0.01}'))
        build_opener.return_value.open.side_effect = [limited, io.BytesIO(json.dumps({"id": "555", "channel_id": THREAD}).encode())]
        self.assertEqual(sender.publish(WEBHOOK, THREAD, TEST_PAYLOAD), "555")
        sleep.assert_called_once_with(0.01)
        build_opener.return_value.open.reset_mock()
        build_opener.return_value.open.side_effect = error.URLError(WEBHOOK)
        with self.assertRaises(sender.NotificationError) as caught:
            sender.publish(WEBHOOK, THREAD, TEST_PAYLOAD)
        self.assertNotIn("TEST_TOKEN", str(caught.exception))
        self.assertEqual(build_opener.return_value.open.call_count, 1)

    @mock.patch.object(sender.request, "build_opener")
    def test_http_error_does_not_expose_webhook_or_response_body(self, build_opener):
        build_opener.return_value.open.side_effect = error.HTTPError(WEBHOOK, 404, WEBHOOK, {}, io.BytesIO(WEBHOOK.encode()))
        with self.assertRaises(sender.NotificationError) as caught:
            sender.publish(WEBHOOK, THREAD, TEST_PAYLOAD)
        self.assertNotIn("TEST_TOKEN", str(caught.exception))
        self.assertIn("HTTP 404", str(caught.exception))

    @mock.patch.object(sender, "publish")
    def test_manual_preview_needs_no_secrets_and_never_posts(self, publish):
        with tempfile.TemporaryDirectory() as folder:
            event_path = Path(folder) / "event.json"
            event_path.write_text(json.dumps({"inputs": {"message": "Test preview", "dry_run": "true"}}))
            with mock.patch.dict(sender.os.environ, {"GITHUB_EVENT_PATH": str(event_path), "GITHUB_EVENT_NAME": "workflow_dispatch"}, clear=True):
                with mock.patch("sys.stdout", new_callable=io.StringIO) as stdout:
                    self.assertEqual(sender.main(), 0)
                    self.assertIn("Test preview", stdout.getvalue())
        publish.assert_not_called()


if __name__ == "__main__":
    unittest.main()
