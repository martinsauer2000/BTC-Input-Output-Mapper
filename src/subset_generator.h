#ifndef SUBSET_GENERATOR_H
#define SUBSET_GENERATOR_H

#include <vector>
#include <string>
#include "transaction_data.h"

/**
* Enum to specify whether to generate subsets for inputs or outputs
*/
enum class SubsetType {
   INPUTS,
   OUTPUTS
};

/**
* Generates all non-empty subsets (power set without the empty set) of transaction inputs or outputs.
* 
* @param tx_data The transaction data containing inputs and outputs
* @param type Specifies whether to generate subsets for inputs or outputs
* @return A vector of vectors, where each inner vector represents one subset of input/output IDs
*/
std::vector<std::vector<std::string>> generate_subsets(const TransactionData& tx_data, SubsetType type) {
   // Get the appropriate IDs based on the subset type
   const std::vector<std::string>& ids = (type == SubsetType::INPUTS) 
                                       ? tx_data.get_input_ids() 
                                       : tx_data.get_output_ids();
   
   // Calculate the number of elements
   size_t n = ids.size();
   
   // Calculate the total number of non-empty subsets: 2^n - 1
   size_t total_subsets = (1ULL << n) - 1;
   
   // Initialize the result vector
   std::vector<std::vector<std::string>> subsets;
   subsets.reserve(total_subsets);
   
   // Generate all non-empty subsets using binary counting
   for (size_t i = 1; i <= total_subsets; ++i) {
       std::vector<std::string> subset;
       
       // Check each bit position
       for (size_t j = 0; j < n; ++j) {
           // If the jth bit is set, include the jth element
           if (i & (1ULL << j)) {
               subset.push_back(ids[j]);
           }
       }
       
       // Add the subset to our result
       subsets.push_back(subset);
   }
   
   return subsets;
}

/**
* Calculates the sum of values for a given subset of transaction elements.
* 
* @param tx_data The transaction data containing inputs and outputs
* @param subset A vector of IDs representing a subset of inputs or outputs
* @param type Specifies whether the subset contains input or output IDs
* @return The sum of values for the given subset
*/
double calculate_subset_value(const TransactionData& tx_data, 
                            const std::vector<std::string>& subset,
                            SubsetType type) {
   double total = 0.0;
   
   for (const auto& id : subset) {
       if (type == SubsetType::INPUTS) {
           total += tx_data.get_input_value(id);
       } else {
           total += tx_data.get_output_value(id);
       }
   }
   
   return total;
}

/**
* Utility function to print a subset for debugging purposes.
* 
* @param subset A vector of IDs representing a subset of inputs or outputs
* @param tx_data The transaction data containing inputs and outputs
* @param type Specifies whether the subset contains input or output IDs
*/
void print_subset(const std::vector<std::string>& subset, 
                const TransactionData& tx_data,
                SubsetType type) {
   std::cout << "{ ";
   for (size_t i = 0; i < subset.size(); ++i) {
       std::cout << subset[i];
       if (i < subset.size() - 1) {
           std::cout << ", ";
       }
   }
   std::cout << " } = " << calculate_subset_value(tx_data, subset, type) << " BTC" << std::endl;
}

#endif // SUBSET_GENERATOR_H
