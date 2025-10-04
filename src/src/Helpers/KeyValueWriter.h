#pragma once

#include "../../ESPEasy_common.h"

#include "../Helpers/PrintToString.h"
#include "../Helpers/StringProvider.h"

#include "../WebServer/HTML_Print.h"

#include <vector>
#include <memory>

// ********************************************************************************
// ValueStruct
// ********************************************************************************
struct ValueStruct
{
  enum class ValueType {
    Auto,
    String,
    Float,
    Double,
    Int,
    Bool

  };


  ValueStruct() {}

  ValueStruct(const String& val,
              ValueType     vType = ValueType::Auto);
  ValueStruct(const __FlashStringHelper *val,
              ValueType                  vType = ValueType::Auto);
  ValueStruct(String && val,
              ValueType vType = ValueType::Auto);


  String    str;
  ValueType valueType = ValueType::Auto;

};

// ********************************************************************************
// KeyValueStruct
// ********************************************************************************
struct KeyValueStruct
{

  KeyValueStruct() {}

  KeyValueStruct(const __FlashStringHelper *key);
  KeyValueStruct(const String& key);

  KeyValueStruct(const String         & key,
                 bool                   val,
                 ValueStruct::ValueType vType = ValueStruct::ValueType::Bool);

  KeyValueStruct(const String         & key,
                 int                    val,
                 ValueStruct::ValueType vType = ValueStruct::ValueType::Int);

  KeyValueStruct(const String         & key,
                 const uint64_t       & val,
                 ValueStruct::ValueType vType = ValueStruct::ValueType::Int);

  KeyValueStruct(const String         & key,
                 const int64_t        & val,
                 ValueStruct::ValueType vType = ValueStruct::ValueType::Int);


  KeyValueStruct(const String         & key,
                 const float          & val,
                 int                    nrDecimals = 4,
                 ValueStruct::ValueType vType      = ValueStruct::ValueType::Float);

  KeyValueStruct(const String         & key,
                 const double         & val,
                 int                    nrDecimals = 4,
                 ValueStruct::ValueType vType      = ValueStruct::ValueType::Double);


  KeyValueStruct(const __FlashStringHelper *key,
                 const String             & val,
                 ValueStruct::ValueType     vType = ValueStruct::ValueType::Auto);

  KeyValueStruct(const __FlashStringHelper *key,
                 const __FlashStringHelper *val,
                 ValueStruct::ValueType     vType = ValueStruct::ValueType::Auto);

  KeyValueStruct(const String         & key,
                 const String         & val,
                 ValueStruct::ValueType vType = ValueStruct::ValueType::Auto);

  KeyValueStruct(const __FlashStringHelper *key,
                 String                  && val,
                 ValueStruct::ValueType     vType = ValueStruct::ValueType::Auto);


  KeyValueStruct(const String         & key,
                 String              && val,
                 ValueStruct::ValueType vType = ValueStruct::ValueType::Auto);

  template<typename T>
  KeyValueStruct(const String         & key,
                 const T              & val,
                 ValueStruct::ValueType vType = ValueStruct::ValueType::Auto)
    : _key(key) {
    _values.emplace_back(String(val), vType);
  }

  KeyValueStruct(LabelType::Enum label);

  void setUnit(const String& unit);
  void setUnit(const __FlashStringHelper *unit);

  void appendValue(const ValueStruct& value);

  void appendValue(ValueStruct&& value);


  String _key;
  String _id;
  String _unit;

  std::vector<ValueStruct>_values;

  // output as pre-formatted monospaced
  bool _value_pre{};
  bool _isArray{};

};


class KeyValueWriter;
typedef std::shared_ptr<KeyValueWriter> Sp_KeyValueWriter;

// ********************************************************************************
// KeyValueWriter
// ********************************************************************************
class KeyValueWriter
{
public:

  KeyValueWriter(bool emptyHeader = false, PrintToString *toStr = nullptr) : _toString(toStr),  _hasHeader(emptyHeader) {}

protected:

  KeyValueWriter(KeyValueWriter*parent, PrintToString *toStr = nullptr) :  _toString(toStr), _parent(parent) {}

  KeyValueWriter(bool emptyHeader, KeyValueWriter*parent, PrintToString *toStr = nullptr) :  _toString(toStr), _parent(parent),
    _hasHeader(emptyHeader) {}

  KeyValueWriter(const String& header, KeyValueWriter*parent, PrintToString *toStr = nullptr) :  _toString(toStr),  _header(header), _parent(parent)
    {}

public:

  virtual ~KeyValueWriter() {}

  virtual void setHeader(const String& header) {
    _header    = header;
    _hasHeader = true;
  }

  virtual void clear();

  // Mark a write, typically called from a child calling its parent it is about to write
  virtual void write() = 0;

  virtual void write(const KeyValueStruct& kv) = 0;

  void         writeLabels(const LabelType::Enum labels[]);

  //  virtual void setParent(KeyValueWriter*parent) { _parent = parent; }

  virtual int  getLevel() const;

  virtual void setIsArray() { _isArray = true; }

  // Only supported for JSON writer right now
  // TODO TD-er: Change this to std::shared_ptr<PrintToString>
  virtual void              setOutputToString(PrintToString*printToStr) { _toString = printToStr; }

  virtual void              indent()                                    {}

  // Create writer of the same derived type, with this set as parent
  virtual Sp_KeyValueWriter createChild()                     = 0;
  virtual Sp_KeyValueWriter createChild(const String& header) = 0;

  // Create new writer of the same derived type, without parent
  virtual Sp_KeyValueWriter createNew()                     = 0;
  virtual Sp_KeyValueWriter createNew(const String& header) = 0;

  const String&             get() const {
    if (_toString == nullptr) { return EMPTY_STRING; }
    return _toString->get();
  }

  String && getMove()
  {
    if (_toString == nullptr) { 
        static String tmp;
        return std::move(tmp); 
    }
    return std::move(_toString->getMove());
  }

  bool reserve(unsigned int size) { return _toString && _toString->reserve(size); }

protected:

  Print& getPrint() {
    if (_toString == nullptr) { return _toWeb; }
    return *_toString;
  }

  // TODO TD-er: Change this to std::shared_ptr<PrintToString>
  PrintToString *_toString = nullptr;

private:

  PrintToWebServer _toWeb;

protected:

  String _header;

  KeyValueWriter *_parent = nullptr;

  bool _hasHeader = true;

  bool _isEmpty = true;

  bool _isArray{};


}; // class KeyValueWriter
