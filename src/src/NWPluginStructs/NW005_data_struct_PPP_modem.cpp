#include "../NWPluginStructs/NW005_data_struct_PPP_modem.h"

#ifdef USES_NW005

# include "../Helpers/StringConverter.h"


NW005_data_struct_PPP_modem::NW005_data_struct_PPP_modem() {}

NW005_data_struct_PPP_modem::~NW005_data_struct_PPP_modem() {}

struct testStruct {
  String   foo1 = F("test123");
  int64_t  foo2 = -123;
  uint32_t foo3 = 123;


};

void NW005_data_struct_PPP_modem::testWrite()
{
  if (!_KVS_initialized()) { return; }
  for (int i = 0; i < 5; ++i) {
    testStruct _test{ .foo1=concat(F("str_"), (3 * (i) + 1)), .foo2=-1l * (3 * (i) + 2), .foo3=3 * (i) + 3 };
    _kvs->setValue(3 * (i) + 1, _test.foo1);
    _kvs->setValue(3 * (i) + 2, _test.foo2);
    _kvs->setValue(3 * (i) + 3, _test.foo3);
  }

  _kvs->dump();


//  _kvs->clear();
  _store();

    _load();

    _kvs->dump();

}

void NW005_data_struct_PPP_modem::testRead()
{
  if (!_KVS_initialized()) { return; }
  _load();

  for (uint32_t i = 1; i <= 30; ++i) {
    String val;
    _kvs->getValue(i, val);
    addLog(LOG_LEVEL_INFO, strformat(F("KVS, foo%d: %s"), i, val.c_str()));
  }

}

#endif // ifdef USES_NW005
