#include "stdafx.h"
#include "SerializerJSON.h"

void to_json(nlohmann::json& j, const CString& c) {
   j = std::string((LPCTSTR)c);
}

void from_json(const nlohmann::json& j, CString& c) {
   c = (j.get<std::string>().c_str());
}
