# Bitcoin Transaction Input-Output Analysis

A C++ tool for analyzing Bitcoin transactions to identify relationships between inputs and outputs. This tool performs comprehensive analysis of bitcoin transactions to find valid mappings between inputs and outputs.

## Features

- **Simple Subset Analysis**: Find all valid combinations of input and output subsets where output value ≤ input value
- **Comprehensive Partition Analysis**: Identify valid partitions of all inputs and outputs with valid mappings
- **Memory-Efficient Processing**: Uses chunked processing and efficient data structures to handle large transactions
- **Multi-threaded Performance**: Leverages parallel processing for faster analysis (but still slow for larger inputs or outputs)
- **CSV Export**: Exports results to CSV files for further analysis in spreadsheet software or data tools
- **Progress Tracking**: Provides an estimate of completion time
- **Custom Transaction Creation**: Users can create and analyze custom transactions for testing and research
- **Fetch real Transactions**: Ability to fetch real Bitcoin transactions from the blockchain

## Requirements for compilation

### MacOS
curl: ```brew install curl```

Bitcoin Core: ```brew install bitcoin```

nlohmann-JSON: ```brew install nlohmann-json```

### Linux
curl: ```sudo apt install libcurl4-openssl-dev```

Bitcoin Core: ```sudo apt install bitcoind bitcoin-qt```

nlohmann-JSON: ```sudo apt install nlohmann-json3-dev```

## Installation

```bash
git clone https://github.com/martinsauer2000/BTC-Input-Output-Mapper.git
cd BTC-Input-Output-Mapper
make
```

## Additional Links
libbitcoin: https://libbitcoin.info

Bitcoin Core: https://bitcoincore.org/en/download/

Bitcoin RPC API: https://developer.bitcoin.org/reference/rpc/

bitcoin.config Generator: https://jlopp.github.io/bitcoin-core-config-generator/

Example Bitcoin Testnet transaction: https://tbtc.bitaps.com/97c587182189bc9a78af79c257e5af7332cd20524f410bff80bd6cfb9c6024f9