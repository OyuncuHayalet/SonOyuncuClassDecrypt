#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <windows.h>
#include <wincrypt.h>
#include "base64.h"
/*
💀 vmpforjava
*/
namespace fs = std::filesystem;

void parseFiles(const std::string& directoryPath, HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, DWORD dwFlags) {
    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        const auto& path = entry.path();

        if (fs::is_directory(path)) {
            // Recursively parse files in subdirectory
            parseFiles(path.string(), hKey, hHash, Final, dwFlags);
        }
        else if (fs::is_regular_file(path)) {
            // Process regular file
            std::cout << "File: " << path << std::endl;

            // Add your code to parse/process the file here

            std::ifstream inFile(path, std::ios::binary);
            if (!inFile)
            {
                std::cout << "Failed to open the input file." << std::endl;
            }

            // Get the size of the encrypted data
            inFile.seekg(0, std::ios::end);
            std::streampos fileSize = inFile.tellg();
            inFile.seekg(0, std::ios::beg);

            // Read the encrypted data from the file
            std::vector<BYTE> encryptedData(fileSize);
            inFile.read(reinterpret_cast<char*>(encryptedData.data()), fileSize);
            inFile.close();

            // Decrypt the data
            DWORD decryptedSize = static_cast<DWORD>(encryptedData.size());
            if (!CryptDecrypt(hKey, hHash, Final, dwFlags, encryptedData.data(), &decryptedSize))
            {
                std::cout << "Failed to decrypt the data." << std::endl;
            }

            // Open the output file
            std::ofstream outFile(path, std::ios::binary);

            // Write the decrypted data to the output file
            outFile.write(reinterpret_cast<const char*>(encryptedData.data()), decryptedSize);
            outFile.close();

            std::cout << "Decryption complete. Decrypted data saved." << std::endl;
        }
    }
}

int main()
{
    // Acquire the decryption key using CryptImportKey
    HCRYPTKEY hKey = NULL;
    HCRYPTPROV hProvider = NULL;
    if (!CryptAcquireContext(&hProvider,
        NULL,  // pszContainer = no named container
        MS_ENH_RSA_AES_PROV_W,  // pszProvider = default provider
        PROV_RSA_AES,
        CRYPT_VERIFYCONTEXT)) {
        throw std::runtime_error("Unable to create crypto provider context.");
    }

    std::string key = base64_decode("CAIAABBmAAAgAAAAanwzX9MspnMKo6ru8MM7jUdkSRtRnXvvJXCndqaDEYg=");

    std::byte key_bytes[44];
    std::memcpy(key_bytes, key.data(), key.length());
    
    if (!CryptImportKey(hProvider,
        reinterpret_cast<BYTE*>(key_bytes),
        44,
        NULL,
        0,
        &hKey)) {
        throw std::runtime_error("Unable to create crypto key.");
    }

    std::string iv = base64_decode("ZY5L+FAj2hvtJNvW3nsiJw==");

    std::byte iv_bytes[44];
    std::memcpy(iv_bytes, iv.data(), iv.length());
    
    // Set the initialization vector (IV)
    if (!CryptSetKeyParam(hKey, KP_IV, reinterpret_cast<BYTE*>(iv_bytes), 0))
    {
        CryptDestroyKey(hKey);
        CryptReleaseContext(hProvider, 0);
        throw std::runtime_error("Failed to set IV.");
    }

    parseFiles("Path to the folder that the .jar file extracted to (Use rename all when extracting because windows is not case-sensitive)", hKey, NULL, TRUE, 0);
    
    // Clean up resources
    CryptDestroyKey(hKey);
    CryptReleaseContext(hProvider, 0);

    int x;
    std::cin >> x;
}
