#if !defined(HX_WINRT) && !defined(EPPC)
#include <hxcpp.h>
#include <hx/OS.h>

#include <string.h>


#ifdef NEKO_WINDOWS

#ifdef __GNUC__
   // Mingw / gcc on windows
   #define _WIN32_WINNT 0x0501
   #include <winsock2.h>
   #   include <Ws2tcpip.h>
#else
   // Windows...
   #include <winsock2.h>
   #include <In6addr.h>
   #include <Ws2tcpip.h>
#endif


#define DYNAMIC_INET_FUNCS 1
typedef WINSOCK_API_LINKAGE  INT (WSAAPI *inet_pton_func)( INT Family, PCSTR pszAddrString, PVOID pAddrBuf);
typedef WINSOCK_API_LINKAGE  PCSTR (WSAAPI *inet_ntop_func)(INT  Family, PVOID pAddr, PSTR pStringBuf, size_t StringBufSize);





#   define FDSIZE(n)   (sizeof(u_int) + (n) * sizeof(SOCKET))
#   define SHUT_WR      SD_SEND
#   define SHUT_RD      SD_RECEIVE
#   define SHUT_RDWR   SD_BOTH
   static bool init_done = false;
   static WSADATA init_data;
typedef int SocketLen;
#else
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <sys/time.h>
#   include <netinet/in.h>
#  include <netinet/tcp.h>
#   include <arpa/inet.h>
#   include <unistd.h>
#   include <netdb.h>
#   include <fcntl.h>
#   include <errno.h>
#   include <stdio.h>
#   include <poll.h>
   typedef int SOCKET;
#   define closesocket close
#   define SOCKET_ERROR (-1)
#   define INVALID_SOCKET (-1)
typedef socklen_t SocketLen;
#endif

#if defined(NEKO_WINDOWS) || defined(NEKO_MAC)
#   define MSG_NOSIGNAL 0
#endif



namespace
{

static int socketType = 0;

struct SocketWrapper : public hx::Object
{
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdSocket };

   SOCKET socket;

   int __GetType() const { return socketType; }
};


SOCKET val_sock(Dynamic inValue)
{
   if (inValue.mPtr)
   {
      int type = inValue->__GetType();
      if (type==vtClass)
      {
         inValue = inValue->__Field( HX_CSTRING("__s"), hx::paccNever );
         if (inValue.mPtr==0)
            return 0;
         type = inValue->__GetType();
      }

      if (type==socketType)
         return static_cast<SocketWrapper *>(inValue.mPtr)->socket;
   }

   hx::Throw(HX_CSTRING("Invalid socket handle"));
   return 0;
}


/**
   <doc>
   <h1>Socket</h1>
   <p>
   TCP and UDP sockets
   </p>
   </doc>
**/

static void block_error()
{
   hx::ExitGCFreeZone();
#ifdef NEKO_WINDOWS
   int err = WSAGetLastError();
   if( err == WSAEWOULDBLOCK || err == WSAEALREADY )
#else
   if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS || errno == EALREADY )
#endif
      hx::Throw(HX_CSTRING("Blocking"));
    else {
       hx::Throw(HX_CSTRING("EOF"));
    }
}

}

/**
   socket_init : void -> void
   <doc>
   Initialize the socket API. Must be called at least once per process
   before using any socket or host function.
   </doc>
**/
void _hx_std_socket_init()
{
#ifdef NEKO_WINDOWS
   if( !init_done ) {
      WSAStartup(MAKEWORD(2,0),&init_data);
      init_done = true;
   }
#endif
}


/**
   socket_new : udp:bool -> 'socket
   <doc>Create a new socket, TCP or UDP</doc>
**/
Dynamic _hx_std_socket_new( bool udp, bool ipv6 )
{
   if (!socketType)
      socketType = hxcpp_alloc_kind();

   SOCKET s;
   int family = ipv6 ? AF_INET6 : AF_INET;
   if( udp )
      s = socket(family,SOCK_DGRAM,0);
   else
      s = socket(family,SOCK_STREAM,0);

   if( s == INVALID_SOCKET )
      return null();

   #ifdef NEKO_MAC
      int set = 1;
      setsockopt(s,SOL_SOCKET,SO_NOSIGPIPE,(void *)&set, sizeof(int));
   #endif

   #ifdef NEKO_POSIX
      // we don't want sockets to be inherited in case of exec
      int old = fcntl(s,F_GETFD,0);
      if( old >= 0 ) fcntl(s,F_SETFD,old|FD_CLOEXEC);
   #endif

   SocketWrapper *wrap = new SocketWrapper();
   wrap->socket = s;
   return wrap;
}

/**
   socket_close : 'socket -> void
   <doc>Close a socket. Any subsequent operation on this socket will fail</doc>
**/
void _hx_std_socket_close( Dynamic handle )
{
   SOCKET s = val_sock(handle);
   POSIX_LABEL(close_again);
   if( closesocket(s) ) {
      HANDLE_EINTR(close_again);
   }
}

