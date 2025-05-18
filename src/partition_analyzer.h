#ifndef PARTITION_ANALYZER_H
#define PARTITION_ANALYZER_H

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <thread>
#include <future>
#include <mutex>
#include <atomic>
#include <iomanip>  // For std::setprecision
#include <cmath>    // For std::min
#include <fstream>  // For file output
#include <sstream>  // For string stream
#include "transaction_data.h"
#include "subset_generator.h"

// Memory-efficient type definitions
using ElementIndex = uint16_t;
using IndexSet = std::vector<ElementIndex>;
using IndexPartition = std::vector<IndexSet>;

/**
* Struct to hold element mappings between strings and indices
*/
struct ElementMapper {
   std::vector<std::string> elements;
   std::unordered_map<std::string, ElementIndex> element_to_index;
   
   ElementMapper(const std::vector<std::string>& element_list) {
       elements = element_list;
       for (ElementIndex i = 0; i < elements.size(); ++i) {
           element_to_index[elements[i]] = i;
       }
   }
   
   // Convert index set back to string set
   std::vector<std::string> to_string_set(const IndexSet& indices) const {
       std::vector<std::string> result;
       result.reserve(indices.size());
       for (const auto& idx : indices) {
           result.push_back(elements[idx]);
       }
       return result;
   }
   
   // Convert index partition back to string partition
   std::vector<std::vector<std::string>> to_string_partition(const IndexPartition& partition) const {
       std::vector<std::vector<std::string>> result;
       result.reserve(partition.size());
       for (const auto& set : partition) {
           result.push_back(to_string_set(set));
       }
       return result;
   }
};

/**
* Generates Bell triangle for efficient partition generation.
* The Bell triangle is used to enumerate all partitions of a set.
* 
* @param n The size of the set
* @return A 2D vector representing the Bell triangle
*/
std::vector<std::vector<size_t>> generate_bell_triangle(size_t n) {
   if (n == 0) return {};
   
   std::vector<std::vector<size_t>> triangle(n);
   
   // First row is always [1]
   triangle[0] = {1};
   
   // Generate the rest of the triangle
   for (size_t i = 1; i < n; ++i) {
       triangle[i].resize(i + 1);
       
       // First number in row is the last number of the previous row
       triangle[i][0] = triangle[i-1][i-1];
       
       // Rest of the numbers in the row
       for (size_t j = 1; j <= i; ++j) {
           triangle[i][j] = triangle[i][j-1] + triangle[i-1][j-1];
       }
   }
   
   return triangle;
}

/**
* Class to generate partitions in chunks to reduce memory usage
*/
class PartitionGenerator {
private:
   std::vector<ElementIndex> elements;
   size_t current_idx;
   size_t max_partitions;
   size_t elements_size;
   
