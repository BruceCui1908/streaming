#include "amf.h"

#include <cstring>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace rtmp {

/// AMFValue
AMFValue::AMFValue(AMF0Type type) : type_{type} { init(); }

AMFValue::AMFValue(const char *s) : AMFValue(AMF0Type::AMF_STRING) {
  *value_.string = s;
}

AMFValue::AMFValue(const std::string &s) : AMFValue(s.c_str()) {}

AMFValue::AMFValue(double n) : AMFValue(AMF0Type::AMF_NUMBER) {
  value_.number = n;
}

AMFValue::AMFValue(bool b) : AMFValue(AMF0Type::AMF_BOOLEAN) {
  value_.boolean = b;
}

AMFValue::~AMFValue() { destroy(); }

void AMFValue::destroy() {
  switch (type_) {
  case AMF0Type::AMF_STRING: {
    if (value_.string) {
      delete value_.string;
      value_.string = nullptr;
    }
    break;
  }

  case AMF0Type::AMF_OBJECT:
  case AMF0Type::AMF_ECMA_ARRAY: {
    if (value_.object) {
      delete value_.object;
      value_.object = nullptr;
    }
    break;
  }

  case AMF0Type::AMF_STRICT_ARRAY: {
    if (value_.array) {
      delete value_.array;
      value_.array = nullptr;
    }
    break;
  }

  default:
    break;
  }
}

void AMFValue::init() {
  switch (type_) {
  case AMF0Type::AMF_OBJECT:
  case AMF0Type::AMF_ECMA_ARRAY: {
    value_.object = new mapType;
    break;
  }

  case AMF0Type::AMF_STRING: {
    value_.string = new std::string;
    break;
  }

  case AMF0Type::AMF_STRICT_ARRAY: {
    value_.array = new arrayType;
    break;
  }

  default:
    break;
  }
}

AMFValue::AMFValue(const AMFValue &other) : type_{AMF0Type::AMF_NULL} {
  *this = other;
}

AMFValue &AMFValue::operator=(const AMFValue &other) {
  destroy();
  type_ = other.type_;
  init();

  switch (type_) {
  case AMF0Type::AMF_STRING: {
    *value_.string = (*other.value_.string);
    break;
  }

  case AMF0Type::AMF_OBJECT:
  case AMF0Type::AMF_ECMA_ARRAY: {
    *value_.object = (*other.value_.object);
    break;
  }

  case AMF0Type::AMF_STRICT_ARRAY: {
    *value_.array = (*other.value_.array);
    break;
  }

  case AMF0Type::AMF_NUMBER: {
    value_.number = other.value_.number;
    break;
  }

  case AMF0Type::AMF_BOOLEAN: {
    value_.boolean = other.value_.boolean;
    break;
  }

  default:
    break;
  }

  return *this;
}

void AMFValue::clear() {
  switch (type_) {
  case AMF0Type::AMF_STRING: {
    if (value_.string) {
      value_.string->clear();
    }
    break;
  }

  case AMF0Type::AMF_OBJECT:
  case AMF0Type::AMF_ECMA_ARRAY: {
    if (value_.object) {
      value_.object->clear();
    }
    break;
  }

  default:
    break;
  }
}

AMF0Type AMFValue::type() const { return type_; }

const std::string &AMFValue::as_string() const {
  if (type_ != AMF0Type::AMF_STRING) {
    throw std::runtime_error("AMF not a string");
  }

  return *value_.string;
}

double AMFValue::as_number() const {
  switch (type_) {
  case AMF0Type::AMF_NUMBER:
    return value_.number;
  case AMF0Type::AMF_BOOLEAN:
    return value_.boolean;
  default:
    throw std::runtime_error("AMF not a number");
  }
}

int AMFValue::as_integer() const {
  switch (type_) {
  case AMF0Type::AMF_NUMBER:
    return static_cast<int>(value_.number);
  case AMF0Type::AMF_BOOLEAN:
    return value_.boolean;
  default:
    throw std::runtime_error("AMF not a integer");
  }
}

bool AMFValue::as_boolean() const {
  switch (type_) {
  case AMF0Type::AMF_NUMBER:
    return value_.number;
  case AMF0Type::AMF_BOOLEAN:
    return value_.boolean;
  default:
    throw std::runtime_error("AMF not a boolean");
  }
}

const AMFValue &AMFValue::operator[](const char *key) const {
  if (type_ != AMF0Type::AMF_OBJECT && type_ != AMF0Type::AMF_ECMA_ARRAY) {
    throw std::runtime_error("AMF is neither an object nor an array");
  }

  if (!value_.object) {
    throw std::runtime_error("AMF obj not initialized");
  }

  auto it = value_.object->find(key);
  if (it == value_.object->end()) {
    static AMFValue val(AMF0Type::AMF_NULL);
    return val;
  }

  return it->second;
}

AMFValue::operator bool() const { return type_ != AMF_NULL; }

void AMFValue::set(const std::string &key, const AMFValue &value) {
  if (type_ != AMF0Type::AMF_OBJECT && type_ != AMF0Type::AMF_ECMA_ARRAY) {
    throw std::runtime_error("AMF is neither an object nor an array");
  }

  if (!value_.object) {
    throw std::runtime_error("AMF obj not initialized");
  }

  value_.object->emplace(key, value);
}