/**
   socket_send_char : 'socket -> int -> void
   <doc>Send a character over a connected socket. Must be in the range 0..255</doc>
**/
void _hx_std_socket_send_char( Dynamic o, int c )
{
   SOCKET sock = val_sock(o);
   if( c < 0 || c > 255 )
      return;

   unsigned char cc = (unsigned char)c;

   hx::EnterGCFreeZone();
   POSIX_LABEL(send_char_again);
   if( send(sock,(const char *)&cc,1,MSG_NOSIGNAL) == SOCKET_ERROR )
   {
      HANDLE_EINTR(send_char_again);
      block_error();
   }
   hx::ExitGCFreeZone();
}

/**
   socket_send : 'socket -> buf:string -> pos:int -> len:int -> int
   <doc>Send up to [len] bytes from [buf] starting at [pos] over a connected socket.
   Return the number of bytes sent.</doc>
**/
int _hx_std_socket_send( Dynamic o, Array<unsigned char> buf, int p, int l )
{
   SOCKET sock = val_sock(o);
   int dlen = buf->length;
   if( p < 0 || l < 0 || p > dlen || p + l > dlen )
      return 0;

   const char *base = (const char *)&buf[0];
   hx::EnterGCFreeZone();
   dlen = send(sock, base + p , l, MSG_NOSIGNAL);
   if( dlen == SOCKET_ERROR )
      block_error();
   hx::ExitGCFreeZone();
   return dlen;
}

/**
   socket_recv : 'socket -> buf:string -> pos:int -> len:int -> int
   <doc>Read up to [len] bytes from [buf] starting at [pos] from a connected socket.
   Return the number of bytes readed.</doc>
**/
int _hx_std_socket_recv( Dynamic o, Array<unsigned char> buf, int p, int l )
{
   SOCKET sock = val_sock(o);
   int dlen = buf->length;
   if( p < 0 || l < 0 || p > dlen || p + l > dlen )
      return 0;

   char *base = (char *)&buf[0];
   hx::EnterGCFreeZone();
   POSIX_LABEL(recv_again);
   dlen = recv(sock, base + p, l, MSG_NOSIGNAL);
   if( dlen == SOCKET_ERROR )
   {
      HANDLE_EINTR(recv_again);
      block_error();
   }
   hx::ExitGCFreeZone();
   return dlen;
}

/**
   socket_recv_char : 'socket -> int
   <doc>Read a single char from a connected socket.</doc>
**/
int _hx_std_socket_recv_char( Dynamic o )
{
   SOCKET sock = val_sock(o);

   hx::EnterGCFreeZone();
   POSIX_LABEL(recv_char_again);
   unsigned char cc = 0;
   int ret = recv(sock,(char *)&cc,1,MSG_NOSIGNAL);
   if( ret == SOCKET_ERROR )
   {
      HANDLE_EINTR(recv_char_again);
      block_error();
   }
   hx::ExitGCFreeZone();
   if( ret == 0 )
      hx::Throw(HX_CSTRING("Connection closed"));
   return cc;
}


/**
   socket_write : 'socket -> string -> void
   <doc>Send the whole content of a string over a connected socket.</doc>
**/
void _hx_std_socket_write( Dynamic o, Array<unsigned char> buf )
{
   SOCKET sock = val_sock(o);
   int datalen = buf->length;
   const char *cdata = (const char *)&buf[0];
   int pos = 0;

   hx::EnterGCFreeZone();
   while( datalen > 0 )
   {
      POSIX_LABEL(write_again);
      int slen = send(sock,cdata + pos,datalen,MSG_NOSIGNAL);
      if( slen == SOCKET_ERROR ) {
         HANDLE_EINTR(write_again);
         block_error();
      }
      pos += slen;
      datalen -= slen;
   }
   hx::ExitGCFreeZone();
}


/**
   socket_read : 'socket -> string
   <doc>Read the whole content of a the data available from a socket until the connection close.
   If the socket hasn't been close by the other side, the function might block.
   </doc>
**/
Array<unsigned char> _hx_std_socket_read( Dynamic o )
{
   SOCKET sock = val_sock(o);
   Array<unsigned char> result = Array_obj<unsigned char>::__new();
   char buf[256];

   hx::EnterGCFreeZone();
   while( true )
   {
      POSIX_LABEL(read_again);
      int len = recv(sock,buf,256,MSG_NOSIGNAL);
      if( len == SOCKET_ERROR ) {
         HANDLE_EINTR(read_again);
         block_error();
      }
      if( len == 0 )
         break;

      hx::ExitGCFreeZone();
      result->memcpy(result->length, (unsigned char *)buf, len );
      hx::EnterGCFreeZone();
   }

   hx::ExitGCFreeZone();
   return result;
}

/**
   host_resolve : string -> 'int32
   <doc>Resolve the given host string into an IP address.</doc>
**/
int _hx_std_host_resolve( String host )
{
   unsigned int ip;

   hx::EnterGCFreeZone();
   hx::strbuf buf;
   ip = inet_addr(host.utf8_str(&buf));
   if( ip == INADDR_NONE )
   {
      struct hostent *h = 0;
      hx::strbuf hostBuf;

#   if defined(NEKO_WINDOWS) || defined(NEKO_MAC) || defined(BLACKBERRY) || defined(EMSCRIPTEN)
      h = gethostbyname(host.utf8_str(&hostBuf));
#   else
      struct hostent hbase;
      char buf[1024];
      int errcode;
      gethostbyname_r(host.utf8_str(&hostBuf),&hbase,buf,1024,&h,&errcode);
#   endif
      if( !h ) {
         hx::ExitGCFreeZone();
         return hx::Throw( HX_CSTRING("Unknown host:") + host );
      }
      ip = *((unsigned int*)h->h_addr);
   }
   hx::ExitGCFreeZone();
   return ip;
}

