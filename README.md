# lsc

An `ls`-inspired command written in C as a learning project. Implements a custom 
subset of features with colored output — not a 1:1 clone.

## Features

- Color-coded output by file type (directories, symlinks, executables, devices)
- Long format (`-l`) with permissions, size, owner, and date
- Human-readable file sizes (`-h`)
- Sorting by name (default), time (`-t`), or size (`-s`)
- Reverse order (`-r`)
- Show hidden files (`-a`)
- Show hidden files except `.` and `..` (`-A`)
- Relative and absolute path support

## Build

```bash
make        # release build
make debug  # debug   build
make clean  # remove  binaries
```

## Usage

```bash
./bin/release/lsc [OPTIONS] [PATH]
```

## Options

| Short | Long               | Description                           |
|-------|--------------------|---------------------------------------|
| `-l`  | `--long`           | Long format                           |
| `-a`  | `--all`            | Show hidden files                     |
| `-A`  | `--almost-all`     | Show hidden files except `.` and `..` |
| `-h`  | `--human-readable` | Human-readable sizes                  |
| `-t`  | `--sort-by-time`   | Sort by time                          |
| `-s`  | `--sort-by-size`   | Sort by size                          |
| `-r`  | `--reverse`        | Reverse order                         |

## Examples

```bash
./bin/release/lsc
./bin/release/lsc -la /etc
./bin/release/lsc ~ -lAhts
./bin/release/lsc --long --all --human-readable
./bin/release/lsc /etc --almost-all --long -tr # we can also combine short and long options
```

## What I learned

- Linux syscalls: `opendir`, `readdir`, `lstat`, `getpwuid`, `ioctl`
- Dynamic arrays with `realloc`
- ANSI terminal escape codes
- Function pointers for sorting