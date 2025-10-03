#include "../Helpers/KeyValueWriter.h"

#include "../WebServer/KeyValueWriter_JSON.h"
#include "../WebServer/KeyValueWriter_WebForm.h"

Sp_KeyValueWriter KeyValueWriter::createChild()
{
  switch (_writerType)
  {
    case KEYVALUEWRITER_BASE:
      break;
    case KEYVALUEWRITER_JSON:
    {
      return std::make_shared<KeyValueWriter_JSON>(static_cast<KeyValueWriter_JSON *>(this));
    }
    case KEYVALUEWRITER_WEBFORM:
    {
      return std::make_shared<KeyValueWriter_WebForm>(static_cast<KeyValueWriter_WebForm *>(this));
    }
  }
  return nullptr;
}

Sp_KeyValueWriter KeyValueWriter::createChild(const String& header)
{
  switch (_writerType)
  {
    case KEYVALUEWRITER_BASE:
      break;
    case KEYVALUEWRITER_JSON:
    {
      return std::make_shared<KeyValueWriter_JSON>(header, static_cast<KeyValueWriter_JSON *>(this));
    }
    case KEYVALUEWRITER_WEBFORM:
    {
      return std::make_shared<KeyValueWriter_WebForm>(header, static_cast<KeyValueWriter_WebForm *>(this));
    }
  }
  return nullptr;
}


Sp_KeyValueWriter KeyValueWriter::createNew()
{
  switch (_writerType)
  {
    case KEYVALUEWRITER_BASE:
      break;
    case KEYVALUEWRITER_JSON:
    {
      return std::make_shared<KeyValueWriter_JSON>();
    }
    case KEYVALUEWRITER_WEBFORM:
    {
      return std::make_shared<KeyValueWriter_WebForm>();
    }
  }
  return nullptr;
}

Sp_KeyValueWriter KeyValueWriter::createNew(const String& header)
{
  switch (_writerType)
  {
    case KEYVALUEWRITER_BASE:
      break;
    case KEYVALUEWRITER_JSON:
    {
      return std::make_shared<KeyValueWriter_JSON>(header);
    }
    case KEYVALUEWRITER_WEBFORM:
    {
      return std::make_shared<KeyValueWriter_WebForm>(header);
    }
  }
  return nullptr;
}
