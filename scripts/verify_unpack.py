#!/usr/bin/env python3
import filecmp
import sys
import os

def compare_dirs(dir1, dir2):
    """Recursively compare two directories. Returns True if identical."""
    cmp = filecmp.dircmp(dir1, dir2)

    differences = False

    # Check for files only in one directory
    if cmp.left_only or cmp.right_only:
        differences = True
        if cmp.left_only:
            print("Only in", dir1, ":", cmp.left_only)
        if cmp.right_only:
            print("Only in", dir2, ":", cmp.right_only)

    # Compare common files
    for file_name in cmp.common_files:
        path1 = os.path.join(dir1, file_name)
        path2 = os.path.join(dir2, file_name)

        # Check if both are symlinks
        if os.path.islink(path1) and os.path.islink(path2):
            target1 = os.readlink(path1)
            target2 = os.readlink(path2)
            if target1 != target2:
                differences = True
                print(f"Symlink targets differ: {path1} -> {target1}, {path2} -> {target2}")
        # One is a symlink, the other is not
        elif os.path.islink(path1) or os.path.islink(path2):
            differences = True
            print(f"Type mismatch (symlink vs file): {path1}, {path2}")
        # Regular file comparison
        elif not filecmp.cmp(path1, path2, shallow=False):
            differences = True
            print("Differing file:", path1, "<->", path2)

    # Check funny files (inaccessible, etc.)
    if cmp.funny_files:
        differences = True
        print("Problematic files:", cmp.funny_files)

    # Recurse into subdirectories
    for subdir in cmp.common_dirs:
        if not compare_dirs(os.path.join(dir1, subdir), os.path.join(dir2, subdir)):
            differences = True

    return not differences

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <original_dir> <unpacked_dir>")
        sys.exit(1)

    dir1, dir2 = sys.argv[1], sys.argv[2]

    if not os.path.isdir(dir1) or not os.path.isdir(dir2):
        print("Both arguments must be directories.")
        sys.exit(1)

    identical = compare_dirs(dir1, dir2)

    if identical:
        print("Directories are identical!")
        sys.exit(0)
    else:
        print("Directories differ!")
        sys.exit(2)

if __name__ == "__main__":
    main()
