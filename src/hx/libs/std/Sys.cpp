#include <hxcpp.h>
#include <hx/OS.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifndef EPPC
#include <sys/types.h>
#include <sys/stat.h>
#endif



#ifdef NEKO_WINDOWS
   #include <windows.h>
   #include <direct.h>
   #include <conio.h>
   #include <locale.h>
#else
   #include <errno.h>
   #ifndef EPPC
      #include <unistd.h>
      #include <dirent.h>
      #include <termios.h>
      #include <sys/time.h>
      #include <sys/times.h>
   #endif
   #include <limits.h>
   #ifndef ANDROID
      #include <locale.h>
      #if !defined(BLACKBERRY) && !defined(EPPC) && !defined(GCW0)
         #include <xlocale.h>
      #endif
   #endif
#endif

#ifdef EMSCRIPTEN
   #include <sys/wait.h>
#endif

#if !defined(IPHONE) && !defined(APPLETV) && !defined(HX_APPLEWATCH)
   #ifdef NEKO_MAC
      #include <sys/syslimits.h>
      #include <limits.h>
      #include <mach-o/dyld.h>
   #endif
#endif

#if defined(HX_WINRT) && !defined(_XBOX_ONE)
   #include <string>
#endif

#ifndef CLK_TCK
   #define CLK_TCK   100
#endif


/**
   <doc>
   <h1>System</h1>
   <p>
   Interactions with the operating system.
   </p>
   </doc>
**/

/**
   get_env : string -> string?
   <doc>Get some environment variable if exists</doc>
**/

String _hx_std_get_env( String v )
{
   const char *s = 0;
   #ifndef HX_WINRT
   s = getenv(v.__s);
   #endif
   return String(s);
}

/**
   put_env : var:string -> val:string -> void
   <doc>Set some environment variable value</doc>
**/
void _hx_std_put_env( String e, String v )
{
#ifdef HX_WINRT
   // Do nothing
#elif defined(NEKO_WINDOWS)
   String set = e + HX_CSTRING("=") + v;
   bool ok = putenv(set.__s) == 0;
#else
   bool ok = setenv(e.__s,v.__s,1) != 0;
#endif
}

/**
   sys_sleep : number -> void
   <doc>Sleep a given number of seconds</doc>
**/

void _hx_std_sys_sleep( double f )
{
   hx::EnterGCFreeZone();
#if defined(NEKO_WINDOWS)
   Sleep((DWORD)(f * 1000));
#elif defined(EPPC)
//TODO: Implement sys_sleep for EPPC
#else
   {
      struct timespec t;
      struct timespec tmp;
      t.tv_sec = (int)(f);
      t.tv_nsec = (int)(((f) - t.tv_sec) * 1e9);
      while( nanosleep(&t,&tmp) == -1 )
      {
         if( errno != EINTR )
         {
            hx::ExitGCFreeZone();
            return;
         }
         t = tmp;
      }
   }
#endif
   hx::ExitGCFreeZone();
}

/**
   set_time_locale : string -> bool
   <doc>Set the locale for LC_TIME, returns true on success</doc>
**/
bool _hx_std_set_time_locale( String l )
{
#if defined(ANDROID) || defined(GCW0)
    return false;
#else

#ifdef NEKO_POSIX
   locale_t lc, old;
   lc = newlocale(LC_TIME_MASK,l.__s,NULL);
   if( !lc )
      return false;
   old = uselocale(lc);
   if( !old )
   {
      freelocale(lc);
      return false;
   }
   if( old != LC_GLOBAL_LOCALE )
      freelocale(old);
   return true;
#else
   return setlocale(LC_TIME,l.__s);
#endif

#endif // !Android
}

/**
   get_cwd : void -> string
   <doc>Return current working directory</doc>
**/
String _hx_std_get_cwd()
{
   #ifdef HX_WINRT
   return String(HX_CSTRING("ms-appdata:///local/"));
   #elif defined(EPPC)
   return String();
   #else
   char buf[1025];
   int l;
   if( getcwd(buf,1024) == NULL )
      return String();
   l = (int)strlen(buf);
   if( buf[l-1] != '/' && buf[l-1] != '\\' ) {
      buf[l] = '/';
      buf[l+1] = 0;
   }
   return String(buf);
   #endif
}

