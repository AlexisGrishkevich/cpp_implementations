#include "long_arithmetic/rational.h"

#include <algorithm>
#include <exception>


namespace details {

void reduction(BigInteger& num, BigInteger& denom) {
  BigInteger tmp_gcd = gcd(num, denom);
  num /= tmp_gcd;
  denom /= tmp_gcd;
  if (denom < 0) {
    num *= -1;
    denom *= -1;
  }
}

} // namespace details //-----------------------------------------------//

// Rational implementation //-------------------------------------------//
Rational::Rational(const BigInteger& num, const BigInteger& denom)
    : num_{num}, demon_{denom} {
  details::reduction(num_, demon_);
}

Rational::Rational(const BigInteger& num) : num_{num}, demon_{1}
{ }

Rational::Rational(int num) : num_{num}, demon_{1}
{ }

Rational Rational::operator-() const {
  auto tmp{*this};
  tmp *= -1;
  return tmp;
}

Rational& Rational::operator+=(const Rational& rhs) {
  num_ = num_ * rhs.demon_ + rhs.num_ * demon_;
  demon_ = demon_ * rhs.demon_;
  details::reduction(num_, demon_);
  return *this;
}

Rational& Rational::operator-=(const Rational& rhs) {
  num_ = num_ * rhs.demon_ - rhs.num_ * demon_;
  demon_ = demon_ * rhs.demon_;
  details::reduction(num_, demon_);
  return *this;
}

Rational& Rational::operator*=(const Rational& rhs) {
  num_ *= rhs.num_;
  demon_ *= rhs.demon_;
  details::reduction(num_, demon_);
  return *this;
}

Rational& Rational::operator/=(const Rational& rhs) {
  num_ *= rhs.demon_;
  demon_ *= rhs.num_;
  details::reduction(num_, demon_);
  return *this;
}


std::string Rational::toString() const {
  if (demon_ == 1)
    return num_.toString();

  return num_.toString() + "/" + demon_.toString();
}

std::string Rational::asDecimal(std::size_t precision) const {
  std::string res{};
  Rational number{*this};
  bool is_dot = false;
  if (number < 0) {
    res += '-';
    number *= -1;
    ++precision;
  }

  while (precision) {
    BigInteger tmp = number.num_ / number.demon_;
    number -= tmp;
    number *= 10;
    res += tmp.toString();
    --precision;

    if (!is_dot && precision) {
      is_dot = true;
      res += '.';
    }
  }

  return res;
}

void Rational::swap(Rational& rhs) {
  num_.swap(rhs.num_);
  demon_.swap(rhs.demon_);
}

int Rational::compare(const Rational& rhs) const {
  auto tmp_lhs{*this};
  auto tmp_rhs{rhs};
  tmp_lhs.num_ *= tmp_rhs.demon_;
  tmp_rhs.num_ *= tmp_lhs.demon_;
  if (tmp_lhs.num_ < tmp_rhs.num_)
    return -1;

  if (tmp_lhs.num_ > tmp_rhs.num_)
    return 1;

  return 0;
}

std::istream& Rational::dump(std::istream& in) {
  std::string num{};
  std::getline(in, num, '/');
  std::string denom{};
  std::getline(in, denom);
  num_ = num;
  demon_ = denom;
  if (!demon_)
    throw std::runtime_error("the denominator is zero");

  return in;
}

Rational operator+(const Rational& lhs, const Rational& rhs) {
  auto tmp{lhs};
  tmp += rhs;
  return tmp;
}

Rational operator-(const Rational& lhs, const Rational& rhs) {
  auto tmp{lhs};
  tmp -= rhs;
  return tmp;
}

Rational operator*(const Rational& lhs, const Rational& rhs) {
  auto tmp{lhs};
  tmp *= rhs;
  return tmp;
}

Rational operator/(const Rational& lhs, const Rational& rhs) {
  auto tmp{lhs};
  tmp /= rhs;
  return tmp;
}

bool operator==(const Rational& lhs, const Rational& rhs) {
  return lhs.compare(rhs) == 0;
}

bool operator!=(const Rational& lhs, const Rational& rhs) {
  return !(lhs == rhs);
}

bool operator<(const Rational& lhs, const Rational& rhs) {
  return lhs.compare(rhs) < 0;
}

bool operator>(const Rational& lhs, const Rational& rhs) {
  return rhs < lhs;
}

bool operator<=(const Rational& lhs, const Rational& rhs) {
  return !(rhs < lhs);
}

bool operator>=(const Rational& lhs, const Rational& rhs) {
  return !(lhs < rhs);
}

std::ostream& operator<<(std::ostream& out, const Rational& rhs) {
  out << rhs.toString();
  return out;
}

std::istream& operator>>(std::istream& in, Rational& rhs) {
  return rhs.dump(in);
}

void swap(Rational& lhs, Rational& rhs) {
  lhs.swap(rhs);
}
