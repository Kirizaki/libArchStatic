//  libArchStatic - Copyright (c) 2025 Kirizaki
//  This file is part of libArchStatic, licensed under the MIT License. 
//  Note: This software incorporates libarchive, which is licensed under the BSD

#include <archive.h>
#include <archive_entry.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#ifdef DEBUG
    #include <sys/errno.h>
#endif

namespace fs = std::filesystem;

// RAII wrappers for archive_x pointers
struct ArchiveWriteDeleter {
    void operator()(archive* a) const noexcept {
        if (a) {
            archive_write_close(a);
            archive_write_free(a);
        }
    }
};

struct ArchiveReadDeleter {
    void operator()(archive* a) const noexcept {
        if (a) {
            archive_read_close(a);
            archive_read_free(a);
        }
    }
};

struct ArchiveDiskDeleter {
    void operator()(archive* a) const noexcept {
        if (a) {
            archive_write_close(a);
            archive_write_free(a);
        }
    }
};

struct ArchiveEntryDeleter {
    void operator()(archive_entry* e) const noexcept {
        if(e) archive_entry_free(e);
    }
};

mode_t permsToMode(fs::perms p) {
    mode_t mode = 0;
    if ((p & fs::perms::owner_read) != fs::perms::none) mode |= S_IRUSR;
    if ((p & fs::perms::owner_write) != fs::perms::none) mode |= S_IWUSR;
    if ((p & fs::perms::owner_exec) != fs::perms::none) mode |= S_IXUSR;
    if ((p & fs::perms::group_read) != fs::perms::none) mode |= S_IRGRP;
    if ((p & fs::perms::group_write) != fs::perms::none) mode |= S_IWGRP;
    if ((p & fs::perms::group_exec) != fs::perms::none) mode |= S_IXGRP;
    if ((p & fs::perms::others_read) != fs::perms::none) mode |= S_IROTH;
    if ((p & fs::perms::others_write) != fs::perms::none) mode |= S_IWOTH;
    if ((p & fs::perms::others_exec) != fs::perms::none) mode |= S_IXOTH;
    return mode;
}

// Strip \\?\ prefix on Windows for libarchive
std::string sanitizePathForArchive(const fs::path& p) {
#ifdef _WIN32
    std::string s = p.string();
    if (s.rfind("\\\\?\\", 0) == 0) {
        s = s.substr(4);
    }
    return s;
#else
    return p.string();
#endif
}

/*
    Adds new entries to currently created/opened archive.
    Each entry (symlink|dir|file) handles as follows:
    - create instance of 'archive_entry'
    - read permissions 'filesystem::symlink_status': https://en.cppreference.com/w/cpp/filesystem/status.html
    - setup entry and write header
      (NOTE: For symlinks sets super wide permissions, as the target file's permissions are the only the importatant)
    - (AE_IFREG):
        - open file
        - stream data file->archive with chunks in 8 KiBiByTe(s) :3
          NOTE: Configure to specific applications (if bigger files -> bigger chunks -> lower count of sys calls!)
    TODO: Could re-use archive_entry object by entry->archive_entry_clear().
*/
bool addFile(struct archive* a, const fs::path& baseDir, const fs::path& path) {
    // creates new archive_entry which actually holds 'aest' -> Archive Entry Stats,
    // which holds wider fields due to some OS's diffs: libarchive/libarchive/archive_entry_private.h
    std::unique_ptr<archive_entry, ArchiveEntryDeleter> entry(archive_entry_new());
    if (!entry) {
        std::cerr << "FAILED: archive_entry_new" << std::endl;
        return false;
    }

    std::string relPath = fs::relative(path, baseDir).string();
    archive_entry_set_pathname(entry.get(), sanitizePathForArchive(relPath).c_str());

    auto st = fs::symlink_status(path);

    int r;

    if (fs::is_symlink(path)) {
        fs::path target = fs::read_symlink(path);
        archive_entry_set_filetype(entry.get(), AE_IFLNK);
        archive_entry_set_perm(entry.get(), 0777);
        archive_entry_set_symlink(entry.get(), sanitizePathForArchive(target).c_str());
        r = archive_write_header(a, entry.get());
        if (r != ARCHIVE_OK) return false;

    } else if (fs::is_directory(path)) {
        archive_entry_set_filetype(entry.get(), AE_IFDIR);
        archive_entry_set_perm(entry.get(), permsToMode(st.permissions()));
        r = archive_write_header(a, entry.get());
        if (r != ARCHIVE_OK) return false;

    } else if (fs::is_regular_file(path)) {
        archive_entry_set_filetype(entry.get(), AE_IFREG);
        archive_entry_set_perm(entry.get(), permsToMode(st.permissions()));
        archive_entry_set_size(entry.get(), fs::file_size(path));

        r = archive_write_header(a, entry.get());
        if (r != ARCHIVE_OK) {
            if (r == ARCHIVE_WARN) {
                std::cerr << "WARNING: Inappropriate file type or format: " << path << std::endl; 
#ifdef DEBUG
                std::cerr << "Error type: " << archive_errno(a) << std::endl;
#endif
            } else {
                std::cerr << "Serious errors that make remaining operations impossible!" << std::endl; 
                return false;
            }
        }

        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return false;

        char buf[8192];
        while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
            if (archive_write_data(a, buf, f.gcount()) < 0) {
#ifdef DEBUG
                std::cerr << "FAILED: archive_write_data for " << path << std::endl;
#endif
                return false;
            }
        }
    }

    return true;
}

