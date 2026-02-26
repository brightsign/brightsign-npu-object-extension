#include "mock_registry.h"
#include <map>
#include <string>

// Mock registry storage
static std::map<std::string, std::string> mock_registry_data;

// Mock registry functions for testing
void mock_registry_set(const std::string& key, const std::string& value) {
    mock_registry_data[key] = value;
}

std::string mock_registry_get(const std::string& key) {
    auto it = mock_registry_data.find(key);
    if (it != mock_registry_data.end()) {
        return it->second;
    }
    return "";
}

void mock_registry_clear() {
    mock_registry_data.clear();
}

// Mock the registry command for testing
extern "C" {
    // This function mimics the registry command behavior
    int mock_registry_command(const char* section, const char* key, char* result, size_t result_size) {
        std::string full_key = std::string(section) + "-" + std::string(key);
        std::string value = mock_registry_get(full_key);
        
        if (value.empty()) {
            return -1;  // Not found
        }
        
        if (value.length() >= result_size) {
            return -1;  // Buffer too small
        }
        
        strcpy(result, value.c_str());
        return 0;  // Success
    }
}