   // Generate a chunk of partitions using iterative approach
   std::vector<IndexPartition> generate_partitions_chunk(size_t chunk_size) {
       std::vector<IndexPartition> result;
       
       if (elements.empty()) {
           return result;
       }
       
       if (elements.size() == 1) {
           if (current_idx == 0) {
               result.push_back({{elements[0]}});
               current_idx = 1;
           }
           return result;
       }
       
       // Initialize with the first element in its own subset
       std::vector<IndexPartition> current_level = {{{elements[0]}}};
       
       // Process each remaining element
       for (size_t i = 1; i < elements.size(); ++i) {
           std::vector<IndexPartition> next_level;
           
           // For each partition at the current level
           for (const auto& partition : current_level) {
               // Option 1: Add the element to each existing subset
               for (size_t j = 0; j < partition.size(); ++j) {
                   IndexPartition new_partition = partition;
                   new_partition[j].push_back(elements[i]);
                   next_level.push_back(new_partition);
                   
                   // If we've reached our chunk size, return what we have
                   if (result.size() + next_level.size() >= chunk_size) {
                       result.insert(result.end(), next_level.begin(), next_level.end());
                       current_idx += result.size();
                       return result;
                   }
               }
               
               // Option 2: Create a new subset with just this element
               IndexPartition new_partition = partition;
               new_partition.push_back({elements[i]});
               next_level.push_back(new_partition);
               
               // If we've reached our chunk size, return what we have
               if (result.size() + next_level.size() >= chunk_size) {
                   result.insert(result.end(), next_level.begin(), next_level.end());
                       current_idx += result.size();
                       return result;
                   }
               }
           
           // Move to the next level
           current_level = std::move(next_level);
       }
       
       // Add all partitions from the final level
       result.insert(result.end(), current_level.begin(), current_level.end());
       current_idx += result.size();
       
       return result;
   }
   
public:
   PartitionGenerator(const std::vector<ElementIndex>& elems) 
       : elements(elems), current_idx(0), elements_size(elems.size()) {
       // Calculate Bell number to know total partitions
       std::vector<size_t> bell_numbers(elements_size + 1, 0);
       bell_numbers[0] = 1;
       
       for (size_t i = 1; i <= elements_size; ++i) {
           bell_numbers[i] = 0;
           for (size_t j = 0; j < i; ++j) {
               bell_numbers[i] += bell_numbers[j] * binomial_coefficient(i - 1, j);
           }
       }
       
       max_partitions = bell_numbers[elements_size];
   }
   
   // Helper function to calculate binomial coefficient
   size_t binomial_coefficient(size_t n, size_t k) {
       if (k > n) return 0;
       if (k == 0 || k == n) return 1;
       
       size_t result = 1;
       for (size_t i = 1; i <= k; ++i) {
           result *= (n - (k - i));
           result /= i;
       }
       
       return result;
   }
   
   // Check if more partitions are available
   bool has_more() const {
       return current_idx < max_partitions;
   }
   
   // Get next chunk of partitions
   std::vector<IndexPartition> next_chunk(size_t chunk_size) {
       if (!has_more()) {
           return {};
       }
       
       return generate_partitions_chunk(chunk_size);
   }
   
   // Reset the generator
   void reset() {
       current_idx = 0;
   }
   
   // Get total number of partitions
   size_t total_partitions() const {
       return max_partitions;
   }
   
   // Get current progress
   size_t current_progress() const {
       return current_idx;
   }
};

/**
* Performs value-based pruning to quickly determine if a partition pair could possibly be valid.
* Sorts group values in descending order and checks if any output group exceeds its corresponding input group.
* 
* @param tx_data The transaction data
* @param input_partition A partition of input indices
* @param output_partition A partition of output indices
* @param input_mapper Mapper for input elements
* @param output_mapper Mapper for output elements
* @return true if the partition pair might be valid, false if it's definitely invalid
*/
bool could_have_valid_mapping(
   const TransactionData& tx_data,
   const IndexPartition& input_partition,
   const IndexPartition& output_partition,
   const ElementMapper& input_mapper,
   const ElementMapper& output_mapper
) {
   // If the number of groups doesn't match, no valid mapping is possible
   if (input_partition.size() != output_partition.size()) {
       return false;
   }
   
   // Calculate and store the value of each input group
   std::vector<double> input_values;
   input_values.reserve(input_partition.size());
   
   for (const auto& group : input_partition) {
       auto input_set = input_mapper.to_string_set(group);
       double group_value = calculate_subset_value(tx_data, input_set, SubsetType::INPUTS);
       input_values.push_back(group_value);
   }
   
   // Calculate and store the value of each output group
   std::vector<double> output_values;
   output_values.reserve(output_partition.size());
   
   for (const auto& group : output_partition) {
       auto output_set = output_mapper.to_string_set(group);
       double group_value = calculate_subset_value(tx_data, output_set, SubsetType::OUTPUTS);
       output_values.push_back(group_value);
   }
   
   // Sort both value lists in descending order
   std::sort(input_values.begin(), input_values.end(), std::greater<double>());
   std::sort(output_values.begin(), output_values.end(), std::greater<double>());
   
   // Check if any output group value exceeds its corresponding input group value
   for (size_t i = 0; i < output_values.size(); ++i) {
       if (output_values[i] > input_values[i]) {
           // This partition pair can never be valid, no matter the permutation
           return false;
       }
   }
   
   // This partition pair might have valid mappings
   return true;
}

