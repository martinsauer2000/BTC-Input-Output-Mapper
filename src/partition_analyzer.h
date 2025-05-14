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
* Generates all permutations of a partition and checks each one for validity.
* Uses indices for memory efficiency.
* 
* @param tx_data The transaction data
* @param input_partition A partition of input indices
* @param output_partition A partition of output indices
* @param input_mapper Mapper for input elements
* @param output_mapper Mapper for output elements
* @param valid_count Reference to the counter for valid mappings
* @param results_mutex Mutex for thread-safe access to shared data
* @param valid_mappings Vector to store valid mappings
*/
void check_all_permutations(
   const TransactionData& tx_data,
   const IndexPartition& input_partition,
   IndexPartition output_partition,
   const ElementMapper& input_mapper,
   const ElementMapper& output_mapper,
   std::atomic<size_t>& valid_count,
   std::mutex& results_mutex,
   std::vector<std::tuple<IndexPartition, IndexPartition, std::vector<size_t>>>& valid_mappings
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
           
           // Store the valid mapping
           {
               std::lock_guard<std::mutex> lock(results_mutex);
               valid_mappings.push_back(std::make_tuple(input_partition, permuted_output, indices));
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
* @param results_mutex Mutex for thread-safe access to shared data
* @param valid_mappings Vector to store valid mappings
* @param pruned_count Reference to counter for pruned partition pairs
*/
void process_partition_batch(
   const TransactionData& tx_data,
   const std::vector<std::pair<IndexPartition, IndexPartition>>& partition_pairs,
   const ElementMapper& input_mapper,
   const ElementMapper& output_mapper,
   std::atomic<size_t>& valid_count,
   std::mutex& results_mutex,
   std::vector<std::tuple<IndexPartition, IndexPartition, std::vector<size_t>>>& valid_mappings,
   std::atomic<size_t>& pruned_count
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
           results_mutex, 
           valid_mappings
       );
   }
}

