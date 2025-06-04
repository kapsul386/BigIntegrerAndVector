#pragma once

#define BIG_INTEGER_DIVISION_IMPLEMENTED

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <cstdint>

class BigIntegerException : public std::runtime_error {
 public:
  explicit BigIntegerException(const std::string& msg) : std::runtime_error(msg) {
  }
};

class BigIntegerOverflow : public BigIntegerException {
 public:
  BigIntegerOverflow() : BigIntegerException("BigInteger overflow") {
  }
};

class BigIntegerDivisionByZero : public BigIntegerException {
 public:
  BigIntegerDivisionByZero() : BigIntegerException("Division by zero") {
  }
};

class BigInteger {
 private:
  static constexpr int kBase = 10000;
  static constexpr int kBaseDigits = 4;

  std::vector<int> digits_;
  bool is_negative_;

  void Normalize();
  void ParseString(const std::string& str);
  void AddDigits(int64_t value);
  void HandleCarry(size_t index, int& carry);
  void HandleBorrow(size_t index, int& borrow);
  void EnsureCapacity(size_t size);
  void RemoveLeadingZeros();
  void CheckOverflow(int value) const;
  void CheckDivision(const BigInteger& divisor) const;

  static void MultiplyHelper(const BigInteger& a, const BigInteger& b, BigInteger& result);
  static void DivideHelper(const BigInteger& dividend, const BigInteger& divisor, BigInteger& quotient,
                           BigInteger& remainder);
  static void CompareDigits(const BigInteger& a, const BigInteger& b, int& result);

 public:
  BigInteger();
  BigInteger(int value);                      // NOLINT
  BigInteger(int64_t value);                  // NOLINT
  BigInteger(const std::string& value);       // NOLINT
  BigInteger(const char* value);              // NOLINT

  BigInteger(const BigInteger&) = default;
  BigInteger(BigInteger&&) noexcept = default;

  BigInteger& operator=(const BigInteger&) = default;
  BigInteger& operator=(BigInteger&&) noexcept = default;

  bool IsNegative() const;
  BigInteger Absolute() const;

  BigInteger operator+() const;
  BigInteger operator-() const;

  BigInteger& operator+=(const BigInteger& other);
  BigInteger& operator-=(const BigInteger& other);
  BigInteger& operator*=(const BigInteger& other);
  BigInteger& operator/=(const BigInteger& other);
  BigInteger& operator%=(const BigInteger& other);

  BigInteger& operator++();
  BigInteger operator++(int);
  BigInteger& operator--();
  BigInteger operator--(int);

  explicit operator bool() const;

  friend bool operator==(const BigInteger& a, const BigInteger& b);
  friend bool operator!=(const BigInteger& a, const BigInteger& b);
  friend bool operator<(const BigInteger& a, const BigInteger& b);
  friend bool operator<=(const BigInteger& a, const BigInteger& b);
  friend bool operator>(const BigInteger& a, const BigInteger& b);
  friend bool operator>=(const BigInteger& a, const BigInteger& b);

  friend std::ostream& operator<<(std::ostream& os, const BigInteger& value);
  friend std::istream& operator>>(std::istream& is, BigInteger& value);

  size_t DigitCount() const;
};

BigInteger operator+(BigInteger a, const BigInteger& b);
BigInteger operator-(BigInteger a, const BigInteger& b);
BigInteger operator*(BigInteger a, const BigInteger& b);
BigInteger operator/(BigInteger a, const BigInteger& b);
BigInteger operator%(BigInteger a, const BigInteger& b);
