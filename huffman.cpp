#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <string>

// A Huffman tree node
struct Node {
    char ch;
    unsigned freq;
    Node *left, *right;

    Node(char character, unsigned frequency) {
        ch = character;
        freq = frequency;
        left = right = nullptr;
    }

    ~Node() {
        delete left;
        delete right;
    }
};

// Compare function to build the priority queue (min-heap)
struct Compare {
    bool operator()(Node* l, Node* r) {
        return l->freq > r->freq;
    }
};

// Traverse Huffman Tree and store codes in a map
void encode(Node* root, std::string str, std::unordered_map<char, std::string>& huffmanCode) {
    if (!root) return;

    if (!root->left && !root->right) {
        huffmanCode[root->ch] = str;
    }

    encode(root->left, str + "0", huffmanCode);
    encode(root->right, str + "1", huffmanCode);
}

// Build the Huffman Tree and generate codes
Node* buildHuffmanTree(const std::string& text, std::unordered_map<char, std::string>& huffmanCode) {
    std::unordered_map<char, unsigned> freq;
    for (char ch : text) {
        freq[ch]++;
    }

    std::priority_queue<Node*, std::vector<Node*>, Compare> pq;

    for (auto pair : freq) {
        pq.push(new Node(pair.first, pair.second));
    }

    while (pq.size() != 1) {
        Node *left = pq.top(); pq.pop();
        Node *right = pq.top(); pq.pop();

        unsigned sum = left->freq + right->freq;
        Node* node = new Node('\0', sum);
        node->left = left;
        node->right = right;
        pq.push(node);
    }

    Node* root = pq.top();
    encode(root, "", huffmanCode);
    return root;
}

// Compress function
void compressFile(const std::string& inputFilename, const std::string& outputFilename) {
    std::ifstream inFile(inputFilename, std::ios::binary);
    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open input file.\n";
        return;
    }

    std::string text((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    if (text.empty()) {
        std::cerr << "Error: File is empty.\n";
        return;
    }

    std::unordered_map<char, std::string> huffmanCode;
    Node* root = buildHuffmanTree(text, huffmanCode);

    // Convert text into a stream of bits represented as string
    std::string encodedString = "";
    for (char ch : text) {
        encodedString += huffmanCode[ch];
    }

    // Pack bits into bytes and write to binary output file
    std::ofstream outFile(outputFilename, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not create output file.\n";
        delete root;
        return;
    }

    // Store frequency table size and mapping first so we can rebuild tree during decompression
    size_t mapSize = huffmanCode.size();
    outFile.write(reinterpret_cast<const char*>(&mapSize), sizeof(mapSize));

    for (auto pair : huffmanCode) {
        char ch = pair.first;
        size_t codeLen = pair.second.size();
        outFile.write(&ch, sizeof(ch));
        outFile.write(reinterpret_cast<const char*>(&codeLen), sizeof(codeLen));
        outFile.write(pair.second.c_str(), codeLen);
    }

    // Store total number of valid bits in the last byte padding
    size_t totalBits = encodedString.size();
    outFile.write(reinterpret_cast<const char*>(&totalBits), sizeof(totalBits));

    // Pad encoded string to multiple of 8 bits
    unsigned char byte = 0;
    int bitCount = 0;
    for (char bit : encodedString) {
        byte = (byte << 1) | (bit - '0');
        bitCount++;
        if (bitCount == 8) {
            outFile.put(byte);
            byte = 0;
            bitCount = 0;
        }
    }
    if (bitCount > 0) {
        byte = byte << (8 - bitCount);
        outFile.put(byte);
    }

    outFile.close();
    delete root;
    std::cout << "File compressed successfully using Huffman Coding!\n";
}

// Decompress function
void decompressFile(const std::string& inputFilename, const std::string& outputFilename) {
    std::ifstream inFile(inputFilename, std::ios::binary);
    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open compressed file.\n";
        return;
    }

    size_t mapSize;
    inFile.read(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));

    std::unordered_map<std::string, char> reverseHuffmanCode;
    for (size_t i = 0; i < mapSize; ++i) {
        char ch;
        size_t codeLen;
        inFile.read(&ch, sizeof(ch));
        inFile.read(reinterpret_cast<char*>(&codeLen), sizeof(codeLen));
        std::string code(codeLen, ' ');
        inFile.read(&code[0], codeLen);
        reverseHuffmanCode[code] = ch;
    }

    size_t totalBits;
    inFile.read(reinterpret_cast<char*>(&totalBits), sizeof(totalBits));

    // Read remaining bytes
    std::string encodedString = "";
    unsigned char byte;
    while (inFile.get(reinterpret_cast<char&>(byte))) {
        for (int i = 7; i >= 0; --i) {
            encodedString += ((byte >> i) & 1) ? '1' : '0';
        }
    }
    inFile.close();

    // Trim trailing padding bits
    if (encodedString.size() > totalBits) {
        encodedString = encodedString.substr(0, totalBits);
    }

    // Decode bits back to original text
    std::ofstream outFile(outputFilename, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not create output file for decompression.\n";
        return;
    }

    std::string currentBits = "";
    for (char bit : encodedString) {
        currentBits += bit;
        if (reverseHuffmanCode.find(currentBits) != reverseHuffmanCode.end()) {
            outFile.put(reverseHuffmanCode[currentBits]);
            currentBits = "";
        }
    }

    outFile.close();
    std::cout << "File decompressed successfully!\n";
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cout << "Usage:\n";
        std::cout << "  Compress:   ./huffman c <input_file> <output_file>\n";
        std::cout << "  Decompress: ./huffman d <input_file> <output_file>\n";
        return 1;
    }

    std::string mode = argv[1];
    std::string inputFile = argv[2];
    std::string outputFile = argv[3];

    if (mode == "c") {
        compressFile(inputFile, outputFile);
    } else if (mode == "d") {
        decompressFile(inputFile, outputFile);
    } else {
        std::cout << "Invalid mode! Use 'c' for compression or 'd' for decompression.\n";
    }

    return 0;
}
