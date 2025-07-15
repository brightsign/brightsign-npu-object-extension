#include <iostream>
#include <cassert>
#include <vector>
#include <unordered_map>
#include "utils.h"

// Test utilities
void test_loadCocoClassMapping() {
    std::cout << "Testing loadCocoClassMapping..." << std::endl;
    
    // Test with our test data
    auto class_mapping = loadCocoClassMapping("data/coco_80_labels_list.txt");
    
    // Test some known classes
    assert(class_mapping.find("person") != class_mapping.end());
    assert(class_mapping["person"] == 0);
    
    assert(class_mapping.find("car") != class_mapping.end());
    assert(class_mapping["car"] == 2);
    
    assert(class_mapping.find("dog") != class_mapping.end());
    assert(class_mapping["dog"] == 16);
    
    assert(class_mapping.size() == 80);  // Should have 80 classes
    
    std::cout << "✓ loadCocoClassMapping test passed" << std::endl;
}

void test_parseClassNames() {
    std::cout << "Testing parseClassNames..." << std::endl;
    
    // Load class mapping
    auto class_mapping = loadCocoClassMapping("data/coco_80_labels_list.txt");
    
    // Test single class
    auto result = parseClassNames("person", class_mapping);
    assert(result.size() == 1);
    assert(result[0] == 0);
    
    // Test multiple classes
    result = parseClassNames("person,car,dog", class_mapping);
    assert(result.size() == 3);
    assert(result[0] == 0);  // person
    assert(result[1] == 2);  // car
    assert(result[2] == 16); // dog
    
    // Test with spaces
    result = parseClassNames("person, car , dog", class_mapping);
    assert(result.size() == 3);
    assert(result[0] == 0);  // person
    assert(result[1] == 2);  // car
    assert(result[2] == 16); // dog
    
    // Test empty string
    result = parseClassNames("", class_mapping);
    assert(result.empty());
    
    // Test with invalid class
    result = parseClassNames("person,invalid_class,car", class_mapping);
    assert(result.size() == 2);  // Should skip invalid class
    assert(result[0] == 0);  // person
    assert(result[1] == 2);  // car
    
    std::cout << "✓ parseClassNames test passed" << std::endl;
}

void test_isClassSelected() {
    std::cout << "Testing isClassSelected..." << std::endl;
    
    // Test with empty vector (all classes selected)
    std::vector<int> empty_selection;
    assert(isClassSelected(0, empty_selection) == true);
    assert(isClassSelected(5, empty_selection) == true);
    assert(isClassSelected(79, empty_selection) == true);
    
    // Test with specific classes
    std::vector<int> selected_classes = {0, 2, 16};  // person, car, dog
    assert(isClassSelected(0, selected_classes) == true);   // person
    assert(isClassSelected(2, selected_classes) == true);   // car
    assert(isClassSelected(16, selected_classes) == true);  // dog
    assert(isClassSelected(1, selected_classes) == false);  // bicycle
    assert(isClassSelected(3, selected_classes) == false);  // motorcycle
    
    std::cout << "✓ isClassSelected test passed" << std::endl;
}

int main() {
    std::cout << "Running class parsing tests..." << std::endl;
    
    try {
        test_loadCocoClassMapping();
        test_parseClassNames();
        test_isClassSelected();
        
        std::cout << "All tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}