#include "HeaderSpace.hpp"

#include <bitset>

int HeaderSpace::GLOBAL_LENGTH = 1;

static int get_len(const char* str) {
    auto commas = (bool)strchr(str, ',');
    int div = CHAR_BIT + commas;
    int len = (int)strlen(str) + commas;
    assert(len % div == 0);
    len /= div;
    return len;
}

BitMask::BitMask(std::string str):
    length_(get_len(str.c_str()))
{
    assert(length_ > 0);
    array_ = array_from_str(str.c_str());
}

BitMask::~BitMask()
{
    delete array_;
}

BitMask::BitMask(const BitMask& other):
    length_ (other.length_), array_(array_copy(other.array_, length_))
{

}

BitMask::BitMask(BitMask&& other) noexcept:
    length_ (other.length_), array_(other.array_)
{
    other.array_ = nullptr;
}

BitMask BitMask::wholeSpace(int length)
{
    array_t* whole_space = array_create(length, BIT_X);
    return {length, whole_space};
}


BitMask& BitMask::operator=(const BitMask& other)
{
    length_ = other.length_;
    array_ = array_copy(other.array_, length_);
    return *this;
}

BitMask& BitMask::operator=(BitMask&& other) noexcept
{
    length_ = other.length_;
    array_ = other.array_;
    other.array_ = nullptr;
    return *this;
}

bool BitMask::operator==(const BitMask& other) const
{
    return array_is_eq(array_, other.array_, length_);
}

bool BitMask::operator<=(const BitMask& other) const
{
    return array_is_sub(other.array_, array_, length_);
}

bool BitMask::operator>=(const BitMask& other) const
{
    return array_is_sub(array_, other.array_, length_);
}

BitMask::BitMask(int length, array_t* array):
    length_(length), array_(array_copy(array, length))
{

}

BitValue BitMask::getBit(uint32_t index) const
{
    int byte = index/CHAR_BIT;
    int bit = index%CHAR_BIT;
    return get_external_bit_value(array_get_bit(array_, byte, bit));
}

void BitMask::setBit(uint32_t index, BitValue bit_value)
{
    uint32_t byte = index/CHAR_BIT;
    uint32_t bit = index%CHAR_BIT;
    array_set_bit(array_, get_internal_bit_value(bit_value), byte, bit);
}

BitValue BitMask::get_external_bit_value(enum bit_val bit_value) const
{
    switch (bit_value) {
    case BIT_0: return BitValue::ZERO;
    case BIT_1: return BitValue::ONE;
    case BIT_X: return BitValue::ANY;
    case BIT_Z: return BitValue::NONE;
    default:    return BitValue::NONE;
    }
}

enum bit_val BitMask::get_internal_bit_value(BitValue bit_value) const
{
    switch (bit_value) {
    case BitValue::ZERO: return BIT_0;
    case BitValue::ONE:  return BIT_1;
    case BitValue::ANY:  return BIT_X;
    case BitValue::NONE: return BIT_Z;
    default:             return BIT_Z;
    }
}

HeaderSpace::HeaderSpace(std::string str):
    length_(get_len(str.c_str()))
{
    assert(length_ > 0);
    hs_ = hs_create(length_);
    hs_add(hs_, array_from_str(str.c_str()));
}

HeaderSpace::HeaderSpace(const BitMask& bit_vector):
    length_(bit_vector.length_)
{
    hs_ = hs_create(length_);
    if (bit_vector.array_) {
        auto array = array_copy(bit_vector.array_, length_);
        hs_add(hs_, array);
    }
}

HeaderSpace::HeaderSpace(BitMask&& bit_vector):
    length_(bit_vector.length_)
{
    hs_ = hs_create(length_);
    if(bit_vector.array_) hs_add(hs_, bit_vector.array_);
    bit_vector.array_ = nullptr;
}

HeaderSpace::HeaderSpace(const HeaderSpace& other):
    length_(other.length_)
{
    hs_ = hs_create(other.length_);
    hs_copy(hs_, other.hs_);
}

HeaderSpace::HeaderSpace(HeaderSpace&& other) noexcept:
    length_(other.length_), hs_(other.hs_)
{
    other.hs_ = nullptr;
}

HeaderSpace::HeaderSpace(int length):
    length_(length)
{
    hs_ = hs_create(length_);
}

HeaderSpace::HeaderSpace(int length, struct hs* hs):
    length_(length)
{
    hs_ = hs_create(length_);
    if (hs) hs_copy(hs_, hs);
}