#ifdef DYNAMIC_INET_FUNCS
bool dynamic_inet_pton_tried = false;
inet_pton_func dynamic_inet_pton = 0;
#endif

Array<unsigned char> _hx_std_host_resolve_ipv6( String host, bool )
{
   in6_addr ipv6;

   hx::strbuf hostBuf;
   const char *hostStr = host.utf8_str(&hostBuf);
   #ifdef DYNAMIC_INET_FUNCS
   if (!dynamic_inet_pton_tried)
   {
      dynamic_inet_pton_tried = true;
      HMODULE module = LoadLibraryA("WS2_32.dll");
      if (module)
         dynamic_inet_pton = (inet_pton_func)GetProcAddress(module,"inet_pton");
   }
   int ok = dynamic_inet_pton ? dynamic_inet_pton(AF_INET6, hostStr, (void *)&ipv6) : 0;
   #else
   int ok = inet_pton(AF_INET6, hostStr, (void *)&ipv6);
   #endif

   if (!ok)
   {
      addrinfo hints;

      memset(&hints, 0, sizeof(struct addrinfo));
      hints.ai_family = AF_INET6;  //  IPv6
      hints.ai_socktype = 0;  // any - SOCK_STREAM or SOCK_DGRAM
      hints.ai_flags = AI_PASSIVE;  // For wildcard IP address 
      hints.ai_protocol = 0;        // Any protocol
      hints.ai_canonname = 0;
      hints.ai_addr = 0;
      hints.ai_next = 0;

      addrinfo *result = 0;
      hx::EnterGCFreeZone();
      int err =  getaddrinfo( hostStr, 0, &hints, &result);
      hx::ExitGCFreeZone();
      if (err==0)
      {
         for(addrinfo * rp = result; rp; rp = rp->ai_next)
         {
            if (rp->ai_family==AF_INET6)
            {
               sockaddr_in6 *s6 = (sockaddr_in6 *)rp->ai_addr;
               ipv6 = s6->sin6_addr;
               ok = true;
               break;
            }
            else
            {
               freeaddrinfo(result);
               hx::Throw( HX_CSTRING("Unkown ai_family") );
            }
         }
         freeaddrinfo(result);
      }
      else
      {
         hx::Throw( host + HX_CSTRING(":") + String(gai_strerror(err)) );
      }
   }

   if (!ok)
      return null();

   return Array_obj<unsigned char>::fromData( (unsigned char *)&ipv6, 16 );
}



/**
   host_to_string : 'int32 -> string
   <doc>Return a string representation of the IP address.</doc>
**/
String _hx_std_host_to_string( int ip )
{
   struct in_addr i;
   *(int*)&i = ip;
   return String( inet_ntoa(i) );
}


#ifdef DYNAMIC_INET_FUNCS
bool dynamic_inet_ntop_tried = false;
inet_ntop_func dynamic_inet_ntop = 0;
#endif


String _hx_std_host_to_string_ipv6( Array<unsigned char> ip )
{
   char buf[100];
   #ifdef DYNAMIC_INET_FUNCS
   if (!dynamic_inet_ntop_tried)
   {
      dynamic_inet_ntop_tried = true;
      HMODULE module = LoadLibraryA("WS2_32.dll");
      if (module)
         dynamic_inet_ntop = (inet_ntop_func)GetProcAddress(module,"inet_ntop");
   }
   if (!dynamic_inet_ntop)
      return String();
   return String( dynamic_inet_ntop(AF_INET6, &ip[0], buf, 100) );
   #else
   return String( inet_ntop(AF_INET6, &ip[0], buf, 100) );
   #endif
}

/**
   host_reverse : 'int32 -> string
   <doc>Reverse the DNS of the given IP address.</doc>
**/
String _hx_std_host_reverse( int host )
{
   struct hostent *h = 0;
   unsigned int ip = host;
   hx::EnterGCFreeZone();
   #if defined(NEKO_WINDOWS) || defined(NEKO_MAC) || defined(ANDROID) || defined(BLACKBERRY) || defined(EMSCRIPTEN)
   h = gethostbyaddr((char *)&ip,4,AF_INET);
   #else
   struct hostent htmp;
   int errcode;
   char buf[1024];
   gethostbyaddr_r((char *)&ip,4,AF_INET,&htmp,buf,1024,&h,&errcode);
   #endif
   hx::ExitGCFreeZone();
   if( !h )
      return String();
   return String( h->h_name );
}

