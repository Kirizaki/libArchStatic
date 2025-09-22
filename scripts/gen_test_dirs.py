#!/usr/bin/env python3
"""
generate_test_cases.py

Creates multiple directory trees (test cases) to validate a pack/unpack tool.
- Produces normal/edge/wrong cases.
- Creates duplicate files (same content in different folders), files with identical sizes,
  binary and text files, empty dirs, special names, symlinks (if permitted), permission errors,
  many small files for stress tests, and an optional sparse large file.
- Writes a manifest (SHA256) for each case for verification.

Usage:
    python3 generate_test_cases.py

Configuration at top of file.
"""
import os
import shutil
import random
import string
import hashlib
import pathlib
import stat
import sys
import errno
import platform

# -------- CONFIGURATION --------
BASE_DIR = "test_cases"                     # root where separate case trees will be created
DEFAULT_RANDOM_SEED = 1337

# Sizes in bytes
SMALL_TEXT_SIZE = 256
SMALL_BINARY_SIZE = 512
LARGE_FILE_SIZE = 10 * 1024 * 1024 * 250    # 250 MB default for large-file simulation (use sparse to avoid disk use)
SPARSE_LARGE_FILE = True                    # if True, create sparse large file (low actual disk usage when filesystem supports it)

# Stress / scaling
MAX_FILES_FOR_STRESS = 1048576              # number of files in "many small files" case; lower by default for safety
MAX_FILES_SAFE_DEFAULT = 200                # default small count to run quickly; can be bumped for stress testing

# Whether to attempt creation of special things that may fail on some OS (e.g., symlinks on Windows)
TRY_SYMLINKS = True
TRY_PERMISSIONS = True

# Manifest and checksumming
WRITE_MANIFEST = True                       # writes manifest.txt with path -> sha256 for each case

# --------------------------------

random.seed(DEFAULT_RANDOM_SEED)

# small helper content (non-copyright placeholder)
PLACEHOLDER_TEXT = (
    "Time machine log fragment — placeholder text used for tests.\n"
    "This simulates textual logs and should NOT contain copyrighted lyrics.\n"
)

