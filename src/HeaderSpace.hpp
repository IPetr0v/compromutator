
#include <string.h>
#include <iostream>

extern "C" {
  #include "./headerspace/hs.h"
}

class HeaderSpace
{
public:
    HeaderSpace(int lenght): length_(lenght) {
        hs_ = hs_create(length_);
    }
    HeaderSpace(const char *str) {
        assert (strlen(str) % 8 == 0);
        length_ = strlen(str) / 8;
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
    
    friend std::ostream& operator<<(std::ostream& os, const HeaderSpace& header);  
    
    const int length() {return length_;}

private:
    int length_;
    struct hs* hs_;
    
};

std::ostream& operator<<(std::ostream& os, const HeaderSpace& header) {
    os << hs_to_str(header.hs_);
    return os;
}