String _hx_std_host_reverse_ipv6( Array<unsigned char> host )
{
   if (!host.mPtr || host->length!=16)
      return String();

   struct hostent *h = 0;
   hx::EnterGCFreeZone();
   #if defined(NEKO_WINDOWS) || defined(NEKO_MAC) || defined(ANDROID) || defined(BLACKBERRY) || defined(EMSCRIPTEN)
   h = gethostbyaddr((char *)&host[0],16,AF_INET6);
   #else
   struct hostent htmp;
   int errcode;
   char buf[1024];
   gethostbyaddr_r((char *)&host[0],16,AF_INET6,&htmp,buf,1024,&h,&errcode);
   #endif
   hx::ExitGCFreeZone();
   if( !h )
      return String();
   return String( h->h_name );
}


/**
   host_local : void -> string
   <doc>Return the local host name.</doc>
**/
String _hx_std_host_local()
{
   char buf[256];
   hx::EnterGCFreeZone();
   if( gethostname(buf,256) == SOCKET_ERROR )
   {
      hx::ExitGCFreeZone();
      return String();
   }
   hx::ExitGCFreeZone();
   return String(buf);
}

/**
   socket_connect : 'socket -> host:'int32 -> port:int -> void
   <doc>Connect the socket the given [host] and [port]</doc>
**/
void _hx_std_socket_connect( Dynamic o, int host, int port )
{
   struct sockaddr_in addr;
   memset(&addr,0,sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   *(int*)&addr.sin_addr.s_addr = host;

   hx::EnterGCFreeZone();
   if( connect(val_sock(o),(struct sockaddr*)&addr,sizeof(addr)) != 0 )
   {
      // This will throw a "Blocking" exception if the "error" was because
      // it's a non-blocking socket with connection in progress, otherwise
      // it will do nothing.
      //
      // - now it always throws
      block_error();
   }
   hx::ExitGCFreeZone();
}


/**
   socket_connect - to ipv6 host
**/
void _hx_std_socket_connect_ipv6( Dynamic o, Array<unsigned char> host, int port )
{
   struct sockaddr_in6 addr;
   memset(&addr,0,sizeof(addr));
   addr.sin6_family = AF_INET6;
   addr.sin6_port = htons(port);
   memcpy(&addr.sin6_addr,&host[0],16);

   hx::EnterGCFreeZone();
   if( connect(val_sock(o),(struct sockaddr*)&addr,sizeof(addr)) != 0 )
   {
      // This will throw a "Blocking" exception if the "error" was because
      // it's a non-blocking socket with connection in progress, otherwise
      // it will do nothing.
      //
      // - now it always throws
      block_error();
   }
   hx::ExitGCFreeZone();
}

/**
   socket_listen : 'socket -> int -> void
   <doc>Listen for a number of connections</doc>
**/
void _hx_std_socket_listen( Dynamic o, int n )
{
   SOCKET sock = val_sock(o);
   hx::EnterGCFreeZone();
   if( listen(sock,n) == SOCKET_ERROR )
   {
      hx::ExitGCFreeZone();
      return;
   }
   hx::ExitGCFreeZone();
}

static fd_set INVALID;

static fd_set *make_socket_array( Array<Dynamic> a, fd_set *tmp, SOCKET *n )
{
   FD_ZERO(tmp);
   if( !a.mPtr )
      return tmp;

   int len = a->length;
   if( len > FD_SETSIZE )
      hx::Throw(HX_CSTRING("Too many sockets in select"));

   for(int i=0;i<len;i++)
   {
      // make sure it is a socket...
      SOCKET sock = val_sock( a[i] );
      if( sock > *n )
         *n = sock;
      FD_SET(sock,tmp);
   }
   return tmp;
}

static Array<Dynamic> make_array_result( Array<Dynamic> a, fd_set *tmp )
{
   if (!tmp || !a.mPtr)
      return null();

   int len = a->length;
   Array<Dynamic> r = Array_obj<Dynamic>::__new();
   for(int i=0;i<len;i++)
   {
      Dynamic s = a[i];
      if( FD_ISSET(val_sock(s),tmp) )
         r->push(s);
   }
   return r;
}

static void make_array_result_inplace(Array<Dynamic> a, fd_set *tmp)
{
    if (!a.mPtr)
      return;

    if (tmp == NULL)
    {
       a->__SetSize(0);
       return;
    }

    int len = a->length;
    int destPos = 0;
    for(int i = 0; i < len; i++)
    {
       Dynamic s = a[i];
       if (FD_ISSET(val_sock(s), tmp)) {
           a[destPos++] = s;
       }
    }

    a->__SetSize(destPos);
}

static struct timeval *init_timeval( double f, struct timeval *t ) {
   if (f<0)
      return 0;
   t->tv_usec = (f - (int)f ) * 1000000;
   t->tv_sec = (int)f;
   return t;
}

/**
   socket_select : read : 'socket array -> write : 'socket array -> others : 'socket array -> timeout:number? -> 'socket array array
   <doc>Perform the [select] operation. Timeout is in seconds or [null] if infinite</doc>
**/
Array<Dynamic> _hx_std_socket_select( Array<Dynamic> rs, Array<Dynamic> ws, Array<Dynamic> es, Dynamic timeout )
{
   SOCKET n = 0;
   fd_set rx, wx, ex;
   fd_set *ra, *wa, *ea;

   POSIX_LABEL(select_again);
   ra = make_socket_array(rs,&rx,&n);
   wa = make_socket_array(ws,&wx,&n);
   ea = make_socket_array(es,&ex,&n);
   if( ra == &INVALID || wa == &INVALID || ea == &INVALID )
      hx::Throw( HX_CSTRING("No valid sockets") );

   struct timeval tval;
   struct timeval *tt = 0;
   if( timeout.mPtr )
      tt = init_timeval(timeout,&tval);

   hx::EnterGCFreeZone();
   if( select((int)(n+1),ra,wa,ea,tt) == SOCKET_ERROR )
   {
      hx::ExitGCFreeZone();
      HANDLE_EINTR(select_again);
      hx::Throw( HX_CSTRING("Select error ") + String((int)errno) );
   }
   hx::ExitGCFreeZone();

   Array<Dynamic> r = Array_obj<Dynamic>::__new(3,3);
   r[0] = make_array_result(rs,ra);
   r[1] = make_array_result(ws,wa);
   r[2] = make_array_result(es,ea);
   return r;
}

/**
   socket_select : read : 'socket array -> write : 'socket array -> others : 'socket array -> timeout:number?
   <doc>Perform the [select] operation. Timeout is in seconds or [null] if infinite</doc>
**/
void _hx_std_socket_fast_select( Array<Dynamic> rs, Array<Dynamic> ws, Array<Dynamic> es, Dynamic timeout )
{
   SOCKET n = 0;
   fd_set rx, wx, ex;
   fd_set *ra, *wa, *ea;

   POSIX_LABEL(select_again);
   ra = make_socket_array(rs,&rx,&n);
   wa = make_socket_array(ws,&wx,&n);
   ea = make_socket_array(es,&ex,&n);

   if( ra == &INVALID || wa == &INVALID || ea == &INVALID )
      hx::Throw( HX_CSTRING("No valid sockets") );


   struct timeval tval;
   struct timeval *tt = 0;
   if( timeout.mPtr )
      tt = init_timeval(timeout,&tval);

   hx::EnterGCFreeZone();
   if( select((int)(n+1),ra,wa,ea,tt) == SOCKET_ERROR )
   {
      hx::ExitGCFreeZone();
      HANDLE_EINTR(select_again);
      #ifdef NEKO_WINDOWS
      hx::Throw( HX_CSTRING("Select error ") + String((int)WSAGetLastError()) );
      #else
      hx::Throw( HX_CSTRING("Select error ") + String((int)errno) );
      #endif
   }

   hx::ExitGCFreeZone();
   make_array_result_inplace(rs, ra);
   make_array_result_inplace(ws, wa);
   make_array_result_inplace(es, ea);
}

/**
   socket_bind : 'socket -> host : 'int -> port:int -> void
   <doc>Bind the socket for server usage on the given host and port</doc>
**/
void _hx_std_socket_bind( Dynamic o, int host, int port )
{
   SOCKET sock = val_sock(o);

   int opt = 1;
   struct sockaddr_in addr;
   memset(&addr,0,sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   *(int*)&addr.sin_addr.s_addr = host;
   #ifndef NEKO_WINDOWS
   setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&opt,sizeof(opt));
   #endif

   hx::EnterGCFreeZone();
   if( bind(sock,(struct sockaddr*)&addr,sizeof(addr)) == SOCKET_ERROR )
   {
      hx::ExitGCFreeZone();
      hx::Throw(HX_CSTRING("Bind failed"));
   }
   hx::ExitGCFreeZone();
}


/**
   socket_bind - ipv6 version
   <doc>Bind the socket for server usage on the given host and port</doc>
**/
void _hx_std_socket_bind_ipv6( Dynamic o, Array<unsigned char> host, int port )
{
   SOCKET sock = val_sock(o);

   int opt = 1;

   struct sockaddr_in6 addr;
   memset(&addr,0,sizeof(addr));
   addr.sin6_family = AF_INET6;
   addr.sin6_port = htons(port);
   memcpy(&addr.sin6_addr,&host[0], 16);
   #ifndef NEKO_WINDOWS
   setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&opt,sizeof(opt));
   #endif

   hx::EnterGCFreeZone();
   if( bind(sock,(struct sockaddr*)&addr,sizeof(addr)) == SOCKET_ERROR )
   {
      hx::ExitGCFreeZone();
      hx::Throw(HX_CSTRING("Bind failed"));
   }
   hx::ExitGCFreeZone();
}


