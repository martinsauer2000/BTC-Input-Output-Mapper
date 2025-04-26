#ifndef PARTITION_ANALYZER_H
#define PARTITION_ANALYZER_H

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
#include "transaction_data.h"
#include "subset_generator.h"

// Type definitions for clarity
using StringSet = std::vector<std::string>;
using Partition = std::vector<StringSet>;

/**
 * Recursively generates all possible partitions of a set.
 * A partition is a collection of non-empty, disjoint subsets that cover the entire set.
 * 
 * @param elements The set of elements to partition
 * @param current The current partition being built
 * @param remaining The remaining elements to be added to the partition
 * @param result The vector to store all generated partitions
 */
void generate_partitions_recursive(
    const StringSet& elements,
    Partition& current,
    StringSet remaining,
    std::vector<Partition>& result
) {
    // If no elements remain, we have a complete partition
    if (remaining.empty()) {
        result.push_back(current);
        return;
    }
    
    // Take the first remaining element
    std::string element = remaining[0];
    remaining.erase(remaining.begin());
    
    // Option 1: Add the element to each existing subset
    for (size_t i = 0; i < current.size(); ++i) {
        current[i].push_back(element);
        generate_partitions_recursive(elements, current, remaining, result);
        current[i].pop_back();
    }
    
    // Option 2: Create a new subset with just this element
    current.push_back({element});
    generate_partitions_recursive(elements, current, remaining, result);
    current.pop_back();
}

/**
 * Generates all possible partitions of a set.
 * 
 * @param elements The set of elements to partition
 * @return A vector of all possible partitions
 */
std::vector<Partition> generate_partitions(const StringSet& elements) {
    std::vector<Partition> result;
    Partition current;
    
    // Handle empty set
    if (elements.empty()) {
        return result;
    }
    
    // Start with the first element in its own subset
    current.push_back({elements[0]});
    
    // Generate partitions for the remaining elements
    StringSet remaining(elements.begin() + 1, elements.end());
    generate_partitions_recursive(elements, current, remaining, result);
    
    return result;
}

/**
 * Checks if a valid matching exists between input and output partitions.
 * A valid matching means each input subset can be paired with an output subset
 * where output_value <= input_value for EACH pair.
 * 
 * @param tx_data The transaction data
 * @param input_partition A partition of input IDs
 * @param output_partition A partition of output IDs
 * @return true if a valid matching exists, false otherwise
 */
bool is_valid_matching(
    const TransactionData& tx_data,
    const Partition& input_partition,
    const Partition& output_partition
) {
    // If the number of subsets doesn't match, no valid matching is possible
    if (input_partition.size() != output_partition.size()) {
        return false;
    }
    
    // Calculate values for each input subset
    std::vector<double> input_values;
    for (const auto& subset : input_partition) {
        input_values.push_back(calculate_subset_value(tx_data, subset, SubsetType::INPUTS));
    }
    
    // Calculate values for each output subset
    std::vector<double> output_values;
    for (const auto& subset : output_partition) {
        output_values.push_back(calculate_subset_value(tx_data, subset, SubsetType::OUTPUTS));
    }
    
    // Create pairs of (input_value, output_value) and sort by the difference (input - output) in descending order
    std::vector<std::pair<double, double>> value_pairs;
    for (size_t i = 0; i < input_values.size(); ++i) {
        value_pairs.push_back({input_values[i], output_values[i]});
    }
    
    // Sort by the difference (input - output) in descending order
    std::sort(value_pairs.begin(), value_pairs.end(), 
        [](const auto& a, const auto& b) {
            return (a.first - a.second) > (b.first - b.second);
        });
    
    // Check if each output can be matched with an input and ensure no negative differences
    for (const auto& [input_value, output_value] : value_pairs) {
        if (output_value > input_value) {
            return false;  // This output cannot be covered by its input
        }
    }
    
    return true;
}

