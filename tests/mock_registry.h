#pragma once

#include <string>
#include <cstring>

// Mock registry functions for testing
void mock_registry_set(const std::string& key, const std::string& value);
std::string mock_registry_get(const std::string& key);
void mock_registry_clear();

// Mock registry command
extern "C" {
    int mock_registry_command(const char* section, const char* key, char* result, size_t result_size);
}