/**
   set_cwd : string -> void
   <doc>Set current working directory</doc>
**/
bool _hx_std_set_cwd( String d )
{
   #if !defined(HX_WINRT) && !defined(EPPC)
   return chdir(d.__s) == 0;
   #else
   return false;
   #endif
}


/**
   sys_string : void -> string
   <doc>
   Return the local system string. The current value are possible :
   <ul>
   <li>[Windows]</li>
   <li>[Linux]</li>
   <li>[BSD]</li>
   <li>[Mac]</li>
   </ul>
   </doc>
**/
String _hx_std_sys_string()
{
#if defined(HX_WINRT)
   return HX_CSTRING("WinRT");
#elif defined(NEKO_WINDOWS)
   return HX_CSTRING("Windows");
#elif defined(NEKO_GNUKBSD)
   return HX_CSTRING("GNU/kFreeBSD");
#elif defined(NEKO_LINUX)
   return HX_CSTRING("Linux");
#elif defined(NEKO_BSD)
   return HX_CSTRING("BSD");
#elif defined(NEKO_MAC)
   return HX_CSTRING("Mac");
#elif defined(ANDROID)
   return HX_CSTRING("Android");
#elif defined(BLACKBERRY)
   return HX_CSTRING("BlackBerry");
#elif defined(EMSCRIPTEN)
   return HX_CSTRING("Emscripten");
#elif defined(EPPC)
   return HX_CSTRING("EPPC");
#else
#error Unknow system string
#endif
}

/**
   sys_is64 : void -> bool
   <doc>
   Returns true if we are on a 64-bit system
   </doc>
**/
bool _hx_std_sys_is64()
{
#ifdef NEKO_64BITS
   return true;
#else
   return false;
#endif
}

/**
   sys_command : string -> int
   <doc>Run the shell command and return exit code</doc>
**/
int _hx_std_sys_command( String cmd )
{
   #if defined(HX_WINRT) || defined(EMSCRIPTEN) || defined(EPPC) || defined(APPLETV) || defined(HX_APPLEWATCH)
   return -1;
   #else
   if( !cmd.__s || !cmd.length )
      return -1;

   hx::EnterGCFreeZone();
   int result = system(cmd.__s);
   hx::ExitGCFreeZone();

   #if !defined(NEKO_WINDOWS) && !defined(ANDROID)
   result = WEXITSTATUS(result) | (WTERMSIG(result) << 8);
   #endif

   return result;
   #endif
}


/**
   sys_exit : int -> void
   <doc>Exit with the given errorcode. Never returns.</doc>
**/
void _hx_std_sys_exit( int code )
{
   exit(code);
}

/**
   sys_exists : string -> bool
   <doc>Returns true if the file or directory exists.</doc>
**/
bool _hx_std_sys_exists( String path )
{
   #ifdef EPPC
   return true;
   #else
   struct stat st;
   hx::EnterGCFreeZone();
   bool result =  stat(path.__s,&st) == 0;
   hx::ExitGCFreeZone();
   return result;
   #endif
}

/**
   file_delete : string -> void
   <doc>Delete the file. Exception on error.</doc>
**/
void _hx_std_file_delete( String path )
{
   #ifndef EPPC
   hx::EnterGCFreeZone();
   bool err = unlink(path.__s);
   hx::ExitGCFreeZone();

   if (err)
      hx::Throw( HX_CSTRING("Could not delete ") + path );
   #endif
}

/**
   sys_rename : from:string -> to:string -> void
   <doc>Rename the file or directory. Exception on error.</doc>
**/
void  _hx_std_sys_rename( String path, String newname )
{
   hx::EnterGCFreeZone();
   bool err = rename(path.__s,newname.__s);
   hx::ExitGCFreeZone();

   if (err)
      hx::Throw(HX_CSTRING("Could not rename"));
}

#define STATF(f) o->Add(HX_CSTRING(#f),(int)(s.st_##f))

