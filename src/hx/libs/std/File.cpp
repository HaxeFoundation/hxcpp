#include <hxcpp.h>
#include <stdio.h>
#include <string.h>

#ifdef NEKO_WINDOWS
#   include <windows.h>
#endif

/**
   <doc>
   <h1>File</h1>
   <p>
   The file api can be used for different kind of file I/O.
   </p>
   </doc>
**/


#if defined(ANDROID) || defined(IPHONE)
typedef char FilenameChar;
#define val_filename val_string
#define alloc_filename alloc_string

#define MAKE_STDIO(k) \
   static value file_##k() { \
      fio *f; \
      f = new fio(#k,k); \
      value result = alloc_abstract(k_file,f); \
      val_gc(result,free_stdfile); \
      return result; \
   } \
   DEFINE_PRIM(file_##k,0);


#else
typedef wchar_t FilenameChar;
#define val_filename val_wstring
#define alloc_filename alloc_wstring

#define MAKE_STDIO(k) \
   static value file_##k() { \
      fio *f; \
      f = new fio(L###k,k); \
      value result = alloc_abstract(k_file,f); \
      val_gc(result,free_stdfile); \
      return result; \
   } \
   DEFINE_PRIM(file_##k,0);


#endif

namespace
{

struct fio : public hx::Object
{
   String name;
   FILE   *io;
   bool   closeIo;

   void create(FILE *inFile, String inName, bool inClose)
   {
      name = inName;
      io = inFile;
      closeIo = inClose;

      __hxcpp_set_finalizer(this, (void *)finalize);
   }

   void destroy(bool inForceClose = false)
   {
      if (io && (inForceClose || closeIo))
         fclose(io);
      io = 0;
      name = String();
   }

   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER(name); }
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(name); }
   #endif

   static void finalize(void *inPtr)
   {
      ((fio *)inPtr)->destroy();
   }

   String toString() { return HX_CSTRING("fio:") + name; }

};

fio *getFio(Dynamic handle, bool inRequireFile=true)
{
   fio *result = dynamic_cast<fio *>(handle);
   if (!result || (!result->io && inRequireFile))
      hx::Throw( HX_CSTRING("Bad file handle") );
   return result;
}


static void file_error(const char *msg, String inName, bool inExitZone)
{
   if (inExitZone)
      hx::ExitGCFreeZone();
   Array<String> err = Array_obj<String>::__new(2,2);
   err[0] = String(msg);
   err[1] = inName;
   hx::Throw(err);
}

}

/**
   file_open : f:string -> r:string -> 'file
   <doc>
   Call the C function [fopen] with the file path and access rights. 
   Return the opened file or throw an exception if the file couldn't be open.
   </doc>
**/
Dynamic _hx_std_file_open( String fname, String r )
{
   hx::EnterGCFreeZone();
   File *file = fopen(fname.__s,r.__s);
   hx::ExitGCFreeZone();
   if (!file)
      file_error("file_open",fname,false);

   fio *f = new fio;
   f->create(file,fname,true);
   return f;
}

/**
   file_close : 'file -> void
   <doc>Close an file. Any other operations on this file will fail</doc> 
**/
void _hx_std_file_close( Dyamic handle )
{
   fio *fio = getFio(handle);
   fio->destroy();
}

/**
   file_name : 'file -> string
   <doc>Return the name of the file which was opened</doc>
**/
String hx_std_file_name( Dynamic handle )
{
   fio *fio = getFio(handle);
   return fio->name;
}

/**
   file_write : 'file -> s:string -> p:int -> l:int -> int
   <doc>
   Write up to [l] chars of string [s] starting at position [p]. 
   Returns the number of chars written which is >= 0.
   </doc>
**/
value file_write( value o, value s, value pp, value n )
{
   int p, len;
   int buflen;
   fio *f;
   val_check_kind(o,k_file);
   val_check(s,buffer);
   buffer buf = val_to_buffer(s);
   buflen = buffer_size(buf);
   val_check(pp,int);
   val_check(n,int);
   f = val_file(o);
   p = val_int(pp);
   len = val_int(n);
   if( p < 0 || len < 0 || p > buflen || p + len > buflen )
      return alloc_null();

   hx::EnterGCFreeZone();
   while( len > 0 ) {
      int d;
      POSIX_LABEL(file_write_again);
      d = (int)fwrite(buffer_data(buf)+p,1,len,f->io);
      if( d <= 0 ) {
         HANDLE_FINTR(f->io,file_write_again);
         file_error("file_write",f);
      }
      p += d;
      len -= d;
   }
   hx::ExitGCFreeZone();
   return n;
}

