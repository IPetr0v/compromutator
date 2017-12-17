#pragma once

extern "C" {
  #include "hs.h"
}

#include <limits.h>
#include <string.h>
#include <iostream>

// TODO: create class inherited from struct hs,
// so I can write a destructor and use smart pointers

class HeaderSpace
{
public:
    // TODO: add smart pointers on hs_
    // and change copy methods, so they will only copy a pointer
    // And think how to implement copy on write to hs_
    // that will be needed if I will use smart pointers
    explicit HeaderSpace(int length);
    explicit HeaderSpace(const char* str);
    HeaderSpace(const HeaderSpace& other);
    ~HeaderSpace();

    bool operator==(const HeaderSpace &other) const;
    bool operator!=(const HeaderSpace &other) const;
    HeaderSpace& operator=(const HeaderSpace& other);
    HeaderSpace& operator=(HeaderSpace&& other) noexcept;

    HeaderSpace& operator~();
    HeaderSpace& operator|=(const HeaderSpace& right);
    HeaderSpace& operator&=(const HeaderSpace& right);
    HeaderSpace& operator-=(const HeaderSpace& right);
    HeaderSpace operator|(const HeaderSpace& right) const;
    HeaderSpace operator&(const HeaderSpace& right) const;
    HeaderSpace operator-(const HeaderSpace& right) const;
    
    // TODO: check correctness and make more optimal empty()
    // (do not compact every time)
    bool empty() const {return !hs_compact(hs_);}
    
    int length() const {return length_;}
    
    friend std::ostream& operator<<(std::ostream& os,
                                    const HeaderSpace& header);
    friend class HeaderChanger;

private:
    HeaderSpace(struct hs* hs, int length);
    void clear();

    int length_;
    struct hs* hs_;
    
};

//std::ostream& operator<<(std::ostream& os, const HeaderSpace& header);

class HeaderChanger
{
public:
    // TODO: set bool:identity_ if the function is identity
    explicit HeaderChanger(int length);
    HeaderChanger(const HeaderChanger& other);
    explicit HeaderChanger(const char* transfer_str);
    HeaderChanger(const char* mask_str, const char* rewrite_str);
    ~HeaderChanger();
    
    HeaderSpace apply(const HeaderSpace& header) const;
    HeaderSpace inverse(const HeaderSpace& header) const;
    
    inline const int length() const {return length_;}
    inline const int identity() const {return identity_;}
    
    friend std::ostream& operator<<(std::ostream& os,
                                    const HeaderChanger& transfer);

private:
    int length_;
    bool identity_;
    
    array_t* transfer_;
    array_t* mask_;
    array_t* rewrite_;
    array_t* inverse_rewrite_;

};

//std::ostream& operator<<(std::ostream& os, const HeaderChanger& transfer);
