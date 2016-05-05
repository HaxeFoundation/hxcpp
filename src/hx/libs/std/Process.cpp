#include <hxcpp.h>
#include <hx/OS.h>

#if !defined(HX_WINRT) && !defined(EPPC)

#ifdef NEKO_WINDOWS
#   include <windows.h>
#else
#   include <sys/types.h>
#   include <unistd.h>
#   include <memory.h>
#   include <errno.h>
#   if defined(ANDROID) || defined(BLACKBERRY) || defined(EMSCRIPTEN)
#      include <sys/wait.h>
#   elif !defined(NEKO_MAC)
#      include <wait.h>
#   endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>

namespace
{

#ifndef NEKO_WINDOWS
static int do_close( int fd )
{
   POSIX_LABEL(close_again);
   if( close(fd) != 0 ) {
      HANDLE_EINTR(close_again);
      return 1;
   }
   return 0;
}
#endif


struct vprocess : public hx::Object
{
   bool open;
#ifdef NEKO_WINDOWS
   HANDLE oread;
   HANDLE eread;
   HANDLE iwrite;
   PROCESS_INFORMATION pinf;
   #define HANDLE_INIT 0
#else
   int oread;
   int eread;
   int iwrite;
   int pid;
   #define HANDLE_INIT -1
#endif

   void create()
   {
      open = true;
      oread = HANDLE_INIT;
      eread = HANDLE_INIT;
      iwrite = HANDLE_INIT;
      _hx_set_finalizer(this, finalize);
   }

   void destroy()
   {
      if (open)
      {
         #ifdef NEKO_WINDOWS
            if (eread)
               CloseHandle(eread);
            if (oread)
               CloseHandle(oread);
            if (iwrite)
               CloseHandle(iwrite);
            CloseHandle(pinf.hProcess);
            CloseHandle(pinf.hThread);
         #else
            if (eread!=-1)
               do_close(eread);
            if (oread!=-1)
               do_close(oread);
            if (iwrite!=-1)
               do_close(iwrite);
         #endif
         open = false;
      }
   }

   static void finalize(Dynamic obj)
   {
      ((vprocess *)(obj.mPtr))->destroy();
   }

   String toString() { return HX_CSTRING("vprocess"); }
};

vprocess *getProcess(Dynamic handle)
{
   vprocess *p = dynamic_cast<vprocess *>(handle.mPtr);
   if (!p)
      hx::Throw(HX_CSTRING("Invalid process"));
   return p;
}


/**
   <doc>
   <h1>Process</h1>
   <p>
   An API for starting and communication with sub processes.
   </p>
   </doc>
**/

} // end anon namespace

