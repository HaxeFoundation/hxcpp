
void nativeLoop()
{
   while( gc::TestGCThreaded_obj::keepRunning)
   {
 
      for(int i=0; i<100; i++)
         ::gc::Wrapper_obj::__alloc( HX_CTX_GET ,1);

      ::Sys_obj::sleep(0.01);
   }
   gc::TestGCThreaded_obj::nativeRunning = false;
}


