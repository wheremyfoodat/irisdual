
#pragma once

#include <algorithm>
#include <filesystem>
#include <cstring>
#include <string>
#include <fstream>
#include <memory>
#include <vector>

#include <atom/integer.hpp>
#include <atom/panic.hpp>

namespace dual {

  class BackupFile {
    public:
      static auto OpenOrCreate(
        std::string const& save_path,
        std::vector<size_t> const& valid_sizes,
        size_t& default_size
      ) -> std::unique_ptr<BackupFile> {
        namespace fs = std::filesystem;

        bool create = true;
        auto flags = std::ios::binary | std::ios::in | std::ios::out;
        auto file = std::unique_ptr<BackupFile>{new BackupFile()};

        // @todo: check that we have read and write permissions for the file.
        if(fs::is_regular_file(save_path)) {
          const auto size = fs::file_size(save_path);

          const auto begin = valid_sizes.begin();
          const auto end = valid_sizes.end();

          if(std::find(begin, end, size) != end) {
            file->m_stream.open(save_path, flags);
            if(file->m_stream.fail()) {
              ATOM_PANIC("unable to open file: {}", save_path);
            }
            default_size = size;
            file->m_memory.reset(new u8[size]);
            file->m_stream.read((char*)file->m_memory.get(), size);
            create = false;
          }
        }

        file->m_file_size = default_size;

        /* A new save file is created either when no file exists yet,
         * or when the existing file has an invalid size.
         */
        if(create) {
          file->m_stream.open(save_path, flags | std::ios::trunc);
          if(file->m_stream.fail()) {
            ATOM_PANIC("unable to create file: {}", save_path);
          }
          file->m_memory.reset(new u8[default_size]);
          file->MemorySet(0, default_size, 0xFF);
        }

        return file;
      }

      bool SetAutoUpdate(bool auto_update) {
        m_auto_update = auto_update;
      }

      auto Read(size_t index) -> u8 {
        if(index >= m_file_size) {
          ATOM_PANIC("out-of-bounds index while reading.");
        }
        return m_memory[index];
      }

      void Write(size_t index, u8 value) {
        if(index >= m_file_size) {
          ATOM_PANIC("out-of-bounds index while writing.");
        }
        m_memory[index] = value;
        if(m_auto_update) {
          Update(index, 1);
        }
      }

      void MemorySet(size_t index, size_t length, u8 value) {
        if((index + length) > m_file_size) {
          ATOM_PANIC("out-of-bounds index while setting memory.");
        }
        std::memset(&m_memory[index], value, length);
        if(m_auto_update) {
          Update(index, length);
        }
      }

      void Update(size_t index, size_t length) {
        if((index + length) > m_file_size) {
          ATOM_PANIC("out-of-bounds index while updating file.");
        }
        m_stream.seekg(index);
        m_stream.write((char*)&m_memory[index], length);
      }

    private:
      BackupFile() = default;

      bool m_auto_update = true;
      size_t m_file_size{};
      std::fstream m_stream;
      std::unique_ptr<u8[]> m_memory;
  };

} // namespace dual
