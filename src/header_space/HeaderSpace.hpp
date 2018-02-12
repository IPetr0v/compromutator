#pragma once

extern "C" {
  #include "hs.h"
}

#include <climits>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

enum class BitValue {
    ZERO,
    ONE,
    ANY,
    NONE
};

class BitVector
{
public:
    explicit BitVector(std::string str);
    BitVector(const BitVector& other);
    BitVector(BitVector&& other) noexcept;
    static BitVector wholeSpace(int length);

    // BitVector does not own array_t pointer
    ~BitVector() = default;

    BitVector& operator=(const BitVector& other);
    BitVector& operator=(BitVector&& other) noexcept;

    BitValue getBit(uint32_t index) const;
    void setBit(uint32_t index, BitValue bit_value);
    //void operator[](uint32_t index);

    friend class HeaderSpace;

private:
    BitVector(int length, array_t* array);

    int length_;
    array_t* array_;

    // Header space bit value wrapping
    BitValue get_external_bit_value(enum bit_val bit_value) const;
    enum bit_val get_internal_bit_value(BitValue bit_value) const;
};

// TODO: create class inherited from struct hs,
// so I can write a destructor and use smart pointers

class HeaderSpace
{
public:
    // TODO: add smart pointers on hs_
    // and change copy methods, so they will only copy a pointer
    // And think how to implement copy on write to hs_
    // that will be needed if I will use smart pointers
    explicit HeaderSpace(std::string str);
    explicit HeaderSpace(BitVector bit_vector);
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

    std::vector<BitVector> getBitVectors() const;

    static int GLOBAL_LENGTH;

    friend std::ostream& operator<<(std::ostream& os,
                                    const HeaderSpace& header);
    friend class HeaderChanger;

private:
    explicit HeaderSpace(int length);
    HeaderSpace(int length, struct hs* hs);
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
