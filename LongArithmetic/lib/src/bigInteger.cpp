#include "long_arithmetic/bigInteger.h"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <vector>
#include <utility>


namespace details {

void removeZeros(std::vector<int>& number) {
  while (!number.empty() && number.back() == 0) {
    number.pop_back();
  }
}

void toBase10(std::vector<int>& number) {
  int accumulator{0};
  for (std::size_t i = 0; accumulator || i < number.size(); ++i) {
    if (i >= number.size()) number.push_back(0);
    accumulator += number[i];
    number[i] = accumulator % 10;
    accumulator /= 10;
  }
}

int absCompare(const std::vector<int>& lhs, const std::vector<int>& rhs,
               std::size_t pos = 0) {
  if (lhs.size() < rhs.size() + pos)
    return -1;

  if (lhs.size() > rhs.size() + pos)
    return 1;

  for (std::size_t i = rhs.size(); i--;) {
    if (lhs[i + pos] < rhs[i]) return -1;
    if (lhs[i + pos] > rhs[i]) return 1;
  }

  return 0;
}

void absSubstraction(std::vector<int>& lhs, const std::vector<int>& rhs,
                     std::size_t pos = 0) {
  for (std::size_t i = 0; i + pos != lhs.size(); ++i) {
    if (i < rhs.size())
      lhs[i + pos] -= rhs[i];

    if (lhs[i + pos] < 0) {
      lhs[i + pos] += 10;
      --lhs[i + pos + 1];
    }
  }
}

std::vector<int> devide(std::vector<int>& lhs, const std::vector<int>& rhs) {
  std::vector<int> tmp(lhs.size(), 0);
  for (std::size_t i = tmp.size(); i--;) {
    for (std::size_t j = 0; j != 9; ++j) {
      removeZeros(lhs);
      if (absCompare(lhs, rhs, i) >= 0) {
        absSubstraction(lhs, rhs, i);
        ++tmp[i];
      } else {
        break;
      }
    }
  }

  return tmp;
}

} // namespace details //-----------------------------------------------//

// BigInteger implementation //-----------------------------------------//
BigInteger::BigInteger(int number) {
  if (number < 0) {
    sign_ = -1;
    number *= -1;
  }

  while (number) {
    number_.push_back(number % 10);
    number /= 10;
  }

  details::removeZeros(number_);
  if (number_.empty()) sign_ = 1;
}

BigInteger::BigInteger(std::string number_str) {
  number_.reserve(number_str.size());
  std::reverse(std::begin(number_str), std::end(number_str));
  if (number_str.back() == '-') {
    sign_ = -1;
    number_str.pop_back();
  }

  for (auto&& item : number_str) {
    number_.push_back(item - '0');
  }

  details::removeZeros(number_);
  if (number_.empty()) sign_ = 1;
}

BigInteger BigInteger::operator-() const {
  BigInteger tmp{*this};
  if (tmp)
    tmp.sign_ *= -1;

  return tmp;
}

BigInteger& BigInteger::operator++() {
  *this += 1;
  return *this;
}

BigInteger BigInteger::operator++(int) {
  BigInteger tmp{*this};
  ++(*this);
  return tmp;
}

BigInteger& BigInteger::operator--() {
  *this -= 1;
  return *this;
}

BigInteger BigInteger::operator--(int) {
  BigInteger tmp{*this};
  --(*this);
  return tmp;
}

BigInteger& BigInteger::operator-=(const BigInteger& rhs) {
  auto tmp{rhs};
  if (*this == tmp) {
    number_.clear();
    sign_ = 1;
    return *this;
  }

  if (sign_ == tmp.sign_) {
    auto current_sign = sign_;
    sign_ = tmp.sign_ = 1;
    if (tmp < *this) {
      details::absSubstraction(number_, tmp.number_);
      sign_ = (current_sign > 0 ? 1 : -1);
    } else if (*this < tmp) {
      details::absSubstraction(tmp.number_, number_);
      number_ = tmp.number_;
      sign_ = (current_sign > 0 ? -1 : 1);
    }
  } else {
    tmp.sign_ *= -1;
    *this += tmp;
  }

  details::removeZeros(number_);
  return *this;
}

BigInteger& BigInteger::operator+=(const BigInteger& rhs) {
  auto tmp{rhs};
  if (sign_ == tmp.sign_) {
    if (number_.size() < tmp.number_.size())
      number_.resize(tmp.number_.size(), 0);

    for (std::size_t i = 0; i != tmp.number_.size(); ++i) {
      number_[i] += tmp.number_[i];
    }

    details::toBase10(number_);
    details::removeZeros(number_);
  } else {
    tmp.sign_ *= -1;
    *this -= tmp;
  }

  return *this;
}

