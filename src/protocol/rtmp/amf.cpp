#include "amf.h"

#include <cstring>
#include <stdexcept>

namespace rtmp {

/// AMFValue
AMFValue::AMFValue(AMFType type) : type_{type} { init(); }

AMFValue::AMFValue(const char *s) : AMFValue(AMF_STRING) { *value_.string = s; }

AMFValue::AMFValue(const std::string &s) : AMFValue(s.c_str()) {}

AMFValue::AMFValue(double n) : AMFValue(AMF_NUMBER) { value_.number = n; }

AMFValue::AMFValue(int n) : AMFValue(AMF_INTEGER) { value_.integer = n; }

AMFValue::AMFValue(bool b) : AMFValue(AMF_BOOLEAN) { value_.boolean = b; }

AMFValue::~AMFValue() { destroy(); }

void AMFValue::destroy() {
  switch (type_) {
  case AMF_STRING: {
    if (value_.string) {
      delete value_.string;
      value_.string = nullptr;
    }
    break;
  }

  case AMF_OBJECT:
  case AMF_ECMA_ARRAY: {
    if (value_.object) {
      delete value_.object;
      value_.object = nullptr;
    }
    break;
  }

  case AMF_STRICT_ARRAY: {
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
  case AMF_OBJECT:
  case AMF_ECMA_ARRAY: {
    value_.object = new mapType;
    break;
  }

  case AMF_STRING: {
    value_.string = new std::string;
    break;
  }

  case AMF_STRICT_ARRAY: {
    value_.array = new arrayType;
    break;
  }

  default:
    break;
  }
}

AMFValue::AMFValue(const AMFValue &other) : type_{AMF_NULL} { *this = other; }

AMFValue &AMFValue::operator=(const AMFValue &other) {
  destroy();
  type_ = other.type_;
  init();

  switch (type_) {
  case AMF_STRING: {
    *value_.string = (*other.value_.string);
    break;
  }

  case AMF_OBJECT:
  case AMF_ECMA_ARRAY: {
    *value_.object = (*other.value_.object);
    break;
  }

  case AMF_STRICT_ARRAY: {
    *value_.array = (*other.value_.array);
    break;
  }

  case AMF_NUMBER: {
    value_.number = other.value_.number;
    break;
  }

  case AMF_INTEGER: {
    value_.integer = other.value_.integer;
    break;
  }

  case AMF_BOOLEAN: {
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
  case AMF_STRING: {
    if (value_.string) {
      value_.string->clear();
    }
    break;
  }

  case AMF_OBJECT:
  case AMF_ECMA_ARRAY: {
    if (value_.object) {
      value_.object->clear();
    }
    break;
  }

  default:
    break;
  }
}

AMFType AMFValue::type() const { return type_; }

const std::string &AMFValue::as_string() const {
  if (type_ != AMF_STRING) {
    throw std::runtime_error("AMF not a string");
  }

  return *value_.string;
}

double AMFValue::as_number() const {
  switch (type_) {
  case AMF_NUMBER:
    return value_.number;
  case AMF_INTEGER:
    return value_.integer;
  case AMF_BOOLEAN:
    return value_.boolean;
  default:
    throw std::runtime_error("AMF not a number");
  }
}

int AMFValue::as_integer() const {
  switch (type_) {
  case AMF_NUMBER:
    return static_cast<int>(value_.number);
  case AMF_INTEGER:
    return value_.integer;
  case AMF_BOOLEAN:
    return value_.boolean;
  default:
    throw std::runtime_error("AMF not a integer");
  }
}

bool AMFValue::as_boolean() const {
  switch (type_) {
  case AMF_NUMBER:
    return value_.number;
  case AMF_INTEGER:
    return value_.integer;
  case AMF_BOOLEAN:
    return value_.boolean;
  default:
    throw std::runtime_error("AMF not a boolean");
  }
}

const AMFValue &AMFValue::operator[](const char *key) const {
  if (type_ != AMF_OBJECT && type_ != AMF_ECMA_ARRAY) {
    throw std::runtime_error("AMF is neither an object nor an array");
  }

  if (!value_.object) {
    throw std::runtime_error("AMF obj not initialized");
  }

  auto it = value_.object->find(key);
  if (it == value_.object->end()) {
    static AMFValue val(AMF_NULL);
    return val;
  }

  return it->second;
}

AMFValue::operator bool() const { return type_ != AMF_NULL; }

void AMFValue::set(const std::string &key, const AMFValue &value) {
  if (type_ != AMF_OBJECT && type_ != AMF_ECMA_ARRAY) {
    throw std::runtime_error("AMF is neither an object nor an array");
  }

  if (!value_.object) {
    throw std::runtime_error("AMF obj not initialized");
  }

  value_.object->emplace(key, value);
}

const AMFValue::mapType &AMFValue::get_map() const {
  if (type_ != AMF_OBJECT && type_ != AMF_ECMA_ARRAY) {
    throw std::runtime_error("AMF is neither an object nor an array");
  }
  return *value_.object;
}

enum {
  AMF0_NUMBER,
  AMF0_BOOLEAN,
  AMF0_STRING,
  AMF0_OBJECT,
  AMF0_MOVIECLIP,
  AMF0_NULL,
  AMF0_UNDEFINED,
  AMF0_REFERENCE,
  AMF0_ECMA_ARRAY,
  AMF0_OBJECT_END,
  AMF0_STRICT_ARRAY,
  AMF0_DATE,
  AMF0_LONG_STRING,
  AMF0_UNSUPPORTED,
  AMF0_RECORD_SET,
  AMF0_XML_OBJECT,
  AMF0_TYPED_OBJECT,
  AMF0_SWITCH_AMF3,
};

enum {
  AMF3_UNDEFINED,
  AMF3_NULL,
  AMF3_FALSE,
  AMF3_TRUE,
  AMF3_INTEGER,
  AMF3_NUMBER,
  AMF3_STRING,
  AMF3_LEGACY_XML,
  AMF3_DATE,
  AMF3_ARRAY,
  AMF3_OBJECT,
  AMF3_XML,
  AMF3_BYTE_ARRAY,
};

/// AMFDecoder
uint8_t AMFDecoder::pop_front() {
  uint8_t x = buf_.read_uint8();
  if (version_ == 0 && x == AMF0_SWITCH_AMF3) {
    version_ = 3;
  }
  return x;
}

uint8_t AMFDecoder::peek_front() { return buf_.peek_uint8(); }

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
  uint8_t type = pop_front();

  if (version_ == 3) {
    if (type != AMF3_STRING) {
      throw std::runtime_error("Expected a string");
    }
    str_len = load<unsigned int>() / 2;
  } else {
    if (type != AMF0_STRING) {
      throw std::runtime_error("Expected a string");
    }
    str_len = buf_.read_uint16();
  }

  return buf_.to_string(str_len);
}

template <> double AMFDecoder::load<double>() {
  uint8_t type = pop_front();
  if (type != AMF0_NUMBER) {
    throw std::runtime_error("Expected a number");
  }

  uint64_t num = buf_.read_uint64();
  double n = 0;
  std::memcpy(&n, &num, sizeof(num));
  return n;
}

std::string AMFDecoder::load_key() {
  return buf_.to_string(buf_.read_uint16());
}

AMFValue AMFDecoder::load_object() {
  if (pop_front() != AMF0_OBJECT) {
    throw std::runtime_error("expect an object");
  }

  AMFValue object(AMF_OBJECT);
  while (1) {
    std::string key = load_key();
    if (key.empty()) {
      break;
    }
    std::string value = load<std::string>();
    object.set(key, value);
  }

  pop_front();

  return object;
}

} // namespace rtmp