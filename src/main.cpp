#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdlib>

// Function to handle the response from the RPC call
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to make the RPC request
std::string rpc_request(const nlohmann::json& json_data)
{
    // Bitcoin Core RPC URL (assuming it's running locally)
    std::string url = "http://127.0.0.1:8332";  // Default RPC port
    std::string user = "rpcuser";          // Your RPC username (see bitcoin.conf)
    std::string pass = "rpcpw";          // Your RPC password (see bitcoin.conf)
	
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

int main() {
    // Create the JSON RPC request payload
    nlohmann::json json_request = {
        {"jsonrpc", "1.0"},
        {"id", "curltest"},
        {"method", "getblockchaininfo"},
        {"params", nlohmann::json::array()}
    };

    // Make the RPC request and get the response
    std::string response = rpc_request(json_request);

    // Parse and print the JSON response
    try {
        nlohmann::json json_response = nlohmann::json::parse(response);
        std::cout << "Blockchain Info: " << json_response.dump(4) << std::endl;
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Error parsing JSON response: " << e.what() << std::endl;
    }

    return EXIT_SUCCESS;
}
