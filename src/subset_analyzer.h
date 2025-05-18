#ifndef SUBSET_ANALYZER_H
#define SUBSET_ANALYZER_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include "transaction_data.h"
#include "subset_generator.h"

/**
* Finds valid combinations of input and output subsets and writes them to a file.
* A combination is considered valid if the total value of the output subset
* is less than or equal to the total value of the input subset.
* 
* @param tx_data The transaction data containing inputs and outputs
* @param input_subsets A vector of input subset vectors
* @param output_subsets A vector of output subset vectors
* @param output_filename The name of the file to write results to
* @return The number of valid combinations found
*/
size_t find_valid_combinations(
   const TransactionData& tx_data,
   const std::vector<std::vector<std::string>>& input_subsets,
   const std::vector<std::vector<std::string>>& output_subsets,
   const std::string& output_filename = "valid_combinations.csv"
) {
   size_t valid_count = 0;
   
   std::cout << "Finding valid combinations of input and output subsets..." << std::endl;
   std::cout << "A combination is valid if output_value <= input_value" << std::endl;
   std::cout << "Results will be written to: " << output_filename << std::endl;
   std::cout << "-----------------------------------------------------------" << std::endl;
   
   // Open output file
   std::ofstream output_file(output_filename);
   if (!output_file.is_open()) {
       std::cerr << "Error: Could not open output file " << output_filename << std::endl;
       return 0;
   }
   
   // Write CSV header
   output_file << "Combination_ID,Input_Subset,Input_Value,Output_Subset,Output_Value,Difference\n";
   
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
               
               // Format input subset as string
               std::string input_str = "\"";
               for (size_t i = 0; i < input_subset.size(); ++i) {
                   input_str += input_subset[i];
                   if (i < input_subset.size() - 1) {
                       input_str += ",";
                   }
               }
               input_str += "\"";
               
               // Format output subset as string
               std::string output_str = "\"";
               for (size_t i = 0; i < output_subset.size(); ++i) {
                   output_str += output_subset[i];
                   if (i < output_subset.size() - 1) {
                       output_str += ",";
                   }
               }
               output_str += "\"";
               
               // Write to CSV file
               output_file << valid_count << ","
                          << input_str << ","
                          << input_value << ","
                          << output_str << ","
                          << output_value << ","
                          << (input_value - output_value) << "\n";
               
               // Periodically flush to ensure data is written
               if (valid_count % 1000 == 0) {
                   output_file.flush();
               }
           }
       }
   }
   
   // Close the file
   output_file.close();
   
   std::cout << "-----------------------------------------------------------" << std::endl;
   std::cout << "Total valid combinations found: " << valid_count << std::endl;
   std::cout << "Results have been written to: " << output_filename << std::endl;
   
   return valid_count;
}

/**
* Overloaded version that generates the subsets internally.
* 
* @param tx_data The transaction data containing inputs and outputs
* @param output_filename The name of the file to write results to
* @return The number of valid combinations found
*/
size_t find_valid_combinations(const TransactionData& tx_data, const std::string& output_filename = "valid_combinations.csv") {
   // Generate all non-empty subsets of inputs and outputs
   auto input_subsets = generate_subsets(tx_data, SubsetType::INPUTS);
   auto output_subsets = generate_subsets(tx_data, SubsetType::OUTPUTS);
   
   // Find valid combinations and write to file
   return find_valid_combinations(tx_data, input_subsets, output_subsets, output_filename);
}

#endif // SUBSET_ANALYZER_H
