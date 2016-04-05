#include <hxcpp.h>
#include <string.h>

#ifndef PCRE_STATIC
#define PCRE_STATIC
#endif
#include <pcre.h>

#define PCRE(o)      ((pcredata*)o.mPtr)

struct pcredata : public hx::Object
{
   pcre *r;
   int nmatchs;
   int *matchs;
   String string;

   void create(pcre *inR)
   {
      r = inR;
      nmatchs = 0;
      pcre_fullinfo(r,NULL,PCRE_INFO_CAPTURECOUNT,&nmatchs);
      nmatchs++;
      matchs = (int*)malloc(sizeof(int) * 3 * nmatchs);

      _hx_set_finalizer(this, finalize);
   }

   void destroy()
   {
      if (r)
          pcre_free( r );
      if (matchs)
         free(matchs);
   }

   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER(string); }
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(string); }
   #endif

   static void finalize(Dynamic obj)
   {
      ((pcredata *)(obj.mPtr))->destroy();
   }

   String toString() { return HX_CSTRING("pcredata"); }
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
   int options = 0;
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
      case 'u':
         options |= PCRE_UTF8;
         break;
      case 'g':
         options |= PCRE_UNGREEDY;
         break;
      default:
         return null();
         break;
      }
   }

   const char *error = 0;
   int err_offset = 0;
   pcre *p =  pcre_compile(s.__s,options,&error,&err_offset,NULL);
   if( !p )
      hx::Throw( HX_CSTRING("Regexp compilation error : ")+String(error)+HX_CSTRING(" in ")+s);

   pcredata *pdata = new pcredata;
   pdata->create(p);
   return pdata;
}

bool _hx_regexp_match(Dynamic handle, String string, int pp, int ll)
{
   if( pp < 0 || ll < 0 || pp > string.length || pp + ll > string.length )
      return false;

   pcredata *d = PCRE(handle);

   if( pcre_exec(d->r,NULL,string.__s,ll+pp,pp,0,d->matchs,d->nmatchs * 3) >= 0 )
   {
      d->string = string;
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


