#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <cstddef>
#include <vector>
#include <unordered_map>
#include <memory>

using namespace std;

// Huffman tree node
struct HuffmanNode {
    unsigned char character;
    int frequency;
    unique_ptr<HuffmanNode> left;
    unique_ptr<HuffmanNode> right;

    HuffmanNode(unsigned char ch, int freq)
        : character(ch), frequency(freq), left(nullptr), right(nullptr) {
    }

    HuffmanNode(int freq, unique_ptr<HuffmanNode> l, unique_ptr<HuffmanNode> r)
        : character(0), frequency(freq), left(move(l)), right(move(r)) {
    }

    bool isLeaf() const { return !left && !right; }
};

// Huffman coding result
struct HuffmanResult {
    vector<bool> encoded_data;
    unordered_map<unsigned char, vector<bool>> codes;
    unique_ptr<HuffmanNode> root;
};

HuffmanResult huffman_encode(const vector<unsigned char>& input);
vector<unsigned char> huffman_decode(const vector<bool>& encoded_data, HuffmanNode* root);

#endif // HUFFMAN_H#pragma once
