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
 * Checks if a specific mapping between input and output partition groups is valid.
 * A valid mapping means each input group has sufficient value to cover its corresponding output group.
 * 
 * @param tx_data The transaction data
 * @param input_partition A partition of input IDs
 * @param output_partition A partition of output IDs with groups in a specific order
 * @return true if the mapping is valid, false otherwise
 */
bool is_valid_mapping(
    const TransactionData& tx_data,
    const Partition& input_partition,
    const Partition& output_partition
) {
    // If the number of groups doesn't match, no valid mapping is possible
    if (input_partition.size() != output_partition.size()) {
        return false;
    }
    
    // Check each group pair
    for (size_t i = 0; i < input_partition.size(); ++i) {
        double input_value = calculate_subset_value(tx_data, input_partition[i], SubsetType::INPUTS);
        double output_value = calculate_subset_value(tx_data, output_partition[i], SubsetType::OUTPUTS);
        
        // If any output group exceeds its input group, the mapping is invalid
        if (output_value > input_value) {
            return false;
        }
    }
    
    return true;
}

/**
 * Generates all permutations of a partition and checks each one for validity.
 * 
 * @param tx_data The transaction data
 * @param input_partition A partition of input IDs
 * @param output_partition A partition of output IDs
 * @param valid_count Reference to the counter for valid mappings
 */
void check_all_permutations(
    const TransactionData& tx_data,
    const Partition& input_partition,
    Partition output_partition,
    size_t& valid_count
) {
    // Create indices for permutation
    std::vector<size_t> indices(output_partition.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = i;
    }
    
    // Generate all permutations of indices
    do {
        // Create permuted output partition
        Partition permuted_output;
        permuted_output.resize(output_partition.size());
        
        for (size_t i = 0; i < indices.size(); ++i) {
            permuted_output[i] = output_partition[indices[i]];
        }
        
        // Check if this mapping is valid
        if (is_valid_mapping(tx_data, input_partition, permuted_output)) {
            valid_count++;
            
            // Print the valid partition and mapping
            std::cout << "\nValid Partition and Mapping #" << valid_count << ":" << std::endl;
            
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
            for (size_t i = 0; i < permuted_output.size(); ++i) {
                const auto& subset = permuted_output[i];
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
            
            // Print the mapping
            std::cout << "  Mapping:" << std::endl;
            for (size_t i = 0; i < input_partition.size(); ++i) {
                double input_value = calculate_subset_value(tx_data, input_partition[i], SubsetType::INPUTS);
                double output_value = calculate_subset_value(tx_data, permuted_output[i], SubsetType::OUTPUTS);
                double difference = input_value - output_value;
                
                std::cout << "    Input Group " << (i + 1) << " -> Output Group " << (i + 1) << std::endl;
                std::cout << "      Input: { ";
                for (size_t j = 0; j < input_partition[i].size(); ++j) {
                    std::cout << input_partition[i][j];
                    if (j < input_partition[i].size() - 1) {
                        std::cout << ", ";
                    }
                }
                std::cout << " } = " << input_value << " BTC" << std::endl;
                
                std::cout << "      Output: { ";
                for (size_t j = 0; j < permuted_output[i].size(); ++j) {
                    std::cout << permuted_output[i][j];
                    if (j < permuted_output[i].size() - 1) {
                        std::cout << ", ";
                    }
                }
                std::cout << " } = " << output_value << " BTC" << std::endl;
                
                std::cout << "      Difference: " << difference << " BTC" << std::endl;
            }
            
            // Print the total difference
            double total_input = 0.0;
            double total_output = 0.0;
            
            for (size_t i = 0; i < input_partition.size(); ++i) {
                total_input += calculate_subset_value(tx_data, input_partition[i], SubsetType::INPUTS);
                total_output += calculate_subset_value(tx_data, permuted_output[i], SubsetType::OUTPUTS);
            }
            
            std::cout << "  Total Difference: " << (total_input - total_output) << " BTC" << std::endl;
        }
    } while (std::next_permutation(indices.begin(), indices.end()));
}

/**
 * Finds all valid partitions and mappings of inputs and outputs in a transaction.
 * A valid partition and mapping means the inputs and outputs can be grouped in a way
 * where each input group covers the value of a corresponding output group.
 * 
 * @param tx_data The transaction data
 * @return The number of valid partitions and mappings found
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
    
    // Calculate total partition combinations
    size_t total_combinations = input_partitions.size() * output_partitions.size();
    std::cout << "Total possible partition combinations: " << total_combinations << std::endl;
    
    // Find valid partitions and mappings
    std::cout << "Finding valid partitions and mappings..." << std::endl;
    size_t valid_count = 0;
    
    // Check each combination of input and output partitions
    for (const auto& input_partition : input_partitions) {
        for (const auto& output_partition : output_partitions) {
            // Skip if the number of groups doesn't match
            if (input_partition.size() != output_partition.size()) {
                continue;
            }
            
            // Check all permutations of this output partition
            check_all_permutations(tx_data, input_partition, output_partition, valid_count);
        }
    }
    
    std::cout << "\nTotal valid partitions and mappings found: " << valid_count << std::endl;
    return valid_count;
}

#endif // PARTITION_ANALYZER_H
