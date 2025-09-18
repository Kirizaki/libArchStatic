# libArchStatic

## About

`libArchStatic` is a cross-platform C++ tool to **pack and unpack directory trees** into `.tar.gz` archives, preserving:

- Regular files
- Directories (nested)
- Symlinks (working, broken, relative, and pointing outside)
- File permissions, timestamps, and sparse files (where supported)

The repository also includes Python scripts to **generate diverse test cases** and **verify unpacked directories**.

---

## Repository Structure

| File / Directory             | Description |
|-------------------------------|------------|
| `main.cpp`                   | Main C++ implementation of packing/unpacking using `libarchive`. |
| `build.sh`                   | Builds the project using CMake. |
| `run.sh`                     | Runs a full workflow: rebuild, generate test cases, pack/unpack, and verify. |
| `verify_unpack.py`           | Python script that recursively compares two directory trees (files, directories, symlinks). |
| `gen_test_dirs.py`           | Python script that generates various test cases including normal files, empty directories, symlinks, permission errors, and large files. |
| `CMakeLists.txt`             | CMake build configuration with paths to dependencies (`libarchive`, `zstd`, `bzip2`, `xz`, `zlib`, `openssl`). |
| `.gitignore`                 | Ignores build artifacts, OS files, editor settings, and packed archives. |
| `.github/workflows/build.yml`| GitHub Actions workflow for cross-platform builds (Linux, macOS, Windows). |

---

## Building

Ensure the dependencies (`libarchive`, `zstd`, `xz`, `bzip2`, `zlib`, `openssl`) are installed and their paths configured in `CMakeLists.txt`.  

```bash
# Clean and create build directory
./build.sh
```

This will:

1. Remove any previous `build` directory.
2. Run CMake to configure the project.
3. Build `libArchStatic` executable.

---

## Running

Use `libArchStatic` to **pack** or **unpack** directories:

```bash
# Pack a directory
./build/libArchStatic pack <source_dir> <archive.tar.gz>

# Unpack an archive
./build/libArchStatic unpack <archive.tar.gz> <destination_dir>
```

Example workflow (automated in `run.sh`):

```bash
./run.sh
```

This will:

1. Build the project.
2. Generate test directories (`gen_test_dirs.py`).
3. Pack `test_cases` into `test_cases.tar.gz`.
4. Unpack to `test_cases_unpacked`.
5. Pack again from unpacked directory to `test_cases_unpacked.tar.gz`.
6. Verify unpacked contents against the original using `verify_unpack.py`.

---

## Testing

The Python script `verify_unpack.py` recursively compares two directory trees:

- Checks for missing files or directories.
- Compares regular file contents.
- Validates symlink targets (including broken symlinks).
- Prints differences if any.

Example usage:

```bash
python verify_unpack.py test_cases test_cases_unpacked/test_cases
```

Expected output if directories match:

```
Directories are identical!
```

---

## Known Issues / Limitations

- **Symlinks on Windows**: Creation of symlinks may fail if not running with admin privileges. Broken or outside symlinks may be skipped.
- **Sparse large files**: Only supported on filesystems that allow sparse files (e.g., most Unix systems). On unsupported filesystems, files may consume full space.
- **Permission errors**: Files with restricted permissions may be ignored or cause warnings depending on the OS.
- **Full static linking**: On macOS, some system libraries (`libc++`, `libSystem.B.dylib`) remain dynamically linked due to OS restrictions.
- **Extreme stress tests**: Very large numbers of files (`MAX_FILES_FOR_STRESS`) or huge file sizes may require tuning to avoid memory or disk exhaustion.

---

## GitHub Actions

The workflow (`.github/workflows/build.yml`) allows **on-demand cross-platform builds**:

- Linux (`ubuntu-latest`)
- macOS (`macos-latest`)
- Windows (`windows-latest`)

Artifacts (`libArchStatic`) are uploaded per OS after the build.

```yaml
on:
  workflow_dispatch: # manual trigger

jobs:
  build:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]
```

---

## License

MIT License for libArchStatic

---