/**
   socket_accept : 'socket -> 'socket
   <doc>Accept an incoming connection request</doc>
**/

#ifdef _WIN32
   typedef int SockLen;
#elif defined(ANDROID)
   typedef socklen_t SockLen;
#else
   typedef unsigned int SockLen;
#endif

Dynamic _hx_std_socket_accept( Dynamic o )
{
   SOCKET sock = val_sock(o);
   struct sockaddr_in addr;
   SockLen addrlen = sizeof(addr);
   SOCKET s;
   hx::EnterGCFreeZone();
   s = accept(sock,(struct sockaddr*)&addr,&addrlen);
   if( s == INVALID_SOCKET )
      block_error();
   hx::ExitGCFreeZone();

   SocketWrapper *wrap = new SocketWrapper();
   wrap->socket = s;
   return wrap;
}

/**
   socket_peer : 'socket -> #address
   <doc>Return the socket connected peer address composed of an (host,port) array</doc>
**/
Array<int> _hx_std_socket_peer( Dynamic o )
{
   SOCKET sock = val_sock(o);
   struct sockaddr_in addr;
   SockLen addrlen = sizeof(addr);
   hx::EnterGCFreeZone();
   if( getpeername(sock,(struct sockaddr*)&addr,&addrlen) == SOCKET_ERROR )
   {
      hx::ExitGCFreeZone();
      return null();
   }
   hx::ExitGCFreeZone();

   Array<int> ret = Array_obj<int>::__new(2,2);
   ret[0] = *(int*)&addr.sin_addr;
   ret[1] = ntohs(addr.sin_port);
   return ret;
}

