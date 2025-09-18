//  libArchStatic
//  Copyright (c) 2025 Kirizaki
//  This file is part of libArchStatic, licensed under the MIT License. 
//  Note: This software incorporates libarchive, which is licensed under the BSD

#include <archive.h>
#include <archive_entry.h>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

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

// Add file/dir/symlink to archive
bool addFile(struct archive* a, const fs::path& baseDir, const fs::path& path) {
    archive_entry* entry = archive_entry_new();
    std::string relPath = fs::relative(path, baseDir).string();
    archive_entry_set_pathname(entry, sanitizePathForArchive(relPath).c_str());

    auto st = fs::symlink_status(path);

    if (fs::is_symlink(path)) {
        fs::path target = fs::read_symlink(path);
        archive_entry_set_filetype(entry, AE_IFLNK);
        archive_entry_set_perm(entry, 0777);
        archive_entry_set_symlink(entry, sanitizePathForArchive(target).c_str());
        archive_write_header(a, entry);
    }
    else if (fs::is_directory(path)) {
        archive_entry_set_filetype(entry, AE_IFDIR);
        archive_entry_set_perm(entry, permsToMode(st.permissions()));
        archive_write_header(a, entry);
    }
    else if (fs::is_regular_file(path)) {
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, permsToMode(st.permissions()));
        archive_entry_set_size(entry, fs::file_size(path));
        archive_write_header(a, entry);

        std::ifstream f(path, std::ios::binary);
        char buf[8192];
        while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
            archive_write_data(a, buf, f.gcount());
        }
    }

    archive_entry_free(entry);
    return true;
}

bool packDirectory(const fs::path& dir, const fs::path& archivePath) {
    struct archive* a = archive_write_new();
    archive_write_set_format_pax_restricted(a); // POSIX tar
    archive_write_add_filter_gzip(a);

    if (archive_write_open_filename(a, archivePath.string().c_str()) != ARCHIVE_OK)
        return false;

    for (auto& p : fs::recursive_directory_iterator(dir,
            fs::directory_options::follow_directory_symlink)) {
        try
        {
#ifdef DEBUG
            std::cout << "addFile: " << dir << " -> " << p.path() << std::endl;
#endif
            addFile(a, dir, p.path());
        }
        catch(const std::exception& e)
        {
            std::cerr << "FAILED: packDirectory: " << dir << std::endl << e.what() << std::endl;
        }
        
    }

    archive_write_close(a);
    archive_write_free(a);
    return true;
}

bool unpackArchive(const fs::path& archivePath, const fs::path& destDir) {
    struct archive* a = archive_read_new();
    struct archive* ext = archive_write_disk_new();

    archive_read_support_format_tar(a);
    archive_read_support_filter_gzip(a);

    archive_write_disk_set_options(ext,
        ARCHIVE_EXTRACT_OWNER |
        ARCHIVE_EXTRACT_PERM |
        ARCHIVE_EXTRACT_TIME |
        ARCHIVE_EXTRACT_FFLAGS |
        ARCHIVE_EXTRACT_ACL |
        ARCHIVE_EXTRACT_UNLINK |
        ARCHIVE_EXTRACT_XATTR |
        ARCHIVE_EXTRACT_SPARSE
    );

    archive_write_disk_set_standard_lookup(ext);

    if (archive_read_open_filename(a, archivePath.string().c_str(), 10240) != ARCHIVE_OK)
        return false;

    archive_entry* entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        try {
            std::string relPath = archive_entry_pathname(entry);

#ifdef _WIN32
            if (relPath.rfind("\\\\?\\", 0) == 0) relPath = relPath.substr(4);
            if (relPath.size() >= 2 && relPath[1] == ':') relPath = relPath.substr(2);
            while (!relPath.empty() && (relPath[0] == '\\' || relPath[0] == '/')) relPath.erase(0,1);
#endif

            fs::path fullPath = destDir / relPath;

#ifdef DEBUG
            std::cout << "unpackArchive: " << relPath << " -> " << fullPath.parent_path() << std::endl;
#endif
            std::error_code ec;
            fs::create_directories(fullPath.parent_path(), ec);

            archive_entry_set_pathname(entry, sanitizePathForArchive(fullPath).c_str());

            mode_t old_umask = umask(0);
            if (archive_write_header(ext, entry) != ARCHIVE_OK) {
                std::cerr << "FAILED: archive_write_header: " << archive_error_string(ext) << "\n";
                umask(old_umask);
                continue;
            }
            umask(old_umask);

            if (archive_entry_filetype(entry) == AE_IFREG) {
                const void* buff;
                size_t size;
                la_int64_t offset;
                while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK) {
                    archive_write_data_block(ext, buff, size, offset);
                }
            }
        } catch(const std::exception& e) {
            std::cerr << "FAILED: archive_read_next_header" << std::endl << e.what() << std::endl;
        }
    }

    archive_write_close(ext);
    archive_write_free(ext);
    archive_read_close(a);
    archive_read_free(a);
    return true;
}

void printHeader() {
    std::cout << "libArchStatic" << std::endl
              << "Copyright (c) 2025 Kirizaki" << std::endl
              << "This file is part of libArchStatic, licensed under the MIT License." << std::endl
              << "Note: This software incorporates libarchive, which is licensed under the BSD" << std::endl << std::endl;
}

int main(int argc, char* argv[]) {
    printHeader();

    if (argc != 4) {
        std::cerr << "Usage: pack|unpack <source> <destination>\n";
        return 1;
    }


    std::string cmd = argv[1];
    fs::path src = argv[2];
    fs::path dst = argv[3];

    std::cout << cmd << ": " << src << " -> " << dst << std::endl;
    
    // Normalize destination
    dst = fs::absolute(dst);

    if (cmd == "pack") return packDirectory(src, dst) ? 0 : 1;

    if (cmd == "unpack") {
        if (!fs::exists(dst)) {
            std::error_code ec;
            fs::create_directories(dst, ec);
            if (ec) {
                std::cerr << "Failed to create destination directory: " << ec.message() << "\n";
                return 1;
            }
        }
        return unpackArchive(src, dst) ? 0 : 1;
    }

    std::cerr << "Unknown command: " << cmd << "\n";
    return 2;
}
