#include "LibInclude.h"

int RGB::getLuma() { return (r+g+b)/3; }

int RGB::toInt() { return (r<<16) | (g<<8) | b; }