/**
   socket_host : 'socket -> #address
   <doc>Return the socket local address composed of an (host,port) array</doc>
**/
Array<int> _hx_std_socket_host( Dynamic o )
{
   SOCKET sock = val_sock(o);
   struct sockaddr_in addr;
   SockLen addrlen = sizeof(addr);
   hx::EnterGCFreeZone();
   if( getsockname(sock,(struct sockaddr*)&addr,&addrlen) == SOCKET_ERROR )
   {
      hx::ExitGCFreeZone();
      return null();
   }
   hx::ExitGCFreeZone();

   Array<int> ret = Array_obj<int>::__new(2,2);
   ret[0] = *(int*)&addr.sin_addr;
   ret[1] = ntohs(addr.sin_port);
   return ret;
}

/**
   socket_set_timeout : 'socket -> timout:number? -> void
   <doc>Set the socket send and recv timeout in seconds to the given value (or null for blocking)</doc>
**/
void _hx_std_socket_set_timeout( Dynamic o, Dynamic t )
{
   SOCKET sock = val_sock(o);

#ifdef NEKO_WINDOWS
   int time;
   if( !t.mPtr )
      time = 0;
   else {
      time = (int)((double)(t) * 1000);
   }
#else
   struct timeval time;
   if( t.mPtr==0 ) {
      time.tv_usec = 0;
      time.tv_sec = 0;
   } else {
      init_timeval(t,&time);
   }
#endif

   hx::EnterGCFreeZone();
   if( setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,(char*)&time,sizeof(time)) != 0 )
   {
      hx::ExitGCFreeZone();
      return;
   }
   if( setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char*)&time,sizeof(time)) != 0 )
   {
      hx::ExitGCFreeZone();
      return;
   }
   hx::ExitGCFreeZone();
}

/**
   socket_shutdown : 'socket -> read:bool -> write:bool -> void
   <doc>Prevent the socket from further reading or writing or both.</doc>
**/
void _hx_std_socket_shutdown( Dynamic o, bool r, bool w )
{
   SOCKET sock = val_sock(o);
   if( !r && !w )
      return;

   hx::EnterGCFreeZone();
   if( shutdown(sock,(r)?((w)?SHUT_RDWR:SHUT_RD):SHUT_WR) )
   {
      hx::ExitGCFreeZone();
      return;
   }
   hx::ExitGCFreeZone();
}

/**
   socket_set_blocking : 'socket -> bool -> void
   <doc>Turn on/off the socket blocking mode.</doc>
**/
#include <stdlib.h>

void _hx_std_socket_set_blocking( Dynamic o, bool b )
{
   SOCKET sock = val_sock(o);
   hx::EnterGCFreeZone();
#ifdef NEKO_WINDOWS
   {
      unsigned long arg = b?0:1;
      if( ioctlsocket(sock,FIONBIO,&arg) != 0 )
      {
         hx::ExitGCFreeZone();
         return;
      }
   }
#else
   {
      int rights = fcntl(sock,F_GETFL);
      if( rights == -1 )
      {
         hx::ExitGCFreeZone();
         return;
      }
      if( b )
         rights &= ~O_NONBLOCK;
      else
         rights |= O_NONBLOCK;
      if( fcntl(sock,F_SETFL,rights) == -1 )
      {
         hx::ExitGCFreeZone();
         return;
      }
   }
#endif
   hx::ExitGCFreeZone();
   return;
}


void _hx_std_socket_set_fast_send( Dynamic o, bool b )
{
   SOCKET sock = val_sock(o);
   int fast = (b);
   hx::EnterGCFreeZone();
   setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,(char*)&fast,sizeof(fast));
   hx::ExitGCFreeZone();
}

void _hx_std_socket_set_broadcast( Dynamic o, bool b )
{
   SOCKET sock = val_sock(o);
   int broadcast = (b);
   hx::EnterGCFreeZone();
   setsockopt(sock,SOL_SOCKET,SO_BROADCAST,(char*)&broadcast,sizeof(broadcast));
   hx::ExitGCFreeZone();
}


/**
   socket_poll_alloc : int -> 'poll
   <doc>Allocate memory to perform polling on a given number of sockets</doc>
**/

namespace
{

static int pollType = 0;

struct polldata : public hx::Object
{
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdPollData };

