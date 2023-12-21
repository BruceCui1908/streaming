#pragma once
#include <map>
#include <string>
#include <vector>

#include "network/flat_buffer.h"

namespace rtmp {

typedef enum {
  AMF_NUMBER = 0x00,
  AMF_BOOLEAN = 0x01,
  AMF_STRING = 0x02,
  AMF_OBJECT = 0x03,
  AMF_NULL = 0x05,
  AMF_ECMA_ARRAY = 0x08,
  AMF_OBJECT_END = 0x09,
  AMF_STRICT_ARRAY = 0x0a,
  AMF_SWITCH_3 = 0x11,
} AMF0Type;

class AMFValue;

class AMFValue {
public:
  using mapType = std::map<std::string, AMFValue>;
  using arrayType = std::vector<AMFValue>;

  AMFValue(AMF0Type type = AMF0Type::AMF_NULL);
  AMFValue(const char *);
  AMFValue(const std::string &);
  AMFValue(double);
  AMFValue(bool);

  AMFValue(const AMFValue &);
  AMFValue &operator=(const AMFValue &);
  ~AMFValue();

  void clear();
  AMF0Type type() const;
  const std::string &as_string() const;
  double as_number() const;
  int as_integer() const;
  bool as_boolean() const;

  const AMFValue &operator[](const char *) const;

  operator bool() const;

  void set(const std::string &, const AMFValue &);

  const mapType &get_map() const;

private:
  void destroy();
  void init();

private:
  AMF0Type type_;

  union {
    double number;
    bool boolean;
    std::string *string;
    mapType *object;
    arrayType *array;
  } value_;
};

class AMFDecoder {
public:
  AMFDecoder(const network::flat_buffer::ptr &buf, int version = 0)
      : version_(version) {
    if (!buf) {
      throw std::runtime_error("cannot init AMFDecoder with empty flat_buffer");
    }
    buf_ = buf;
  }

  template <typename TP> TP load();

  AMFValue load_object();

  const network::flat_buffer::ptr &data() {
    if (!buf_) {
      throw std::runtime_error("AMFDecoder has empty flat_buffer");
    }
    return buf_;
  }

  uint8_t peek_front();
  uint8_t pop_front();

  std::string load_key();

  const int version() const { return version_; }

private:
  int version_;
  network::flat_buffer::ptr buf_{nullptr};
};

class AMFEncoder {
public:
  AMFEncoder &operator<<(const char *);
  AMFEncoder &operator<<(const std::string &);
  AMFEncoder &operator<<(std::nullptr_t);
  AMFEncoder &operator<<(const int);
  AMFEncoder &operator<<(const double);
  AMFEncoder &operator<<(const bool);
  // only for object
  AMFEncoder &operator<<(const AMFValue &);

  const std::string &data() const;
  void clear();

private:
  void write_key(const std::string &);

private:
  std::string buf;
};

} // namespace rtmp