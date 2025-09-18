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

## Building

Ensure the dependencies (`libarchive`, `zstd`, `xz`, `bzip2`, `zlib`, `openssl`) are installed and their paths configured in `CMakeLists.txt`.  

```bash
./build.sh
```

This will:

1. Run CMake to configure the project.
3. Build release version of `libArchStatic` executable.

```bash
./build.sh --clean --debug
```

This will:

1. Remove any previous `build` directory.
2. Run CMake to configure the project.
3. Build debug version of `libArchStatic` executable.

NOTE: Debug enables additional runtime print-outs.

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

## GitHub Actions

TODO: Add proper workflows description with binary releases.

---

## License

MIT License for libArchStatic

---