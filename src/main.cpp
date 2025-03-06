#include <iostream>
#include <string.h>
#include <curl/curl.h>
#include <cstdlib>

// Callback function to write the Http response data into a string
size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, std::string* output)
{
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

int main(int argc, char const *argv[])
{
	// Initialize curl
	CURL* curl;
    CURLcode res;
    std::string readBuffer;

	curl_global_init(CURL_GLOBAL_DEFAULT);

	// Error Handling
	if ((curl = curl_easy_init()) == nullptr)
	{
		std::cerr << "Failed to initialize CURL." << std::endl;
		curl_global_cleanup();
		return EXIT_FAILURE;
	}

	// Specify Bitcoin transaction hash to gather information via Https-Get-Request
	std::string txurl = "https://blockchain.info/rawtx/";
	std::string txhash = "5333c8c2bc2090e01e96d9243e5681af9245918f7681620c38aad2ce7eff7fe5";
	std::string url = txurl + txhash;

	// Set the URL to send the GET request to
	// blockchain.info API
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	// Specify that the response should be written to a string using a callback function CurlWriteCallback
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	// Perform the GET request
	if ((res = curl_easy_perform(curl)) != CURLE_OK)
	{
		// Error Handling
		std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
		return EXIT_FAILURE;
	}

	// Print the response
	std::cout << "Response from server: " << readBuffer << std::endl;

	// Clean up global libcurl resources
	curl_easy_cleanup(curl);
    curl_global_cleanup();

	return EXIT_SUCCESS;
}