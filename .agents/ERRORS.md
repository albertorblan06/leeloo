# Agent Errors and Lessons

> Record every error, bug, or failed approach here so future work can avoid repeating it. Append new entries; do not rewrite prior entries.

## 2026-10-13 — Incorrect GitHub owner in first repository-create attempt

- **Failed approach:** The first `gh repo create` command targeted `albertorblan/leeloo`.
- **Error:** GitHub returned `albertorblan06 cannot create a repository for albertorblan. (createRepository)`.
- **Resolution:** Confirmed the authenticated account and retried with the exact owner `albertorblan06`.
- **Lesson:** Before creating GitHub resources, use the exact authenticated username or organization login rather than a display name or shorthand.

## 2026-10-13 — GitHub CLI created an HTTPS remote by default

- **Failed approach:** `gh repo create --remote=origin` configured `origin` using HTTPS because the GitHub CLI Git protocol preference is HTTPS.
- **Resolution:** Explicitly changed the remote to `git@github.com:albertorblan06/leeloo.git`.
- **Lesson:** When SSH is required, verify the resulting remote URL and set it explicitly after `gh repo create`.
