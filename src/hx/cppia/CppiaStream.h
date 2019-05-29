#include <string>
#include <map>

namespace hx
{

struct CppiaStream
{
   std::map<int,std::string> tokenMap;
   std::map<std::string,int> opMap;
   bool binary;
   class CppiaModule  *module;
   const char *data;
   const char *max;
   int line;
   int pos;

   CppiaStream(class CppiaModule *inModule,const unsigned char *inData, int inLen)
   {
      binary = false;
      module = inModule;
      data = (const char *)inData;
      max = data + inLen;
      line = 1;
      pos = 1;
   }

   struct OpName { int id; const char *name; };
   void setBinary(bool inBinary)
   {
      binary = inBinary;
      OpName names[] = {
         #define CPPIA_OP(ident,name,id) { id, name },
         #include "CppiaOps.inc"
         #undef CPPIA_OP
      };
      if (binary)
      {
         for(int i=0;i<sizeof(names)/sizeof(names[0]);i++)
         {
            OpName &name = names[i];
            if (!tokenMap[name.id].empty())
               throw "Double identifier";
            tokenMap[ name.id ] = name.name;
         }
      }
      else
      {
         for(int i=0;i<sizeof(names)/sizeof(names[0]);i++)
         {
            OpName &name = names[i];
            opMap[ name.name ] = name.id;
         }
      }
   }
   void skipWhitespace()
   {
      while(true)
      {
         while(data<max && *data<=32)
            skipChar();
         if (data<max && *data=='#')
         {
            while(data<max && *data!='\n')
               skipChar();
         }
         else
            break;
      }
   }
   int getLineId()
   {
      return binary ? 0 : getInt();
   }
   int getFileId()
   {
      return binary ? 0 : getInt();
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
   std::string getAsciiToken()
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


   CppiaOp getOp()
   {
      if (binary)
         return (CppiaOp) getInt();
      std::string tok = getAsciiToken();
      CppiaOp result = (CppiaOp)opMap[tok];
      if (result==0)
         throw "Unknown token";
      return result;
   }


   std::string getToken()
   {
      if (!binary)
         return getAsciiToken();
      int id = getInt();
      //printf("Token %s\n", tokenMap[id].c_str());
      return tokenMap[id];
   }

   int getAsciiInt()
   {
      int result = 0;
      int sign = 1;
      skipWhitespace();
      while(data<max && *data>32)
      {
         if (*data=='-')
            sign = -1;
         else
         {
            int digit = *data - '0';
            if (digit<0 || digit>9)
               throw "expected digit";
            result = result * 10 + digit;
         }
         pos++;
         data++;
      }
      return result*sign;
   }

   int getByte()
   {
      int result = *(unsigned char *)data;
      data++;
      pos++;
      return result;
   }
   int getInt()
   {
      if (!binary)
         return getAsciiInt();

      int code = getByte();
      if (code<254)
         return code;
      if (code==254)
      {
         int result = getByte();
         result |= (getByte()<<8);
         return result;
      }

      int result = getByte();
      result |= (getByte()<<8);
      result |= (getByte()<<16);
      result |= (getByte()<<24);
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
      return getBool();
   }

   String readString(std::string *outStdStdString=0)
   {
      int len = getAsciiInt();
      skipChar();
      const char *data0 = data;
      int hasBig = false;
      for(int i=0;i<len;i++)
         skipChar();
      if (outStdStdString)
         *outStdStdString = std::string(data0, data);
      return String::createPermanent(data0,data-data0);
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