   bool ok;
   int max;
   #ifdef NEKO_WINDOWS
   struct fd_set *fdr;
   struct fd_set *fdw;
   struct fd_set *outr;
   struct fd_set *outw;
   #else
   struct pollfd *fds;
   int rcount;
   int wcount;
   #endif
   Array<int> ridx;
   Array<int> widx;

   void create(int nsocks)
   {
      ok = true;
      max = nsocks;
      #ifdef NEKO_WINDOWS
      fdr = (fd_set*)malloc(FDSIZE(max));
      fdw = (fd_set*)malloc(FDSIZE(max));
      outr = (fd_set*)malloc(FDSIZE(max));
      outw = (fd_set*)malloc(FDSIZE(max));
      fdr->fd_count = 0;
      fdw->fd_count = 0;

      #else
      fds = (struct pollfd*)malloc(sizeof(struct pollfd) * max);
      rcount = 0;
      wcount = 0;
      #endif

      ridx = Array_obj<int>::__new(max+1,max+1);
      HX_OBJ_WB_GET(this, ridx.mPtr);
      widx = Array_obj<int>::__new(max+1,max+1);
      HX_OBJ_WB_GET(this, widx.mPtr);
      for(int i=0;i<=max;i++)
      {
         ridx[i] = -1;
         widx[i] = -1;
      }

      _hx_set_finalizer(this, finalize);
   }

   void destroy()
   {
      if (ok)
      {
         ok = false;
         #ifdef NEKO_WINDOWS
         free(fdr);
         free(fdw);
         free(outr);
         free(outw);
         #else
         // ???
         free(fds);
         #endif
      }
   }

   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER(ridx); HX_MARK_MEMBER(widx); }
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(ridx); HX_VISIT_MEMBER(widx); }
   #endif

   int __GetType() const { return pollType; }

   static void finalize(Dynamic obj)
   {
      ((polldata *)(obj.mPtr))->destroy();
   }

   String toString() { return HX_CSTRING("polldata"); }
};

polldata *val_poll(Dynamic o)
{
   if (!o.mPtr || o->__GetType()!=pollType)
      hx::Throw(HX_CSTRING("Invalid polldata:") + o);
   return static_cast<polldata *>(o.mPtr);
}


} // end namespace



Dynamic _hx_std_socket_poll_alloc( int nsocks )
{
   if( nsocks < 0 || nsocks > 1000000 )
      return null();

   if (pollType==0)
      pollType = hxcpp_alloc_kind();

   polldata *p = new polldata;
   p->create(nsocks);
   return p;
}

/**
   socket_poll_prepare : 'poll -> read:'socket array -> write:'socket array -> int array array
   <doc>
   Prepare a poll for scanning events on sets of sockets.
   </doc>
**/
Array<Dynamic> _hx_std_socket_poll_prepare( Dynamic pdata, Array<Dynamic> rsocks, Array<Dynamic> wsocks )
{
   polldata *p = val_poll(pdata);

   int len = rsocks.mPtr ? rsocks->length : 0;
   int wlen = wsocks.mPtr ? wsocks->length : 0;
   if( len + wlen > p->max )
      hx::Throw(HX_CSTRING("Too many sockets in poll"));

   #ifdef NEKO_WINDOWS
   for(int i=0;i<len;i++)
      p->fdr->fd_array[i] = val_sock( rsocks[i] );
   p->fdr->fd_count = len;

   len = wlen;
   for(int i=0;i<len;i++)
      p->fdw->fd_array[i] = val_sock( wsocks[i]);
   p->fdw->fd_count = len;

   #else

   for(int i=0;i<len;i++)
   {
      p->fds[i].fd = val_sock( rsocks[i] );
      p->fds[i].events = POLLIN;
      p->fds[i].revents = 0;
   }
   p->rcount = len;
   len = wlen;
   for(int i=0;i<len;i++)
   {
      int k = i + p->rcount;
      p->fds[k].fd = val_sock(wsocks[i]);
      p->fds[k].events = POLLOUT;
      p->fds[k].revents = 0;
   }
   p->wcount = len;

   #endif

   Array<Dynamic> a = Array_obj<Dynamic>::__new(2,2);
   a[0] = p->ridx;
   a[1] = p->widx;
   return a;
}


