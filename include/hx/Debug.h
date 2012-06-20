#ifndef HX_DEBUG_H
#define HX_DEBUG_H

#ifdef HXCPP_DEBUG
void __hxcpp_dbg_var_pop();
void __hxcpp_dbg_var_push(struct __AutoVar *inVar);

struct __AutoVar
{
   enum Type { typeObject, typeDouble, typeFloat, typeInt, typeBool, typeString };


   explicit __AutoVar(const __AutoVar &);

   template<typename OBJ>
   __AutoVar(OBJ *inPtr,const char *inName)
   {
      mName = inName;
      mPtr = inPtr;
      mType = typeObject;
      __hxcpp_dbg_var_push(this);
   }
   __AutoVar(double *inPtr,const char *inName)
   {
      mName = inName;
      mPtr = inPtr;
      mType = typeDouble;
      __hxcpp_dbg_var_push(this);
   }
   __AutoVar(float *inPtr,const char *inName)
   {
      mName = inName;
      mPtr = inPtr;
      mType = typeFloat;
      __hxcpp_dbg_var_push(this);
   }
   __AutoVar(int *inPtr,const char *inName)
   {
      mName = inName;
      mPtr = inPtr;
      mType = typeInt;
      __hxcpp_dbg_var_push(this);
   }
   __AutoVar(bool *inPtr,const char *inName)
   {
      mName = inName;
      mPtr = inPtr;
      mType = typeBool;
      __hxcpp_dbg_var_push(this);
   }
   __AutoVar(::String *inPtr,const char *inName)
   {
      mName = inName;
      mPtr = inPtr;
      mType = typeString;
      __hxcpp_dbg_var_push(this);
   }

   ~__AutoVar() { __hxcpp_dbg_var_pop(); }

   void       *mPtr;
   const char *mName;
   __AutoVar  *mNext;
   Type       mType;
};

#endif


#endif
