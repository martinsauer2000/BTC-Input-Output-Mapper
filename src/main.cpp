#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include "transaction_data.h"

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
    // Create the JSON RPC request payload for getting raw transaction data
    nlohmann::json json_request = {
        {"jsonrpc", "1.0"},
        {"id", "curltest"},
        {"method", "getrawtransaction"},
        {"params", nlohmann::json::array({"97c587182189bc9a78af79c257e5af7332cd20524f410bff80bd6cfb9c6024f9", true})}
    };

    // Make the RPC request and get the response
    std::string response = rpc_request(json_request);

    // Parse the JSON response
    try {
        nlohmann::json json_response = nlohmann::json::parse(response);
        std::cout << "Transaction Data: " << json_response.dump(4) << std::endl;
        
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
        
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Error parsing JSON response: " << e.what() << std::endl;
    }

    return EXIT_SUCCESS;
}