/**
* Checks if a specific mapping between input and output partition groups is valid.
* Uses indices for memory efficiency.
* 
* @param tx_data The transaction data
* @param input_partition A partition of input indices
* @param output_partition A partition of output indices
* @param input_mapper Mapper for input elements
* @param output_mapper Mapper for output elements
* @return true if the mapping is valid, false otherwise
*/
bool is_valid_mapping(
   const TransactionData& tx_data,
   const IndexPartition& input_partition,
   const IndexPartition& output_partition,
   const ElementMapper& input_mapper,
   const ElementMapper& output_mapper
) {
   // If the number of groups doesn't match, no valid mapping is possible
   if (input_partition.size() != output_partition.size()) {
       return false;
   }
   
   // Check each group pair
   for (size_t i = 0; i < input_partition.size(); ++i) {
       // Convert index sets to string sets for value calculation
       auto input_set = input_mapper.to_string_set(input_partition[i]);
       auto output_set = output_mapper.to_string_set(output_partition[i]);
       
       double input_value = calculate_subset_value(tx_data, input_set, SubsetType::INPUTS);
       double output_value = calculate_subset_value(tx_data, output_set, SubsetType::OUTPUTS);
       
       // If any output group exceeds its input group, the mapping is invalid
       if (output_value > input_value) {
           return false;
       }
   }
   
   return true;
}

/**
* Formats a mapping for CSV output
* 
* @param tx_data The transaction data
* @param input_partition A partition of input indices
* @param output_partition A partition of output indices
* @param indices Permutation indices
* @param input_mapper Mapper for input elements
* @param output_mapper Mapper for output elements
* @param mapping_idx The index of this mapping
* @return A string containing the CSV-formatted mapping
*/
std::string format_mapping_for_csv(
   const TransactionData& tx_data,
   const IndexPartition& input_partition,
   const IndexPartition& output_partition,
   const std::vector<size_t>& indices,
   const ElementMapper& input_mapper,
   const ElementMapper& output_mapper,
   size_t mapping_idx
) {
   std::stringstream ss;
   
   // Convert index partitions back to string partitions for output
   auto input_string_partition = input_mapper.to_string_partition(input_partition);
   auto output_string_partition = output_mapper.to_string_partition(output_partition);
   
   // Calculate total values
   double total_input = 0.0;
   double total_output = 0.0;
   
   for (size_t i = 0; i < input_string_partition.size(); ++i) {
       total_input += calculate_subset_value(tx_data, input_string_partition[i], SubsetType::INPUTS);
       total_output += calculate_subset_value(tx_data, output_string_partition[i], SubsetType::OUTPUTS);
   }
   
   // Write mapping header
   ss << mapping_idx << ",";
   ss << input_partition.size() << ","; // Number of groups
   ss << total_input << ",";
   ss << total_output << ",";
   ss << (total_input - total_output) << "\n";
   
   // Write each group mapping
   for (size_t i = 0; i < input_string_partition.size(); ++i) {
       double input_value = calculate_subset_value(tx_data, input_string_partition[i], SubsetType::INPUTS);
       double output_value = calculate_subset_value(tx_data, output_string_partition[i], SubsetType::OUTPUTS);
       double difference = input_value - output_value;
       
       // Group number
       ss << mapping_idx << "," << i << ",";
       
       // Input group
       ss << "\"";
       for (size_t j = 0; j < input_string_partition[i].size(); ++j) {
           ss << input_string_partition[i][j];
           if (j < input_string_partition[i].size() - 1) {
               ss << ",";
           }
       }
       ss << "\",";
       
       // Input value
       ss << input_value << ",";
       
       // Output group
       ss << "\"";
       for (size_t j = 0; j < output_string_partition[i].size(); ++j) {
           ss << output_string_partition[i][j];
           if (j < output_string_partition[i].size() - 1) {
               ss << ",";
           }
       }
       ss << "\",";
       
       // Output value and difference
       ss << output_value << "," << difference << "\n";
   }
   
   return ss.str();
}

