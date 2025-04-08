#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include "transaction_data.h"
#include "subset_generator.h"
#include "bell_number.h"
#include "subset_analyzer.h"

// Function to handle the response from the RPC call
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to make the RPC request
std::string rpc_request(const nlohmann::json& json_data) {
    // Bitcoin Core RPC URL (assuming it's running locally)
    std::string url = "http://127.0.0.1:8332";  // Default RPC port
    std::string user = "rpcuser";               // Your RPC username (see bitcoin.conf)
    std::string pass = "rpcpw";                 // Your RPC password (see bitcoin.conf)
    
    CURL* curl;
    CURLcode res;
    std::string read_buffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        struct curl_slist* headers = NULL;

        // Set the content-type header
        headers = curl_slist_append(headers, "Content-Type: application/json");

        // Set the URL, user and password for authentication
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERPWD, (user + ":" + pass).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Convert the JSON object to a string and send it as the POST body
        std::string json_str = json_data.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());

        // Specify a callback to handle the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);

        // Perform the request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            std::cerr << "CURL request failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Clean up
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    curl_global_cleanup();
    return read_buffer;
}

// Helper function to get a previous transaction by its txid
nlohmann::json get_transaction(const std::string& txid) {
    // Create the JSON RPC request payload for getting the transaction
    nlohmann::json json_request = {
        {"jsonrpc", "1.0"},
        {"id", "curltest"},
        {"method", "getrawtransaction"},
        {"params", nlohmann::json::array({txid, true})}
    };
    
    // Make the RPC request and get the response
    std::string response = rpc_request(json_request);
    
    // Parse and return the JSON response
    try {
        return nlohmann::json::parse(response);
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Error parsing JSON response for txid " << txid << ": " << e.what() << std::endl;
        return nlohmann::json();
    }
}