const AMFValue::mapType &AMFValue::get_map() const {
  if (type_ != AMF0Type::AMF_OBJECT && type_ != AMF0Type::AMF_ECMA_ARRAY) {
    throw std::runtime_error("AMF is neither an object nor an array");
  }
  return *value_.object;
}

/// AMFDecoder
uint8_t AMFDecoder::pop_front() {
  uint8_t x = data()->read_uint8();
  if (version_ == 0 && x == AMF0Type::AMF_SWITCH_3) {
    version_ = 3;
  }
  return x;
}

uint8_t AMFDecoder::peek_front() { return data()->peek_uint8(); }

template <> unsigned int AMFDecoder::load<unsigned int>() {
  unsigned int value = 0;
  for (int i = 0; i < 4; ++i) {
    uint8_t b = pop_front();
    if (i == 3) {
      /* use all bits from 4th byte */
      value = (value << 8) | b;
      break;
    }
    value = (value << 7) | (b & 0x7f);
    if ((b & 0x80) == 0)
      break;
  }
  return value;
}

template <> std::string AMFDecoder::load<std::string>() {
  size_t str_len = 0;
  if (pop_front() != AMF0Type::AMF_STRING) {
    throw std::runtime_error("Expected a string");
  }
  str_len = data()->read_uint16();
  return data()->to_string(str_len);
}

template <> double AMFDecoder::load<double>() {
  uint8_t type = pop_front();
  if (type != AMF0Type::AMF_NUMBER) {
    throw std::runtime_error("Expected a number");
  }

  uint64_t num = data()->read_uint64();
  double n = 0;
  std::memcpy(&n, &num, sizeof(num));
  return n;
}

template <> bool AMFDecoder::load<bool>() {
  uint8_t type = pop_front();
  if (type != AMF0Type::AMF_BOOLEAN) {
    throw std::runtime_error("Expected a bool");
  }

  return pop_front() != 0;
}

std::string AMFDecoder::load_key() {
  return data()->to_string(data()->read_uint16());
}

AMFValue AMFDecoder::load_object() {
  if (pop_front() != AMF0Type::AMF_OBJECT) {
    throw std::runtime_error("expect an object");
  }

  AMFValue object(AMF0Type::AMF_OBJECT);
  while (1) {
    std::string key = load_key();
    if (key.empty()) {
      break;
    }
    std::string value = load<std::string>();
    object.set(key, value);
  }

  if (pop_front() != AMF0Type::AMF_OBJECT_END) {
    throw std::runtime_error("expect an object end");
  }

  return object;
}

/// AMFEncoder
AMFEncoder &AMFEncoder::operator<<(const char *s) {
  if (!s) {
    throw std::runtime_error("AMFEncoder cannot accpet empty string");
  }

  buf += char(AMF0Type::AMF_STRING);
  auto len = std::strlen(s);
  uint16_t str_len = htons((uint16_t)len);
  buf.append((char *)&str_len, 2);
  buf += s;
  return *this;
}

AMFEncoder &AMFEncoder::operator<<(const std::string &s) {
  return (*this) << (s.c_str());
}

AMFEncoder &AMFEncoder::operator<<(std::nullptr_t) {
  buf += char(AMF0Type::AMF_NULL);
  return *this;
}

AMFEncoder &AMFEncoder::operator<<(const int n) { return (*this) << (double)n; }

AMFEncoder &AMFEncoder::operator<<(const double num) {
  buf += char(AMF0Type::AMF_NUMBER);
  uint64_t encoded = 0;
  memcpy(&encoded, &num, 8);
  uint32_t val = htonl(encoded >> 32);
  buf.append((char *)&val, 4);
  val = htonl(encoded & 0xFFFFFFFF);
  buf.append((char *)&val, 4);
  return *this;
}

AMFEncoder &AMFEncoder::operator<<(const bool b) {
  buf += char(AMF0Type::AMF_BOOLEAN);
  buf += char(b);
  return *this;
}

AMFEncoder &AMFEncoder::operator<<(const AMFValue &obj) {
  int amf_type = (int)obj.type();
  switch (amf_type) {
  case AMF0Type::AMF_STRING: {
    *this << obj.as_string();
    break;
  }
  case AMF0Type::AMF_NUMBER: {
    *this << obj.as_number();
    break;
  }

  case AMF0Type::AMF_OBJECT: {
    if (obj.type() != AMF0Type::AMF_OBJECT) {
      throw std::runtime_error("only accept AMF0_OBJECT type");
    }
    buf += char(AMF0Type::AMF_OBJECT);
    for (auto &pr : obj.get_map()) {
      write_key(pr.first);
      *this << pr.second;
    }
    write_key("");
    buf += char(AMF0Type::AMF_OBJECT_END);
    break;
  }

  default:
    spdlog::error("unsupported amf type {}", amf_type);
    break;
  }
  return *this;
}

const std::string &AMFEncoder::data() const { return buf; }

void AMFEncoder::clear() { buf.clear(); }

void AMFEncoder::write_key(const std::string &s) {
  uint16_t str_len = htons((uint16_t)s.size());
  buf.append((char *)&str_len, 2);
  buf += s;
}

} // namespace rtmp