# Third-Party Dependencies

This repository manages its third-party dependencies using **`git subtree`** where meaningful. This approach ensures
that the source code of the dependencies is physically included in the repository for reliable, offline-capable builds,
while tracking upstream repositories cleanly without the historical bloat.

Where it is not meaningful to use subtrees, contents of release tarballs of the third-party dependencies
are tracked manually.

## Third-party dependencies tracked by subtrees

All subtrees are located within the `3rd_party/` directory.

| Dependency | Upstream Repository | Tracked Version / Tag | Local Path |
| :--- | :--- | :--- | :--- |
| **Lace** | [github.com/trolando/lace](https://github.com/trolando/lace.git) | `v1.6.0` | `3rd_party/lace` |
| **Sylvan** | [github.com/trolando/sylvan](https://github.com/trolando/sylvan.git) | `v1.10.0` | `3rd_party/sylvan` |

---

### Cheat Sheet: Managing Subtrees

Always use the `--squash` flag when adding or pulling subtrees to keep the main repository history clean.

#### 1. Adding a New Subtree
To add a new dependency to the project:
```bash
git subtree add --prefix 3rd_party/<library-name> <repository-url> <branch-or-tag> --squash
```

#### 2. Updating an Existing Subtree
To pull latest updates or a new version tag from upstream:
```bash
git subtree pull --prefix 3rd_party/<library-name> <repository-url> <branch-or-tag> --squash
```

#### 3. Contributing Changes Back Upstream (Optional)
If you make a local bugfix within the subtree directory and want to push it back to the original library:
```bash
git subtree push --prefix 3rd_party/<library-name> <repository-url> <target-branch>
```

## Third-party dependencies tracked manually

All such third-party dependencies are located within the `3rd_party/` directory.

| Dependency | Upstream Repository | Tracked Version / Tag | Local Path |
| :--- | :--- | :--- | :--- |
| **CLI11** | [github.com/CLIUtils/CLI11](https://github.com/CLIUtils/CLI11.git) | `v2.6.2` | `3rd_party/CLI11` |
| **cpp-peglib** | [github.com/yhirose/cpp-peglib](https://github.com/yhirose/cpp-peglib.git) | `v1.15.0` | `3rd_party/cpp-peglib` |
| **doctest** | [github.com/doctest/doctest](https://github.com/doctest/doctest.git) | `v2.4.12` | `3rd_party/doctest` |