// Function to parse transaction data from JSON response
TransactionData parse_transaction_data(const nlohmann::json& json_response) {
    TransactionData tx_data;
    
    try {
        // Check if we have a result in the response
        if (!json_response.contains("result") || json_response["result"].is_null()) {
            std::cerr << "Error: No transaction data found in response" << std::endl;
            return tx_data;
        }
        
        const auto& result = json_response["result"];
        
        // Process inputs (vin)
        if (result.contains("vin") && result["vin"].is_array()) {
            const auto& vin = result["vin"];
            for (size_t i = 0; i < vin.size(); i++) {
                const auto& input = vin[i];
                double value = 0.0;
                
                // Get the previous transaction ID and output index
                if (input.contains("txid") && input.contains("vout")) {
                    std::string prev_txid = input["txid"].get<std::string>();
                    size_t prev_vout = static_cast<size_t>(input["vout"].get<int>());
                    
                    // Get the previous transaction
                    nlohmann::json prev_tx_response = get_transaction(prev_txid);
                    
                    // Extract the value from the previous transaction's output
                    if (prev_tx_response.contains("result") && 
                        prev_tx_response["result"].contains("vout") && 
                        prev_tx_response["result"]["vout"].is_array() && 
                        prev_vout < prev_tx_response["result"]["vout"].size()) {
                        
                        const auto& prev_output = prev_tx_response["result"]["vout"][prev_vout];
                        if (prev_output.contains("value")) {
                            value = prev_output["value"].get<double>();
                        }
                    } else {
                        std::cerr << "Warning: Could not retrieve value for input " << i 
                                  << " (prev_txid: " << prev_txid << ", vout: " << prev_vout << ")" << std::endl;
                    }
                }
                
                // Create input ID and add to transaction data
                std::string input_id = "input_" + std::to_string(i);
                tx_data.add_input(input_id, value);
            }
        }
        
        // Process outputs (vout)
        if (result.contains("vout") && result["vout"].is_array()) {
            const auto& vout = result["vout"];
            for (size_t i = 0; i < vout.size(); i++) {
                const auto& output = vout[i];
                double value = 0.0;
                
                // Get output value
                if (output.contains("value")) {
                    value = output["value"].get<double>();
                }
                
                // Create output ID and add to transaction data
                std::string output_id = "output_" + std::to_string(i);
                tx_data.add_output(output_id, value);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing transaction data: " << e.what() << std::endl;
    }
    
    return tx_data;
}

int main() {
    // Prompt the user to enter a transaction ID
    std::string txid;
    std::cout << "Enter a Bitcoin transaction ID: ";
    std::cin >> txid;
    
    std::cout << "Fetching transaction data for: " << txid << std::endl;
    
    // Create the JSON RPC request payload for getting raw transaction data
    nlohmann::json json_request = {
        {"jsonrpc", "1.0"},
        {"id", "curltest"},
        {"method", "getrawtransaction"},
        {"params", nlohmann::json::array({txid, true})}
    };
    
    // Make the RPC request and get the response
    std::string response = rpc_request(json_request);
    
    // Parse the JSON response
    try {
        nlohmann::json json_response = nlohmann::json::parse(response);
        
        // Check if there's an error in the response
        if (json_response.contains("error") && !json_response["error"].is_null()) {
            std::cerr << "Error from Bitcoin RPC: " << json_response["error"].dump(4) << std::endl;
            return EXIT_FAILURE;
        }
        
        std::cout << "Transaction data retrieved successfully." << std::endl;
        
        // Parse transaction data into our custom class
        TransactionData tx_data = parse_transaction_data(json_response);
        
        // Display transaction summary
        std::cout << "\nTransaction Summary:" << std::endl;
        std::cout << "Total Input Value: " << tx_data.total_input_value() << " BTC" << std::endl;
        std::cout << "Total Output Value: " << tx_data.total_output_value() << " BTC" << std::endl;
        std::cout << "Transaction Fee: " << tx_data.get_fee() << " BTC" << std::endl;
        std::cout << "Transaction Valid: " << (tx_data.is_valid() ? "Yes" : "No") << std::endl;
        
        // Display inputs
        std::cout << "\nInputs:" << std::endl;
        for (const auto& id : tx_data.get_input_ids()) {
            std::cout << id << ": " << tx_data.get_input_value(id) << " BTC" << std::endl;
        }
        
        // Display outputs
        std::cout << "\nOutputs:" << std::endl;
        for (const auto& id : tx_data.get_output_ids()) {
            std::cout << id << ": " << tx_data.get_output_value(id) << " BTC" << std::endl;
        }
        
        // Generate all subsets of inputs
        std::cout << "\nGenerating input subsets..." << std::endl;
        auto input_subsets = generate_subsets(tx_data, SubsetType::INPUTS);
        std::cout << "Generated " << input_subsets.size() << " input subsets." << std::endl;
        
        // Generate all subsets of outputs
        std::cout << "\nGenerating output subsets..." << std::endl;
        auto output_subsets = generate_subsets(tx_data, SubsetType::OUTPUTS);
        std::cout << "Generated " << output_subsets.size() << " output subsets." << std::endl;
        
        // Display some statistics
        std::cout << "\nSubset Statistics:" << std::endl;
        std::cout << "Number of inputs: " << tx_data.get_input_ids().size() << std::endl;
        std::cout << "Number of outputs: " << tx_data.get_output_ids().size() << std::endl;
        std::cout << "Number of possible input subsets: " << input_subsets.size() << std::endl;
        std::cout << "Number of possible output subsets: " << output_subsets.size() << std::endl;
        
        // Ask if user wants to see all subsets (could be a lot for large transactions)
        char show_all;
        std::cout << "\nDo you want to see all subsets? (y/n): ";
        std::cin >> show_all;
        
        if (show_all == 'y' || show_all == 'Y') {
            // Display all input subsets
            std::cout << "\nAll Input Subsets:" << std::endl;
            for (const auto& subset : input_subsets) {
                print_subset(subset, tx_data, SubsetType::INPUTS);
            }
            
            // Display all output subsets
            std::cout << "\nAll Output Subsets:" << std::endl;
            for (const auto& subset : output_subsets) {
                print_subset(subset, tx_data, SubsetType::OUTPUTS);
            }
        }
        
        // Ask if user wants to see all valid input-output combinations
        char show_combinations;
        std::cout << "\nDo you want to see all valid input-output combinations? (y/n): ";
        std::cin >> show_combinations;
        
        if (show_combinations == 'y' || show_combinations == 'Y') {
            // Find and display all valid combinations
            std::cout << "\n===== Valid Input-Output Combinations =====" << std::endl;
            
            // Calculate the maximum possible combinations
            size_t max_combinations = input_subsets.size() * output_subsets.size();
            std::cout << "Maximum possible combinations: " << max_combinations << std::endl;
            
            // Check if there are too many combinations
            if (max_combinations > 1000) {
                char confirm;
                std::cout << "Warning: This will generate a large number of combinations." << std::endl;
                std::cout << "Are you sure you want to continue? (y/n): ";
                std::cin >> confirm;
                
                if (confirm != 'y' && confirm != 'Y') {
                    std::cout << "Operation cancelled by user." << std::endl;
                    return EXIT_SUCCESS;
                }
            }
            
            // Find and display valid combinations
            size_t valid_count = find_valid_combinations(tx_data, input_subsets, output_subsets);
        }
        
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Error parsing JSON response: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