/**
   sys_stat : string -> {
      gid => int,
      uid => int,
      atime => 'int,
      mtime => 'int,
      ctime => 'int,
      dev => int,
      ino => int,
      nlink => int,
      rdev => int,
      mode => int,
      size => int
   }
   <doc>Run the [stat] command on the given file or directory.</doc>
**/
Dynamic _hx_std_sys_stat( String path )
{
   #ifdef EPPC
   return alloc_null();
   #else
   hx::EnterGCFreeZone();
   struct stat s;
   bool err = stat(path.__s,&s);
   hx::ExitGCFreeZone();
   if (err)
      return null();
   hx::Anon o = hx::Anon_obj::Create();

   STATF(gid);
   STATF(uid);
   STATF(atime);
   STATF(mtime);
   STATF(ctime);
   STATF(dev);
   STATF(ino);
   STATF(mode);
   STATF(nlink);
   STATF(rdev);
   STATF(size);
   STATF(mode);

   return o;
   #endif
}

/**
   sys_file_type : string -> string
   <doc>
   Return the type of the file. The current values are possible :
   <ul>
   <li>[file]</li>
   <li>[dir]</li>
   <li>[symlink]</li>
   <li>[sock]</li>
   <li>[char]</li>
   <li>[block]</li>
   <li>[fifo]</li>
   </ul>
   </doc>
**/
String _hx_std_sys_file_type( String path )
{
   #ifdef EPPC
   return String();
   #else
   struct stat s;
   hx::EnterGCFreeZone();
   bool err = stat(path.__s,&s);
   hx::ExitGCFreeZone();
   if (err)
      return String();

   if( s.st_mode & S_IFREG )
      return HX_CSTRING("file");
   if( s.st_mode & S_IFDIR )
      return HX_CSTRING("dir");
   if( s.st_mode & S_IFCHR )
      return HX_CSTRING("char");
#ifndef NEKO_WINDOWS
   if( s.st_mode & S_IFLNK )
      return HX_CSTRING("symlink");
   if( s.st_mode & S_IFBLK )
      return HX_CSTRING("block");
   if( s.st_mode & S_IFIFO )
      return HX_CSTRING("fifo");
   if( s.st_mode & S_IFSOCK )
      return HX_CSTRING("sock");
#endif
   return String();
   #endif
}

/**
   sys_create_dir : string -> mode:int -> void
   <doc>Create a directory with the specified rights</doc>
**/
bool _hx_std_sys_create_dir( String path, int mode )
{
   #ifdef EPPC
   return true;
   #else

   hx::EnterGCFreeZone();
#ifdef NEKO_WINDOWS
   bool err = mkdir(path.__s);
#else
   bool err = mkdir(path.__s,mode);
#endif
   hx::ExitGCFreeZone();

   return !err;
   #endif
}

/**
   sys_remove_dir : string -> void
   <doc>Remove a directory. Exception on error</doc>
**/
void _hx_std_sys_remove_dir( String path )
{
   #ifdef EPPC
   return true;
   #else
   hx::EnterGCFreeZone();
   bool ok = rmdir(path.__s) == 0;
   hx::EnterGCFreeZone();
   if (!ok)
      hx::Throw(HX_CSTRING("Could not remove directory"));
   #endif
}

/**
   sys_time : void -> float
   <doc>Return an accurate local time stamp in seconds since Jan 1 1970</doc>
**/
double _hx_std_sys_time()
{
#ifdef NEKO_WINDOWS
#define EPOCH_DIFF   (134774*24*60*60.0)
   SYSTEMTIME t;
   FILETIME ft;
    ULARGE_INTEGER ui;
   GetSystemTime(&t);
   if( !SystemTimeToFileTime(&t,&ft) )
      return 0;
    ui.LowPart = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;
   return ( ((double)ui.QuadPart) / 10000000.0 - EPOCH_DIFF );
#elif defined(EPPC)
   time_t tod;
   time(&tod);
   return ((double)tod);
#else
   struct timeval tv;
   if( gettimeofday(&tv,NULL) != 0 )
      return 0;
   return ( tv.tv_sec + ((double)tv.tv_usec) / 1000000.0 );
#endif
}

