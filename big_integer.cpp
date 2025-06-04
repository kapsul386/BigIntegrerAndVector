#include "big_integer.h"

BigInteger::BigInteger() : is_negative_(false) {
}

BigInteger::BigInteger(int value) : is_negative_(value < 0) {
  AddDigits(static_cast<int64_t>(std::abs(static_cast<int64_t>(value))));
}

BigInteger::BigInteger(int64_t value) : is_negative_(value < 0) {
  AddDigits(std::abs(value));
}

BigInteger::BigInteger(const std::string& value) {
  ParseString(value);
}

BigInteger::BigInteger(const char* value) {
  ParseString(std::string(value));
}

void BigInteger::AddDigits(int64_t value) {
  while (value > 0) {
    digits_.push_back(static_cast<int>(value % kBase));
    value /= kBase;
  }
}

void BigInteger::ParseString(const std::string& str) {
  is_negative_ = false;
  digits_.clear();

  size_t start = 0;
  if (str[0] == '-') {
    is_negative_ = true;
    start = 1;
  } else if (str[0] == '+') {
    start = 1;
  }

  for (int i = static_cast<int>(str.length()) - 1; i >= static_cast<int>(start); i -= kBaseDigits) {
    int digit = 0;
    int end = std::max(static_cast<int>(start), i - kBaseDigits + 1);

    for (int j = end; j <= i; ++j) {
      CheckOverflow(digit * 10 + (str[j] - '0'));
      digit = digit * 10 + (str[j] - '0');
    }

    digits_.push_back(digit);
  }

  RemoveLeadingZeros();
}

void BigInteger::Normalize() {
  RemoveLeadingZeros();
  if (digits_.empty()) {
    is_negative_ = false;
  }
}

void BigInteger::RemoveLeadingZeros() {
  while (!digits_.empty() && digits_.back() == 0) {
    digits_.pop_back();
  }
}

void BigInteger::CheckOverflow(int value) const {
  if (value < 0 || value >= kBase) {
    throw BigIntegerOverflow();
  }
}

void BigInteger::CheckDivision(const BigInteger& divisor) const {
  if (divisor.digits_.empty()) {
    throw BigIntegerDivisionByZero();
  }
}

bool BigInteger::IsNegative() const {
  return is_negative_;
}

BigInteger BigInteger::Absolute() const {
  BigInteger result = *this;
  result.is_negative_ = false;
  return result;
}

BigInteger BigInteger::operator+() const {
  return *this;
}

BigInteger BigInteger::operator-() const {
  BigInteger result = *this;
  result.is_negative_ = !is_negative_;
  result.Normalize();
  return result;
}

BigInteger& BigInteger::operator+=(const BigInteger& other) {
  if (is_negative_ == other.is_negative_) {
    size_t required_size = std::max(digits_.size(), other.digits_.size()) + 1;
    for (; digits_.size() < required_size; digits_.push_back(0)) {
    }

    int carry = 0;
    for (size_t i = 0; i < digits_.size() || carry != 0; ++i) {
      if (i == digits_.size()) {
        digits_.push_back(0);
      }

      digits_[i] += carry + (i < other.digits_.size() ? other.digits_[i] : 0);
      carry = digits_[i] >= kBase;
      if (carry) {
        digits_[i] -= kBase;
      }
      CheckOverflow(digits_[i]);
    }
  } else {
    *this -= -other;
  }

  Normalize();
  return *this;
}

BigInteger& BigInteger::operator-=(const BigInteger& other) {
  if (is_negative_ == other.is_negative_) {
    if (Absolute() >= other.Absolute()) {
      int borrow = 0;
      for (size_t i = 0; i < other.digits_.size() || borrow != 0; ++i) {
        if (i == digits_.size()) {
          digits_.push_back(0);
        }
        digits_[i] -= borrow + (i < other.digits_.size() ? other.digits_[i] : 0);
        borrow = digits_[i] < 0;
        if (borrow) {
          digits_[i] += kBase;
        }
        CheckOverflow(digits_[i]);
      }
    } else {
      *this = -(other - *this);
    }
  } else {
    *this += -other;
  }

  Normalize();
  return *this;
}

BigInteger& BigInteger::operator*=(const BigInteger& other) {
  BigInteger result;
  MultiplyHelper(*this, other, result);
  *this = result;
  return *this;
}

void BigInteger::MultiplyHelper(const BigInteger& a, const BigInteger& b, BigInteger& result) {
  result.digits_.resize(a.digits_.size() + b.digits_.size());
  result.is_negative_ = a.is_negative_ != b.is_negative_;

  for (size_t i = 0; i < a.digits_.size(); ++i) {
    int carry = 0;
    for (size_t j = 0; j < b.digits_.size() || carry != 0; ++j) {
      int64_t product = static_cast<int64_t>(a.digits_[i]) * (j < b.digits_.size() ? b.digits_[j] : 0) + carry;
      if (i + j < result.digits_.size()) {
        product += result.digits_[i + j];
      }

      if (i + j < result.digits_.size()) {
        result.digits_[i + j] = static_cast<int>(product % kBase);
      }
      carry = static_cast<int>(product / kBase);
    }
  }

  result.Normalize();

  if (result.DigitCount() > 30009) {
    throw BigIntegerOverflow();
  }
}

BigInteger& BigInteger::operator/=(const BigInteger& other) {
  CheckDivision(other);
  BigInteger quotient;
  BigInteger remainder;
  DivideHelper(*this, other, quotient, remainder);
  *this = quotient;
  return *this;
}