bool packDirectory(const fs::path& dir, const fs::path& archivePath) {
    // try to get new pointer to 'archive_write' struct:
    // libarchive/libarchive/archive_write_private.h
    // which actually containes 'archive' struct with 'archive_vtable' with "real" behaviours/methods ;)
    std::unique_ptr<archive, ArchiveWriteDeleter> a(archive_write_new());
    if (!a) return false;

    // sets POSIX tar format: libarchive/libarchive/archive_write_set_format_pax.c
    // structued Tape Archive concatenation
    archive_write_set_format_pax_restricted(a.get());
    // adds compression (default ZLIB / level 6?): libarchive/libarchive/archive_write_add_filter_gzip.c
    // A level 1 compression might offer the fastest processing but the largest file size.
    // A level 9 compression prioritizes the smallest file size at the expense of processing speed.
    // wiki: https://www.ibm.com/docs/en/linux-on-systems?topic=izdc-compression-levels
    archive_write_add_filter_gzip(a.get());

    if (archive_write_open_filename(a.get(), archivePath.string().c_str()) != ARCHIVE_OK) {
#ifdef DEBUG
        std::cerr << "FAILED: archive_write_open_filename: " << archive_error_string(a.get()) << std::endl;
#endif
        return false;
    }

    // iterate recursively
    // TODO: Should check against symlinks loop
    //       by collecting symlink dirs (visited) and visit only !visited
    for (auto& p : fs::recursive_directory_iterator(
             dir, fs::directory_options::follow_directory_symlink)) {
        try {
#ifdef DEBUG
            std::cout << "addFile: " << dir << " -> " << p.path() << std::endl;
#endif
            // 
            if (!addFile(a.get(), dir, p.path())) {
#ifdef DEBUG
                std::cerr << "FAILED: addFile: " << p.path() << std::endl;
#endif
            }
        } catch (const std::exception& e) {
#ifdef DEBUG
            std::cerr << "FAILED: packDirectory: " << dir << std::endl << e.what() << std::endl;
#endif
        }
    }
    return true;
}

