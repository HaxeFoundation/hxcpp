
::foreach PARAMS:: ::if (ARG>=12)::
Dynamic Dynamic::NS::operator()(::DYNAMIC_ARG_LIST::)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::NS::__new(0,::ARG::)::DYNAMIC_ADDS::);
}
::end::
::end::