/**
   socket_poll_events : 'poll -> timeout:float -> void
   <doc>
   Update the read/write flags arrays that were created with [socket_poll_prepare].
   </doc>
**/
void _hx_std_socket_poll_events( Dynamic pdata, double timeout )
{
   polldata *p = val_poll(pdata);

   #ifdef NEKO_WINDOWS
   memcpy(p->outr,p->fdr,FDSIZE(p->fdr->fd_count));
   memcpy(p->outw,p->fdw,FDSIZE(p->fdw->fd_count));

   struct timeval t;
   struct timeval *tt = init_timeval(timeout,&t);

   hx::EnterGCFreeZone();
   if( select(0/* Ignored */, p->fdr->fd_count ? p->outr : 0, p->fdw->fd_count ?p->outw : 0,NULL,tt) == SOCKET_ERROR )
   {
      hx::ExitGCFreeZone();
      return;
   }
   hx::ExitGCFreeZone();

   int k = 0;
   for(int i=0;i<p->fdr->fd_count;i++)
      if( FD_ISSET(p->fdr->fd_array[i],p->outr) )
         p->ridx[k++] = i;
   p->ridx[k] = -1;

   k = 0;
   for(int i=0;i<p->fdw->fd_count;i++)
      if( FD_ISSET(p->fdw->fd_array[i],p->outw) )
         p->widx[k++] = i;
   p->widx[k] = -1;

   #else

   int tot = p->rcount + p->wcount;
   hx::EnterGCFreeZone();
   POSIX_LABEL(poll_events_again);
   if( poll(p->fds,tot,(int)(timeout * 1000)) < 0 )
   {
      HANDLE_EINTR(poll_events_again);
      hx::ExitGCFreeZone();
      return;
   }
   hx::ExitGCFreeZone();

   int k = 0;
   int i = 0;
   for(i=0;i<p->rcount;i++)
      if( p->fds[i].revents & (POLLIN|POLLHUP) )
         p->ridx[k++] = i;
   p->ridx[k] = -1;
   k = 0;
   for(;i<tot;i++)
      if( p->fds[i].revents & (POLLOUT|POLLHUP) )
         p->widx[k++] = i - p->rcount;
   p->widx[k] = -1;
   #endif
}


/**
   socket_poll : 'socket array -> 'poll -> timeout:float -> 'socket array
   <doc>
   Perform a polling for data available over a given set of sockets. This is similar to [socket_select]
   except that [socket_select] is limited to a given number of simultaneous sockets to check.
   </doc>
**/
Array<Dynamic> _hx_std_socket_poll( Array<Dynamic> socks, Dynamic pdata, double timeout )
{
   polldata *p = val_poll(pdata);

   _hx_std_socket_poll_prepare(pdata,socks,null());

   _hx_std_socket_poll_events(pdata,timeout);

   int rcount = 0;
   while( p->ridx[rcount] != -1 )
      rcount++;

   Array<Dynamic> a = Array_obj<Dynamic>::__new(rcount,rcount);
   for(int i=0;i<rcount;i++)
      a[i] = socks[p->ridx[i]];
   return a;
}



/**
   socket_send_to : 'socket -> buf:string -> pos:int -> length:int -> addr:{host:'int32,port:int} -> int
   <doc>
   Send data from an unconnected UDP socket to the given address.
   </doc>
**/
int _hx_std_socket_send_to( Dynamic o, Array<unsigned char> buf, int p, int l, Dynamic inAddr )
{
   SOCKET sock = val_sock(o);

   const char *cdata = (const char *)&buf[0];
   int dlen = buf->length;
   if( p < 0 || l < 0 || p > dlen || p + l > dlen )
      hx::Throw(HX_CSTRING("Invalid data position"));


   int host = inAddr->__Field(HX_CSTRING("host"), hx::paccDynamic);
   int port = inAddr->__Field(HX_CSTRING("port"), hx::paccDynamic);
   struct sockaddr_in addr;
   memset(&addr,0,sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   *(int*)&addr.sin_addr.s_addr = host;


   hx::EnterGCFreeZone();
   POSIX_LABEL(send_again);
   dlen = sendto(sock, cdata + p , l, MSG_NOSIGNAL, (struct sockaddr*)&addr, sizeof(addr));
   if( dlen == SOCKET_ERROR ) {
      HANDLE_EINTR(send_again);
      block_error();
   }
   hx::ExitGCFreeZone();
   return dlen;
}

/**
   socket_recv_from : 'socket -> buf:string -> pos:int -> length:int -> addr:{host:'int32,port:int} -> int
   <doc>
   Read data from an unconnected UDP socket, store the address from which we received data in addr.
   </doc>
**/
#define NRETRYS   20
int _hx_std_socket_recv_from( Dynamic o, Array<unsigned char> buf, int p, int l, Dynamic outAddr)
{
   int retry = 0;
   SOCKET sock = val_sock(o);

   char *data = (char *)&buf[0];
   int dlen = buf->length;
   if( p < 0 || l < 0 || p > dlen || p + l > dlen )
      hx::Throw(HX_CSTRING("Invalid data position"));

   struct sockaddr_in saddr;
   SockLen slen = sizeof(saddr);

   int ret = 0;
   hx::EnterGCFreeZone();
   POSIX_LABEL(recv_from_again);
   if( retry++ > NRETRYS ) {
      ret = recv(sock,data+p,l,MSG_NOSIGNAL);
   } else
      ret = recvfrom(sock, data + p , l, MSG_NOSIGNAL, (struct sockaddr*)&saddr, &slen);
   if( ret == SOCKET_ERROR ) {
      HANDLE_EINTR(recv_from_again);
      block_error();
   }

   hx::ExitGCFreeZone();
   outAddr->__SetField(HX_CSTRING("host"),*(int*)&saddr.sin_addr, hx::paccDynamic);
   outAddr->__SetField(HX_CSTRING("port"),ntohs(saddr.sin_port), hx::paccDynamic);

   return ret;
}


#else // !HX_WINRT
// TODO: WinRT StreamSocket port
#endif // HX_WINRT

