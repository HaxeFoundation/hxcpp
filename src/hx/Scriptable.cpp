#include <hxcpp.h>
#include <hx/Scriptable.h>
#include <stdio.h>
#include <map>

namespace hx
{

enum OpCode
{
   opAccNull,
   opAccTrue,
   opAccFalse,
   opAccThis,
   opAccInt,
   opAccStack,
   opAccGlobal,
   opAccEnv,
   opAccField,
   opAccArray,
   opAccIndex,
   opAccBuiltin,
   opSetStack,
   opSetGlobal,
   opSetEnv,
   opSetField,
   opSetArray,
   opSetIndex,
   opSetThis,
   opPush,
   opPop,
   opCall,
   opObjCall,
   opJump,
   opJumpIf,
   opJumpIfNot,
   opTrap,
   opEndTrap,
   opRet,
   opMakeEnv,
   opMakeArray,
   opBool,
   opIsNull,
   opIsNotNull,
   opAdd,
   opSub,
   opMult,
   opDiv,
   opMod,
   opShl,
   opShr,
   opUShr,
   opOr,
   opAnd,
   opXor,
   opEq,
   opNeq,
   opGt,
   opGte,
   opLt,
   opLte,
   opNot,
   opTypeOf,
   opCompare,
   opHash,
   opNew,
   opJumpTable,
   opApply,
   opAccStack0,
   opAccStack1,
   opAccIndex0,
   opAccIndex1,
   opPhysCompare,
   opTailCall,

   opLast,
};


const char *opNames[] =
{
   "opAccNull",
   "opAccTrue",
   "opAccFalse",
   "opAccThis",
   "opAccInt",
   "opAccStack",
   "opAccGlobal",
   "opAccEnv",
   "opAccField",
   "opAccArray",
   "opAccIndex",
   "opAccBuiltin",
   "opSetStack",
   "opSetGlobal",
   "opSetEnv",
   "opSetField",
   "opSetArray",
   "opSetIndex",
   "opSetThis",
   "opPush",
   "opPop",
   "opCall",
   "opObjCall",
   "opJump",
   "opJumpIf",
   "opJumpIfNot",
   "opTrap",
   "opEndTrap",
   "opRet",
   "opMakeEnv",
   "opMakeArray",
   "opBool",
   "opIsNull",
   "opIsNotNull",
   "opAdd",
   "opSub",
   "opMult",
   "opDiv",
   "opMod",
   "opShl",
   "opShr",
   "opUShr",
   "opOr",
   "opAnd",
   "opXor",
   "opEq",
   "opNeq",
   "opGt",
   "opGte",
   "opLt",
   "opLte",
   "opNot",
   "opTypeOf",
   "opCompare",
   "opHash",
   "opNew",
   "opJumpTable",
   "opApply",
   "opAccStack0",
   "opAccStack1",
   "opAccIndex0",
   "opAccIndex1",
   "opPhysCompare",
   "opTailCall",

   "opLast",
};



class NekoModule
{
public:
   NekoModule(const char *inFilename)
   {
      mGlobalCount = 0;
      mFieldCount = 0;
      mCodeSize = 0;
      mOpCodes = 0;
      mOk = false;

      FILE *file = fopen(inFilename,"rb");
      if (!file)
      {
         printf("Could not load module: %s\n", inFilename);
      }
      else
      {
         mFile = file;
         try {
            Load(file);
         }
         catch (const char *inError)
         {
            printf("Error on load : %s\n", inError);
         }
         mFile = 0;
         fclose(file);
      }
   }

   ~NekoModule()
   {
      delete [] mOpCodes;
   }

   void DumpOp(const int *inOp)
   {
      int idx = inOp - mOpCodes;
      int arg = inOp[1];
      printf(" %03d %s", idx, opNames[*inOp]);
      switch(*inOp)
      {
         case opAccField:
            {
            unsigned int fid = mFieldHash[arg & 0x7fffffff];
            printf(" %s", mFields[fid].__s);
            break;
            }
         case opAccGlobal:
            if (arg>=mGlobalCount || arg<0)
               printf(" %d ???", arg );
            else
               printf(" %s", !mGlobals[arg].mPtr ? "(null)" : mGlobals[arg]->__ToString().c_str() );
            break;
      }
      printf("\n", idx, opNames[*inOp]);
   }

   void Error(const char *inMsg)
   {
      throw inMsg;
   }

   int ReadByte()
   {
      unsigned char byte;
      if (fread(&byte,1,1,mFile)!=1)
         Error("Unexpected end of stream");
      return byte;
   }

   int ReadInt()
   {
      int i;
      if (fread(&i,4,1,mFile)!=1)
         Error("Unexpected end of stream");
      return i;
   }

   int ReadUInt16()
   {
      unsigned short i;
      if (fread(&i,2,1,mFile)!=1)
         Error("Unexpected end of stream");
      return i;
   }


   const char *ReadString()
   {
      int i = 0;
      char c;
      while( i < sizeof(mStringBuf) )
      {
         char c = ReadByte();
         mStringBuf[i++] = c;
         if( c == 0 )
            return mStringBuf;
      }
      Error("Unterminated string");
      return "";
   }

   Dynamic ReadString16()
   {
      int len = ReadUInt16();
      if (len==0)
         return String("",0).dup();

      char *buffer = (char *)malloc(len);
      if (fread(buffer,len,1,mFile)!=1)
      {
         free(buffer);
         Error("Could not read string16");
      }
      Dynamic result = String(buffer,len).dup();
      free(buffer);
      return result;
   }


