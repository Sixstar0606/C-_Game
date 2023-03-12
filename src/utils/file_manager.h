#pragma once
#include <string_view>
#include <filesystem>
#include <iostream>
#include <fstream>

namespace GTServer::FileManager {
    inline uint8_t* read_all_bytes(const std::string_view& path, std::size_t& file_size) noexcept {
        std::ifstream file{ path.data(), std::ifstream::in | std::ifstream::binary };
        if (!file.is_open())
            return nullptr;
        file_size = file.seekg(0, std::ios::end).tellg();
        uint8_t* data = (uint8_t*)std::malloc(file_size);

        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(file_size));
        file.close();   

        return data;
    }
    inline std::vector<uint8_t> read_all_bytes(const std::string_view& path) noexcept {
        std::ifstream file{ path.data(), std::ifstream::in | std::ifstream::binary };
        if (!file.is_open())
            return std::vector<uint8_t>{};
        std::size_t file_size = file.seekg(0, std::ios::end).tellg();
        std::vector<uint8_t> data{};
        data.resize(file_size);

        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(file_size));
        file.close();   

        return data;
    }
    inline bool write_all_bytes(const std::string_view& path, char* data, const std::size_t& data_len) noexcept {
        std::ofstream file{ path.data(), std::ios::out | std::ios::binary };
        if (!file.is_open())
            return false;

        file.seekp(0, std::ios::beg);
        file.write(data, static_cast<std::streamsize>(data_len));
        file.close();   

        return true;
    }
}