/**
 * Finds all valid partitions of inputs and outputs in a transaction.
 * A valid partition means the inputs and outputs can be grouped in a way
 * where each input group covers the value of a corresponding output group.
 * 
 * @param tx_data The transaction data
 * @return The number of valid partitions found
 */
size_t find_valid_partitions(const TransactionData& tx_data) {
    // Get input and output IDs
    const auto& input_ids = tx_data.get_input_ids();
    const auto& output_ids = tx_data.get_output_ids();
    
    // Generate all possible partitions
    std::cout << "Generating all possible partitions of inputs..." << std::endl;
    auto input_partitions = generate_partitions(input_ids);
    std::cout << "Generated " << input_partitions.size() << " input partitions." << std::endl;
    
    std::cout << "Generating all possible partitions of outputs..." << std::endl;
    auto output_partitions = generate_partitions(output_ids);
    std::cout << "Generated " << output_partitions.size() << " output partitions." << std::endl;
    
    // Calculate total combinations
    size_t total_combinations = input_partitions.size() * output_partitions.size();
    std::cout << "Total possible combinations: " << total_combinations << std::endl;
    
    // Find valid partitions
    std::cout << "Finding valid partitions..." << std::endl;
    size_t valid_count = 0;
    
    // Check each combination of input and output partitions
    for (const auto& input_partition : input_partitions) {
        for (const auto& output_partition : output_partitions) {
            if (is_valid_matching(tx_data, input_partition, output_partition)) {
                valid_count++;
                
                // Print the valid partition
                std::cout << "\nValid Partition #" << valid_count << ":" << std::endl;
                
                // Print input partition
                std::cout << "  Input Partition:" << std::endl;
                for (size_t i = 0; i < input_partition.size(); ++i) {
                    const auto& subset = input_partition[i];
                    double value = calculate_subset_value(tx_data, subset, SubsetType::INPUTS);
                    
                    std::cout << "    Group " << (i + 1) << ": { ";
                    for (size_t j = 0; j < subset.size(); ++j) {
                        std::cout << subset[j];
                        if (j < subset.size() - 1) {
                            std::cout << ", ";
                        }
                    }
                    std::cout << " } = " << value << " BTC" << std::endl;
                }
                
                // Print output partition
                std::cout << "  Output Partition:" << std::endl;
                for (size_t i = 0; i < output_partition.size(); ++i) {
                    const auto& subset = output_partition[i];
                    double value = calculate_subset_value(tx_data, subset, SubsetType::OUTPUTS);
                    
                    std::cout << "    Group " << (i + 1) << ": { ";
                    for (size_t j = 0; j < subset.size(); ++j) {
                        std::cout << subset[j];
                        if (j < subset.size() - 1) {
                            std::cout << ", ";
                        }
                    }
                    std::cout << " } = " << value << " BTC" << std::endl;
                }
                
                // Print the differences
                std::cout << "  Differences:" << std::endl;
                double total_input = 0.0;
                double total_output = 0.0;
                bool has_negative_difference = false;
                
                for (size_t i = 0; i < input_partition.size(); ++i) {
                    double input_value = calculate_subset_value(tx_data, input_partition[i], SubsetType::INPUTS);
                    double output_value = calculate_subset_value(tx_data, output_partition[i], SubsetType::OUTPUTS);
                    double difference = input_value - output_value;
                    
                    if (difference < 0) {
                        has_negative_difference = true;
                    }
                    
                    std::cout << "    Group " << (i + 1) << ": " << difference << " BTC" << std::endl;
                    
                    total_input += input_value;
                    total_output += output_value;
                }
                
                std::cout << "  Total Difference: " << (total_input - total_output) << " BTC" << std::endl;
                
                // Double-check for negative differences (this should never happen with the corrected is_valid_matching)
                if (has_negative_difference) {
                    std::cout << "  WARNING: This partition has negative differences and should not be considered valid!" << std::endl;
                }
            }
        }
    }
    
    std::cout << "\nTotal valid partitions found: " << valid_count << std::endl;
    return valid_count;
}

#endif // PARTITION_ANALYZER_H