/**
   sys_cpu_time : void -> float
   <doc>Return the most accurate CPU time spent since the process started (in seconds)</doc>
**/
double _hx_std_sys_cpu_time()
{
#if defined(HX_WINRT) && !defined(_XBOX_ONE)
    return ((double)GetTickCount64()/1000.0);
#elif defined(NEKO_WINDOWS)
   FILETIME unused;
   FILETIME stime;
   FILETIME utime;
   if( !GetProcessTimes(GetCurrentProcess(),&unused,&unused,&stime,&utime) )
      return 0;
   return ( ((double)(utime.dwHighDateTime+stime.dwHighDateTime)) * 65.536 * 6.5536 + (((double)utime.dwLowDateTime + (double)stime.dwLowDateTime) / 10000000) );
#elif defined(EPPC)
    return ((double)clock()/(double)CLOCKS_PER_SEC);
#else
   struct tms t;
   times(&t);
   return ( ((double)(t.tms_utime + t.tms_stime)) / CLK_TCK );
#endif
}

/**
   sys_read_dir : string -> string list
   <doc>Return the content of a directory</doc>
**/
Array<String> _hx_std_sys_read_dir( String p )
{
   Array<String> result = Array_obj<String>::__new();

#if defined(NEKO_WINDOWS)
   const wchar_t *path = p.__WCStr();
   size_t len = wcslen(path);
   if (len>MAX_PATH)
      return null();

   WIN32_FIND_DATAW d;
   HANDLE handle;
  #if defined(HX_WINRT) && !defined(_XBOX_ONE)
   std::wstring tempWStr(path);
   std::string searchPath(tempWStr.begin(), tempWStr.end());
  #else
   wchar_t searchPath[ MAX_PATH + 4 ];
   memcpy(searchPath,path, len*sizeof(wchar_t));
  #endif


   if( len && path[len-1] != '/' && path[len-1] != '\\' )
      searchPath[len++] = '/';
   searchPath[len++] = '*';
   searchPath[len++] = '.';
   searchPath[len++] = '*';
   searchPath[len] = '\0';

   hx::EnterGCFreeZone();
  #if defined(HX_WINRT) && !defined(_XBOX_ONE)
   handle = FindFirstFileEx(searchPath.c_str(), FindExInfoStandard, &d, FindExSearchNameMatch, NULL, 0);
  #else
   handle = FindFirstFileW(searchPath,&d);
  #endif
   if( handle == INVALID_HANDLE_VALUE )
   {
      hx::ExitGCFreeZone();
      return null();
   }
   while( true )
   {
      // skip magic dirs
      if( d.cFileName[0] != '.' || (d.cFileName[1] != 0 && (d.cFileName[1] != '.' || d.cFileName[2] != 0)) )
      {
         hx::ExitGCFreeZone();
         result->push(String(d.cFileName));
         hx::EnterGCFreeZone();
      }
      if( !FindNextFileW(handle,&d) )
         break;
   }
   FindClose(handle);
#elif !defined(EPPC)
   const char *name = p.__s;
   hx::EnterGCFreeZone();
   DIR *d = opendir(name);
   if( d == NULL )
   {
      hx::ExitGCFreeZone();
      hx::Throw(HX_CSTRING("Invalid directory"));
   }
   while( true )
   {
      struct dirent *e = readdir(d);
      if( e == NULL )
         break;
      // skip magic dirs
      if( e->d_name[0] == '.' && (e->d_name[1] == 0 || (e->d_name[1] == '.' && e->d_name[2] == 0)) )
         continue;
      hx::ExitGCFreeZone();
      result->push( String(e->d_name) );
      hx::ExitGCFreeZone();
   }
   closedir(d);
#endif
   hx::ExitGCFreeZone();

   return result;
}

/**
   file_full_path : string -> string
   <doc>Return an absolute path from a relative one. The file or directory must exists</doc>
**/
String _hx_std_file_full_path( String path )
{
#if defined(HX_WINRT)
   return path;
#elif defined(NEKO_WINDOWS)
   char buf[MAX_PATH+1];
   if( GetFullPathNameA(path.__s,MAX_PATH+1,buf,NULL) == 0 )
      return null();
   return String(buf);
#elif defined(EPPC)
   return path;
#else
   char buf[PATH_MAX];
   if( realpath(path.__s,buf) == NULL )
      return null();
   return String(buf);
#endif
}