BigInteger& BigInteger::operator%=(const BigInteger& other) {
  CheckDivision(other);
  BigInteger quotient;
  BigInteger remainder;
  DivideHelper(*this, other, quotient, remainder);
  *this = remainder;
  return *this;
}

void BigInteger::DivideHelper(const BigInteger& dividend, const BigInteger& divisor, BigInteger& quotient,
                              BigInteger& remainder) {
  BigInteger abs_dividend = dividend.Absolute();
  BigInteger abs_divisor = divisor.Absolute();

  quotient.digits_.resize(abs_dividend.digits_.size());

  for (int i = static_cast<int>(abs_dividend.digits_.size()) - 1; i >= 0; --i) {
    remainder.digits_.insert(remainder.digits_.begin(), abs_dividend.digits_[i]);
    remainder.Normalize();

    int left = 0;
    int right = kBase;
    int digit = 0;

    while (left <= right) {
      int mid = (left + right) / 2;
      BigInteger temp = abs_divisor * BigInteger(mid);

      if (temp <= remainder) {
        digit = mid;
        left = mid + 1;
      } else {
        right = mid - 1;
      }
    }

    quotient.digits_[i] = digit;
    remainder -= abs_divisor * BigInteger(digit);
  }

  quotient.is_negative_ = dividend.is_negative_ != divisor.is_negative_;
  remainder.is_negative_ = dividend.is_negative_;

  quotient.Normalize();
  remainder.Normalize();
}

BigInteger::operator bool() const {
  return !digits_.empty();
}

BigInteger& BigInteger::operator++() {
  *this += BigInteger(1);
  return *this;
}

BigInteger BigInteger::operator++(int) {
  BigInteger temp = *this;
  ++*this;
  return temp;
}

BigInteger& BigInteger::operator--() {
  *this -= BigInteger(1);
  return *this;
}

BigInteger BigInteger::operator--(int) {
  BigInteger temp = *this;
  --*this;
  return temp;
}

size_t BigInteger::DigitCount() const {
  if (digits_.empty()) {
    return 1;
  }

  size_t count = (digits_.size() - 1) * kBaseDigits;
  int last = digits_.back();

  while (last > 0) {
    last /= 10;
    ++count;
  }

  return count;
}

void BigInteger::HandleCarry(size_t index, int& carry) {
  while (carry != 0 && index < digits_.size()) {
    digits_[index] += carry;
    carry = digits_[index] / kBase;
    digits_[index] %= kBase;
    ++index;
  }
  if (carry != 0) {
    digits_.push_back(carry);
  }
}

void BigInteger::HandleBorrow(size_t index, int& borrow) {
  while (borrow != 0 && index < digits_.size()) {
    digits_[index] -= borrow;
    borrow = 0;
    if (digits_[index] < 0) {
      digits_[index] += kBase;
      borrow = 1;
    }
    ++index;
  }
}

void BigInteger::EnsureCapacity(size_t size) {
  if (digits_.size() < size) {
    digits_.resize(size, 0);
  }
}

void BigInteger::CompareDigits(const BigInteger& a, const BigInteger& b, int& result) {
  if (a.digits_.size() != b.digits_.size()) {
    result = (a.digits_.size() < b.digits_.size()) ? -1 : 1;
    return;
  }

  for (int i = static_cast<int>(a.digits_.size()) - 1; i >= 0; --i) {
    if (a.digits_[i] != b.digits_[i]) {
      result = (a.digits_[i] < b.digits_[i]) ? -1 : 1;
      return;
    }
  }

  result = 0;
}

bool operator==(const BigInteger& a, const BigInteger& b) {
  return !(a < b) && !(b < a);
}

bool operator!=(const BigInteger& a, const BigInteger& b) {
  return !(a == b);
}

bool operator<(const BigInteger& a, const BigInteger& b) {
  if (a.is_negative_ != b.is_negative_) {
    return a.is_negative_;
  }
  if (a.digits_.size() != b.digits_.size()) {
    return (a.digits_.size() < b.digits_.size()) != a.is_negative_;
  }
  for (int i = static_cast<int>(a.digits_.size()) - 1; i >= 0; --i) {
    if (a.digits_[i] != b.digits_[i]) {
      return (a.digits_[i] < b.digits_[i]) != a.is_negative_;
    }
  }
  return false;
}

bool operator<=(const BigInteger& a, const BigInteger& b) {
  return !(b < a);
}

bool operator>(const BigInteger& a, const BigInteger& b) {
  return b < a;
}

bool operator>=(const BigInteger& a, const BigInteger& b) {
  return !(a < b);
}

std::ostream& operator<<(std::ostream& os, const BigInteger& value) {
  if (value.is_negative_) {
    os << '-';
  }
  if (value.digits_.empty()) {
    os << '0';
  } else {
    os << value.digits_.back();
    for (int i = static_cast<int>(value.digits_.size()) - 2; i >= 0; --i) {
      os << std::setw(BigInteger::kBaseDigits) << std::setfill('0') << value.digits_[i];
    }
  }
  return os;
}

std::istream& operator>>(std::istream& is, BigInteger& value) {
  std::string s;
  is >> s;
  value = BigInteger(s);
  return is;
}

BigInteger operator+(BigInteger a, const BigInteger& b) {
  return a += b;
}

BigInteger operator-(BigInteger a, const BigInteger& b) {
  return a -= b;
}

BigInteger operator*(BigInteger a, const BigInteger& b) {
  return a *= b;
}

BigInteger operator/(BigInteger a, const BigInteger& b) {
  return a /= b;
}

BigInteger operator%(BigInteger a, const BigInteger& b) {
  return a %= b;
}