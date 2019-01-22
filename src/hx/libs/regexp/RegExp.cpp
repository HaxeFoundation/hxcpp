#include <hxcpp.h>
#include <string.h>

#ifndef PCRE_STATIC
#define PCRE_STATIC
#endif
#include <pcre.h>

#define PCRE(o)      ((pcredata*)o.mPtr)


struct pcredata : public hx::Object
{
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdPcreData };

   pcre   *rUtf8;
   #ifdef HX_SMART_STRINGS
   pcre16 *rUtf16;
   #endif

   int nmatchs;
   int *matchs;
   unsigned int flags;
   String string;
   String expr;

   void create8(pcre *inR, String inExpr, int inFlags)
   {
      rUtf8 = inR;
      #ifdef HX_SMART_STRINGS
      rUtf16 = 0;
      #endif
      expr = inExpr;
      HX_OBJ_WB_GET(this, expr.__s);
      flags = inFlags;


      nmatchs = 0;
      pcre_fullinfo(rUtf8,NULL,PCRE_INFO_CAPTURECOUNT,&nmatchs);
      nmatchs++;
      matchs = (int*)malloc(sizeof(int) * 3 * nmatchs);

      _hx_set_finalizer(this, finalize);
   }

   #ifdef HX_SMART_STRINGS
   void create16(pcre16 *inR, String inExpr, int inFlags)
   {
      rUtf8 = 0;
      rUtf16 = inR;
      expr = inExpr;
      HX_OBJ_WB_GET(this, expr.__s);
      flags = inFlags;

      nmatchs = 0;
      pcre16_fullinfo(rUtf16,NULL,PCRE_INFO_CAPTURECOUNT,&nmatchs);
      nmatchs++;
      matchs = (int*)malloc(sizeof(int) * 3 * nmatchs);

      _hx_set_finalizer(this, finalize);
   }
   #endif


   bool run(String string,int pos,int len)
   {
      #ifdef HX_SMART_STRINGS
      if (string.isUTF16Encoded())
      {
         if (!rUtf16)
         {
            const char *error = 0;
            int err_offset = 0;
            rUtf16 = pcre16_compile((PCRE_SPTR16)expr.wc_str(),flags|PCRE_UTF16,&error,&err_offset,NULL);
            if (!rUtf16)
            {
               return false;
            }
         }
         return pcre16_exec(rUtf16,NULL,(const unsigned short *)string.__w,pos+len,pos,0,matchs,nmatchs * 3) >= 0;
      }

      if (!rUtf8)
      {
         rUtf8 =  pcre_compile(expr.c_str(),flags|PCRE_UTF8,0,0,0);
         if (!rUtf8)
            return false;
      }

      #endif
      return pcre_exec(rUtf8,NULL,string.__s,pos+len,pos,0,matchs,nmatchs * 3) >= 0;
   }

   void destroy()
   {
      if (rUtf8)
          pcre_free( rUtf8 );
      #ifdef HX_SMART_STRINGS
      if (rUtf16)
          pcre16_free( rUtf16 );
      #endif
      if (matchs)
         free(matchs);
   }

   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER(string); HX_MARK_MEMBER(expr); }
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(string); HX_VISIT_MEMBER(expr); }
   #endif

   static void finalize(Dynamic obj)
   {
      ((pcredata *)(obj.mPtr))->destroy();
   }

   String toString() { return expr; }
};


/**
   regexp_new_options : reg:string -> options:string -> 'regexp
   <doc>Build a new regexpr with the following options :
   <ul>
      <li>i : case insensitive matching</li>
      <li>s : . match anything including newlines</li>
      <li>m : treat the input as a multiline string</li>
      <li>u : run in utf8 mode</li>
      <li>g : turn off greedy behavior</li>
   </ul>
   </doc>
**/

Dynamic _hx_regexp_new_options(String s, String opt)
{
   const char *o = opt.__s;
   int options = PCRE_UCP;
   while( *o )
   {
      switch( *o++ )
      {
      case 'i':
         options |= PCRE_CASELESS;
         break;
      case 's':
         options |= PCRE_DOTALL;
         break;
      case 'm':
         options |= PCRE_MULTILINE;
         break;
      case 'g':
         options |= PCRE_UNGREEDY;
         break;
      case 'u':
         break;
      default:
         hx::Throw( HX_CSTRING("Regexp unknown modifier : ") + String::fromCharCode(o[-1]) );
         break;
      }
   }

   #ifdef HX_SMART_STRINGS
   if (s.isUTF16Encoded())
   {
      const char *error = 0;
      int err_offset = 0;
      pcre16 *p =  pcre16_compile((PCRE_SPTR16)s.__w,options|PCRE_UTF16,&error,&err_offset,NULL);
      if( !p )
         hx::Throw( HX_CSTRING("Regexp compilation error : ")+String(error)+HX_CSTRING(" in ")+s);

      pcredata *pdata = new pcredata;
      pdata->create16(p,s,options);
      return pdata;
   }
   else
   #endif
   {
      const char *error = 0;
      int err_offset = 0;

      pcre *p =  pcre_compile(s.__s,options|PCRE_UTF8,&error,&err_offset,NULL);
      if( !p )
         hx::Throw( HX_CSTRING("Regexp compilation error : ")+String(error)+HX_CSTRING(" in ")+s);

      pcredata *pdata = new pcredata;
      pdata->create8(p,s,options);
      return pdata;
   }
}

bool _hx_regexp_match(Dynamic handle, String string, int pos, int len)
{
   if( pos < 0 || len < 0 || pos > string.length || pos + len > string.length )
      return false;

   pcredata *d = PCRE(handle);

   if( d->run(string,pos,len) )
   {
      d->string = string;
      HX_OBJ_WB_GET(d, d->string.__s);
      return true;
   }
   else
   {
      d->string = String();
      return false;
   }
}

String  _hx_regexp_matched(Dynamic handle, int m)
{
   pcredata *d = PCRE(handle);

   if( m < 0 || m >= d->nmatchs || !d->string.__s )
      hx::Throw( HX_CSTRING("regexp_matched - no valid match"));

   int start = d->matchs[m*2];
   int len = d->matchs[m*2+1] - start;
   if( start == -1 )
      return String();
   return d->string.substr(start, len);
}

/**
   regexp_matched_pos : 'regexp -> n:int -> { pos => int, len => int }
   <doc>Return the [n]th matched block position by the regexp. If [n] is 0 then
   return the whole matched substring position</doc>
**/
Dynamic _hx_regexp_matched_pos(Dynamic handle, int m)
{
   pcredata *d = PCRE(handle);
   if( m < 0 || m >= d->nmatchs || !d->string.__s )
      return null();

   int start = d->matchs[m*2];
   int len = d->matchs[m*2+1] - start;

   return hx::Anon_obj::Create(2)
            ->setFixed(0,HX_("len",d5,4b,52,00),len)
            ->setFixed(1,HX_("pos",94,5d,55,00),start);
}


