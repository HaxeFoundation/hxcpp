#include <hxcpp.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

/**
   <doc>
   <h1>ZLib</h1>
   <p>
   Give access to the popular ZLib compression library, used in several file
   formats such as ZIP and PNG.
   </p>
   </doc>
**/

namespace {

struct ZipResult
{
   inline ZipResult(bool inOk, bool inDone, int inRead, int inWrite)
      : ok(inOk)
      , done(inDone)
      , read(inRead)
      , write(inWrite) { }
   bool ok;
   bool done;
   int  read;
   int  write;
};

struct ZStream : public hx::Object
{
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdZLib };

   z_stream *z;
   bool     isInflate;
   int      flush;

   void create(bool inIsInflate, int inParam)
   {
      isInflate = inIsInflate;
      flush = Z_NO_FLUSH;
      z = (z_stream*)malloc(sizeof(z_stream));
      memset(z,0,sizeof(z_stream));
      int err = 0;
      if (!isInflate)
      {
         if( (err = deflateInit(z,inParam)) != Z_OK )
         {
            free(z);
            z = 0;
            onError(err);
         }
      }
      else
      {
         if( (err = inflateInit2(z,inParam)) != Z_OK )
         {
            free(z);
            z = 0;
            onError(err);
         }
      }

      _hx_set_finalizer(this, finalize);
   }

   static void finalize(Dynamic obj)
   {
      ((ZStream *)(obj.mPtr))->destroy();
   }

   void destroy()
   {
      if (z)
      {
         if (isInflate)
            inflateEnd(z);
         else
            deflateEnd(z);
         free(z);
         z = 0;
      }
   }

   ZipResult deflate(Array<unsigned char> src, int srcpos, Array<unsigned char> dest, int dstpos)
   {
      if( srcpos < 0 || dstpos < 0 )
         return ZipResult( 0,0,0,0 );

      int slen = src->length - srcpos;
      int dlen = dest->length - dstpos;
      if( slen < 0 || dlen < 0 )
         return ZipResult( 0,0,0,0 );

      z->next_in = (Bytef*)(&src[srcpos]);
      z->next_out = (Bytef*)(&dest[dstpos]);
      z->avail_in = slen;
      z->avail_out = dlen;
      int err = 0;
      if( (err = ::deflate(z,flush)) < 0 )
         onError(err);

      z->next_in = 0;
      z->next_out = 0;
      return ZipResult( true, err == Z_STREAM_END, (int)(slen - z->avail_in), (int)(dlen - z->avail_out) );
   }

   ZipResult inflate(Array<unsigned char> src, int srcpos, Array<unsigned char> dest, int dstpos)
   {
      int slen = src->length;
      int dlen = dest->length;

      if( srcpos < 0 || dstpos < 0 )
         return ZipResult( 0,0,0,0 );
      slen -= srcpos;
      dlen -= dstpos;
      if( slen < 0 || dlen < 0 )
         return ZipResult( 0,0,0,0 );

      z->next_in = (Bytef*)&src[srcpos];
      z->next_out = (Bytef*)&dest[dstpos];
      z->avail_in = slen;
      z->avail_out = dlen;
      int err = 0;
      if( (err = ::inflate(z,flush)) < 0 )
         onError(err);
      z->next_in = 0;
      z->next_out = 0;
      return ZipResult( true, err == Z_STREAM_END, (int)(slen - z->avail_in), (int)(dlen - z->avail_out) );
   }

   int getDeflateBound(int inLength)
   {
      return deflateBound(z,inLength);
   }

   void setFlushMode(String inMode)
   {
      if( inMode == HX_CSTRING("NO") )
         flush = Z_NO_FLUSH;
      else if( inMode==HX_CSTRING("SYNC"))
         flush = Z_SYNC_FLUSH;
      else if( inMode==HX_CSTRING("FULL"))
         flush = Z_FULL_FLUSH;
      else if( inMode==HX_CSTRING("FINISH"))
         flush = Z_FINISH;
      else if( inMode==HX_CSTRING("BLOCK"))
         flush = Z_BLOCK;
   }


