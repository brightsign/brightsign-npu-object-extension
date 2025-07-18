#include "transport.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>

FileTransport::FileTransport(const std::string& filepath) 
    : filepath(filepath), enabled(true) {
    // Ensure the directory exists
    std::filesystem::path path(filepath);
    std::filesystem::path dir = path.parent_path();
    
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::error_code ec;
        if (!std::filesystem::create_directories(dir, ec)) {
            std::cerr << "Failed to create directory: " << dir << " - " << ec.message() << std::endl;
            enabled = false;
        }
    }
}

bool FileTransport::send(const std::string& data) {
    if (!enabled) {
        return false;
    }
    
    try {
        // Write atomically by using a temporary file and then renaming
        std::string temp_filepath = filepath + ".tmp";
        
        {
            std::ofstream file(temp_filepath, std::ios::out | std::ios::trunc);
            if (!file.is_open()) {
                std::cerr << "Failed to open temp file: " << temp_filepath << std::endl;
                return false;
            }
            
            file << data;
            file.flush();
            
            if (file.fail()) {
                std::cerr << "Failed to write to temp file: " << temp_filepath << std::endl;
                return false;
            }
            
            // Close the file to ensure all C++ buffers are flushed
            file.close();
            
            // Reopen to get file descriptor and fsync
            int fd = ::open(temp_filepath.c_str(), O_RDONLY);
            if (fd != -1) {
                if (::fsync(fd) != 0) {
                    std::cerr << "Failed to fsync file: " << strerror(errno) << std::endl;
                    ::close(fd);
                    return false;
                }
                ::close(fd);
            }
        } // File closed here
        
        // Atomic rename
        std::error_code ec;
        std::filesystem::rename(temp_filepath, filepath, ec);
        if (ec) {
            std::cerr << "Failed to rename temp file: " << ec.message() << std::endl;
            // Try to clean up temp file
            std::filesystem::remove(temp_filepath, ec);
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in FileTransport::send: " << e.what() << std::endl;
        return false;
    }
}

bool FileTransport::isConnected() const {
    return enabled;
}