
#pragma once

#include <atom/integer.hpp>
#include <atom/meta.hpp>
#include <algorithm>
#include <array>
#include <functional>
#include <vector>

namespace dual::nds {

  template<size_t page_count, u32 page_size = 16384>
  class Region {
    public:
      using Callback = std::function<void(u32, size_t)>;

      Region(size_t mask) : m_mask{mask} {}

      template<typename T>
      T Read(u32 offset) const {
        static_assert(atom::is_one_of_v<T, u8, u16, u32, u64>, "T must be u8, u16, u32 or u64");

        auto const& desc = m_pages[(offset >> k_page_shift) & m_mask];

        offset &= k_page_mask & ~(sizeof(T) - 1);

        if(desc.page != nullptr) [[likely]] {
          return *reinterpret_cast<T*>(&desc.page[offset]);
        }

        if(desc.pages != nullptr) {
          T value = 0;
          for(u8* page : *desc.pages) {
            value |= *reinterpret_cast<T*>(&page[offset]);
          }
          return value;
        }

        return 0;
      }

      template<typename T>
      void Write(u32 offset, T value) {
        static_assert(atom::is_one_of_v<T, u8, u16, u32, u64>, "T must be u8, u16, u32 or u64");

        auto const& desc = m_pages[(offset >> k_page_shift) & m_mask];

        offset &= k_page_mask & ~(sizeof(T) - 1);

        if(desc.page != nullptr) [[likely]] {
          *reinterpret_cast<T*>(&desc.page[offset]) = value;
          return;
        }

        if(desc.pages != nullptr) {
          for(u8* page : *desc.pages) {
            *reinterpret_cast<T*>(&page[offset]) = value;
          }
        }
      }

      template<typename T>
      T const* GetUnsafePointer(u32 offset) const {
        auto const& desc = m_pages[(offset >> k_page_shift) & m_mask];

        offset &= k_page_mask & ~(sizeof(T) - 1);

        if(desc.page != nullptr) [[likely]] {
          return reinterpret_cast<T*>(&desc.page[offset]);
        }
        return nullptr;
      }

      template<typename T>
      T* GetUnsafePointer(u32 offset) {
        auto const& desc = m_pages[(offset >> k_page_shift) & m_mask];

        offset &= k_page_mask & ~(sizeof(T) - 1);

        if(desc.page != nullptr) [[likely]] {
          return reinterpret_cast<T*>(&desc.page[offset]);
        }
        return nullptr;
      }

      template<size_t bank_size>
      void Map(u32 offset, std::array<u8, bank_size>& bank, size_t size = bank_size) {
        auto id = static_cast<size_t>(offset >> k_page_shift);
        auto final_id = id + (size >> k_page_shift);
        auto data = bank.data();

        while(id < final_id) {
          auto& desc = m_pages.at(id++);

          if(desc.page != nullptr) [[unlikely]] {
            desc.pages = new std::vector<u8*>{};
            desc.pages->push_back(desc.page);
            desc.pages->push_back(data);
            desc.page = nullptr;
          } else if(desc.pages != nullptr) [[unlikely]] {
            desc.pages->push_back(data);
          } else {
            desc.page = data;
          }

          data += page_size;
        }

        for(auto const& callback : m_callbacks) callback(offset, size);
      }

      template<size_t bank_size>
      void Unmap(u32 offset, std::array<u8, bank_size> const& bank, size_t size = bank_size) {
        auto id = static_cast<size_t>(offset >> k_page_shift);
        auto final_id = id + (size >> k_page_shift);
        auto data = bank.data();

        while(id < final_id) {
          auto& desc = m_pages.at(id++);

          if(desc.page == data) [[likely]] {
            desc.page = nullptr;
          } else if(desc.pages != nullptr) {
            const auto begin = desc.pages->begin();
            const auto end = desc.pages->end();
            const auto match = std::find(begin, end, data);

            if(match != end) {
              desc.pages->erase(match);

              if(desc.pages->size() == 1) {
                desc.page = (*desc.pages)[0];
                delete desc.pages;
                desc.pages = nullptr;
              }
            }
          }

          data += page_size;
        }

        for(auto const& callback : m_callbacks) callback(offset, size);
      }

      void AddCallback(Callback const& callback) const {
        m_callbacks.push_back(callback);
      }

    private:
      // VRAM page descriptor (default size is 16 KiB)
      struct PageDescriptor {
        // Pointer to a single physical page (regular case).
        u8* page = nullptr;

        // List of pointers to multiple physical pages (rare special case).
        std::vector<u8*>* pages = nullptr;
      };

      size_t m_mask{};
      std::array<PageDescriptor, page_count> m_pages{};
      mutable std::vector<Callback> m_callbacks{};

      static constexpr int k_page_shift = []() constexpr -> int {
        for(int i = 0; i < 32; i++)
          if(page_size == (1 << i))
            return i;
        return -1;
      }();

      static constexpr int k_page_mask = page_size - 1;

      // Make sure that the provided page size actually is a power-of-two.
      static_assert(k_page_shift != -1, "Region: page size must be a power-of-two.");
  };

} // namespace dual::nds