/**
* Generates all permutations of a partition and checks each one for validity.
* Writes valid mappings directly to file.
* 
* @param tx_data The transaction data
* @param input_partition A partition of input indices
* @param output_partition A partition of output indices
* @param input_mapper Mapper for input elements
* @param output_mapper Mapper for output elements
* @param valid_count Reference to the counter for valid mappings
* @param file_mutex Mutex for thread-safe file access
* @param output_file Reference to the output file stream
*/
void check_all_permutations(
   const TransactionData& tx_data,
   const IndexPartition& input_partition,
   IndexPartition output_partition,
   const ElementMapper& input_mapper,
   const ElementMapper& output_mapper,
   std::atomic<size_t>& valid_count,
   std::mutex& file_mutex,
   std::ofstream& output_file
) {
   // Create indices for permutation
   std::vector<size_t> indices(output_partition.size());
   for (size_t i = 0; i < indices.size(); ++i) {
       indices[i] = i;
   }
   
   // Generate all permutations of indices
   do {
       // Create permuted output partition
       IndexPartition permuted_output;
       permuted_output.resize(output_partition.size());
       
       for (size_t i = 0; i < indices.size(); ++i) {
           permuted_output[i] = output_partition[indices[i]];
       }
       
       // Check if this mapping is valid
       if (is_valid_mapping(tx_data, input_partition, permuted_output, input_mapper, output_mapper)) {
           // Increment the atomic counter
           size_t current_count = valid_count.fetch_add(1) + 1;
           
           // Format the mapping for CSV output
           std::string csv_data = format_mapping_for_csv(
               tx_data, 
               input_partition, 
               permuted_output, 
               indices, 
               input_mapper, 
               output_mapper,
               current_count
           );
           
           // Write to file with mutex protection
           {
               std::lock_guard<std::mutex> lock(file_mutex);
               output_file << csv_data;
               output_file.flush(); // Ensure data is written immediately
           }
       }
   } while (std::next_permutation(indices.begin(), indices.end()));
}

/**
* Processes a batch of partition pairs in parallel.
* Uses indices for memory efficiency.
* 
* @param tx_data The transaction data
* @param partition_pairs Vector of input-output partition pairs to process
* @param input_mapper Mapper for input elements
* @param output_mapper Mapper for output elements
* @param valid_count Reference to the counter for valid mappings
* @param file_mutex Mutex for thread-safe file access
* @param output_file Reference to the output file stream
* @param pruned_count Reference to counter for pruned partition pairs
* @param checked_count Reference to counter for checked partition pairs
*/
void process_partition_batch(
   const TransactionData& tx_data,
   const std::vector<std::pair<IndexPartition, IndexPartition>>& partition_pairs,
   const ElementMapper& input_mapper,
   const ElementMapper& output_mapper,
   std::atomic<size_t>& valid_count,
   std::mutex& file_mutex,
   std::ofstream& output_file,
   std::atomic<size_t>& pruned_count,
   std::atomic<size_t>& checked_count
) {
   for (const auto& [input_partition, output_partition] : partition_pairs) {
       // Skip if the number of groups doesn't match
       if (input_partition.size() != output_partition.size()) {
           continue;
       }
       
       // Apply value-based pruning
       if (!could_have_valid_mapping(tx_data, input_partition, output_partition, input_mapper, output_mapper)) {
           pruned_count.fetch_add(1);
           continue;
       }
       
       // Check all permutations of this output partition
       check_all_permutations(
           tx_data, 
           input_partition, 
           output_partition, 
           input_mapper, 
           output_mapper, 
           valid_count, 
           file_mutex, 
           output_file
       );
       
       // Increment the counter for checked partition pairs
       checked_count.fetch_add(1);
   }
}

