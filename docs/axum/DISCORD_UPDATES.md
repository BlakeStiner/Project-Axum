# Discord drug-trade updates

The workflow `Discord - Drug Trade Updates` posts through a parent-channel
webhook into an existing drug-trade thread. It does not create a bot or read
Discord discussions. This integration is separate from the gameplay addon.

## Repository settings

Under Settings > Secrets and variables > Actions configure:

| Kind | Exact name | Value |
|---|---|---|
| Repository secret | `DISCORD_UPDATES_WEBHOOK_URL` | Standard webhook URL for the thread's parent channel |
| Repository variable | `DISCORD_DRUGTRADEFRAMEWORK_THREAD_ID` | Numeric ID of the drug-trade thread |

The webhook and thread must belong to the same parent channel. Use the raw
webhook URL without `/github`; the sender supplies `thread_id` and `wait=true`.
No webhook URL or token belongs in Git, a PR body, screenshots, or chat.

## Activate and send the first test

1. Merge the integration PR into `main`. This does not merge the separate
   `codex/drug-trade-foundation` branch or publish any gameplay feature.
2. Open the repository's **Actions** tab. If Actions is disabled for this fork,
   enable workflows. Under Settings > Actions > General, repository policy
   must allow GitHub Actions and the official `actions/checkout` action.
3. Select **Discord - Drug Trade Updates**, then **Run workflow**.
4. Choose branch **main**, keep the default integration-test message, leave
   **Preview without sending to Discord** unchecked, and run it once.
5. Confirm the run succeeds and the message appears inside the intended thread.
   Success logs include a confirmed Discord message ID; they never include the
   webhook URL. A missing setting fails the run with its exact required name.

For a preview, check **Preview without sending to Discord**. It prints the
embed payload as JSON in the workflow log without using the webhook. A preview
does not verify Discord credentials, permissions, or thread routing.

## Embed appearance

Each new update is a single rich embed with the Project Axum / Drug Trade
author line, title, Markdown summary, event color, and Axum Development footer.
Progress updates use gold, merged PRs use green, and releases use blue. PR and
release titles link to their GitHub source. Timestamps use the merge/publication
time where available, or the current time for manual updates. The webhook's
configured avatar remains the posting identity.

Titles and descriptions are clipped independently to Discord's 256- and
4096-character limits, conservatively counting UTF-16 units for emoji. The
single embed stays below the combined 6000-character text limit. Longer notes
remain available through the source link. Existing Discord messages are not
edited; merge this change and run the manual workflow to send an embed test.

## Automatic routing

- A same-repository PR merged into `main` posts when its branch starts with
  `codex/drug-trade-` **or** it has label `area:drug-trade`. Put the human-readable
  change summary and validation in the PR body; these become the Discord update.
- Published releases post only when their tag starts with `drugtrade-`, for
  example `drugtrade-v0.1.0`. Release notes become the message body.
- A manual run posts the entered progress update, including design decisions
  or test results. The default message is explicitly an integration test.
- Other branches, unrelated releases, unmerged PRs, and fork PRs do not post.
  The initial setup PR itself uses `codex/discord-drugtrade-updates`, so merging
  it does not automatically announce itself in the drug-trade thread.

No automatic build-failure alerts, roadmap-tag synchronization, historical
backfill, or issue tracking is enabled by this first integration. Add those
destinations explicitly rather than sending every project event to one thread.

## Sender behavior

The workflow executes the sender from the trusted default branch; PR bodies
and manual messages are treated as data. Tests run separately without secrets.
Embeds suppress mentions, retain the GitHub source link, and truncate long
notes to fit Discord limits. No inventory or server credentials are involved.

The sender retries explicit rate-limit responses, up to three attempts, with a
bounded delay. It does not retry ambiguous network failures automatically.
Reruns of the same Actions run are blocked from posting; after a failure, inspect
the Discord thread before starting a fresh manual run. There is no persistent
cross-run delivery ledger, so a separately dispatched run can post the same
text again. Never interpret posting success as gameplay validation.

HTTP 400/404 usually requires checking IDs and the webhook's parent channel;
403 requires checking permissions or a locked thread. If a webhook URL is
exposed, regenerate it in Discord and replace the repository secret.

Local checks (Python 3.10+; no network or credentials):

```powershell
python -m unittest discover -s .github/tests -v
```

References: [Discord webhook API](https://docs.discord.com/developers/resources/webhook#execute-webhook),
[GitHub workflow events](https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows),
and [GitHub Actions secrets](https://docs.github.com/en/actions/how-tos/write-workflows/choose-what-workflows-do/use-secrets).
