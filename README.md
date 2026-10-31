<div align="center">

# lithub

**A small, from-scratch implementation of Git in modern C++.**

[![C++ Build](https://github.com/vitasuuk/lithub/actions/workflows/ci.yml/badge.svg)](https://github.com/vitasuuk/lithub/actions/workflows/ci.yml)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)
![CMake](https://img.shields.io/badge/build-CMake-064F8C?logo=cmake)

</div>

---

lithub rebuilds Git's core from the ground up: content-addressed objects,
trees, commits, refs and the staging area. It follows the excellent
[Write Yourself a Git](https://wyag.thb.lt/) guide, translated from Python
into C++17.

It uses Git's own on-disk formats (SHA-1 hashed, zlib compressed objects),
but keeps everything in a `.lit` directory instead of `.git`, so it never
touches your real repositories.

## Quick look

```console
$ lithub init .
$ echo "hello" > hello.txt
$ lithub add hello.txt

$ lithub status
On branch master.
Changes to be committed:
  added:    hello.txt

Changes not staged for commit:

Untracked files:

$ lithub commit -m "First commit"
$ lithub log HEAD
commit 3f9a1c0d...
Author: Your Name <you@example.com>
Date:   Fri Oct 30 22:07:56 2026 -0700

	First commit
```

> [!NOTE]
> `commit` takes your name and email from Git's global config
> (`~/.gitconfig`), so set `user.name` and `user.email` there first.

## Commands

| Area | Command | What it does |
|------|---------|--------------|
| **Repository** | `init [dir]` | Create a new `.lit` repository |
| **Objects** | `hash-object [-w] [-t type] <file>` | Hash a file, optionally store it as a blob |
| | `cat-file <type> <object>` | Print an object's raw content |
| | `ls-tree [-r] <tree>` | List a tree's contents |
| | `checkout <commit> <dir>` | Write a commit's files into an empty directory |
| **History & refs** | `log <commit>` | Show commit history |
| | `show-ref [dir]` | List references (all of `refs/` by default) |
| | `tag [-a] [name] [object]` | List tags, or create one |
| | `rev-parse [--lithub-type type] <name>` | Resolve a name to an object hash |
| **Staging** | `ls-files [--verbose]` | List files in the index |
| | `status` | Show staged, unstaged and untracked changes |
| | `add <path>...` | Stage files |
| | `rm <path>...` | Unstage **and delete** files |
| | `commit -m <message>` | Record staged changes as a new commit |

Names like `HEAD`, branch names, tag names and short hashes (4+ characters)
work anywhere an object is expected.

## Building

You'll need a C++17 compiler, CMake 3.16+, OpenSSL and zlib.
On Ubuntu/Debian:

```bash
sudo apt install build-essential cmake libssl-dev zlib1g-dev
```

Then:

```bash
cmake -S . -B build
cmake --build build
./build/lithub --help
```

## Project layout

```
src/
├── main.cc          command-line parsing
├── commands/        one function per lithub command
├── objects/         blob, tree, commit, tag + object store (hash, compress, read/write)
├── repository/      .lit layout, config, refs
└── index/           the staging area's binary index file
external/
└── argparse/        command-line parser library
```

## Not there yet

- `.gitignore` rules: `status` lists every untracked file
- Branch creation and merging
- Packfiles and remotes (no `clone`, `fetch` or `push`)

## Credits

- [Write Yourself a Git](https://wyag.thb.lt/) by Thibault Polge, the guide this project follows
- [argparse](https://github.com/p-ranav/argparse) by Pranav Srinivas Kumar, used for command-line parsing (MIT)
