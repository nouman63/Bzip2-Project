#ifndef BWT_H
#define BWT_H

#include <vector>
using namespace std;

struct BWTResult {
    vector<unsigned char> data;
    int primary_index = 0;
};

BWTResult bwt_encode(const vector<unsigned char>& input);
vector<unsigned char> bwt_decode(const vector<unsigned char>& input, int primary_index);

#endif