HeaderSpace HeaderSpace::emptySpace(int length)
{
    return HeaderSpace(length);
}

HeaderSpace HeaderSpace::wholeSpace(int length)
{
    HeaderSpace header(length);
    array_t* whole_space = array_create(length, BIT_X);
    hs_add(header.hs_, whole_space);
    return header;
}

HeaderSpace::~HeaderSpace()
{
    this->clear();
}

void HeaderSpace::clear()
{
    if (hs_) {
        hs_free(hs_);
        hs_ = nullptr;
    }
}

HeaderSpace& HeaderSpace::operator=(const HeaderSpace& other)
{
    clear();
    length_ = other.length_;
    hs_ = hs_create(other.length_);
    hs_copy(hs_, other.hs_);
    return *this;
}

HeaderSpace& HeaderSpace::operator=(HeaderSpace&& other) noexcept
{
    clear();
    length_ = other.length_;
    hs_ = other.hs_;
    other.hs_ = nullptr;
    return *this;
}

bool HeaderSpace::operator==(const HeaderSpace &other) const
{
    assert(length_ == other.length_);
    auto inclusion = *this - other;
    auto exclusion = other - *this;
    inclusion.computeDifference();
    exclusion.computeDifference();
    return inclusion.empty() && exclusion.empty();
}

bool HeaderSpace::operator!=(const HeaderSpace &other) const
{
    return !(*this == other);
}

HeaderSpace& HeaderSpace::operator~()
{
    hs_cmpl(hs_);
    return *this;
}

HeaderSpace& HeaderSpace::operator+=(const HeaderSpace& right) {
    hs_sum(hs_, right.hs_);
    return *this;
}

HeaderSpace& HeaderSpace::operator&=(const HeaderSpace& right)
{
    struct hs* intersection = hs_isect_a(hs_, right.hs_);
    if (intersection) {
        hs_copy(hs_, intersection);
    }
    else {
        this->clear();
        hs_ = hs_create(length_);
    }
    return *this;
}

HeaderSpace& HeaderSpace::operator-=(const HeaderSpace& right)
{
    if (hs_count_diff(right.hs_)) {
        hs_minus(hs_, right.hs_);
    }
    else {
        for (int i = 0; i < right.hs_->list.used; i++) {
            hs_diff(hs_, right.hs_->list.elems[i]);
        }
        hs_compact(hs_);
    }
    return *this;
}

HeaderSpace HeaderSpace::operator+(const HeaderSpace& right) const
{
    HeaderSpace header(length_, hs_);
    header += right;
    return header;
}

HeaderSpace HeaderSpace::operator&(const HeaderSpace& right) const
{
    return HeaderSpace(length_, hs_isect_a(hs_, right.hs_));
}

HeaderSpace HeaderSpace::operator-(const HeaderSpace& right) const
{
    HeaderSpace header(length_, hs_);
    header -= right;
    return header;
}

HeaderSpace& HeaderSpace::compact()
{
    hs_compact(hs_);
    return *this;
}

HeaderSpace& HeaderSpace::computeDifference()
{
    if (not empty()) hs_comp_diff(hs_);
    return *this;
}

std::vector<BitSpace> HeaderSpace::getBitSpace() const
{
    hs_compact(hs_);

    // Get bit vectors
    std::vector<BitSpace> bit_space;
    for (int i = 0; i < hs_->list.used; i++) {
        // Get mask
        BitMask mask(length_, hs_->list.elems[i]);
        auto space_it = bit_space.emplace(
            bit_space.end(), std::move(mask)
        );

        // Get difference
        if (hs_->list.diff) {
            for (int j = 0; j < hs_->list.diff[i].used; j++) {
                BitMask diff_mask(length_, hs_->list.diff[i].elems[j]);
                space_it->difference.emplace(
                    space_it->difference.end(), std::move(diff_mask)
                );
            }
        }
    }
    return bit_space;
}

std::string HeaderSpace::toString() const
{
    // Check header corruption
    if (nullptr == hs_) {
        return "CORRUPTED";
    }

    // Create header string representation
    struct hs* output_hs = hs_copy_a(hs_);

    char* output_string = hs_to_str(output_hs);
    std::string result(output_string);

    free(output_string);
    hs_free(output_hs);
    return result;
}

std::ostream& operator<<(std::ostream& os, const HeaderSpace& header)
{
    os << header.toString();
    return os;
}