/**
   file_read : 'file -> s:string -> p:int -> l:int -> int
   <doc>
   Read up to [l] chars into the string [s] starting at position [p].
   Returns the number of chars readed which is > 0 (or 0 if l == 0).
   </doc>
**/
static value file_read( value o, value s, value pp, value n ) {
   fio *f;
   int p;
   int len;
   int buf_len;
   val_check_kind(o,k_file);
   val_check(s,buffer);
   buffer buf = val_to_buffer(s);
   buf_len = buffer_size(buf);
   val_check(pp,int);
   val_check(n,int);
   f = val_file(o);
   p = val_int(pp);
   len = val_int(n);
   if( p < 0 || len < 0 || p > buf_len || p + len > buf_len )
      return alloc_null();
   hx::EnterGCFreeZone();
   while( len > 0 ) {
      int d;
      POSIX_LABEL(file_read_again);
      d = (int)fread(buffer_data(buf)+p,1,len,f->io);
      if( d <= 0 ) {
         int size = val_int(n) - len;
         HANDLE_FINTR(f->io,file_read_again);
         if( size == 0 )
            file_error("file_read",f);
         hx::ExitGCFreeZone();
         return alloc_int(size);
      }
      p += d;
      len -= d;
   }
   hx::ExitGCFreeZone();
   return n;
}

/**
   file_write_char : 'file -> c:int -> void
   <doc>Write the char [c]. Error if [c] outside of the range 0..255</doc>
**/
static value file_write_char( value o, value c ) {
   unsigned char cc;
   fio *f;
   val_check(c,int);
   val_check_kind(o,k_file);
   if( val_int(c) < 0 || val_int(c) > 255 )
      return alloc_null();
   cc = (char)val_int(c);
   f = val_file(o);
   hx::EnterGCFreeZone();
   POSIX_LABEL(write_char_again);
   if( fwrite(&cc,1,1,f->io) != 1 ) {
      HANDLE_FINTR(f->io,write_char_again);
      file_error("file_write_char",f);
   }
   hx::ExitGCFreeZone();
   return alloc_bool(true);
}

/**
   file_read_char : 'file -> int
   <doc>Read a char from the file. Exception on error</doc>
**/
static value file_read_char( value o ) {
   unsigned char cc;
   fio *f;
   val_check_kind(o,k_file);
   f = val_file(o);
   hx::EnterGCFreeZone();
   POSIX_LABEL(read_char_again);
   if( fread(&cc,1,1,f->io) != 1 ) {
      HANDLE_FINTR(f->io,read_char_again);
      file_error("file_read_char",f);
   }
   hx::ExitGCFreeZone();
   return alloc_int(cc);
}

/**
   file_seek : 'file -> pos:int -> mode:int -> void
   <doc>Use [fseek] to move the file pointer.</doc>
**/
static value file_seek( value o, value pos, value kind ) {
   fio *f;
   val_check_kind(o,k_file);
   val_check(pos,int);
   val_check(kind,int);
   f = val_file(o);
   hx::EnterGCFreeZone();
   if( fseek(f->io,val_int(pos),val_int(kind)) != 0 )
      file_error("file_seek",f);
   hx::ExitGCFreeZone();
   return alloc_bool(true);
}

/**
   file_tell : 'file -> int
   <doc>Return the current position in the file</doc>
**/
static value file_tell( value o ) {
   int p;
   fio *f;
   val_check_kind(o,k_file);
   f = val_file(o);
   hx::EnterGCFreeZone();
   p = ftell(f->io);
   if( p == -1 )
      file_error("file_tell",f);
   hx::ExitGCFreeZone();
   return alloc_int(p);
}

/**
   file_eof : 'file -> bool
   <doc>Tell if we have reached the end of the file</doc>
**/
static value file_eof( value o ) {
   val_check_kind(o,k_file);
   return alloc_bool( feof(val_file(o)->io) );
}

/**
   file_flush : 'file -> void
   <doc>Flush the file buffer</doc>
**/
static value file_flush( value o ) {
   fio *f;
   val_check_kind(o,k_file);
   f = val_file(o);
   hx::EnterGCFreeZone();
   if( fflush( f->io ) != 0 )
      file_error("file_flush",f);
   hx::ExitGCFreeZone();
   return alloc_bool(true);
}

/**
   file_contents : f:string -> string
   <doc>Read the content of the file [f] and return it.</doc>
**/
static value file_contents( value name ) {
   buffer s=0;
   int len;
   int p;
   val_check(name,string);
   fio f(val_filename(name));
        const char *fname = val_string(name);
        hx::EnterGCFreeZone();
   f.io = fopen(fname,"rb");
   if( f.io == NULL )
      file_error("file_contents",&f);
   fseek(f.io,0,SEEK_END);
   len = ftell(f.io);
   fseek(f.io,0,SEEK_SET);
   hx::ExitGCFreeZone();
   s = alloc_buffer_len(len);
   p = 0;
   hx::EnterGCFreeZone();
   while( len > 0 ) {
      int d;
      POSIX_LABEL(file_contents);
      d = (int)fread((char*)buffer_data(s)+p,1,len,f.io);
      if( d <= 0 ) {
         HANDLE_FINTR(f.io,file_contents);
         fclose(f.io);
         file_error("file_contents",&f);
      }
      p += d;
      len -= d;
   }   
   fclose(f.io);
   hx::ExitGCFreeZone();
   return buffer_val(s);
}


/**
   file_stdin : void -> 'file
   <doc>The standard input</doc>
**/
MAKE_STDIO(stdin);
/**
   file_stdout : void -> 'file
   <doc>The standard output</doc>
**/
MAKE_STDIO(stdout);
/**
   file_stderr : void -> 'file
   <doc>The standard error output</doc>
**/
MAKE_STDIO(stderr);

