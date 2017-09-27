#pragma once

extern "C" {
  #include "hs.h"
}

#include <limits.h>
#include <string.h>
#include <iostream>

// TODO: create class inherited from struct hs,
// so I can write a destructor and use smart pointers

// TODO: rename to Domain

class HeaderChanger;

class HeaderSpace
{
public:
    // TODO: add smart pointers on hs_
    // and change copy methods, so they will only copy a pointer
    // And think how to implement copy on write to hs_
    // that will be needed if I will use smart pointers
    HeaderSpace(int length);
    HeaderSpace(const char* str);
    HeaderSpace(struct hs* hs);
    HeaderSpace(const HeaderSpace& other);
    ~HeaderSpace();
    
    HeaderSpace& operator=(const HeaderSpace& other);
    HeaderSpace& operator|=(const HeaderSpace& right);
    HeaderSpace& operator&=(const HeaderSpace& right);
    HeaderSpace& operator-=(const HeaderSpace& right);
    HeaderSpace operator|(const HeaderSpace& right);
    HeaderSpace operator&(const HeaderSpace& right);
    HeaderSpace operator-(const HeaderSpace& right);
    HeaderSpace& operator~();
    
    inline const int length() {return length_;}
    
    friend std::ostream& operator<<(std::ostream& os, const HeaderSpace& header);
    friend HeaderChanger;

private:
    int length_;
    struct hs* hs_;
    
};

std::ostream& operator<<(std::ostream& os, const HeaderSpace& header);

class HeaderChanger
{
public:
    // TODO: set bool:identity_ if the function is identity
    HeaderChanger(int length);
    HeaderChanger(const HeaderChanger& other);
    HeaderChanger(const char* transfer_str);
    HeaderChanger(const char* mask_str, const char* rewrite_str);
    ~HeaderChanger();
    
    HeaderSpace apply(const HeaderSpace& header);
    HeaderSpace inverse(const HeaderSpace& header);
    
    inline const int length() {return length_;}
    inline const int identity() {return identity_;}
    
    friend std::ostream& operator<<(std::ostream& os, const HeaderChanger& t);

private:
    int length_;
    bool identity_;
    
    array_t* transfer_;
    array_t* mask_;
    array_t* rewrite_;
    array_t* inverse_rewrite_;

};

std::ostream& operator<<(std::ostream& os, const HeaderChanger& transfer);
