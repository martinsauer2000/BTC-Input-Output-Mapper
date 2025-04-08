#ifndef SUBSET_ANALYZER_H
#define SUBSET_ANALYZER_H

#include <iostream>
#include <vector>
#include <string>
#include "transaction_data.h"
#include "subset_generator.h"

/**
 * Finds and prints valid combinations of input and output subsets.
 * A combination is considered valid if the total value of the output subset
 * is less than or equal to the total value of the input subset.
 * 
 * @param tx_data The transaction data containing inputs and outputs
 * @param input_subsets A vector of input subset vectors
 * @param output_subsets A vector of output subset vectors
 * @return The number of valid combinations found
 */
size_t find_valid_combinations(
    const TransactionData& tx_data,
    const std::vector<std::vector<std::string>>& input_subsets,
    const std::vector<std::vector<std::string>>& output_subsets
) {
    size_t valid_count = 0;
    
    std::cout << "Finding valid combinations of input and output subsets..." << std::endl;
    std::cout << "A combination is valid if output_value <= input_value" << std::endl;
    std::cout << "-----------------------------------------------------------" << std::endl;
    
    // Iterate through all input subsets
    for (const auto& input_subset : input_subsets) {
        // Calculate the total value of this input subset
        double input_value = calculate_subset_value(tx_data, input_subset, SubsetType::INPUTS);
        
        // Iterate through all output subsets
        for (const auto& output_subset : output_subsets) {
            // Calculate the total value of this output subset
            double output_value = calculate_subset_value(tx_data, output_subset, SubsetType::OUTPUTS);
            
            // Check if this is a valid combination (output value <= input value)
            if (output_value <= input_value) {
                valid_count++;
                
                // Print the valid combination
                std::cout << "Valid Combination #" << valid_count << ":" << std::endl;
                
                // Print input subset
                std::cout << "  Input Subset: { ";
                for (size_t i = 0; i < input_subset.size(); ++i) {
                    std::cout << input_subset[i];
                    if (i < input_subset.size() - 1) {
                        std::cout << ", ";
                    }
                }
                std::cout << " } = " << input_value << " BTC" << std::endl;
                
                // Print output subset
                std::cout << "  Output Subset: { ";
                for (size_t i = 0; i < output_subset.size(); ++i) {
                    std::cout << output_subset[i];
                    if (i < output_subset.size() - 1) {
                        std::cout << ", ";
                    }
                }
                std::cout << " } = " << output_value << " BTC" << std::endl;
                
                // Print the difference (changed from "Fee" to "Difference")
                std::cout << "  Difference: " << (input_value - output_value) << " BTC" << std::endl;
                std::cout << std::endl;
            }
        }
    }
    
    std::cout << "-----------------------------------------------------------" << std::endl;
    std::cout << "Total valid combinations found: " << valid_count << std::endl;
    
    return valid_count;
}

/**
 * Overloaded version that generates the subsets internally.
 * 
 * @param tx_data The transaction data containing inputs and outputs
 * @return The number of valid combinations found
 */
size_t find_valid_combinations(const TransactionData& tx_data) {
    // Generate all non-empty subsets of inputs and outputs
    auto input_subsets = generate_subsets(tx_data, SubsetType::INPUTS);
    auto output_subsets = generate_subsets(tx_data, SubsetType::OUTPUTS);
    
    // Find and print valid combinations
    return find_valid_combinations(tx_data, input_subsets, output_subsets);
}

#endif // SUBSET_ANALYZER_H
