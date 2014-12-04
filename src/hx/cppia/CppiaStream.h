#include <string>


namespace hx
{

struct CppiaStream
{
   class CppiaModule  *module;
   const char *data;
   const char *max;
   int line;
   int pos;

   CppiaStream(class CppiaModule *inModule,const char *inData, int inLen)
   {
      module = inModule;
      data = inData;
      max = inData + inLen;
      line = 1;
      pos = 1;
   }
   void skipWhitespace()
   {
      while(data<max && *data<=32)
         skipChar();
   }
   void skipChar()
   {
      if (data>=max)
         throw "EOF";
      if (*data=='\n')
      {
         line++;
         pos = 1;
      }
      else
         pos++;
      data++;
   }
   std::string getToken()
   {
      skipWhitespace();
      const char *data0 = data;
      while(data<max && *data>32)
      {
         data++;
         pos++;
      }
      return std::string(data0,data);
   }
   int getInt()
   {
      int result = 0;
      skipWhitespace();
      while(data<max && *data>32)
      {
         int digit = *data - '0';
         if (digit<0 || digit>9)
            throw "expected digit";
         result = result * 10 + digit;
         pos++;
         data++;
      }
      return result;
   }
   bool getBool()
   {
      int ival = getInt();
      if (ival>1)
         throw "invalid bool";
      return ival;
   }
   bool getStatic()
   {
      std::string tok = getToken();
      if (tok=="s")
         return true;
      else if (tok=="m")
         return false;

      throw "invalid function spec";
      return false;
   }

   String readString()
   {
      int len = getInt();
      skipChar();
      const char *data0 = data;
      for(int i=0;i<len;i++)
         skipChar();
      return String(data0,data-data0).dup();
   }

   void readBytes(unsigned char *outBytes, int inLen)
   {
      if (data+inLen>max)
         throw "EOF";
      memcpy(outBytes, data, inLen);
      data+=inLen;
   }

};

} // end namespace hx