BigInteger& BigInteger::operator*=(const BigInteger& rhs) {
  if (*this == 0)
    return *this;

  if (rhs == 0) {
    number_.clear();
    sign_ = 1;
    return *this;
  }

  std::vector<int> accumulator(std::max(number_.size(), rhs.number_.size()) + 1);

  for (std::size_t j = 0; j != rhs.number_.size(); ++j) {
    for (std::size_t k = 0; k != number_.size(); ++k) {
      auto element = number_[k] * rhs.number_[j];
      accumulator[k + j] += element;
    }
  }

  number_ = accumulator;
  sign_ *= rhs.sign_;
  details::toBase10(number_);
  details::removeZeros(number_);
  return *this;
}

BigInteger& BigInteger::operator/=(const BigInteger& rhs) {
  if (!rhs)
    throw std::runtime_error("division by zero");

  number_ = details::devide(number_, rhs.number_);
  details::removeZeros(number_);
  number_.empty() ? sign_ = 1 : sign_ *= rhs.sign_;
  return *this;
}

BigInteger& BigInteger::operator%=(const BigInteger& rhs) {
  if (!rhs)
    throw std::runtime_error("Division by zero.");

  details::devide(number_, rhs.number_);
  number_.empty() ? sign_ = 1 : sign_ *= rhs.sign_;
  return *this;
}

BigInteger::operator bool() const {
  return !number_.empty();
}

std::string BigInteger::toString() const {
  std::string tmp;

  for (auto&& item : number_) {
    tmp.push_back(static_cast<char>(item) + '0');
  }

  if (number_.empty()) {
    tmp.push_back('0');
  } else if (sign_ < 0) {
    tmp.push_back('-');
  }

  std::reverse(std::begin(tmp), std::end(tmp));
  return tmp;
}

void BigInteger::swap(BigInteger& rhs) {
  using std::swap;
  swap(sign_, rhs.sign_);
  swap(number_, rhs.number_);
}

bool BigInteger::compare(const BigInteger& rhs) const {
  if (number_.empty())
    return rhs.number_.empty();

  return (sign_ == rhs.sign_) ? number_ == rhs.number_ : false;
}

bool BigInteger::less(const BigInteger& rhs) const {
    if (*this == rhs)
      return false;

    if (sign_ < rhs.sign_)
      return true;

    if (rhs.sign_ < sign_)
      return false;

    return details::absCompare(number_, number_) * sign_ < 0;
}

BigInteger operator+(const BigInteger& lhs, const BigInteger& rhs) {
  BigInteger tmp{lhs};
  tmp += rhs;
  return tmp;
}

BigInteger operator-(const BigInteger& lhs, const BigInteger& rhs) {
  BigInteger tmp{lhs};
  tmp -= rhs;
  return tmp;
}

BigInteger operator*(const BigInteger& lhs, const BigInteger& rhs) {
  BigInteger tmp{lhs};
  tmp *= rhs;
  return tmp;
}

BigInteger operator/(const BigInteger& lhs, const BigInteger& rhs) {
  BigInteger tmp{lhs};
  tmp /= rhs;
  return tmp;
}

BigInteger operator%(const BigInteger& lhs, const BigInteger& rhs) {
  BigInteger tmp{lhs};
  tmp %= rhs;
  return tmp;
}

bool operator==(const BigInteger& lhs, const BigInteger& rhs) {
  return lhs.compare(rhs);
}

bool operator!=(const BigInteger& lhs, const BigInteger& rhs) {
  return !(lhs == rhs);
}

bool operator<(const BigInteger& lhs, const BigInteger& rhs) {
  return lhs.less(rhs);
}

bool operator>(const BigInteger& lhs, const BigInteger& rhs) {
  return rhs < lhs;
}

bool operator<=(const BigInteger& lhs, const BigInteger& rhs) {
  return !(rhs < lhs);
}

bool operator>=(const BigInteger& lhs, const BigInteger& rhs) {
  return !(lhs < rhs);
}

BigInteger operator""_bi(const char* str) {
  return BigInteger(static_cast<std::string>(str));
}

std::ostream& operator<<(std::ostream& out, const BigInteger& rhs) {
  auto str = rhs.toString();
  out << str;
  return out;
}

std::istream& operator>>(std::istream& in, BigInteger& rhs) {
  std::string tmp;
  in >> tmp;
  rhs = BigInteger{tmp};
  return in;
}

void swap(BigInteger& lhs, BigInteger& rhs) {
  lhs.swap(rhs);
}

BigInteger abs(const BigInteger& number) {
  return (number < 0) ? -number : number;
}

BigInteger gcd(const BigInteger& lhs, const BigInteger& rhs) {
  auto tmp_lhs = abs(lhs);
  auto tmp_rhs = abs(rhs);
  while(tmp_rhs) {
      tmp_lhs %= tmp_rhs;
      swap(tmp_lhs, tmp_rhs);
  }

  return tmp_lhs;
}

BigInteger lcm(const BigInteger& lhs, const BigInteger& rhs) {
  return (lhs * rhs) / gcd(lhs, rhs);
}
