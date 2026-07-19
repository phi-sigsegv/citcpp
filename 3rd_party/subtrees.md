# Third-Party Dependencies (Git Subtrees)

This repository manages its third-party dependencies using **`git subtree`** where feasible. This approach ensures
that the source code of the dependencies is physically included in the repository for reliable, offline-capable builds,
while tracking upstream repositories cleanly without the historical bloat.

## Subtree Tracking Directory

All subtrees are located within the `3rd_party/` directory.

| Dependency | Upstream Repository | Tracked Version / Tag | Local Path |
| :--- | :--- | :--- | :--- |
| **Lace** | [github.com/trolando/lace](https://github.com/trolando/lace.git) | `v1.6.0` | `3rd_party/lace` |
| **Sylvan** | [github.com/trolando/sylvan](https://github.com/trolando/sylvan.git) | `v1.10.0` | `3rd_party/sylvan` |

---

## Cheat Sheet: Managing Subtrees

Always use the `--squash` flag when adding or pulling subtrees to keep the main repository history clean.

### 1. Adding a New Subtree
To add a new dependency to the project:
```bash
git subtree add --prefix 3rd_party/<library-name> <repository-url> <branch-or-tag> --squash
```

### 2. Updating an Existing Subtree
To pull latest updates or a new version tag from upstream:
```bash
git subtree pull --prefix 3rd_party/<library-name> <repository-url> <branch-or-tag> --squash
```

### 3. Contributing Changes Back Upstream (Optional)
If you make a local bugfix within the subtree directory and want to push it back to the original library:
```bash
git subtree push --prefix 3rd_party/<library-name> <repository-url> <target-branch>
```
