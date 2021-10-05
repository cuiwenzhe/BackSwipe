#include <cmath>
#include "quantizer.h"
#include <android/log.h>

Quantizer::Quantizer() : max_(1.f), nbits_(1) {};

Quantizer::Quantizer(float max, int nbits) : max_(max), nbits_(nbits) {};

void Quantizer::init(float max, int nbits) {
    max_ = max;
    nbits_ = nbits;
}

Quantizer::~Quantizer() {}

float Quantizer::max() const {
    return max_;
}

int Quantizer::nbits() const {
    return nbits_;
}

EqualSizeBinQuantizer::EqualSizeBinQuantizer() : max_encoded_(1), encoding_const_(1.f) {};

EqualSizeBinQuantizer::EqualSizeBinQuantizer(float max, int nbits)
: Quantizer(max, nbits){
    max_encoded_ = static_cast<uint32>(pow(2, nbits) - 1);
    encoding_const_ = max / max_encoded_;
};

void EqualSizeBinQuantizer::init(float max, int nbits) {
    max_ = max;
    nbits_ = nbits;
    max_encoded_ = static_cast<uint32>(pow(2, nbits) - 1);
    encoding_const_ = max / max_encoded_;
};

uint32 EqualSizeBinQuantizer::Encode(float f) const {
    if (f < 0.f) {
        return 0;
    }
    for(int i = 0; i < max_encoded_; i ++){
        if(f >= (i - 0.5f) * encoding_const_ && f < (i + 0.5f) * encoding_const_){
            return static_cast<uint32>(i);
        }
    }
    return max_encoded_;
}

float EqualSizeBinQuantizer::Decode(uint32 i) const {
    //__android_log_print(ANDROID_LOG_INFO, "#####", "Quantizer ~~~ Decode");
    if(i > max_encoded_)
        return max();
    float norm_value = static_cast<float>(i) * encoding_const_;
    return norm_value;
};