   void ReadDebugInfo()
   {
      bool lot_of_files = false;
      int nfiles = 0;

      // TODO:
      int c = ReadByte();
      if( c >= 0x80 )
      {
         int c2 = ReadByte();
         nfiles = ((c & 0x7F) << 8) | c2;
         lot_of_files = true;
      } else
         nfiles = c;

      if( nfiles == 0 )
         Error("No debug files");

      for(int i=0;i<nfiles;i++)
         ReadString();

      int npos = ReadInt();
      if( npos != mCodeSize )
         Error("Code size mismatch");

      int i = 0;
      while( i < npos )
      {
         int c = ReadByte();
         if( c & 1 )
         {
            if( lot_of_files )
               ReadByte();
         }
         else if( c & 2 )
         {
            int delta = c >> 6;
            int count = (c >> 2) & 15;
            if( i + count > npos )
               Error("Bad line count");
            i+= count;
         }
         else if( c & 4 )
         {
            i++;
         }
         else
         {
            unsigned char b2 = ReadByte();
            unsigned char b3 = ReadByte();
            i++;
         }
      }
   }

   unsigned int NekoHash(const char *name)
   {
      unsigned int result = 0;
	   while( *name )
      {
		  result = (223 * result + *((unsigned char*)name));
		  name++;
	   }
      return result & 0x7fffffff;
   }


   void Load(FILE *inFile)
   {
      unsigned int magic;
      fread(&magic,4,1,inFile);
      if (magic!=0x4F4B454E)
         Error("Bad Magic");

      mGlobalCount = ReadInt();
      mFieldCount = ReadInt();
      mCodeSize = ReadInt();

      printf("globs=%d, fields=%d, code=%d\n", mGlobalCount, mFieldCount, mCodeSize );
      mGlobals = Array_obj<Dynamic>::__new(mGlobalCount, mGlobalCount);

      for(int g=0;g<mGlobalCount;g++)
      {
         switch(ReadByte())
         {
            case 1:
               mGlobals[g] = String(ReadString()).dup();
               // null
               break;

            case 2:
               {
               int combined = ReadInt();
               int pos = combined &0xffffff;
               int extra = combined >> 24;
               if (pos>=mCodeSize)
                  Error("Bad code size");
               mGlobals[g] = String("Function @") + String(pos);
               break;
               }

            case 3:
               mGlobals[g] = ReadString16();
               break;

            case 4:
               mGlobals[g] = atof(ReadString());
               break;

            case 5:
               ReadDebugInfo();
               break;

            default:
               Error("Unknown global code");
         }
      }
      printf("Read globals %d\n",mGlobalCount);

      printf("Read fields...\n");
      mFields = Array_obj<String>::__new(mFieldCount);
      for(int f=0;f<mFieldCount;f++)
      {
         mFields[f] = String(ReadString()).dup();
         unsigned int id = NekoHash( mFields[f].__s );
         mFieldHash[id] = f;
         printf(" %08x -> %d\n", id,f );
      }
      printf("Read fields\n");

      printf("Unpack op codes...\n");

      int idx = 0;
      // Unpack opcodes
      mOpCodes = new int[mCodeSize+1];
      while( idx < mCodeSize )
      {
         int t = ReadByte();
         switch( t & 3 )
         {
         case 0:
            mOpCodes[idx++] = (t >> 2);
            DumpOp(mOpCodes+idx-1);
            break;
         case 1:
            mOpCodes[idx++] = (t >> 3);
            mOpCodes[idx++] = (t >> 2) & 1;
            DumpOp(mOpCodes+idx-2);
            break;
         case 2:
            mOpCodes[idx++] = (t >> 2);
            mOpCodes[idx++] = ReadByte();
            DumpOp(mOpCodes+idx-2);
            break;
         case 3:
            mOpCodes[idx++] = (t >> 2);
            mOpCodes[idx++] = ReadInt();
            DumpOp(mOpCodes+idx-2);
            break;
         }
      }
      mOpCodes[idx] = opLast;
      mEntry = (int)mOpCodes[1];
      printf("Read %d op codes\n",idx);
   }

   Array<Dynamic> mGlobals;
   Array<String>  mFields;
   std::map<unsigned int,int> mFieldHash;

   bool mOk;
   int mGlobalCount;
   int mFieldCount;
   int mCodeSize;
   int mEntry;
   int *mOpCodes;
   char mStringBuf[256];
   FILE *mFile;
};


void ScriptableRegisterClass( String inName, String *inFunctions, hx::ScriptableClassFactory inFactory)
{
}


void ScriptableRegisterInterface( String inName, const type_info *inType, hx::ScriptableInterfaceFactory inFactory)
{
}



Dynamic ScriptableCall0(void *user, Object *thiz)
{
   return null();
}

Dynamic ScriptableCall1(void *user, Object *thiz,Dynamic)
{
   return null();
}

Dynamic ScriptableCall2(void *user, Object *thiz,Dynamic,Dynamic)
{
   return null();
}

Dynamic ScriptableCall3(void *user, Object *thiz,Dynamic,Dynamic,Dynamic)
{
   return null();
}

Dynamic ScriptableCall4(void *user, Object *thiz,Dynamic,Dynamic,Dynamic,Dynamic)
{
   return null();
}

Dynamic ScriptableCall5(void *user, Object *thiz,Dynamic,Dynamic,Dynamic,Dynamic,Dynamic)
{
   return null();
}

Dynamic ScriptableCallMult(void *user, Object *thiz,Dynamic *inArgs)
{
   return null();
}


} // end namespace hx

void __scriptable_load_neko(String inName)
{
   new hx::NekoModule(inName.__s);
}


