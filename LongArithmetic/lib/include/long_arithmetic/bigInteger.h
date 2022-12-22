#pragma once
#ifndef BIGINTEGER_H_
#define BIGINTEGER_H_

#include <iostream>
#include <string>
#include <vector>


class BigInteger {
private:
  int sign_{1};
  std::vector<int> number_{};

public:
  BigInteger() = default;
  BigInteger(int number);
  BigInteger(std::string number_str);
  BigInteger(const BigInteger&) = default;
  BigInteger& operator=(const BigInteger&) = default;

  BigInteger operator-() const;
  BigInteger& operator++();
  BigInteger operator++(int);
  BigInteger& operator--();
  BigInteger operator--(int);

  BigInteger& operator+=(const BigInteger& rhs);
  BigInteger& operator-=(const BigInteger& rhs);
  BigInteger& operator*=(const BigInteger& rhs);
  BigInteger& operator/=(const BigInteger& rhs);
  BigInteger& operator%=(const BigInteger& rhs);

  explicit operator bool() const;

  std::string toString() const;
  void swap(BigInteger& rhs);
  bool compare(const BigInteger& rhs) const;
  bool less(const BigInteger& rhs) const;
};

BigInteger operator+(const BigInteger& lhs, const BigInteger& rhs);
BigInteger operator-(const BigInteger& lhs, const BigInteger& rhs);
BigInteger operator*(const BigInteger& lhs, const BigInteger& rhs);
BigInteger operator/(const BigInteger& lhs, const BigInteger& rhs);
BigInteger operator%(const BigInteger& lhs, const BigInteger& rhs);

bool operator==(const BigInteger& lhs, const BigInteger& rhs);
bool operator!=(const BigInteger& lhs, const BigInteger& rhs);
bool operator<(const BigInteger& lhs, const BigInteger& rhs);
bool operator>(const BigInteger& lhs, const BigInteger& rhs);
bool operator<=(const BigInteger& lhs, const BigInteger& rhs);
bool operator>=(const BigInteger& lhs, const BigInteger& rhs);

BigInteger operator""_bi(const char* str);

std::ostream& operator<<(std::ostream& out, const BigInteger& rhs);
std::istream& operator>>(std::istream& in, BigInteger& rhs);

void swap(BigInteger& lhs, BigInteger& rhs);
BigInteger abs(const BigInteger& number);
BigInteger gcd(const BigInteger& lhs, const BigInteger& rhs);
BigInteger lcm(const BigInteger& lhs, const BigInteger& rhs);

#endif // BIGINTEGER_H_ //----------------------------------------------//