HeaderChanger::HeaderChanger(int length):
    length_(length), identity_(true)
{
    // Function without rewrite
    mask_            = array_create(length_, BIT_1);
    rewrite_         = array_create(length_, BIT_0);
    inverse_rewrite_ = array_create(length_, BIT_0);
}

HeaderChanger::HeaderChanger(int length, array_t* transfer_array):
    length_(length)
{
    assert(length_ > 0);
    mask_            = array_create(length_, BIT_1);
    rewrite_         = array_create(length_, BIT_0);
    inverse_rewrite_ = array_create(length_, BIT_0);

    bool identity_check = true;
    for (int i = 0; i < length_*CHAR_BIT; i++) {
        int byte = i/CHAR_BIT;
        int bit = i%CHAR_BIT;

        enum bit_val transfer_bit = array_get_bit(transfer_array, byte, bit);
        enum bit_val mask_bit;
        enum bit_val rewrite_bit;
        enum bit_val inverse_rewrite_bit;

        switch(transfer_bit) {
        case BIT_0:
        case BIT_1:
            mask_bit = BIT_0;
            rewrite_bit = transfer_bit;
            inverse_rewrite_bit = BIT_X;
            identity_check = false;
            break;

        case BIT_X:
        case BIT_Z:
        default:
            mask_bit = BIT_1;
            rewrite_bit = BIT_0;
            inverse_rewrite_bit = BIT_0;
            break;
        }
        assert(transfer_bit != BIT_Z and mask_bit != BIT_Z and
               rewrite_bit != BIT_Z and inverse_rewrite_bit != BIT_Z);

        array_set_bit(mask_, mask_bit, byte, bit);
        array_set_bit(rewrite_, rewrite_bit, byte, bit);
        array_set_bit(inverse_rewrite_, inverse_rewrite_bit, byte, bit);
    }
    identity_ = identity_check;
}

HeaderChanger::HeaderChanger(const BitMask& bit_vector):
    HeaderChanger(bit_vector.length_, bit_vector.array_)
{

}

HeaderChanger::HeaderChanger(const HeaderChanger& other):
    length_(other.length_), identity_(other.identity_)
{
    mask_            = array_copy(other.mask_, length_);
    rewrite_         = array_copy(other.rewrite_, length_);
    inverse_rewrite_ = array_copy(other.inverse_rewrite_, length_);
}

HeaderChanger::HeaderChanger(HeaderChanger&& other) noexcept:
    length_(other.length_),  identity_(other.identity_)
{
    mask_            = other.mask_;
    rewrite_         = other.rewrite_;
    inverse_rewrite_ = other.inverse_rewrite_;

    other.mask_            = nullptr;
    other.rewrite_         = nullptr;
    other.inverse_rewrite_ = nullptr;
}

HeaderChanger::HeaderChanger(const char* transfer_str):
    HeaderChanger(get_len(transfer_str),
                  std::unique_ptr<array_t>(array_from_str(transfer_str)).get())
{

}

HeaderChanger::HeaderChanger(const char* mask_str, const char* rewrite_str)
{
    int mask_len = get_len(mask_str);
    int rewrite_len = get_len(rewrite_str);
    assert(mask_len == rewrite_len);
    length_ = mask_len;

    mask_            = array_from_str(mask_str);
    rewrite_         = array_from_str(rewrite_str);
    inverse_rewrite_ = array_create(length_, BIT_0);

    bool identity_check = true;
    for (int i = 0; i < length_*CHAR_BIT; i++) {
        int byte = i/CHAR_BIT;
        int bit = i%CHAR_BIT;

        enum bit_val transfer_bit;
        enum bit_val mask_bit = array_get_bit(mask_, byte, bit);
        enum bit_val rewrite_bit = array_get_bit(rewrite_, byte, bit);
        enum bit_val inverse_rewrite_bit;

        if(mask_bit == BIT_1) {
            transfer_bit = BIT_X;
            inverse_rewrite_bit = BIT_0;
        }
        else {
            transfer_bit = rewrite_bit;
            inverse_rewrite_bit = BIT_X;
        }
        assert(transfer_bit != BIT_Z and mask_bit != BIT_Z and
               rewrite_bit != BIT_Z and inverse_rewrite_bit != BIT_Z);

        if (transfer_bit != BIT_X) {
            identity_check = false;
        }

        array_set_bit(inverse_rewrite_, inverse_rewrite_bit, byte, bit);
    }
    identity_ = identity_check;
}

HeaderChanger::~HeaderChanger()
{
    this->clear();
}