/**
   sys_exe_path : void -> string
   <doc>Return the path of the executable</doc>
**/
String _hx_std_sys_exe_path()
{
#if defined(HX_WINRT) && defined(__cplusplus_winrt)
   Windows::ApplicationModel::Package^ package = Windows::ApplicationModel::Package::Current;
   Windows::Storage::StorageFolder^ installedLocation = package->InstalledLocation;
   return(String(installedLocation->Path));
#elif defined(NEKO_WINDOWS)
   wchar_t path[MAX_PATH];
   if( GetModuleFileNameW(NULL,path,MAX_PATH) == 0 )
      return null();
   return String(path);
#elif defined(NEKO_MAC) && !defined(IPHONE) && !defined(APPLETV) && !defined(HX_APPLEWATCH)
   char path[PATH_MAX+1];
   uint32_t path_len = PATH_MAX;
   if( _NSGetExecutablePath(path, &path_len) )
      return null();
   return String(path);
#elif defined(EPPC)
   return HX_CSTRING("");
#else
   {
      char path[PATH_MAX];
      int length = readlink("/proc/self/exe", path, sizeof(path));
      if( length < 0 )
      {
         const char *p = getenv("_");
         if (p)
            return String(p);
         return null();
      }
       path[length] = '\0';
      return String(path);
   }
#endif
}

#if !defined(IPHONE) && !defined(APPLETV) && !defined(HX_APPLEWATCH)
#ifdef NEKO_MAC
#include <crt_externs.h>
#   define environ (*_NSGetEnviron())
#endif
#endif

#ifndef NEKO_WINDOWS
extern char **environ;
#endif

/**
   sys_env : void -> #list
   <doc>Return all the (key,value) pairs in the environment as a chained list</doc>
**/
Array<String> _hx_std_sys_env()
{
   Array<String> result = Array_obj<String>::__new();
   #ifndef HX_WINRT
   char **e = environ;
   while( *e )
   {
      char *x = strchr(*e,'=');
      if( x == NULL )
      {
         e++;
         continue;
      }
      result->push(String(*e,(int)(x-*e)).dup());
      result->push(String(x+1));
      e++;
   }
   #endif
   return result;
}

#ifdef HX_ANDROID
   #define tcsetattr(fd,opt,s) ioctl(fd,opt,s)
   #define tcgetattr(fd,s) ioctl(fd,TCGETS,s)

   static __inline__ void inline_cfmakeraw(struct termios *s)
   {
       s->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
       s->c_oflag &= ~OPOST;
       s->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
       s->c_cflag &= ~(CSIZE|PARENB);
       s->c_cflag |= CS8;
   }

   #define cfmakeraw inline_cfmakeraw

#endif

/**
   sys_getch : bool -> int
   <doc>Read a character from stdin with or without echo</doc>
**/
int _hx_std_sys_getch( bool b )
{
#if defined(HX_WINRT) || defined(EMSCRIPTEN) || defined(EPPC)
   return 0;
#elif defined(NEKO_WINDOWS)
   hx::EnterGCFreeZone();
   int result = b?getche():getch();
   hx::ExitGCFreeZone();

   return result;
#else
   // took some time to figure out how to do that
   // without relying on ncurses, which clear the
   // terminal on initscr()
   int c;
   struct termios term, old;
   hx::EnterGCFreeZone();
   tcgetattr(fileno(stdin), &old);
   term = old;
   cfmakeraw(&term);
   tcsetattr(fileno(stdin), 0, &term);
   c = getchar();
   tcsetattr(fileno(stdin), 0, &old);
   if( b ) fputc(c,stdout);
   hx::ExitGCFreeZone();
   return c;
#   endif
}

/**
   sys_get_pid : void -> int
   <doc>Returns the current process identifier</doc>
**/
int _hx_std_sys_get_pid()
{
#   ifdef NEKO_WINDOWS
   return (int)(GetCurrentProcessId());
#elif defined(EPPC)
   return (1);
#   else
   return (getpid());
#   endif
}