/**
* Displays the valid mappings found during analysis.
* 
* @param tx_data The transaction data
* @param valid_mappings Vector of valid mappings
* @param input_mapper Mapper for input elements
* @param output_mapper Mapper for output elements
*/
void display_valid_mappings(
   const TransactionData& tx_data,
   const std::vector<std::tuple<IndexPartition, IndexPartition, std::vector<size_t>>>& valid_mappings,
   const ElementMapper& input_mapper,
   const ElementMapper& output_mapper
) {
   // Display valid mappings
   for (size_t mapping_idx = 0; mapping_idx < valid_mappings.size(); ++mapping_idx) {
       const auto& [input_partition, output_partition, indices] = valid_mappings[mapping_idx];
       
       // Convert index partitions back to string partitions for display
       auto input_string_partition = input_mapper.to_string_partition(input_partition);
       auto output_string_partition = output_mapper.to_string_partition(output_partition);
       
       std::cout << "\nValid Partition and Mapping #" << (mapping_idx + 1) << ":" << std::endl;
       
       // Print input partition
       std::cout << "  Input Partition:" << std::endl;
       for (size_t i = 0; i < input_string_partition.size(); ++i) {
           const auto& subset = input_string_partition[i];
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
       for (size_t i = 0; i < output_string_partition.size(); ++i) {
           const auto& subset = output_string_partition[i];
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
       for (size_t i = 0; i < input_string_partition.size(); ++i) {
           double input_value = calculate_subset_value(tx_data, input_string_partition[i], SubsetType::INPUTS);
           double output_value = calculate_subset_value(tx_data, output_string_partition[i], SubsetType::OUTPUTS);
           double difference = input_value - output_value;
           
           std::cout << "    Input Group " << (i + 1) << " -> Output Group " << (i + 1) << std::endl;
           std::cout << "      Input: { ";
           for (size_t j = 0; j < input_string_partition[i].size(); ++j) {
               std::cout << input_string_partition[i][j];
               if (j < input_string_partition[i].size() - 1) {
                   std::cout << ", ";
               }
           }
           std::cout << " } = " << input_value << " BTC" << std::endl;
           
           std::cout << "      Output: { ";
           for (size_t j = 0; j < output_string_partition[i].size(); ++j) {
               std::cout << output_string_partition[i][j];
               if (j < output_string_partition[i].size() - 1) {
                   std::cout << ", ";
               }
           }
           std::cout << " } = " << output_value << " BTC" << std::endl;
           
           std::cout << "      Difference: " << difference << " BTC" << std::endl;
       }
       
       // Print the total difference
       double total_input = 0.0;
       double total_output = 0.0;
       
       for (size_t i = 0; i < input_string_partition.size(); ++i) {
           total_input += calculate_subset_value(tx_data, input_string_partition[i], SubsetType::INPUTS);
           total_output += calculate_subset_value(tx_data, output_string_partition[i], SubsetType::OUTPUTS);
       }
       
       std::cout << "  Total Difference: " << (total_input - total_output) << " BTC" << std::endl;
   }
}

/**
* Processes chunks of partitions to reduce memory usage.
* 
* @param tx_data The transaction data
* @param input_mapper Mapper for input elements
* @param output_mapper Mapper for output elements
* @param chunk_size Size of partition chunks to process at once
* @return Number of valid mappings found
*/
size_t process_partition_chunks(
   const TransactionData& tx_data,
   const ElementMapper& input_mapper,
   const ElementMapper& output_mapper,
   size_t chunk_size
) {
   // Convert element IDs to indices
   std::vector<ElementIndex> input_indices(input_mapper.elements.size());
   std::vector<ElementIndex> output_indices(output_mapper.elements.size());
   
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
   
   // Estimate total compatible combinations (this is an upper bound)
   size_t estimated_total_combinations = total_input_partitions * total_output_partitions;
   std::cout << "Estimated maximum combinations to check: " << estimated_total_combinations << std::endl;
   
   // Storage for valid mappings and pruning statistics
   std::atomic<size_t> valid_count(0);
   std::atomic<size_t> pruned_count(0);
   std::mutex results_mutex;
   std::vector<std::tuple<IndexPartition, IndexPartition, std::vector<size_t>>> valid_mappings;
   
   // Determine the number of threads to use
   unsigned int num_threads = std::thread::hardware_concurrency();
   if (num_threads == 0) num_threads = 4; // Default if hardware_concurrency is not available
   num_threads = std::min(num_threads, 16u); // Limit to reasonable number
   
   std::cout << "Using " << num_threads << " threads for parallel processing." << std::endl;
   std::cout << "Processing partitions in chunks of size " << chunk_size << "..." << std::endl;
   
   // Process input partitions in chunks
   size_t input_chunks_processed = 0;
   size_t pairs_processed = 0;
   size_t compatible_pairs = 0;
   
   // For progress tracking
   auto start_time = std::chrono::high_resolution_clock::now();
   auto last_update_time = start_time;
   
   while (input_generator.has_more()) {
       // Get chunk of input partitions
       auto input_chunk = input_generator.next_chunk(chunk_size);
       input_chunks_processed++;
       
       // Reset output generator for each input chunk
       PartitionGenerator output_generator(output_indices);
       size_t output_chunks_processed = 0;
       
       // Process all output partitions for this input chunk
       while (output_generator.has_more()) {
           // Get chunk of output partitions
           auto output_chunk = output_generator.next_chunk(chunk_size);
           output_chunks_processed++;
           
           // Create partition pairs for this chunk combination
           std::vector<std::pair<IndexPartition, IndexPartition>> partition_pairs;
           for (const auto& input_partition : input_chunk) {
               for (const auto& output_partition : output_chunk) {
                   // Only add pairs with matching group counts
                   if (input_partition.size() == output_partition.size()) {
                       partition_pairs.emplace_back(input_partition, output_partition);
                       compatible_pairs++;
                   }
               }
           }
           
           pairs_processed += partition_pairs.size();
           
           // If no compatible pairs in this chunk, continue
           if (partition_pairs.empty()) {
               continue;
           }
           
           // Process partition pairs in parallel
           if (num_threads <= 1 || partition_pairs.size() <= 1) {
               // If only one thread or one pair, process directly
               process_partition_batch(
                   tx_data,
                   partition_pairs,
                   input_mapper,
                   output_mapper,
                   valid_count,
                   results_mutex,
                   valid_mappings,
                   pruned_count
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
                       std::ref(results_mutex), 
                       std::ref(valid_mappings),
                       std::ref(pruned_count)
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
           
           // Update progress every second or after processing a significant number of pairs
           if (time_elapsed >= 1 || (input_chunks_processed * output_chunks_processed) % 5 == 0) {
               last_update_time = current_time;
               
               // Calculate progress percentage based on input partitions processed
               double input_progress = static_cast<double>(input_generator.current_progress()) / total_input_partitions * 100.0;
               
               // Calculate estimated time remaining
               auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
               double pairs_per_second = pairs_processed / (total_elapsed > 0 ? total_elapsed : 1);
               size_t estimated_remaining_pairs = estimated_total_combinations - pairs_processed;
               double estimated_seconds_remaining = estimated_remaining_pairs / (pairs_per_second > 0 ? pairs_per_second : 1);
               
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
               
               // Clear the current line and print progress
               std::cout << "\r" << std::string(80, ' ') << "\r"; // Clear line
               std::cout << "Progress: " << std::fixed << std::setprecision(1) << input_progress << "% | "
                         << "Processed: " << pairs_processed << " pairs | "
                         << "Pruned: " << pruned_count << " pairs | "
                         << "Found: " << valid_count << " valid mappings | "
                         << "Est. remaining: " << time_remaining << std::flush;
           }
       }
   }
   
   // Print final progress and newline
   std::cout << "\r" << std::string(80, ' ') << "\r"; // Clear line
   std::cout << "Completed! Processed " << pairs_processed << " partition pairs. "
             << "Pruned " << pruned_count << " pairs. Found " 
             << valid_count << " valid mappings." << std::endl;
   
   // Ask user if they want to see all valid mappings
   if (!valid_mappings.empty()) {
       char show_mappings;
       std::cout << "\nDo you want to display all " << valid_mappings.size() 
                 << " valid mappings? (y/n): ";
       std::cin >> show_mappings;
       
       if (show_mappings == 'y' || show_mappings == 'Y') {
           display_valid_mappings(tx_data, valid_mappings, input_mapper, output_mapper);
       }
   }
   
   std::cout << "\nTotal valid partitions and mappings found: " << valid_count << std::endl;
   return valid_count;
}

/**
* Finds all valid partitions and mappings of inputs and outputs in a transaction.
* Uses memory-efficient data structures and chunked processing.
* 
* @param tx_data The transaction data
* @return The number of valid partitions and mappings found
*/
size_t find_valid_partitions(const TransactionData& tx_data) {
   // Get input and output IDs
   const auto& input_ids = tx_data.get_input_ids();
   const auto& output_ids = tx_data.get_output_ids();
   
   std::cout << "Finding valid partitions using memory-efficient chunked processing..." << std::endl;
   
   // Create element mappers
   ElementMapper input_mapper(input_ids);
   ElementMapper output_mapper(output_ids);
   
   // Fixed chunk size of 500
   const size_t chunk_size = 500;
   
   // Process partitions in chunks
   return process_partition_chunks(tx_data, input_mapper, output_mapper, chunk_size);
}

#endif // PARTITION_ANALYZER_H
