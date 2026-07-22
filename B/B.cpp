#include <iostream>
#include <fstream>
#include <vector>

const unsigned char KEY = 0x5A; // 必须和 encrypt.cpp 中一致！

typedef std::vector<unsigned char> ByteVec;

ByteVec readFile(const char* filename) {
    std::ifstream in(filename, std::ios::binary);
    in.seekg(0, std::ios::end);
    long size = in.tellg();
    in.seekg(0, std::ios::beg);
    ByteVec data(size);
    in.read((char*)&data[0], size);
    return data;
}

void writeFile(const char* filename, const ByteVec& data) {
    std::ofstream out(filename, std::ios::binary);
    out.write((const char*)&data[0], data.size());
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: decrypt input.yesong output.txt\n";
        return 1;
    }

    try {
        ByteVec data = readFile(argv[1]);
        for (unsigned long i = 0; i < data.size(); ++i) {
            data[i] ^= KEY; // XOR 解密（与加密相同）
        }
        writeFile(argv[2], data);
        std::cout << "Decrypted to " << argv[2] << "\n";
    } catch (...) {
        std::cerr << "Error processing files.\n";
        return 1;
    }
    return 0;
}