/**
   process_run : cmd:string -> args:string array -> 'process
   <doc>
   Start a process using a command.
   When args is not null, cmd and args will be auto-quoted/escaped.
   If no auto-quoting/escaping is desired, you should append necessary 
   arguments to cmd as if it is inputted to the shell directly, and pass
   null to args.

   inShowParam = only for windows, SHOW_* from "ShowWindow" function
      default = 1 = SHOW_WINDOW
   </doc>
**/
Dynamic _hx_std_process_run( String cmd, Array<String> vargs, int inShowParam )
{
   #ifdef APPLETV
   return null();

   #else
   vprocess *p = 0;
   bool isRaw = !vargs.mPtr;

   #ifdef NEKO_WINDOWS
   {       
      SECURITY_ATTRIBUTES sattr;      
      STARTUPINFOW sinf;
      HANDLE proc = GetCurrentProcess();
      HANDLE oread,eread,iwrite;
      // creates commandline
      String b;
      if (isRaw)
      {
         b = HX_CSTRING("\"");
         char* cmdexe = getenv("COMSPEC");
         if (!cmdexe) cmdexe = "cmd.exe";
         b += String(cmdexe) + HX_CSTRING("\" /C \"") + cmd + HX_CSTRING("\"");
      }
      else
      {
         b = HX_CSTRING("\"") + cmd + HX_CSTRING("\"");

         for(int i=0;i<vargs->length;i++)
         {
            b += HX_CSTRING(" \"");

            String v = vargs[i];
            int len = v.length;

            std::vector<char> quoted;
            quoted.reserve(len*2);

            unsigned int bs_count = 0;
            for(int j=0;j<len;j++)
            {
               char c = v.__s[j];
               switch( c )
               {
               case '"':
                  // Double backslashes.
                  for (int k=0;k<bs_count*2;k++)
                     quoted.push_back('\\');
                  bs_count = 0;
                  quoted.push_back('\\');
                  quoted.push_back('"');
                  break;
               case '\\':
                  // Don't know if we need to double yet.
                  bs_count++;
                  break;
               default:
                  // Normal char
                  for (int k=0;k<bs_count;k++)
                     quoted.push_back('\\');
                  bs_count = 0;
                  quoted.push_back(c);
                  break;
               }
            }
            // Add remaining backslashes, if any.
            for (int k=0;k<bs_count*2;k++)
               quoted.push_back('\\');

            int qLen = quoted.size();
            quoted.push_back('\0');

            if (quoted.size())
               b+= String( &quoted[0], qLen ).dup();

            b += HX_CSTRING("\"");
         }
      }

      const wchar_t *name = b.__WCStr();

      hx::EnterGCFreeZone();
      // startup process
      sattr.nLength = sizeof(sattr);
      sattr.bInheritHandle = TRUE;
      sattr.lpSecurityDescriptor = NULL;
      memset(&sinf,0,sizeof(sinf));
      sinf.cb = sizeof(sinf);
      sinf.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
      sinf.wShowWindow = inShowParam;
      CreatePipe(&oread,&sinf.hStdOutput,&sattr,0);
      CreatePipe(&eread,&sinf.hStdError,&sattr,0);
      CreatePipe(&sinf.hStdInput,&iwrite,&sattr,0);

      HANDLE procOread,procEread,procIwrite;

      DuplicateHandle(proc,oread,proc,&procOread,0,FALSE,DUPLICATE_SAME_ACCESS);
      DuplicateHandle(proc,eread,proc,&procEread,0,FALSE,DUPLICATE_SAME_ACCESS);
      DuplicateHandle(proc,iwrite,proc,&procIwrite,0,FALSE,DUPLICATE_SAME_ACCESS);
      CloseHandle(oread);
      CloseHandle(eread);
      CloseHandle(iwrite);
      //printf("Cmd %s\n",val_string(cmd));
      PROCESS_INFORMATION pinf;
      memset(&pinf,0,sizeof(pinf));
      if( !CreateProcessW(NULL,(wchar_t *)name,NULL,NULL,TRUE,0,NULL,NULL,&sinf,&pinf) )
      {
         hx::ExitGCFreeZone();
         hx::Throw(HX_CSTRING("Could not start process"));
      }
      // close unused pipes
      CloseHandle(sinf.hStdOutput);
      CloseHandle(sinf.hStdError);
      CloseHandle(sinf.hStdInput);
      hx::ExitGCFreeZone();

      p = new vprocess;
      p->create();
      p->oread = procOread;
      p->eread = procEread;
      p->iwrite = procIwrite;
      p->pinf = pinf;
   }
   #else // not windows ...
   {
   int input[2], output[2], error[2];
   if( pipe(input) || pipe(output) || pipe(error) )
      return null();

   std::vector< std::string > values;
   if (isRaw)
   {
      values.resize(3);
      values[0] = "/bin/sh";
      values[1] = "-c";
      values[2] = cmd.__s;
   }
   else
   {
      values.resize(vargs->length+1);

      values[0] = cmd.__s;
      for(int i=0;i<vargs->length;i++)
         values[i+1] = vargs[i].__s;
   }

   std::vector<const char *> argv(values.size()+1);
   for(int i=0;i<values.size();i++)
      argv[i] = values[i].c_str();

   int pid = fork();
   if( pid == -1 )
      return null();

   // child
   if( pid == 0 )
   {
      close(input[1]);
      close(output[0]);
      close(error[0]);
      dup2(input[0],0);
      dup2(output[1],1);
      dup2(error[1],2);
      execvp(argv[0],(char* const*)&argv[0]);
      fprintf(stderr,"Command not found : %s\n",cmd.__s);
      exit(1);
   }

   // parent
   do_close(input[0]);
   do_close(output[1]);
   do_close(error[1]);

   p = new vprocess;
   p->create();
   p->iwrite = input[1];
   p->oread = output[0];
   p->eread = error[0];
   p->pid = pid;
   }
   #endif

   return p;

   #endif // not APPLETV
}


/**
   process_stdout_read : 'process -> buf:string -> pos:int -> len:int -> int
   <doc>
   Read up to [len] bytes in [buf] starting at [pos] from the process stdout.
   Returns the number of bytes readed this way. Raise an exception if this
   process stdout is closed and no more data is available for reading.

        For hxcpp, the input buffer is in bytes, not characters
   </doc>
**/
int _hx_std_process_stdout_read( Dynamic handle, Array<unsigned char> buf, int pos, int len )
{
   if( pos < 0 || len < 0 || pos + len > buf->length )
      return 0;
   vprocess *p = getProcess(handle);

   unsigned char *dest = &buf[0];
   hx::EnterGCFreeZone();
   #ifdef NEKO_WINDOWS
   DWORD nbytes = 0;
   if( !ReadFile(p->oread,dest+pos,len,&nbytes,0) )
      nbytes = 0;
   #else
   int nbytes = read(p->oread,dest + pos,len);
   if( nbytes <= 0 )
      nbytes = 0;
   #endif

   hx::ExitGCFreeZone();
   return nbytes;
}


