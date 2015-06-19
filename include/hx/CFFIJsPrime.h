
extern "C"
{

typedef std::map<std::string,int> IdMap;
static IdMap sIdMap;
static std::vector<val> sIdKeys;

int val_id(const char *inName)
{
   IdMap::iterator id = sIdMap.find(inName);
   if (id==sIdMap.end())
   {
      int result = sIdMap.size();
      sIdMap[inName] = result;
      sIdKeys.push_back(value(inName));
      return result;
   }
   return id->second;
}


double val_field_numeric(value inObject, int inFieldId)
{
   return inObject[sIdKeys[inFieldId]].as<double>();
}


}
