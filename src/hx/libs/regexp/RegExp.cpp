#include <hxcpp.h>
#include <string.h>

#define PCRE2_STATIC
#define PCRE2_CODE_UNIT_WIDTH 0

#include <pcre2.h>

#define PCRE(o)      ((pcredata*)o.mPtr)

static void regexp_compilation_error(String pattern, int error_code, size_t error_offset) {
   PCRE2_UCHAR8 error_buffer[128];
   pcre2_get_error_message_8(error_code, error_buffer, sizeof(error_buffer));
   hx::Throw(HX_CSTRING("Regexp compilation error : ") + String((const char*)error_buffer) + HX_CSTRING(" in ") + pattern);
}

struct pcredata : public hx::Object
{
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdPcreData };

   pcre2_code_8   *rUtf8;
   #ifdef HX_SMART_STRINGS
   pcre2_code_16   *rUtf16;
   #endif

   int n_groups;
   pcre2_match_data_8* match_data8;
   #ifdef HX_SMART_STRINGS
   pcre2_match_data_16* match_data16;
   #endif

   unsigned int flags;
   String string;
   String expr;

   void create8(pcre2_code_8 *inR, String inExpr, int inFlags)
   {
      rUtf8 = inR;
      #ifdef HX_SMART_STRINGS
      rUtf16 = 0;
      #endif
      expr = inExpr;
      HX_OBJ_WB_GET(this, expr.raw_ref());
      flags = inFlags;

      n_groups = 0;
      pcre2_pattern_info_8(rUtf8,PCRE2_INFO_CAPTURECOUNT,&n_groups);
      n_groups++;
      match_data8 = pcre2_match_data_create_from_pattern_8(rUtf8, NULL);
      #ifdef HX_SMART_STRINGS
      match_data16 = 0;
      #endif

      _hx_set_finalizer(this, finalize);
   }

   #ifdef HX_SMART_STRINGS
   void create16(pcre2_code_16 *inR, String inExpr, int inFlags)
   {
      rUtf8 = 0;
      rUtf16 = inR;
      expr = inExpr;
      HX_OBJ_WB_GET(this, expr.raw_ref());
      flags = inFlags;

      n_groups = 0;
      pcre2_pattern_info_16(rUtf16,PCRE2_INFO_CAPTURECOUNT,&n_groups);
      n_groups++;
      match_data8 = 0;
      match_data16 = pcre2_match_data_create_from_pattern_16(rUtf16, NULL);

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
            int error_code;
            size_t error_offset;
            hx::strbuf buf;
            int utf16Length = 0;
            PCRE2_SPTR16 utf16 = (PCRE2_SPTR16)expr.wc_str(&buf, &utf16Length);
            rUtf16 = pcre2_compile_16((PCRE2_SPTR16)expr.wc_str(&buf),utf16Length,flags,&error_code,&error_offset,NULL);
            if (!rUtf16) {
               regexp_compilation_error(expr,error_code,error_offset);
            }
            match_data16 = pcre2_match_data_create_from_pattern_16(rUtf16, NULL);
         }

         int n = pcre2_match_16(rUtf16,(PCRE2_SPTR16)string.raw_wptr(),pos+len,pos,PCRE2_NO_UTF_CHECK,match_data16,NULL);
         return n>=0;
      }

      if (!rUtf8)
      {
         int error_code;
         size_t error_offset;
         int utf8Length = 0;
         PCRE2_SPTR8 utf8 = (PCRE2_SPTR8)expr.utf8_str(NULL, true, &utf8Length);
         rUtf8 = pcre2_compile_8(utf8, utf8Length, flags, &error_code, &error_offset, NULL);
         if (!rUtf8) {
            regexp_compilation_error(expr,error_code,error_offset);
         }
         match_data8 = pcre2_match_data_create_from_pattern_8(rUtf8, NULL);
      }

      #endif
      return pcre2_match_8(rUtf8,(PCRE2_SPTR8)string.utf8_str(),pos+len,pos,PCRE2_NO_UTF_CHECK,match_data8,NULL) >= 0;
   }

   size_t* get_matches() {
      #ifdef HX_SMART_STRINGS
      if (string.isUTF16Encoded()) {
         return pcre2_get_ovector_pointer_16(match_data16);
      }
      #endif
      return pcre2_get_ovector_pointer_8(match_data8);
   }

   void destroy()
   {
      pcre2_code_free_8( rUtf8 );
      pcre2_match_data_free_8( match_data8 );

      #ifdef HX_SMART_STRINGS
      pcre2_code_free_16( rUtf16 );
      pcre2_match_data_free_16( match_data16 );
      #endif
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
   hx::strbuf buf;
   const char *o = opt.utf8_str(&buf);
   int options = PCRE2_UCP | PCRE2_UTF;
   while( *o )
   {
      switch( *o++ )
      {
      case 'i':
         options |= PCRE2_CASELESS;
         break;
      case 's':
         options |= PCRE2_DOTALL;
         break;
      case 'm':
         options |= PCRE2_MULTILINE;
         break;
      case 'g':
         options |= PCRE2_UNGREEDY;
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
      int error_code;
      size_t error_offset;
      pcre2_code_16 *p = pcre2_compile_16((PCRE2_SPTR16)s.raw_wptr(),s.length,options,&error_code,&error_offset,NULL);
      if( !p ) {
         regexp_compilation_error(s,error_code,error_offset);
      }

      pcredata *pdata = new pcredata;
      pdata->create16(p,s,options);
      return pdata;
   }
   else
   #endif
   {
      int error_code = 0;
      size_t error_offset;
      pcre2_code_8 *p =  pcre2_compile_8((PCRE2_SPTR8)s.utf8_str(),s.length,options,&error_code,&error_offset,NULL);
      if( !p ) {
         regexp_compilation_error(s,error_code,error_offset);
      }

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
      HX_OBJ_WB_GET(d, d->string.raw_ref());
      return true;
   }
   else
   {
      d->string = String();
      return false;
   }
}

String _hx_regexp_matched(Dynamic handle, int m)
{
   pcredata *d = PCRE(handle);

   if( m < 0 || m >= d->n_groups || !d->string.raw_ptr() )
      hx::Throw( HX_CSTRING("regexp_matched - no valid match"));

   size_t* matches = d->get_matches();
   int start = matches[m*2];
   int len = matches[m*2+1] - start;
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
   if (m < 0 || m >= d->n_groups || !d->string.raw_ptr())
      return null();

   size_t* matches = d->get_matches();
   int start = matches[m*2];
   int len = matches[m*2+1] - start;

   return hx::Anon_obj::Create(2)
            ->setFixed(0,HX_("len",d5,4b,52,00),len)
            ->setFixed(1,HX_("pos",94,5d,55,00),start);
}

/**
   regexp_matched_num : 'regexp -> int
   <doc>Return the total number of matched groups, or -1 if the regexp has not
   been matched yet</doc>
**/
int _hx_regexp_matched_num(Dynamic handle)
{
   pcredata *d = PCRE(handle);

   if( !d->string.raw_ptr() )
      return -1;
   else
      return d->n_groups;
}


