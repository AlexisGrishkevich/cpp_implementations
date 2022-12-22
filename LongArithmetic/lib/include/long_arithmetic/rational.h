#pragma once
#ifndef RATIONAL_H_
#define RATIONAL_H_

#include "bigInteger.h"

#include <iostream>
#include <string>


class Rational {
private:
  BigInteger num_{0};
  BigInteger demon_{1};

public:
  Rational() = default;
  Rational(const BigInteger& num, const BigInteger& denom);
  Rational(const BigInteger& num);
  Rational(int num);

  Rational operator-() const;

  Rational& operator+=(const Rational& rhs);
  Rational& operator-=(const Rational& rhs);
  Rational& operator*=(const Rational& rhs);
  Rational& operator/=(const Rational& rhs);

  std::string toString() const;
  std::string asDecimal(std::size_t precision = 0) const;
  void swap(Rational& rhs);
  int compare(const Rational& rhs) const;
  std::istream& dump(std::istream& in);
};

Rational operator+(const Rational& lhs, const Rational& rhs);
Rational operator-(const Rational& lhs, const Rational& rhs);
Rational operator*(const Rational& lhs, const Rational& rhs);
Rational operator/(const Rational& lhs, const Rational& rhs);

bool operator==(const Rational& lhs, const Rational& rhs);
bool operator!=(const Rational& lhs, const Rational& rhs);
bool operator<(const Rational& lhs, const Rational& rhs);
bool operator>(const Rational& lhs, const Rational& rhs);
bool operator<=(const Rational& lhs, const Rational& rhs);
bool operator>=(const Rational& lhs, const Rational& rhs);

std::ostream& operator<<(std::ostream& out, const Rational& rhs);
std::istream& operator>>(std::istream& in, Rational& rhs);

void swap(Rational& lhs, Rational& rhs);

#endif // RATIONAL_H_ //------------------------------------------------//