   void onError(int inCode)
   {
      String message = HX_CSTRING("ZLib Error : ");
      if( z && z->msg )
         message += String(z->msg) + HX_CSTRING("(") + String(inCode) + HX_CSTRING(")");
      else
        message += String(inCode);
      hx::Throw(message);
   }


};

ZStream *GetDeflateStream(Dynamic inHandle)
{
   ZStream *z = dynamic_cast<ZStream *>(inHandle.mPtr);
   if (!z || !z->z || z->isInflate)
      hx::Throw( HX_CSTRING("ZLib: Not a valid deflate stream"));
   return z;
}


ZStream *GetInflateStream(Dynamic inHandle)
{
   ZStream *z = dynamic_cast<ZStream *>(inHandle.mPtr);
   if (!z || !z->z || !z->isInflate)
      hx::Throw( HX_CSTRING("ZLib: Not a valid inflate stream"));
   return z;
}

} // end namespace




/**
   deflate_init : level:int -> 'dstream
   <doc>Open a compression stream with the given level of compression</doc>
**/
Dynamic _hx_deflate_init(int level)
{
   ZStream *zStream = new ZStream;
   zStream->create(false, level);
   return zStream;
}

/**
   deflate_buffer : 'dstream -> src:string -> srcpos:int -> dst:string -> dstpos:int -> { done => bool, read => int, write => int }
**/
Dynamic _hx_deflate_buffer(Dynamic handle, Array<unsigned char> src, int srcPos, Array<unsigned char> dest, int destPos)
{
   ZipResult result = GetDeflateStream(handle)->deflate(src,srcPos,dest,destPos);
   if (!result.ok)
      return null();

   return hx::Anon_obj::Create(3)
            ->setFixed(0,HX_("write",df,6c,59,d0),result.write)
            ->setFixed(1,HX_("done",82,f0,6d,42),result.done)
            ->setFixed(2,HX_("read",56,4b,a7,4b),result.read);
}


/**
   deflate_bound : 'dstream -> n:int -> int
   <doc>Return the maximum buffer size needed to write [n] bytes</doc>
**/
int _hx_deflate_bound(Dynamic handle,int length)
{
   return GetDeflateStream(handle)->getDeflateBound(length);
}

void _hx_deflate_end(Dynamic handle)
{
   GetDeflateStream(handle)->destroy();
}


/**
   set_flush_mode : 'stream -> string -> void
   <doc>Change the flush mode ("NO","SYNC","FULL","FINISH","BLOCK")</doc>
**/
void _hx_zip_set_flush_mode(Dynamic handle, String flushMode)
{
   ZStream *zstream = dynamic_cast<ZStream *>(handle.mPtr);
   if (!zstream || !zstream->z)
      hx::Throw( HX_CSTRING("ZLib flush: not a valid stream") );

   zstream->setFlushMode(flushMode);
}




/**
   inflate_init : window_size:int? -> 'istream
   <doc>Open a decompression stream</doc>
**/
Dynamic _hx_inflate_init(Dynamic windowBits)
{
   int bits = windowBits==null() ? MAX_WBITS : (int)windowBits;

   ZStream *zStream = new ZStream();
   zStream->create(true, bits);
   return zStream;
}

/**
   inflate_buffer : 'istream -> src:string -> srcpos:int -> dst:string -> dstpos:int -> { done => bool, read => int, write => int }
**/
Dynamic _hx_inflate_buffer(Dynamic handle, Array<unsigned char> src, int srcPos, Array<unsigned char> dest, int destPos)
{
   ZipResult result = GetInflateStream(handle)->inflate(src,srcPos,dest,destPos);
   if (!result.ok)
      return null();

   return hx::Anon_obj::Create(3)
            ->setFixed(0,HX_("write",df,6c,59,d0),result.write)
            ->setFixed(1,HX_("done",82,f0,6d,42),result.done)
            ->setFixed(2,HX_("read",56,4b,a7,4b),result.read);
}

/**
   inflate_end : 'istream -> void
   <doc>Close a decompression stream</doc>
**/
void _hx_inflate_end(Dynamic handle)
{
   GetInflateStream(handle)->destroy();
}



