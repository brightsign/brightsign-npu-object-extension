#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <unordered_map>

void copyDirectoryFiles(const std::string& sourceDir, const std::string& destinationDir);
bool createFolder(const std::string& folderPath);
void deleteFilesInFolder(const std::string& folderPath);
void moveDirectoryFiles(const std::string& sourceDir, const std::string& destinationDir);

// Class name parsing utilities
std::unordered_map<std::string, int> loadCocoClassMapping(const std::string& labels_file_path);
std::vector<int> parseClassNames(const std::string& classes_str, const std::unordered_map<std::string, int>& class_mapping);
bool isClassSelected(int class_id, const std::vector<int>& selected_classes);

#endif // UTILS_H