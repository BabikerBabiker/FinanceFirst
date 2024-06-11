#ifndef ENCRYPT_HPP
#define ENCRYPT_HPP
#include <string>

class Encryptor {
public:
    Encryptor() : key("63st*^rL6s56&d6z%k2^gXwP2Ao!!Q") {}

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