def sha256_of_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(64 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

def write_text_file(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", errors="replace") as f:
        f.write(text)

def write_binary_file(path, data_bytes):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(data_bytes)

def write_random_text_of_size(path, size):
    s = ''.join(random.choice(string.ascii_letters + string.digits + " \n") for _ in range(size))
    write_text_file(path, s)

def write_random_binary_of_size(path, size):
    if size <= 0:
        write_binary_file(path, b"")
        return
    os.makedirs(os.path.dirname(path), exist_ok=True)
    # To avoid using lots of memory, write in chunks
    with open(path, "wb") as f:
        remaining = size
        chunk = 64 * 1024
        while remaining > 0:
            to_write = min(chunk, remaining)
            f.write(os.urandom(to_write))
            remaining -= to_write

def create_sparse_file(path, size):
    """Create a sparse file by seeking. Works on many Unix filesystems and NTFS."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        if size > 0:
            f.seek(size - 1)
            f.write(b"\0")

def safe_symlink(target, link_name):
    try:
        # create a relative symlink when possible
        rel = os.path.relpath(target, os.path.dirname(link_name))
        os.symlink(rel, link_name)
        return True, None
    except Exception as e:
        return False, str(e)

def make_unreadable(path):
    try:
        # remove read permissions for owner
        current = os.stat(path)
        os.chmod(path, current.st_mode & ~stat.S_IRUSR)
        return True, None
    except Exception as e:
        return False, str(e)

def create_case_normal(root):
    """Typical mix: subdirs, text, binary, a few duplicates across different dirs."""
    os.makedirs(root, exist_ok=True)
    write_text_file(os.path.join(root, "log1.txt"), PLACEHOLDER_TEXT)
    write_random_binary_of_size(os.path.join(root, "frame.bin"), SMALL_BINARY_SIZE)
    # nested
    nested = os.path.join(root, "session_2025", "runA")
    write_text_file(os.path.join(nested, "notes.txt"), "Run A notes\n" + PLACEHOLDER_TEXT)
    write_random_binary_of_size(os.path.join(nested, "dump.bin"), SMALL_BINARY_SIZE)
    # duplicate content in different places
    duplicate = os.urandom(512)
    write_binary_file(os.path.join(root, "d1", "dup1.bin"), duplicate)
    write_binary_file(os.path.join(root, "d2", "dup_copy.bin"), duplicate)
    # identical-size but different content
    s1 = os.urandom(256)
    s2 = os.urandom(256)
    write_binary_file(os.path.join(root, "same_size_1.bin"), s1)
    write_binary_file(os.path.join(root, "same_size_2.bin"), s2)

def create_case_empty_dirs(root):
    """Many empty directories, some nested deeply."""
    os.makedirs(root, exist_ok=True)
    for d in ["empty", "empty2/subempty", "deep/nested/a/b/c/d/e", "zero"]:
        os.makedirs(os.path.join(root, d), exist_ok=True)

def create_case_full_of_files(root, count=MAX_FILES_SAFE_DEFAULT):
    """Directory with many files to test directory listing / performance."""
    os.makedirs(root, exist_ok=True)
    many_dir = os.path.join(root, "many_files")
    os.makedirs(many_dir, exist_ok=True)
    # keep names deterministic for reproducibility
    for i in range(count):
        write_random_text_of_size(os.path.join(many_dir, f"file_{i:05d}.log"), SMALL_TEXT_SIZE)

def create_case_special_names(root):
    """Special filenames: unicode, spaces, punctuation, very long names."""
    os.makedirs(root, exist_ok=True)
    specials = [
        "space in name.txt",
        "ümlaut_unicode_ß.txt",
        "semi;colon.txt",
        "weird#file?.txt",
        "tab\tname.txt",
        "newline\ninname.txt",  # will likely be skipped
    ]
    for name in specials:
        path = os.path.join(root, "special", name)
        try:
            write_text_file(path, "special: " + name + "\n" + PLACEHOLDER_TEXT)
        except OSError as e:
            print(f"[special_names] Skipping invalid name {repr(name)}: {e}")

def create_case_symlinks(root):
    """Create valid symlinks (if allowed) and a broken symlink."""
    os.makedirs(root, exist_ok=True)
    target = os.path.join(root, "target.txt")
    write_text_file(target, "I am the target\n")
    link1 = os.path.join(root, "link_to_target.txt")
    ok, err = safe_symlink(target, link1) if TRY_SYMLINKS else (False, "skipped")
    if not ok:
        print(f"[symlink] Could not create symlink {link1}: {err}")
    # symlink pointing outside tree
    outside_target = os.path.abspath(os.path.join(root, "..", "outside_marker.txt"))
    write_text_file(outside_target, "outside")
    link2 = os.path.join(root, "link_to_outside.txt")
    ok2, e2 = safe_symlink(outside_target, link2) if TRY_SYMLINKS else (False, "skipped")
    if not ok2:
        print(f"[symlink] Could not create symlink {link2}: {e2}")
    # broken symlink
    broken = os.path.join(root, "broken_link.txt")
    try:
        os.symlink(os.path.join(root, "does_not_exist_12345.bin"), broken)
    except Exception as e:
        print(f"[symlink] Broken symlink creation failed (may be okay): {e}")

def create_case_permission_errors(root):
    """Create a file and then remove read permission to simulate unreadable file."""
    os.makedirs(root, exist_ok=True)
    p = os.path.join(root, "protected", "secret.log")
    write_text_file(p, "Top secret logs\n")
    if TRY_PERMISSIONS:
        ok, err = make_unreadable(p)
        if not ok:
            print(f"[permissions] Could not remove read permission: {err}")

def create_case_large_file(root, size=LARGE_FILE_SIZE, sparse=SPARSE_LARGE_FILE):
    os.makedirs(root, exist_ok=True)
    path = os.path.join(root, "big_logs", "bigfile.bin")
    if sparse:
        try:
            create_sparse_file(path, size)
        except Exception as e:
            print(f"[large_file] Sparse file failed, falling back to writing: {e}")
            write_random_binary_of_size(path, min(size, 10 * 1024 * 1024 * 250))
    else:
        write_random_binary_of_size(path, min(size, 10 * 1024 * 1024 * 250))

def create_case_dup_across_many_dirs(root):
    """
    Create several directories containing the same content (binary-identical files)
    but different names and different folder depths. This is important for dedupe testing.
    """
    os.makedirs(root, exist_ok=True)
    chunk = os.urandom(1024)
    for i in range(8):
        d = os.path.join(root, f"branch_{i}", "deep", f"level_{i}")
        os.makedirs(d, exist_ok=True)
        for j in range(3):
            write_binary_file(os.path.join(d, f"same_{j}.bin"), chunk)

def create_case_long_paths(root):
    """
    Create directories and files with very long paths, testing PATH_MAX limits.
    Cross-platform aware: uses conservative path lengths per OS.
    """
    os.makedirs(root, exist_ok=True)

    system = platform.system()
    if system == "Windows":
        target_length = 240
    elif system == "Darwin":
        target_length = 1000
    else:
        target_length = 3000

    base = root
    current_length = len(base)
    level = 0

    try:
        while current_length < target_length - 20:
            dir_name = "a" * 50
            base = os.path.join(base, dir_name)
            os.makedirs(base, exist_ok=True)
            current_length = len(base)
            level += 1

        file_name = "long_file.txt"
        path = os.path.join(base, file_name)
        write_text_file(path, f"File at depth {level}, path length {len(path)}\n")
    except OSError as e:
        if e.errno in (errno.ENAMETOOLONG, errno.EINVAL):
            print(f"[long_paths] Stopped creating paths: {e}")
        else:
            raise

def write_manifest_for_case(root):
    """Walk the case tree and write manifest.txt with SHA256(push newline) per file."""
    if not WRITE_MANIFEST:
        return
    manifest = []
    for dirpath, dirnames, filenames in os.walk(root):
        for fname in filenames:
            fpath = os.path.join(dirpath, fname)
            try:
                h = sha256_of_file(fpath)
            except Exception as e:
                h = f"ERR:{e}"
            rel = os.path.relpath(fpath, root)
            manifest.append((rel, h))
    manifest.sort()
    mpath = os.path.join(root, "manifest.txt")
    with open(mpath, "w", encoding="utf-8") as mf:
        for rel, h in manifest:
            mf.write(f"{h}  {rel}\n")

def clear_base_dir():
    if os.path.exists(BASE_DIR):
        print(f"Removing existing directory {BASE_DIR}")
        shutil.rmtree(BASE_DIR)
    os.makedirs(BASE_DIR, exist_ok=True)

def main():
    clear_base_dir()
    cases = []

    c = os.path.join(BASE_DIR, "case_normal")
    create_case_normal(c)
    write_manifest_for_case(c)
    cases.append(c)

    c = os.path.join(BASE_DIR, "case_empty_dirs")
    create_case_empty_dirs(c)
    write_manifest_for_case(c)
    cases.append(c)

    c = os.path.join(BASE_DIR, "case_many_files")
    create_case_full_of_files(c, count=min(MAX_FILES_SAFE_DEFAULT, MAX_FILES_FOR_STRESS))
    write_manifest_for_case(c)
    cases.append(c)

    c = os.path.join(BASE_DIR, "case_special_names")
    create_case_special_names(c)
    write_manifest_for_case(c)
    cases.append(c)

    c = os.path.join(BASE_DIR, "case_symlinks")
    create_case_symlinks(c)
    write_manifest_for_case(c)
    cases.append(c)

    c = os.path.join(BASE_DIR, "case_permissions")
    create_case_permission_errors(c)
    write_manifest_for_case(c)
    cases.append(c)

    c = os.path.join(BASE_DIR, "case_large_file")
    create_case_large_file(c)
    write_manifest_for_case(c)
    cases.append(c)

    c = os.path.join(BASE_DIR, "case_dups")
    create_case_dup_across_many_dirs(c)
    write_manifest_for_case(c)
    cases.append(c)

    # c = os.path.join(BASE_DIR, "case_long_paths")
    # create_case_long_paths(c)
    # write_manifest_for_case(c)
    # cases.append(c)

    print("\nCreated cases:")
    for p in cases:
        print("  -", p)
    print("\nNote:")
    print(" - Manifest (manifest.txt) written in each case if WRITE_MANIFEST=True.")
    print(" - Special names and symlinks may be skipped on some OSes; errors printed to stdout.")
    print(" - To stress test bigger numbers of files or larger sizes, adjust constants at top of the script.")

if __name__ == "__main__":
    main()