/**
   process_stderr_read : 'process -> buf:string -> pos:int -> len:int -> int
   <doc>
   Read up to [len] bytes in [buf] starting at [pos] from the process stderr.
   Returns the number of bytes readed this way. Raise an exception if this
   process stderr is closed and no more data is available for reading.
   </doc>
**/
int _hx_std_process_stderr_read( Dynamic handle, Array<unsigned char> buf, int pos, int len )
{
   if( pos < 0 || len < 0 || pos + len > buf->length )
      return 0;
   vprocess *p = getProcess(handle);

   unsigned char *dest = &buf[0];
   hx::EnterGCFreeZone();
   #ifdef NEKO_WINDOWS
   DWORD nbytes = 0;
   if( !ReadFile(p->eread,dest+pos,len,&nbytes,0) )
      nbytes = 0;
   #else
   int nbytes = read(p->eread,dest + pos,len);
   if( nbytes <= 0 )
      nbytes = 0;
   #endif

   hx::ExitGCFreeZone();
   return nbytes;
}

/**
   process_stdin_write : 'process -> buf:string -> pos:int -> len:int -> int
   <doc>
   Write up to [len] bytes from [buf] starting at [pos] to the process stdin.
   Returns the number of bytes writen this way. Raise an exception if this
   process stdin is closed.
   </doc>
**/
int _hx_std_process_stdin_write( Dynamic handle, Array<unsigned char> buf, int pos, int len )
{
   if( pos < 0 || len < 0 || pos + len > buf->length )
      return 0;
   vprocess *p = getProcess(handle);

   unsigned char *src = &buf[0];


   hx::EnterGCFreeZone();
   #ifdef NEKO_WINDOWS
   DWORD nbytes =0;
   if( !WriteFile(p->iwrite,src+pos,len,&nbytes,0) )
      nbytes = 0;
   #else
   int nbytes = write(p->iwrite,src+pos,len);
   if( nbytes == -1 )
      nbytes = 0;
   #endif

   hx::ExitGCFreeZone();
   return nbytes;
}

/**
   process_stdin_close : 'process -> void
   <doc>
   Close the process standard input.
   </doc>
**/
void _hx_std_process_stdin_close( Dynamic handle )
{
   vprocess *p = getProcess(handle);

   #ifdef NEKO_WINDOWS
   if ( p->iwrite )
      CloseHandle(p->iwrite);
   #else
   if( p->iwrite!=-1 )
      do_close(p->iwrite);
   #endif
   p->iwrite = HANDLE_INIT;
}

/**
   process_exit : 'process -> int
   <doc>
   Wait until the process terminate, then returns its exit code.
   </doc>
**/
int _hx_std_process_exit( Dynamic handle )
{
   vprocess *p = getProcess(handle);

   hx::EnterGCFreeZone();
   #ifdef NEKO_WINDOWS
   {
      DWORD rval;
      WaitForSingleObject(p->pinf.hProcess,INFINITE);
      hx::ExitGCFreeZone();

      if( !GetExitCodeProcess(p->pinf.hProcess,&rval) )
         return 0;
      return rval;
   }
   #else
   int rval=0;
   while( waitpid(p->pid,&rval,0) != p->pid )
   {
      if( errno == EINTR )
         continue;
      hx::ExitGCFreeZone();
      return 0;
   }
   hx::ExitGCFreeZone();
   if( !WIFEXITED(rval) )
      return 0;

   return WEXITSTATUS(rval);
   #endif
}

/**
   process_pid : 'process -> int
   <doc>
   Returns the process id.
   </doc>
**/
int _hx_std_process_pid( Dynamic handle )
{
   vprocess *p = getProcess(handle);

   #ifdef NEKO_WINDOWS
   return p->pinf.dwProcessId;
   #else
   return p->pid;
   #endif
}


/**
   process_close : 'process -> void
   <doc>
   Close the process I/O.
   </doc>
**/
void _hx_std_process_close( Dynamic handle )
{
   vprocess *p = getProcess(handle);
   p->destroy();
}

#else  // !HX_WINRT

Dynamic _hx_std_process_run( String cmd, Array<String> vargs ) { }
int _hx_std_process_stdout_read( Dynamic handle, Array<unsigned char> buf, int pos, int len ) { return 0; }
int _hx_std_process_stderr_read( Dynamic handle, Array<unsigned char> buf, int pos, int len ) { return 0; }
int _hx_std_process_stdin_write( Dynamic handle, Array<unsigned char> buf, int pos, int len ) { return 0; }
void _hx_std_process_stdin_close( value vp ) { }
int _hx_std_process_exit( Dynamic handle ) { return 0; }
int _hx_std_process_pid( Dynamic handle ) { return 0; }
void _hx_std_process_close( Dynamic handle ) { }

#endif // HX_WINRT