void HeaderChanger::clear()
{
    if (mask_)            array_free(mask_);
    if (rewrite_)         array_free(rewrite_);
    if (inverse_rewrite_) array_free(inverse_rewrite_);
}

HeaderChanger HeaderChanger::identityHeaderChanger(int length)
{
    return HeaderChanger(length);
}

HeaderChanger& HeaderChanger::operator=(const HeaderChanger& other)
{
    this->clear();

    length_ = other.length_;
    identity_ = other.identity_;

    mask_            = array_copy(other.mask_, length_);
    rewrite_         = array_copy(other.rewrite_, length_);
    inverse_rewrite_ = array_copy(other.inverse_rewrite_, length_);

    return *this;
}

HeaderChanger& HeaderChanger::operator=(HeaderChanger&& other) noexcept
{
    this->clear();

    length_ = other.length_;
    identity_ = other.identity_;

    mask_            = other.mask_;
    rewrite_         = other.rewrite_;
    inverse_rewrite_ = other.inverse_rewrite_;

    other.mask_            = nullptr;
    other.rewrite_         = nullptr;
    other.inverse_rewrite_ = nullptr;
    return *this;
}

bool HeaderChanger::operator==(const HeaderChanger &other) const
{
    return array_is_eq(mask_, other.mask_, length_) &&
           array_is_eq(rewrite_, other.rewrite_, length_) &&
           array_is_eq(inverse_rewrite_, other.inverse_rewrite_, length_);
}

bool HeaderChanger::operator!=(const HeaderChanger &other) const
{
    return !(*this == other);
}

HeaderChanger HeaderChanger::operator*=(const HeaderChanger& right)
{
    assert(length_ == right.length_);
    if (identity_) {
        *this = right;
    }
    else {
        identity_ &= right.identity_;
        if (not right.identity_) {
            // New mask is a logical and
            for (int i = 0; i < length_ * CHAR_BIT; i++) {
                int byte = i / CHAR_BIT;
                int bit = i % CHAR_BIT;

                enum bit_val mask_bit = array_get_bit(mask_, byte, bit);
                enum bit_val right_mask_bit = array_get_bit(
                    right.mask_, byte, bit);
                assert(mask_bit != BIT_Z and right_mask_bit != BIT_Z);
                enum bit_val result_mask_bit;

                if (mask_bit == BIT_0 || right_mask_bit == BIT_0) {
                    result_mask_bit = BIT_0;
                }
                else {
                    result_mask_bit = BIT_1;
                }

                array_set_bit(mask_, result_mask_bit, byte, bit);
            }

            array_rewrite(rewrite_, right.mask_, right.rewrite_, length_);
            array_rewrite(inverse_rewrite_, right.mask_,
                          right.inverse_rewrite_, length_);
        }
    }
    return *this;
}

HeaderSpace HeaderChanger::apply(const HeaderSpace& header) const
{
    HeaderSpace new_header(header);
    if (not identity_) {
        hs_rewrite(new_header.hs_, mask_, rewrite_);
    }
    return std::move(new_header);
}

HeaderSpace HeaderChanger::inverse(const HeaderSpace& header) const
{
    HeaderSpace new_header(header);
    if (not identity_) {
        hs_rewrite(new_header.hs_, mask_, inverse_rewrite_);
    }
    return std::move(new_header);
}

std::string HeaderChanger::toString() const
{
    // Check header changer corruption
    if (nullptr == mask_ ||
        nullptr == rewrite_ ||
        nullptr == inverse_rewrite_) {
        return "CORRUPTED";
    }

    // Create header changer string representation
    int length = length_;
    array_t* transfer_array = array_create(length, BIT_X);
    for (int i = 0; i < length*CHAR_BIT; i++) {
        int byte = i/CHAR_BIT;
        int bit = i%CHAR_BIT;

        enum bit_val transfer_bit;
        enum bit_val mask_bit = array_get_bit(mask_, byte, bit);
        enum bit_val rewrite_bit = array_get_bit(rewrite_, byte, bit);

        if(mask_bit == BIT_1) {
            transfer_bit = BIT_X;
        }
        else {
            transfer_bit = rewrite_bit;
        }
        array_set_bit(transfer_array, transfer_bit, byte, bit);
    }

    std::string result(array_to_str(transfer_array, length, false));
    array_free(transfer_array);
    return result;
}

std::ostream& operator<<(std::ostream& os, const HeaderChanger& transfer)
{
    os << transfer.toString();
    return os;
}
