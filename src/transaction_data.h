#ifndef TRANSACTION_DATA_H
#define TRANSACTION_DATA_H

#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

class TransactionData {
private:
   // Using unordered_map for O(1) lookups
   // Key: input/output ID, Value: BTC amount
   std::unordered_map<std::string, double> inputs;
   std::unordered_map<std::string, double> outputs;
   
   // For maintaining order if needed
   std::vector<std::string> input_ids;
   std::vector<std::string> output_ids;

public:
   TransactionData() = default;
   
   // Add an input with its ID and value
   void add_input(const std::string& id, double value) {
       inputs[id] = value;
       input_ids.push_back(id);
   }
   
   // Add an output with its ID and value
   void add_output(const std::string& id, double value) {
       outputs[id] = value;
       output_ids.push_back(id);
   }
   
   // Get input value by ID
   double get_input_value(const std::string& id) const {
       auto it = inputs.find(id);
       return (it != inputs.end()) ? it->second : 0.0;
   }
   
   // Get output value by ID
   double get_output_value(const std::string& id) const {
       auto it = outputs.find(id);
       return (it != outputs.end()) ? it->second : 0.0;
   }
   
   // Get all inputs
   const std::unordered_map<std::string, double>& get_inputs() const {
       return inputs;
   }
   
   // Get all outputs
   const std::unordered_map<std::string, double>& get_outputs() const {
       return outputs;
   }
   
   // Get input IDs in order of addition
   const std::vector<std::string>& get_input_ids() const {
       return input_ids;
   }
   
   // Get output IDs in order of addition
   const std::vector<std::string>& get_output_ids() const {
       return output_ids;
   }
   
   // Calculate total input value
   double total_input_value() const {
       double total = 0.0;
       for (const auto& pair : inputs) {
           total += pair.second;
       }
       return total;
   }
   
   // Calculate total output value
   double total_output_value() const {
       double total = 0.0;
       for (const auto& pair : outputs) {
           total += pair.second;
       }
       return total;
   }
   
   // Check if transaction is valid (inputs >= outputs)
   bool is_valid() const {
       return total_input_value() >= total_output_value();
   }
   
   // Get transaction fee
   double get_fee() const {
       return total_input_value() - total_output_value();
   }
   
   // Clear all data
   void clear() {
       inputs.clear();
       outputs.clear();
       input_ids.clear();
       output_ids.clear();
   }
};

#endif // TRANSACTION_DATA_H
