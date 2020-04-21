#include <openssl/sha.h>

std::string sha256(const std::string str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);

    return std::string((char *)hash, SHA256_DIGEST_LENGTH);
}

template<typename T1>
void Hash(unsigned char* result, const T1 pbegin, const T1 pend) {
    std::string t(pbegin, pend);

    std::string h = sha256(sha256(t));
    /*printf(" - Sha256: ");
    for (auto& el : h)
  	  printf("%02hhx", el);
    printf(" - ");*/

    memcpy(result, (char*)h.c_str(), 4);

}

