#pragma once

extern "C" {
  #include "hs.h"
}

#include <climits>
#include <cstring>
#include <iostream>
#include <string>

// TODO: create class inherited from struct hs,
// so I can write a destructor and use smart pointers

const int HEADER_LENGTH = 1;

class HeaderSpace
{
public:
    // TODO: add smart pointers on hs_
    // and change copy methods, so they will only copy a pointer
    // And think how to implement copy on write to hs_
    // that will be needed if I will use smart pointers
    explicit HeaderSpace(std::string str);
    HeaderSpace(const HeaderSpace& other);
    HeaderSpace(HeaderSpace&& other) noexcept;
    static HeaderSpace emptySpace(int length);
    static HeaderSpace wholeSpace(int length);
    ~HeaderSpace();

    HeaderSpace& operator=(const HeaderSpace& other);
    HeaderSpace& operator=(HeaderSpace&& other) noexcept;

    bool operator==(const HeaderSpace &other) const;
    bool operator!=(const HeaderSpace &other) const;

    HeaderSpace& operator~();
    HeaderSpace& operator+=(const HeaderSpace& right);
    HeaderSpace& operator&=(const HeaderSpace& right);
    HeaderSpace& operator-=(const HeaderSpace& right);
    HeaderSpace operator+(const HeaderSpace& right) const;
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
    explicit HeaderSpace(int length);
    HeaderSpace(struct hs* hs, int length);
    void clear();

    int length_;
    struct hs* hs_;
    
};

class HeaderChanger
{
public:
    HeaderChanger(const HeaderChanger& other);
    HeaderChanger(HeaderChanger&& other) noexcept;
    explicit HeaderChanger(const char* transfer_str);
    HeaderChanger(const char* mask_str, const char* rewrite_str);
    ~HeaderChanger();
    static HeaderChanger identityHeaderChanger(int length);

    HeaderChanger& operator=(const HeaderChanger& other);
    HeaderChanger& operator=(HeaderChanger&& other) noexcept;

    bool operator==(const HeaderChanger &other) const;
    bool operator!=(const HeaderChanger &other) const;

    // HeaderChanger superposition
    HeaderChanger operator*=(const HeaderChanger& right);
    
    HeaderSpace apply(const HeaderSpace& header) const;
    HeaderSpace inverse(const HeaderSpace& header) const;
    
    int length() const {return length_;}
    int identity() const {return identity_;}
    
    friend std::ostream& operator<<(std::ostream& os,
                                    const HeaderChanger& transfer);

private:
    explicit HeaderChanger(int length);
    void clear();

    int length_;
    bool identity_;

    array_t* mask_;
    array_t* rewrite_;
    array_t* inverse_rewrite_;

};