bool unpackArchive(const fs::path& archivePath, const fs::path& destDir) {
    // try to get new pointer to 'archive':
    // libarchive/libarchive/archive_read_private.h
    std::unique_ptr<archive, ArchiveReadDeleter> a(archive_read_new());
    // ..and new directory writer:
    // libarchive/libarchive/archive_write_disk_posix.c
    std::unique_ptr<archive, ArchiveDiskDeleter> ext(archive_write_disk_new());
    if (!a || !ext) return false;

    // set that we expect Tape Archive (TAR) format (see: packArchive)
    archive_read_support_format_tar(a.get());
    // and of course GZIP for compression
    // NOTE: Should be in-sync with format & compression type packArchive<->unpackArchive!
    archive_read_support_filter_gzip(a.get());

    // sets flags to archive about what exactly we want to restore from metadata
    archive_write_disk_set_options(ext.get(),
        ARCHIVE_EXTRACT_OWNER |   // Set file owner/group to match archive
        ARCHIVE_EXTRACT_PERM |    // Set file permissions to match archive
        ARCHIVE_EXTRACT_TIME |    // Preserve modification/access times
        ARCHIVE_EXTRACT_FFLAGS |  // Preserve special file flags (e.g., immutable)
        ARCHIVE_EXTRACT_ACL |     // Preserve access control lists (extended permissions)
        ARCHIVE_EXTRACT_UNLINK |  // Delete existing files before extracting
        ARCHIVE_EXTRACT_XATTR |   // Preserve extended attributes (like user.comment or security.selinux.)
        ARCHIVE_EXTRACT_SPARSE    // Handle sparse files efficiently (Sparse: [non-zero] 0...0 [non-zero])
    );
    // and set system standard method to look up those mentioned
    // libarchive/libarchive/archive_write_disk_set_standard_lookup.c
    archive_write_disk_set_standard_lookup(ext.get());

    // read archive in chunk o 10 KiBiBytes :3 -> why 10 KiB? just arbitrary..
    // NOTE: Configure to specific applications (if bigger files -> bigger chunks -> lower count of sys calls!)
    if (archive_read_open_filename(a.get(), archivePath.string().c_str(), 10240) != ARCHIVE_OK) {
#ifdef DEBUG
        std::cerr << "FAILED: archive_read_open_filename: " << archive_error_string(a.get()) << std::endl;
#endif
        return false;
    }

    archive_entry* entry;
    // iterate each entry in archive
    // NOTE: Calling vtable foo ptr: libarchive/libarchive/archive_read.c
    while (archive_read_next_header(a.get(), &entry) == ARCHIVE_OK) {
        try {
            // let's say encoding-wise "safe" read of path as it tries different encodings:
            // libarchive/libarchive/archive_entry.c
            // NOTE: For encoding strict application, this should be done explicitly!
            std::string relPath = archive_entry_pathname(entry);

#ifdef _WIN32
            // ehh.. this windows (MinGW)
            if (relPath.rfind("\\\\?\\", 0) == 0) relPath = relPath.substr(4);
            if (relPath.size() >= 2 && relPath[1] == ':') relPath = relPath.substr(2);
            while (!relPath.empty() && (relPath[0] == '\\' || relPath[0] == '/')) relPath.erase(0,1);
#endif

            fs::path fullPath = destDir / relPath;

#ifdef DEBUG
            std::cout << "unpackArchive: " << relPath << " -> " << fullPath.parent_path() << std::endl;
#endif
            std::error_code ec;
            // creates dir as if by POSIX mkdir, but base (destDir) dir MUST EXISTS!
            fs::create_directories(fullPath.parent_path(), ec);

            // NOTE: For encoding strict application, this should be done explicitly!
            archive_entry_set_pathname(entry, sanitizePathForArchive(fullPath).c_str());

            // creator-permission-magic:
            // 1) clear umask, to preserve ONLY those from entry we set in packing:
            //    per entry: addFile -> archive_entry_set_perm
            // tldr; prevents current process pollution
            mode_t old_umask = umask(0);
            // 2) writes header :)
            int r = archive_write_header(ext.get(), entry);
            // 3) revert umask
            umask(old_umask);
            if (r != ARCHIVE_OK) {
#ifdef DEBUG
                std::cerr << "FAILED: archive_write_header: " << archive_error_string(ext.get()) << std::endl;
#endif
                continue;
            }

            // and on file 
            if (archive_entry_filetype(entry) == AE_IFREG) {
                const void* buff;
                size_t size;
                la_int64_t offset;
                // let libarchive handle buffer size + _read_data_block handles sparse files ;)
                while (archive_read_data_block(a.get(), &buff, &size, &offset) == ARCHIVE_OK) {
                    archive_write_data_block(ext.get(), buff, size, offset);
                }
            }

            // flush buffers, write metadata, finalize internal structures,
            // to keep libarchive state consistent
            archive_write_finish_entry(ext.get());

        } catch (const std::exception& e) {
#ifdef DEBUG
            std::cerr << "FAILED: unpackArchive: " << e.what() << std::endl;
#endif
        }
    }
    return true;
}

void printHeader() {
    std::cout << "libArchStatic - Copyright (c) 2025 Kirizaki" << std::endl
              << "This file is part of libArchStatic, licensed under the MIT License." << std::endl
              << "Note: This software incorporates libarchive, which is licensed under the BSD" << std::endl << std::endl;
}

int main(int argc, char* argv[]) {
    printHeader();

    if (argc != 4) {
        std::cerr << "Usage: pack|unpack <source> <destination>" << std::endl;
        return 1;
    }


    std::string cmd = argv[1];
    fs::path src = argv[2];
    fs::path dst = argv[3];

    std::cout << "CMD: " << cmd << std::endl;
    std::cout << "SRC: " << src << std::endl;
    std::cout << "DST: " << dst << std::endl << std::endl;

    // Normalize destination
    dst = fs::absolute(dst);

    if (cmd == "pack") {
        std::cout << "packing.." << std::endl;
        return packDirectory(src, dst) ? 0 : 1;
    }

    if (cmd == "unpack") {
        if (!fs::exists(dst)) {
            std::error_code ec;
            fs::create_directories(dst, ec);
            if (ec) {
                std::cerr << "Failed to create destination directory: " << ec.message() << std::endl;
                return 1;
            }
        }
        std::cout << "unpacking.." << std::endl;
        return unpackArchive(src, dst) ? 0 : 1;
    }

    std::cerr << "Unknown command: " << cmd << std::endl;
    return 2;
}
