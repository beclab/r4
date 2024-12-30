#include <iostream>
#include <string>
#include <archive.h>
#include <archive_entry.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

void add_file_to_archive(struct archive *a, const std::string &file_path, const std::string &archive_path)
{
    struct archive_entry *entry = archive_entry_new();
    archive_entry_set_pathname(entry, archive_path.c_str());
    archive_entry_set_size(entry, fs::file_size(file_path));
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    archive_write_header(a, entry);

    std::ifstream file(file_path, std::ios::binary);
    char buffer[8192];
    while (file.read(buffer, sizeof(buffer)))
    {
        archive_write_data(a, buffer, file.gcount());
    }
    archive_write_data(a, buffer, file.gcount());

    archive_entry_free(entry);
}

void add_directory_to_archive(struct archive *a, const std::string &dir_path, const std::string &base_path)
{
    for (const auto &entry : fs::recursive_directory_iterator(dir_path))
    {
        std::string archive_path = base_path + "/" + fs::relative(entry.path(), dir_path).string();
        if (fs::is_directory(entry.path()))
        {
            struct archive_entry *dir_entry = archive_entry_new();
            archive_entry_set_pathname(dir_entry, archive_path.c_str());
            archive_entry_set_filetype(dir_entry, AE_IFDIR);
            archive_entry_set_perm(dir_entry, 0755);
            archive_write_header(a, dir_entry);
            archive_entry_free(dir_entry);
        }
        else if (fs::is_regular_file(entry.path()))
        {
            add_file_to_archive(a, entry.path().string(), archive_path);
        }
    }
}

void create_tar_gz(const std::string &dir_path, const std::string &output_filename)
{
    struct archive *a = archive_write_new();
    archive_write_add_filter_gzip(a);
    archive_write_set_format_pax_restricted(a);
    archive_write_open_filename(a, output_filename.c_str());

    add_directory_to_archive(a, dir_path, fs::path(dir_path).filename().string());

    archive_write_close(a);
    archive_write_free(a);
}
