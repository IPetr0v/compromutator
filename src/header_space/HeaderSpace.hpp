#pragma once

extern "C" {
#include "hs.h"
}

#include <climits>
#include <cstring>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>

enum class BitValue {
    ZERO,
    ONE,
    ANY,
    NONE
};

class BitMask
{
public:
    explicit BitMask(std::string str);
    BitMask(const BitMask& other);
    BitMask(BitMask&& other) noexcept;
    static BitMask wholeSpace(int length);
    ~BitMask();

    BitMask& operator=(const BitMask& other);
    BitMask& operator=(BitMask&& other) noexcept;

    bool operator==(const BitMask& other) const;
    bool operator<=(const BitMask& other) const;
    bool operator>=(const BitMask& other) const;

    BitValue getBit(uint32_t index) const;
    void setBit(uint32_t index, BitValue bit_value);
    //void operator[](uint32_t index);

    int length() const {return length_;}

    friend class HeaderSpace;
    friend class HeaderChanger;

private:
    BitMask(int length, array_t* array);

    // TODO: change int to uint32_t
    int length_;
    array_t* array_;

    // Header space bit value wrapping
    BitValue get_external_bit_value(enum bit_val bit_value) const;
    enum bit_val get_internal_bit_value(BitValue bit_value) const;
};

using BitMaskList = std::vector<BitMask>;

struct BitSpace {
    explicit BitSpace(BitMask mask): mask(mask) {}

    BitMask mask;
    BitMaskList difference;
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
    HeaderSpace(const BitMask& bit_vector);
    HeaderSpace(BitMask&& bit_vector);
    HeaderSpace(const HeaderSpace& other);
    HeaderSpace(HeaderSpace&& other) noexcept;
    static HeaderSpace emptySpace(int length);
    static HeaderSpace wholeSpace(int length);
    ~HeaderSpace();

    HeaderSpace& operator=(const HeaderSpace& other);
    HeaderSpace& operator=(HeaderSpace&& other) noexcept;

    bool operator==(const HeaderSpace &other) const;
    bool operator!=(const HeaderSpace &other) const;

    bool operator>=(const HeaderSpace &other) const;
    bool operator<=(const HeaderSpace &other) const;
    //bool operator>(const HeaderSpace &other) const;
    //bool operator<(const HeaderSpace &other) const;

    HeaderSpace& operator~();
    HeaderSpace& operator+=(const HeaderSpace& right);
    HeaderSpace& operator&=(const HeaderSpace& right);
    HeaderSpace& operator-=(const HeaderSpace& right);
    HeaderSpace operator+(const HeaderSpace& right) const;
    HeaderSpace operator&(const HeaderSpace& right) const;
    HeaderSpace operator-(const HeaderSpace& right) const;

    HeaderSpace& compact();
    HeaderSpace& computeDifference();
    
    // TODO: check correctness and make more optimal empty()
    // (do not compact every time)
    bool empty() const {return !hs_compact(hs_);}
    int length() const {return length_;}

    std::vector<BitSpace> getBitSpace() const;

    static int GLOBAL_LENGTH;

    std::string toString() const;
    friend std::ostream& operator<<(std::ostream& os,
                                    const HeaderSpace& header);
    friend class HeaderChanger;

private:
    explicit HeaderSpace(int length);
    HeaderSpace(int length, struct hs* hs);
    void clear();

    int length_;
    struct hs* hs_;
    //std::shared_ptr<struct hs> hs__;
    
};

class HeaderChanger
{
public:
    explicit HeaderChanger(const BitMask& bit_vector);
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

    std::string toString() const;
    friend std::ostream& operator<<(std::ostream& os,
                                    const HeaderChanger& transfer);

private:
    explicit HeaderChanger(int length);
    HeaderChanger(int length, array_t* transfer_array);
    void clear();

    int length_;
    bool identity_;

    array_t* mask_;
    array_t* rewrite_;
    array_t* inverse_rewrite_;

};
