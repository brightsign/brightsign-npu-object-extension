#include "utils.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

void copyDirectoryFiles(const std::string& sourceDir, const std::string& destinationDir) {
    try {
        fs::create_directories(destinationDir);

        for (const auto& entry : fs::directory_iterator(sourceDir)) {
            const auto& path = entry.path();
            const auto destPath = fs::path(destinationDir) / path.filename();

            if (fs::is_regular_file(path)) {
                fs::copy_file(path, destPath, fs::copy_options::overwrite_existing);
                // std::cout << "Copied: " << path << " to " << destPath << std::endl;
            }
        }
        // std::cout << "All files copied successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void moveDirectoryFiles(const std::string& sourceDir, const std::string& destinationDir) {
    try {
        // Create destination directory if it doesn't exist
        fs::create_directories(destinationDir);
        
        // Iterate through files in source directory
        for (const auto& entry : fs::directory_iterator(sourceDir)) {
            const auto& path = entry.path();
            const auto destPath = fs::path(destinationDir) / path.filename();
            
            if (fs::is_regular_file(path)) {
                // Check if destination file already exists
                if (fs::exists(destPath)) {
                    fs::remove(destPath);  // Remove existing file to avoid rename failure
                }
                
                fs::rename(path, destPath);
                // std::cout << "Moved: " << path << " to " << destPath << std::endl;
            }
        }
        // std::cout << "All files moved successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

bool createFolder(const std::string& folderPath) {
    try {
        if (!fs::exists(folderPath)) {
            if (fs::create_directories(folderPath)) {
                std::cout << "Folder created: " << folderPath << std::endl;
                return true;
            } else {
                std::cerr << "Failed to create folder: " << folderPath << std::endl;
                return false;
            }
        } else {
            std::cout << "Folder already exists: " << folderPath << std::endl;
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error creating folder: " << e.what() << std::endl;
        return false;
    }
}

void deleteFilesInFolder(const std::string& folderPath) {
    try {
        if (!fs::exists(folderPath)) {
            std::cerr << "Folder does not exist: " << folderPath << std::endl;
            return;
        }

        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (fs::is_regular_file(entry)) {
                fs::remove(entry.path());
                // std::cout << "Deleted file: " << entry.path() << std::endl;
            }
        }
        // std::cout << "All files in the folder have been deleted." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error deleting files: " << e.what() << std::endl;
    }
}

// Load COCO class mapping from labels file
std::unordered_map<std::string, int> loadCocoClassMapping(const std::string& labels_file_path) {
    std::unordered_map<std::string, int> class_mapping;
    std::ifstream file(labels_file_path);
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open labels file: " << labels_file_path << std::endl;
        return class_mapping;
    }
    
    std::string line;
    int class_id = 0;
    
    while (std::getline(file, line)) {
        // Remove trailing whitespace and newlines
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), line.end());
        
        if (!line.empty()) {
            class_mapping[line] = class_id;
            class_id++;
        }
    }
    
    file.close();
    return class_mapping;
}

// Parse comma-separated class names and return their IDs
std::vector<int> parseClassNames(const std::string& classes_str, const std::unordered_map<std::string, int>& class_mapping) {
    std::vector<int> selected_classes;
    
    if (classes_str.empty()) {
        return selected_classes;  // Empty vector means all classes selected
    }
    
    std::stringstream ss(classes_str);
    std::string class_name;
    
    while (std::getline(ss, class_name, ',')) {
        // Remove leading and trailing whitespace
        class_name.erase(0, class_name.find_first_not_of(" \t"));
        class_name.erase(class_name.find_last_not_of(" \t") + 1);
        
        if (!class_name.empty()) {
            auto it = class_mapping.find(class_name);
            if (it != class_mapping.end()) {
                selected_classes.push_back(it->second);
                std::cout << "Selected class: " << class_name << " (ID: " << it->second << ")" << std::endl;
            } else {
                std::cerr << "Warning: Unknown class name '" << class_name << "', ignoring." << std::endl;
            }
        }
    }
    
    return selected_classes;
}

// Check if a class ID is in the selected classes list
bool isClassSelected(int class_id, const std::vector<int>& selected_classes) {
    // If no classes specified, all classes are selected
    if (selected_classes.empty()) {
        return true;
    }
    
    // Check if class_id is in the selected_classes vector
    return std::find(selected_classes.begin(), selected_classes.end(), class_id) != selected_classes.end();
}