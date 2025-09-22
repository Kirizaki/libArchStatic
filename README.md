# libArchStatic - Kirizaki

## About

`libArchStatic` is a cross-platform (windows & macos) C++ tool to **pack and unpack directory trees** into `.tar.gz` archives, preserving:

- Regular files
- Directories (nested)
- Symlinks
- File permissions, timestamps, and sparse files (where supported)

The repository also includes Python scripts to **generate diverse test cases** and **verify unpacked directories**.

---

## Repository Structure

| File / Directory             | Description |
|-------------------------------|------------|
| `main.cpp`                   | Main C++ implementation of packing/unpacking using `libarchive`. |
| `scripts/build.sh`           | Builds the project using CMake. |
| `scripts/verify_unpack.py`   | Python script that recursively compares two directory trees (files, directories, symlinks). |
| `scripts/gen_test_dirs.py`   | Python script that generates various test cases including normal files, empty directories, symlinks, permission errors, and large files. |
| `CMakeLists.txt`             | CMake build configuration with paths to dependencies (`libarchive`, `zstd`, `bzip2`, `xz`, `zlib`, `openssl`). |
| `.env`                       | Environment variables with paths to dependencies (`libarchive`, `zstd`, `bzip2`, `xz`, `zlib`, `openssl`) - replace with yours. |
| `.gitignore`                 | Ignores build artifacts, OS files, editor settings, and packed archives. |

---

## Building - windows

Install (MSYS2)[https://www.msys2.org] for MinGW64 and update:

Update MSYS2:
```
# in MSYS2 shell
pacman -Syu
# then reopen MSYS2 mingw64 shell and run:
pacman -Syu
```

NOTE: May need Administrator privileges.

Install all dependencies:
```
pacman -S mingw-w64-x86_64-libarchive-static \
       mingw-w64-x86_64-zlib-static \
       mingw-w64-x86_64-bzip2-static \
       mingw-w64-x86_64-xz-static \
       mingw-w64-x86_64-zstd-static \
       mingw-w64-x86_64-openssl-static
```

Go to root dir of this repo and build:
```
./scripts/build.sh
```

Clean build and debug version:

```bash
./build.sh --clean --debug
```

**NOTE**: Some lib dirs may require adjustment in `CMakeLists.txt` to your environment.

## Building - macos

Ensure the dependencies (`libarchive`, `zstd`, `xz`, `bzip2`, `zlib`, `openssl`) are installed and their paths configured in `CMakeLists.txt`.

**IMPORTANT**: All libraries should be build from sources for **static linking**.
**IMPORTANT**: `libarchive` must be build without `libiconv` for static linking.

Build from root of this repo:

```bash
./scripts/build.sh
```

Clean build and debug version:

```bash
./build.sh --clean --debug
```

NOTE: Debug enables additional runtime print-outs.

**NOTE**: Some lib dirs may require adjustment in `CMakeLists.txt` to your environment.

---

## Running

Use `libArchStatic` to **pack** or **unpack** directories:

```bash
# Pack a directory
./build/libArchStatic pack <source_dir> <archive.tar.gz>

# Unpack an archive
./build/libArchStatic unpack <archive.tar.gz> <destination_dir>
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
python verify_unpack.py test_cases test_cases_unpacked
```

---

## Known Issues / Limitations

- **Symlinks on Windows**: Creation of symlinks may fail if not running as admin/root. Broken or outside symlinks may be skipped.
- **Permission errors**: Files with restricted permissions may be ignored or cause warnings if not running as admin/root.
- **Full static linking**: Some system libraries (`libc++`, `libSystem.B.dylib`) remain dynamically linked due to OS restrictions.

---

## GitHub Actions Workflows

This project uses **GitHub Actions** for continuous integration and continuous deployment (CI/CD).
The workflows defined in [`.github/workflows`](.github/workflows) automatically:

- Build/download all required static dependencies (zlib, bzip2, xz, zstd, OpenSSL, libarchive).
- Compile `libArchStatic` on macOS (download already build libs with includes on Windows).
- Run integration tests by packing/unpacking generated test directories and verifying results.
- Upload compiled binaries as workflow artifacts for easy download.

You can trigger the workflow manually from the **Actions** tab (`workflow_dispatch`) and download the latest builds directly from the workflow summary.

---

## License

MIT License for libArchStatic

---

## TODOs
- Detect & prevent **symlink loops** during recursive walk
- Allow configurable **chunk sizes** (currently fixed at 8KB/10KB)
- Reuse `archive_entry` objects via `archive_entry_clear()`
- More strict **encoding handling** (currently libarchive defaults)
- Cleanup GitHub Actions workflows