/**
* Draws a simple ASCII progress bar.
* 
* @param progress Percentage of progress (0-100)
* @param width Width of the progress bar in characters
* @return String containing the ASCII progress bar
*/
std::string draw_progress_bar(double progress, int width = 20) {
   // Ensure progress is between 0 and 100
   progress = std::max(0.0, std::min(100.0, progress));
   
   // Calculate the number of filled positions
   int filled = static_cast<int>(progress * width / 100.0);
   
   // Create the progress bar
   std::string bar = "[";
   for (int i = 0; i < width; ++i) {
       if (i < filled) {
           bar += "=";
       } else if (i == filled) {
           bar += ">";
       } else {
           bar += " ";
       }
   }
   bar += "]";
   
   return bar;
}

/**
* Calculates Stirling numbers of the second kind.
* S(n,k) = number of ways to partition a set of n objects into k non-empty subsets.
* 
* @param n Number of objects
* @param k Number of non-empty subsets
* @return The Stirling number S(n,k)
*/
size_t stirling_second_kind(size_t n, size_t k) {
    if (k == 0) return (n == 0) ? 1 : 0;
    if (k > n) return 0;
    if (k == 1 || k == n) return 1;
    
    // Use recurrence relation: S(n,k) = k*S(n-1,k) + S(n-1,k-1)
    return k * stirling_second_kind(n-1, k) + stirling_second_kind(n-1, k-1);
}

