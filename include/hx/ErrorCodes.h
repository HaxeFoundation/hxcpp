#ifndef HX_ERROR_CODES
#define HX_ERROR_CODES

// --- Exteral constants, used inline
#define HX_INVALID_CAST          Dynamic(HX_CSTRING("Invalid Cast"))
#define HX_INVALID_INTERFACE     Dynamic(HX_CSTRING("Object does not implement interface"))
#define HX_INDEX_OUT_OF_BOUNDS   Dynamic(HX_CSTRING("Index Out of Bounds"))
#define HX_INVALID_CONSTRUCTOR   Dynamic(HX_CSTRING("Invalid constructor"))
#define HX_INVALID_ENUM_CONSTRUCTOR(_enum_name, _constructor_name)      \
    Dynamic(HX_CSTRING("Invalid enum constructor for ") +               \
            HX_CSTRING(_enum_name) +                                    \
            HX_CSTRING(": ") +                                          \
            _constructor_name)
#define HX_INVALID_OBJECT        Dynamic(HX_CSTRING("Invalid object"))
#define HX_INVALID_ARG_COUNT     Dynamic(HX_CSTRING("Invalid Arg Count"))
#define HX_NULL_FUNCTION_POINTER Dynamic(HX_CSTRING("Null Function Pointer"))
#define HX_INVALID_ENUM_ARG_COUNT(_enum_name, _constructor_name, _count, _expected) \
    Dynamic(HX_CSTRING("Invalid enum arg count for ") +                 \
            HX_CSTRING(_enum_name) +                                    \
            HX_CSTRING(".") +                                           \
            _constructor_name +                                         \
            HX_CSTRING(": expected ") +                                 \
            ::String(_expected) +                                       \
            HX_CSTRING(", got ") +                                      \
            ::String(_count))

#endif
