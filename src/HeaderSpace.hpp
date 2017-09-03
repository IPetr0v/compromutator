
#include <limits.h>
#include <string.h>
#include <iostream>

extern "C" {
  #include "./headerspace/hs.h"
}

class TransferFunction;

int get_len(const char* str) {
    bool commas = strchr (str, ',');
    int div = CHAR_BIT + commas;
    int len = strlen(str) + commas;
    assert(len % div == 0);
    len /= div;
    return len;
}

class HeaderSpace
{
public:
    HeaderSpace(int lenght): length_(lenght) {
        hs_ = hs_create(length_);
    }
    HeaderSpace(const char* str): length_(get_len(str)) {
        hs_ = hs_create(length_);
        hs_add(hs_, array_from_str(str));
    }
    HeaderSpace(struct hs* hs) {
        hs_ = hs_create(hs->len);
        hs_copy(hs_, hs);
    }
    HeaderSpace(const HeaderSpace& other) {
        hs_ = hs_create(other.length_);
        hs_copy(hs_, other.hs_);
    }
    ~HeaderSpace() {
        hs_free(hs_);
    }
    
    HeaderSpace& operator=(const HeaderSpace& other) {
        hs_free(hs_);
        hs_ = hs_create(other.length_);
        hs_copy(hs_, other.hs_);
        return *this;
    }
    
    HeaderSpace& operator+=(const HeaderSpace& right) {
        hs_sum(hs_, right.hs_);
        return *this;
    }
    
    HeaderSpace& operator*=(const HeaderSpace& right) {
        struct hs* tmp = hs_;
        hs_copy(hs_, hs_isect_a(tmp, right.hs_));
        hs_free(tmp);
        return *this;
    }

    HeaderSpace& operator-=(const HeaderSpace& right) {
        hs_minus(hs_, right.hs_);
        return *this;
    }
    
    HeaderSpace operator+(const HeaderSpace& right) {
        HeaderSpace header(hs_);
        header += right;
        return header;
    }
    
    HeaderSpace operator*(const HeaderSpace& right) {
        return HeaderSpace(hs_isect_a(hs_, right.hs_));
    }
    
    HeaderSpace operator-(const HeaderSpace& right) {
        HeaderSpace header(hs_);
        header -= right;
        return header;
    }
    
    HeaderSpace& operator~() {
        hs_cmpl(hs_);
        return *this;
    }
    
    const int length() {return length_;}
    
    friend std::ostream& operator<<(std::ostream& os, const HeaderSpace& header);
    friend TransferFunction;

private:
    int length_;
    struct hs* hs_;
    
};

std::ostream& operator<<(std::ostream& os, const HeaderSpace& header) {
    os << hs_to_str(header.hs_);
    return os;
}

class TransferFunction
{
public:
    TransferFunction(int lenght): length_(lenght) {
        // Function without rewrite
        transfer_    = array_create(length_, BIT_Z);
        mask_        = array_create(length_, BIT_1);
        rewrite_     = array_create(length_, BIT_0);
        inverse_rewrite_ = array_create(length_, BIT_0);
    }
    TransferFunction(const char* transfer_str): length_(get_len(transfer_str)) {
        transfer_    = array_from_str(transfer_str);
        mask_        = array_create(length_, BIT_1);
        rewrite_     = array_create(length_, BIT_0);
        inverse_rewrite_ = array_create(length_, BIT_0);
        
        for (int i = 0; i < length_*CHAR_BIT; i++) {
            int byte = i/CHAR_BIT;
            int bit = i%CHAR_BIT;
            
            enum bit_val transfer_bit = array_get_bit(transfer_, byte, bit);
            enum bit_val mask_bit;
            enum bit_val rewrite_bit;
            enum bit_val inverse_rewrite_bit;
            
            switch(transfer_bit) {
            case BIT_0:
            case BIT_1:
            case BIT_X:
                mask_bit = BIT_0;
                rewrite_bit = transfer_bit;
                inverse_rewrite_bit = BIT_X;
                break;
            
            case BIT_Z:
            default:
                mask_bit = BIT_1;
                rewrite_bit = BIT_0;
                inverse_rewrite_bit = BIT_0;
                break;
            }
            
            array_set_bit(mask_, mask_bit, byte, bit);
            array_set_bit(rewrite_, rewrite_bit, byte, bit);
            array_set_bit(inverse_rewrite_, inverse_rewrite_bit, byte, bit);
        }
    }
    TransferFunction(const char* mask_str, const char* rewrite_str) {
        int mask_len = get_len(mask_str);
        int rewrite_len = get_len(rewrite_str);
        assert(mask_len == rewrite_len);
        length_ = mask_len;
        
        transfer_    = array_create(length_, BIT_Z);
        mask_        = array_from_str(mask_str);
        rewrite_     = array_from_str(rewrite_str);
        inverse_rewrite_ = array_create(length_, BIT_0);
        
        for (int i = 0; i < length_*CHAR_BIT; i++) {
            int byte = i/CHAR_BIT;
            int bit = i%CHAR_BIT;
            
            enum bit_val transfer_bit;
            enum bit_val mask_bit = array_get_bit(mask_, byte, bit);
            enum bit_val rewrite_bit = array_get_bit(rewrite_, byte, bit);
            enum bit_val inverse_rewrite_bit;
            
            if(mask_bit == BIT_1) {
                transfer_bit = BIT_Z;
                inverse_rewrite_bit = BIT_0;
            }
            else {
                transfer_bit = rewrite_bit;
                inverse_rewrite_bit = BIT_X;
            }
            
            array_set_bit(transfer_, transfer_bit, byte, bit);
            array_set_bit(inverse_rewrite_, inverse_rewrite_bit, byte, bit);
        }
    }
    ~TransferFunction() {
        array_free(transfer_);
        array_free(mask_);
        array_free(rewrite_);
    }
    
    HeaderSpace apply(const HeaderSpace& header) {
        HeaderSpace new_header(header);
        hs_rewrite(new_header.hs_, mask_, rewrite_);
        return new_header;
    }
    
    HeaderSpace inverse(const HeaderSpace& header) {
        HeaderSpace new_header(header);
        hs_rewrite(new_header.hs_, mask_, inverse_rewrite_);
        return new_header;
    }
    
    const int length() {return length_;}
    
    friend std::ostream& operator<<(std::ostream& os, const TransferFunction& t);

private:
    int length_;
    array_t* transfer_;
    array_t* mask_;
    array_t* rewrite_;
    array_t* inverse_rewrite_;

};

std::ostream& operator<<(std::ostream& os, const TransferFunction& transfer) {
    os << array_to_str(transfer.transfer_, transfer.length_, false);
    return os;
}