/**
* Processes chunks of partitions to reduce memory usage.
* Writes valid mappings directly to a CSV file.
* 
* @param tx_data The transaction data
* @param input_mapper Mapper for input elements
* @param output_mapper Mapper for output elements
* @param chunk_size Size of partition chunks to process at once
* @param output_filename Name of the output CSV file
* @return Number of valid mappings found
*/
size_t process_partition_chunks(
   const TransactionData& tx_data,
   const ElementMapper& input_mapper,
   const ElementMapper& output_mapper,
   size_t chunk_size,
   const std::string& output_filename
) {
   // Open output file
   std::ofstream output_file(output_filename);
   if (!output_file.is_open()) {
       std::cerr << "Error: Could not open output file " << output_filename << std::endl;
       return 0;
   }
   
   // Write CSV header
   output_file << "Mapping_ID,Group_Count,Total_Input_Value,Total_Output_Value,Total_Difference\n";
   output_file << "Mapping_ID,Group_Number,Input_Group,Input_Value,Output_Group,Output_Value,Difference\n";
   
   // Convert element IDs to indices
   std::vector<ElementIndex> input_indices(input_mapper.elements.size());
   std::vector<ElementIndex> output_indices(output_mapper.elements.size());
   
   const auto& input_ids = input_mapper.elements;
   const auto& output_ids = output_mapper.elements;

   for (ElementIndex i = 0; i < input_indices.size(); ++i) {
       input_indices[i] = i;
   }
   
   for (ElementIndex i = 0; i < output_indices.size(); ++i) {
       output_indices[i] = i;
   }
   
   // Create partition generators
   PartitionGenerator input_generator(input_indices);
   PartitionGenerator output_generator_for_count(output_indices);
   
   // Calculate total possible combinations
   size_t total_input_partitions = input_generator.total_partitions();
   size_t total_output_partitions = output_generator_for_count.total_partitions();
   
   std::cout << "Total possible input partitions: " << total_input_partitions << std::endl;
   std::cout << "Total possible output partitions: " << total_output_partitions << std::endl;
   
   // Calculate distribution of partitions by group size
   std::vector<size_t> input_partitions_by_size(input_ids.size() + 1, 0);
   std::vector<size_t> output_partitions_by_size(output_ids.size() + 1, 0);
   
   // Calculate using Stirling numbers of the second kind
   for (size_t k = 1; k <= input_ids.size(); ++k) {
       input_partitions_by_size[k] = stirling_second_kind(input_ids.size(), k);
   }
   
   for (size_t k = 1; k <= output_ids.size(); ++k) {
       output_partitions_by_size[k] = stirling_second_kind(output_ids.size(), k);
   }
   
   // Calculate total compatible pairs (only those with matching group counts)
   size_t total_compatible_pairs = 0;
   for (size_t k = 1; k <= std::min(input_ids.size(), output_ids.size()); ++k) {
       // For each group size k, we need to consider all pairs of input and output partitions
       // with exactly k groups
       total_compatible_pairs += input_partitions_by_size[k] * output_partitions_by_size[k];
   }
   
   std::cout << "Estimated compatible pairs to check: " << total_compatible_pairs << std::endl;
   std::cout << "Writing results to: " << output_filename << std::endl;
   
   // Storage for statistics
   std::atomic<size_t> valid_count(0);
   std::atomic<size_t> pruned_count(0);
   std::atomic<size_t> checked_count(0);
   std::mutex file_mutex;
   
   // Determine the number of threads to use
   unsigned int num_threads = std::thread::hardware_concurrency();
   if (num_threads == 0) num_threads = 4; // Default if hardware_concurrency is not available
   num_threads = std::min(num_threads, 16u); // Limit to reasonable number
   
   std::cout << "Using " << num_threads << " threads for parallel processing." << std::endl;
   std::cout << "Processing partitions in chunks of size " << chunk_size << "..." << std::endl;
   
   // Process input partitions in chunks
   size_t pairs_processed = 0;
   
   // For progress tracking
   auto start_time = std::chrono::high_resolution_clock::now();
   auto last_update_time = start_time;
   
   // Main processing loop
   while (input_generator.has_more()) {
       // Get chunk of input partitions
       auto input_chunk = input_generator.next_chunk(chunk_size);
       
       // Reset output generator for each input chunk
       PartitionGenerator output_generator(output_indices);
       
       // Process all output partitions for this input chunk
       while (output_generator.has_more()) {
           // Get chunk of output partitions
           auto output_chunk = output_generator.next_chunk(chunk_size);
           
           // Create partition pairs for this chunk combination
           std::vector<std::pair<IndexPartition, IndexPartition>> partition_pairs;
           for (const auto& input_partition : input_chunk) {
               for (const auto& output_partition : output_chunk) {
                   // Only add pairs with matching group counts
                   if (input_partition.size() == output_partition.size()) {
                       partition_pairs.emplace_back(input_partition, output_partition);
                   }
               }
           }
           
           // If no compatible pairs in this chunk, continue
           if (partition_pairs.empty()) {
               continue;
           }
           
           pairs_processed += partition_pairs.size();
           
           // Process partition pairs in parallel
           if (num_threads <= 1 || partition_pairs.size() <= 1) {
               // If only one thread or one pair, process directly
               process_partition_batch(
                   tx_data,
                   partition_pairs,
                   input_mapper,
                   output_mapper,
                   valid_count,
                   file_mutex,
                   output_file,
                   pruned_count,
                   checked_count
               );
           } else {
               // Divide the work among threads
               std::vector<std::future<void>> futures;
               
               // Calculate batch size for threads
               size_t thread_batch_size = (partition_pairs.size() + num_threads - 1) / num_threads;
               
               // Launch threads
               for (unsigned int i = 0; i < num_threads; ++i) {
                   size_t start_idx = i * thread_batch_size;
                   size_t end_idx = std::min(start_idx + thread_batch_size, partition_pairs.size());
                   
                   if (start_idx >= partition_pairs.size()) break;
                   
                   std::vector<std::pair<IndexPartition, IndexPartition>> thread_batch(
                       partition_pairs.begin() + start_idx,
                       partition_pairs.begin() + end_idx
                   );
                   
                   futures.push_back(std::async(std::launch::async, 
                       process_partition_batch, 
                       std::ref(tx_data), 
                       std::move(thread_batch), 
                       std::ref(input_mapper),
                       std::ref(output_mapper),
                       std::ref(valid_count), 
                       std::ref(file_mutex), 
                       std::ref(output_file),
                       std::ref(pruned_count),
                       std::ref(checked_count)
                   ));
               }
               
               // Wait for all threads to complete
               for (auto& future : futures) {
                   future.wait();
               }
           }
           
           // Update progress display
           auto current_time = std::chrono::high_resolution_clock::now();
           auto time_elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - last_update_time).count();
           
           // Update progress once per second
           if (time_elapsed >= 1) {
               last_update_time = current_time;
               
               // Calculate progress based on input partitions processed
               double input_progress = static_cast<double>(input_generator.current_progress()) / total_input_partitions;
               
               // Ensure progress doesn't exceed 100%
               double progress_percentage = std::min(input_progress * 100.0, 99.9);
               
               // Calculate estimated time remaining
               auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
               double seconds_per_percent = total_elapsed / progress_percentage;
               double estimated_seconds_remaining = seconds_per_percent * (100.0 - progress_percentage);
               
               // Format time remaining
               std::string time_remaining;
               if (estimated_seconds_remaining > 3600) {
                   time_remaining = std::to_string(static_cast<int>(estimated_seconds_remaining / 3600)) + "h " +
                                   std::to_string(static_cast<int>((static_cast<int>(estimated_seconds_remaining) % 3600) / 60)) + "m";
               } else if (estimated_seconds_remaining > 60) {
                   time_remaining = std::to_string(static_cast<int>(estimated_seconds_remaining / 60)) + "m " +
                                   std::to_string(static_cast<int>(static_cast<int>(estimated_seconds_remaining) % 60)) + "s";
               } else {
                   time_remaining = std::to_string(static_cast<int>(estimated_seconds_remaining)) + "s";
               }
               
               // Draw progress bar
               std::string progress_bar = draw_progress_bar(progress_percentage);
               
               // Clear the current line and print progress
               std::cout << "\r" << std::string(80, ' ') << "\r"; // Clear line
               std::cout << progress_bar << " " << std::fixed << std::setprecision(1) << progress_percentage << "% | "
                         << "Pairs: " << pairs_processed << " | "
                         << "Valid: " << valid_count << " | "
                         << "Pruned: " << pruned_count << " | "
                         << "ETA: " << time_remaining << std::flush;
           }
       }
   }
   
   // Print final progress and newline
   std::cout << "\r" << std::string(80, ' ') << "\r"; // Clear line
   std::cout << draw_progress_bar(100.0) << " 100.0% | "
             << "Completed! Processed " << pairs_processed << " partition pairs. "
             << "Pruned " << pruned_count << " pairs. Found " 
             << valid_count << " valid mappings." << std::endl;
   
   // Close the output file
   output_file.close();
   
   std::cout << "\nResults have been written to: " << output_filename << std::endl;
   std::cout << "Total valid partitions and mappings found: " << valid_count << std::endl;
   
   return valid_count;
}

/**
* Finds all valid partitions and mappings of inputs and outputs in a transaction.
* Uses memory-efficient data structures and chunked processing.
* Writes results directly to a CSV file.
* 
* @param tx_data The transaction data
* @param output_filename Optional filename for the output CSV file
* @return The number of valid partitions and mappings found
*/
size_t find_valid_partitions(const TransactionData& tx_data, const std::string& output_filename = "valid_mappings.csv") {
   // Get input and output IDs
   const auto& input_ids = tx_data.get_input_ids();
   const auto& output_ids = tx_data.get_output_ids();
   
   std::cout << "Finding valid partitions using memory-efficient chunked processing..." << std::endl;
   std::cout << "Results will be written to: " << output_filename << std::endl;
   
   // Create element mappers
   ElementMapper input_mapper(input_ids);
   ElementMapper output_mapper(output_ids);
   
   // Fixed chunk size of 500
   const size_t chunk_size = 500;
   
   // Process partitions in chunks and write to file
   return process_partition_chunks(tx_data, input_mapper, output_mapper, chunk_size, output_filename);
}

#endif // PARTITION_ANALYZER_H
