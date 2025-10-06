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
  enum class Format {
    Default,
    PreFormatted,
    Note

  };


  KeyValueStruct(Format format = Format::Default) : _format(format) {}

  KeyValueStruct(const __FlashStringHelper *key,
                 Format                     format = Format::Default);
  KeyValueStruct(const String& key,
                 Format        format = Format::Default);

  KeyValueStruct(const String         & key,
                 const bool           & val,
                 Format                 format = Format::Default,
                 ValueStruct::ValueType vType  = ValueStruct::ValueType::Bool);

  KeyValueStruct(const String         & key,
                 int                    val,
                 Format                 format = Format::Default,
                 ValueStruct::ValueType vType  = ValueStruct::ValueType::Int);
#if defined(ESP32) && !defined(__riscv)
  KeyValueStruct(const String         & key,
                 int32_t                val,
                 Format                 format = Format::Default,
                 ValueStruct::ValueType vType  = ValueStruct::ValueType::Int);
#endif
  KeyValueStruct(const String         & key,
                 uint32_t               val,
                 Format                 format = Format::Default,
                 ValueStruct::ValueType vType  = ValueStruct::ValueType::Int);
#if defined(ESP32) && !defined(__riscv)
  KeyValueStruct(const String         & key,
                 size_t                 val,
                 Format                 format = Format::Default,
                 ValueStruct::ValueType vType  = ValueStruct::ValueType::Int);
#endif
  KeyValueStruct(const String         & key,
                 const uint64_t       & val,
                 Format                 format = Format::Default,
                 ValueStruct::ValueType vType  = ValueStruct::ValueType::Int);

  KeyValueStruct(const String         & key,
                 const int64_t        & val,
                 Format                 format = Format::Default,
                 ValueStruct::ValueType vType  = ValueStruct::ValueType::Int);


  KeyValueStruct(const String         & key,
                 const float          & val,
                 int                    nrDecimals = 4,
                 Format                 format     = Format::Default,
                 ValueStruct::ValueType vType      = ValueStruct::ValueType::Float);

  KeyValueStruct(const String         & key,
                 const double         & val,
                 int                    nrDecimals = 4,
                 Format                 format     = Format::Default,
                 ValueStruct::ValueType vType      = ValueStruct::ValueType::Double);


  KeyValueStruct(const __FlashStringHelper *key,
                 const String             & val,
                 Format                     format = Format::Default,
                 ValueStruct::ValueType     vType  = ValueStruct::ValueType::Auto);

  KeyValueStruct(const String             & key,
                 const __FlashStringHelper *val,
                 Format                     format = Format::Default,
                 ValueStruct::ValueType     vType  = ValueStruct::ValueType::Auto);

  KeyValueStruct(const __FlashStringHelper *key,
                 const __FlashStringHelper *val,
                 Format                     format = Format::Default,
                 ValueStruct::ValueType     vType  = ValueStruct::ValueType::Auto);

  KeyValueStruct(const __FlashStringHelper *key,
                 const char                *val,
                 Format                     format = Format::Default,
                 ValueStruct::ValueType     vType  = ValueStruct::ValueType::Auto);

  KeyValueStruct(const String         & key,
                 const String         & val,
                 Format                 format = Format::Default,
                 ValueStruct::ValueType vType  = ValueStruct::ValueType::Auto);

  KeyValueStruct(const __FlashStringHelper *key,
                 String                  && val,
                 Format                     format = Format::Default,
                 ValueStruct::ValueType     vType  = ValueStruct::ValueType::Auto);


  KeyValueStruct(const String         & key,
                 String              && val,
                 Format                 format = Format::Default,
                 ValueStruct::ValueType vType  = ValueStruct::ValueType::Auto);

  /*
     // TD-er: Do not use template types as it may 'explode' in binary size.
     // For example "foo" and "test" will result in 2 compiled instances
     // as they are  const char[3] and const char[4] respectively.
     template<typename T>
     KeyValueStruct(const String         & key,
                    const T              & val,
                    Format                 format = Format::Default,
                    ValueStruct::ValueType vType  = ValueStruct::ValueType::Auto)
      : _key(key), _format(format) {
      _values.emplace_back(String(val), vType);
     }
   */
  KeyValueStruct(LabelType::Enum label,
                 Format          format = Format::Default);

  void setUnit(const String& unit);
  void setUnit(const __FlashStringHelper *unit);

  void appendValue(const ValueStruct& value);

  void appendValue(ValueStruct&& value);


  String _key;
  String _id;
  String _unit;

  std::vector<ValueStruct>_values;

  Format _format = Format::Default;

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

  KeyValueWriter(KeyValueWriter*parent, PrintToString *toStr = nullptr) :  _toString(toStr), _parent(parent)
  {}

  KeyValueWriter(bool emptyHeader, KeyValueWriter*parent, PrintToString *toStr = nullptr) :  _toString(toStr), _parent(parent),
    _hasHeader(emptyHeader) {}

  KeyValueWriter(const String& header, KeyValueWriter*parent, PrintToString *toStr = nullptr) :  _toString(toStr),  _header(header),
    _parent(parent)
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

  virtual void writeNote(const String& note);
  virtual void writeNote(const __FlashStringHelper * note);

  //  virtual void setParent(KeyValueWriter*parent) { _parent = parent; }

  virtual int  getLevel() const;

  virtual void setIsArray() { _isArray = true; }

  // When set to 'plainText', the writer will not try to insert writer specific
  // markings, like <pre> or <br> for example for HTML output
  // When set to true, any child writer will also have this set to true
  virtual void setPlainText() { _plainText = true; }

  virtual bool plainText() const;

  // 'summaryValueOnly' means the key will not be output and this is also a hint to generate a summary of data.
  // Typically this is intended for human readable texts.
  // When set to true, any child writer will also have this set to true
  virtual void              setSummaryValueOnly() { _summaryValueOnly = true; }

  virtual bool              summaryValueOnly() const;

  // This should be override by any writer outputting data which is not intended to be human readable.
  virtual bool              dataOnlyOutput() const                      { return false; }

  // TODO TD-er: Change this to std::shared_ptr<PrintToString>
  virtual void              setOutputToString(PrintToString*printToStr) { _toString = printToStr; }

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

  String&& getMove()
  {
    if (_toString == nullptr) {
      static String tmp;
      return std::move(tmp);
    }
    return std::move(_toString->getMove());
  }

  bool         reserve(unsigned int size) { return _toString && _toString->reserve(size); }

  virtual void indent()                   {}

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

  // private:

  bool _plainText = false;

  bool _summaryValueOnly = false;


}; // class KeyValueWriter
