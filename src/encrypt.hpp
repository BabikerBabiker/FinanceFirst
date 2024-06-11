#ifndef ENCRYPT_HPP
#define ENCRYPT_HPP
#include <iostream>
#include <string>
#include <fstream>
#include <stdexcept>
using std::ifstream;
using std::string;

class Encryptor {
public:
    Encryptor() {
        std::ifstream keyFile("/home/babiker/Documents/GitHub/FinanceFirst/src/key.txt");
        
        if (keyFile.is_open()) {
            keyFile >> this->key;
            keyFile.close();
        } else {
            std::cerr << "Unable to open key.txt. Please ensure the file exists and is accessible." << std::endl;
            std::runtime_error("Unable to open key file");
        }
    }

    std::string encrypt(const std::string& input) {
        return xorEncryptDecrypt(input);
    }

    std::string decrypt(const std::string& encrypted) {
        return xorEncryptDecrypt(encrypted);
    }

private:
    std::string key;

    std::string xorEncryptDecrypt(const std::string& input) {
        std::string output = input;
        for (size_t i = 0; i < input.size(); ++i) {
            output[i] = input[i] ^ key[i % key.size()];
        }
        return output;
    }
};

#endif