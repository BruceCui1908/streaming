#pragma once
#include <map>
#include <string>
#include <vector>

#include "network/flat_buffer.h"

namespace rtmp {

enum AMFType {
  AMF_NUMBER,
  AMF_INTEGER,
  AMF_BOOLEAN,
  AMF_STRING,
  AMF_OBJECT,
  AMF_NULL,
  AMF_UNDEFINED,
  AMF_ECMA_ARRAY,
  AMF_STRICT_ARRAY,
};

class AMFValue;

class AMFValue {
public:
  using mapType = std::map<std::string, AMFValue>;
  using arrayType = std::vector<AMFValue>;

  AMFValue(AMFType type = AMF_NULL);
  AMFValue(const char *);
  AMFValue(const std::string &);
  AMFValue(double);
  AMFValue(int);
  AMFValue(bool);

  AMFValue(const AMFValue &);
  AMFValue &operator=(const AMFValue &);
  ~AMFValue();

  void clear();
  AMFType type() const;
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
  AMFType type_;

  union {
    double number;
    int integer;
    bool boolean;
    std::string *string;
    mapType *object;
    arrayType *array;
  } value_;
};

class AMFDecoder {
public:
  AMFDecoder(network::flat_buffer &buf, int version = 0)
      : buf_(buf), version_(version) {}

  template <typename TP> TP load();

  AMFValue load_object();

private:
  uint8_t peek_front();
  uint8_t pop_front();

  std::string load_key();

private:
  network::flat_buffer &buf_;
  int version_;
};

} // namespace rtmp