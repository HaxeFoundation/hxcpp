// Note: For maximum-speed code, see "Optimizing Code" on the Emscripten wiki, https://github.com/kripken/emscripten/wiki/Optimizing-Code
// Note: Some Emscripten settings may limit the speed of the generated code.
try {
  this['Module'] = Module;
} catch(e) {
  this['Module'] = Module = {};
}
// The environment setup code below is customized to use Module.
// *** Environment setup code ***
var ENVIRONMENT_IS_NODE = typeof process === 'object' && typeof require === 'function';
var ENVIRONMENT_IS_WEB = typeof window === 'object';
var ENVIRONMENT_IS_WORKER = typeof importScripts === 'function';
var ENVIRONMENT_IS_SHELL = !ENVIRONMENT_IS_WEB && !ENVIRONMENT_IS_NODE && !ENVIRONMENT_IS_WORKER;
if (ENVIRONMENT_IS_NODE) {
  // Expose functionality in the same simple way that the shells work
  // Note that we pollute the global namespace here, otherwise we break in node
  Module['print'] = function(x) {
    process['stdout'].write(x + '\n');
  };
  Module['printErr'] = function(x) {
    process['stderr'].write(x + '\n');
  };
  var nodeFS = require('fs');
  var nodePath = require('path');
  Module['read'] = function(filename) {
    filename = nodePath['normalize'](filename);
    var ret = nodeFS['readFileSync'](filename).toString();
    // The path is absolute if the normalized version is the same as the resolved.
    if (!ret && filename != nodePath['resolve'](filename)) {
      filename = path.join(__dirname, '..', 'src', filename);
      ret = nodeFS['readFileSync'](filename).toString();
    }
    return ret;
  };
  Module['load'] = function(f) {
    globalEval(read(f));
  };
  if (!Module['arguments']) {
    Module['arguments'] = process['argv'].slice(2);
  }
}
if (ENVIRONMENT_IS_SHELL) {
  Module['print'] = print;
  if (typeof printErr != 'undefined') Module['printErr'] = printErr; // not present in v8 or older sm
  // Polyfill over SpiderMonkey/V8 differences
  if (typeof read != 'undefined') {
    Module['read'] = read;
  } else {
    Module['read'] = function(f) { snarf(f) };
  }
  if (!Module['arguments']) {
    if (typeof scriptArgs != 'undefined') {
      Module['arguments'] = scriptArgs;
    } else if (typeof arguments != 'undefined') {
      Module['arguments'] = arguments;
    }
  }
}
if (ENVIRONMENT_IS_WEB && !ENVIRONMENT_IS_WORKER) {
  if (!Module['print']) {
    Module['print'] = function(x) {
      console.log(x);
    };
  }
  if (!Module['printErr']) {
    Module['printErr'] = function(x) {
      console.log(x);
    };
  }
}
if (ENVIRONMENT_IS_WEB || ENVIRONMENT_IS_WORKER) {
  Module['read'] = function(url) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, false);
    xhr.send(null);
    return xhr.responseText;
  };
  if (!Module['arguments']) {
    if (typeof arguments != 'undefined') {
      Module['arguments'] = arguments;
    }
  }
}
if (ENVIRONMENT_IS_WORKER) {
  // We can do very little here...
  var TRY_USE_DUMP = false;
  if (!Module['print']) {
    Module['print'] = (TRY_USE_DUMP && (typeof(dump) !== "undefined") ? (function(x) {
      dump(x);
    }) : (function(x) {
      // self.postMessage(x); // enable this if you want stdout to be sent as messages
    }));
  }
  Module['load'] = importScripts;
}
if (!ENVIRONMENT_IS_WORKER && !ENVIRONMENT_IS_WEB && !ENVIRONMENT_IS_NODE && !ENVIRONMENT_IS_SHELL) {
  // Unreachable because SHELL is dependant on the others
  throw 'Unknown runtime environment. Where are we?';
}
function globalEval(x) {
  eval.call(null, x);
}
if (!Module['load'] == 'undefined' && Module['read']) {
  Module['load'] = function(f) {
    globalEval(Module['read'](f));
  };
}
if (!Module['print']) {
  Module['print'] = function(){};
}
if (!Module['printErr']) {
  Module['printErr'] = Module['print'];
}
if (!Module['arguments']) {
  Module['arguments'] = [];
}
// *** Environment setup code ***
// Closure helpers
Module.print = Module['print'];
Module.printErr = Module['printErr'];
// Callbacks
if (!Module['preRun']) Module['preRun'] = [];
if (!Module['postRun']) Module['postRun'] = [];
// === Auto-generated preamble library stuff ===
//========================================
// Runtime code shared with compiler
//========================================
var Runtime = {
  stackSave: function () {
    return STACKTOP;
  },
  stackRestore: function (stackTop) {
    STACKTOP = stackTop;
  },
  forceAlign: function (target, quantum) {
    quantum = quantum || 4;
    if (quantum == 1) return target;
    if (isNumber(target) && isNumber(quantum)) {
      return Math.ceil(target/quantum)*quantum;
    } else if (isNumber(quantum) && isPowerOfTwo(quantum)) {
      var logg = log2(quantum);
      return '((((' +target + ')+' + (quantum-1) + ')>>' + logg + ')<<' + logg + ')';
    }
    return 'Math.ceil((' + target + ')/' + quantum + ')*' + quantum;
  },
  isNumberType: function (type) {
    return type in Runtime.INT_TYPES || type in Runtime.FLOAT_TYPES;
  },
  isPointerType: function isPointerType(type) {
  return type[type.length-1] == '*';
},
  isStructType: function isStructType(type) {
  if (isPointerType(type)) return false;
  if (/^\[\d+\ x\ (.*)\]/.test(type)) return true; // [15 x ?] blocks. Like structs
  if (/<?{ ?[^}]* ?}>?/.test(type)) return true; // { i32, i8 } etc. - anonymous struct types
  // See comment in isStructPointerType()
  return type[0] == '%';
},
  INT_TYPES: {"i1":0,"i8":0,"i16":0,"i32":0,"i64":0},
  FLOAT_TYPES: {"float":0,"double":0},
  or64: function (x, y) {
    var l = (x | 0) | (y | 0);
    var h = (Math.round(x / 4294967296) | Math.round(y / 4294967296)) * 4294967296;
    return l + h;
  },
  and64: function (x, y) {
    var l = (x | 0) & (y | 0);
    var h = (Math.round(x / 4294967296) & Math.round(y / 4294967296)) * 4294967296;
    return l + h;
  },
  xor64: function (x, y) {
    var l = (x | 0) ^ (y | 0);
    var h = (Math.round(x / 4294967296) ^ Math.round(y / 4294967296)) * 4294967296;
    return l + h;
  },
  getNativeTypeSize: function (type, quantumSize) {
    if (Runtime.QUANTUM_SIZE == 1) return 1;
    var size = {
      '%i1': 1,
      '%i8': 1,
      '%i16': 2,
      '%i32': 4,
      '%i64': 8,
      "%float": 4,
      "%double": 8
    }['%'+type]; // add '%' since float and double confuse Closure compiler as keys, and also spidermonkey as a compiler will remove 's from '_i8' etc
    if (!size) {
      if (type.charAt(type.length-1) == '*') {
        size = Runtime.QUANTUM_SIZE; // A pointer
      } else if (type[0] == 'i') {
        var bits = parseInt(type.substr(1));
        assert(bits % 8 == 0);
        size = bits/8;
      }
    }
    return size;
  },
  getNativeFieldSize: function (type) {
    return Math.max(Runtime.getNativeTypeSize(type), Runtime.QUANTUM_SIZE);
  },
  dedup: function dedup(items, ident) {
  var seen = {};
  if (ident) {
    return items.filter(function(item) {
      if (seen[item[ident]]) return false;
      seen[item[ident]] = true;
      return true;
    });
  } else {
    return items.filter(function(item) {
      if (seen[item]) return false;
      seen[item] = true;
      return true;
    });
  }
},
  set: function set() {
  var args = typeof arguments[0] === 'object' ? arguments[0] : arguments;
  var ret = {};
  for (var i = 0; i < args.length; i++) {
    ret[args[i]] = 0;
  }
  return ret;
},
  calculateStructAlignment: function calculateStructAlignment(type) {
    type.flatSize = 0;
    type.alignSize = 0;
    var diffs = [];
    var prev = -1;
    type.flatIndexes = type.fields.map(function(field) {
      var size, alignSize;
      if (Runtime.isNumberType(field) || Runtime.isPointerType(field)) {
        size = Runtime.getNativeTypeSize(field); // pack char; char; in structs, also char[X]s.
        alignSize = size;
      } else if (Runtime.isStructType(field)) {
        size = Types.types[field].flatSize;
        alignSize = Types.types[field].alignSize;
      } else if (field[0] == 'b') {
        // bN, large number field, like a [N x i8]
        size = field.substr(1)|0;
        alignSize = 1;
      } else {
        throw 'Unclear type in struct: ' + field + ', in ' + type.name_ + ' :: ' + dump(Types.types[type.name_]);
      }
      alignSize = type.packed ? 1 : Math.min(alignSize, Runtime.QUANTUM_SIZE);
      type.alignSize = Math.max(type.alignSize, alignSize);
      var curr = Runtime.alignMemory(type.flatSize, alignSize); // if necessary, place this on aligned memory
      type.flatSize = curr + size;
      if (prev >= 0) {
        diffs.push(curr-prev);
      }
      prev = curr;
      return curr;
    });
    type.flatSize = Runtime.alignMemory(type.flatSize, type.alignSize);
    if (diffs.length == 0) {
      type.flatFactor = type.flatSize;
    } else if (Runtime.dedup(diffs).length == 1) {
      type.flatFactor = diffs[0];
    }
    type.needsFlattening = (type.flatFactor != 1);
    return type.flatIndexes;
  },
  generateStructInfo: function (struct, typeName, offset) {
    var type, alignment;
    if (typeName) {
      offset = offset || 0;
      type = (typeof Types === 'undefined' ? Runtime.typeInfo : Types.types)[typeName];
      if (!type) return null;
      if (type.fields.length != struct.length) {
        printErr('Number of named fields must match the type for ' + typeName + ': possibly duplicate struct names. Cannot return structInfo');
        return null;
      }
      alignment = type.flatIndexes;
    } else {
      var type = { fields: struct.map(function(item) { return item[0] }) };
      alignment = Runtime.calculateStructAlignment(type);
    }
    var ret = {
      __size__: type.flatSize
    };
    if (typeName) {
      struct.forEach(function(item, i) {
        if (typeof item === 'string') {
          ret[item] = alignment[i] + offset;
        } else {
          // embedded struct
          var key;
          for (var k in item) key = k;
          ret[key] = Runtime.generateStructInfo(item[key], type.fields[i], alignment[i]);
        }
      });
    } else {
      struct.forEach(function(item, i) {
        ret[item[1]] = alignment[i];
      });
    }
    return ret;
  },
  dynCall: function (sig, ptr, args) {
    if (args && args.length) {
      return FUNCTION_TABLE[ptr].apply(null, args);
    } else {
      return FUNCTION_TABLE[ptr]();
    }
  },
  addFunction: function (func, sig) {
    //assert(sig); // TODO: support asm
    var table = FUNCTION_TABLE; // TODO: support asm
    var ret = table.length;
    table.push(func);
    table.push(0);
    return ret;
  },
  removeFunction: function (index) {
    var table = FUNCTION_TABLE; // TODO: support asm
    table[index] = null;
  },
  warnOnce: function (text) {
    if (!Runtime.warnOnce.shown) Runtime.warnOnce.shown = {};
    if (!Runtime.warnOnce.shown[text]) {
      Runtime.warnOnce.shown[text] = 1;
      Module.printErr(text);
    }
  },
  funcWrappers: {},
  getFuncWrapper: function (func, sig) {
    assert(sig);
    if (!Runtime.funcWrappers[func]) {
      Runtime.funcWrappers[func] = function() {
        Runtime.dynCall(sig, func, arguments);
      };
    }
    return Runtime.funcWrappers[func];
  },
  UTF8Processor: function () {
    var buffer = [];
    var needed = 0;
    this.processCChar = function (code) {
      code = code & 0xff;
      if (needed) {
        buffer.push(code);
        needed--;
      }
      if (buffer.length == 0) {
        if (code < 128) return String.fromCharCode(code);
        buffer.push(code);
        if (code > 191 && code < 224) {
          needed = 1;
        } else {
          needed = 2;
        }
        return '';
      }
      if (needed > 0) return '';
      var c1 = buffer[0];
      var c2 = buffer[1];
      var c3 = buffer[2];
      var ret;
      if (c1 > 191 && c1 < 224) {
        ret = String.fromCharCode(((c1 & 31) << 6) | (c2 & 63));
      } else {
        ret = String.fromCharCode(((c1 & 15) << 12) | ((c2 & 63) << 6) | (c3 & 63));
      }
      buffer.length = 0;
      return ret;
    }
    this.processJSString = function(string) {
      string = unescape(encodeURIComponent(string));
      var ret = [];
      for (var i = 0; i < string.length; i++) {
        ret.push(string.charCodeAt(i));
      }
      return ret;
    }
  },
  stackAlloc: function (size) { var ret = STACKTOP;STACKTOP = (STACKTOP + size)|0;STACKTOP = ((((STACKTOP)+3)>>2)<<2); return ret; },
  staticAlloc: function (size) { var ret = STATICTOP;STATICTOP = (STATICTOP + size)|0;STATICTOP = ((((STATICTOP)+3)>>2)<<2); if (STATICTOP >= TOTAL_MEMORY) enlargeMemory();; return ret; },
  alignMemory: function (size,quantum) { var ret = size = Math.ceil((size)/(quantum ? quantum : 4))*(quantum ? quantum : 4); return ret; },
  makeBigInt: function (low,high,unsigned) { var ret = (unsigned ? (((low)>>>(0))+(((high)>>>(0))*4294967296)) : (((low)>>>(0))+(((high)|(0))*4294967296))); return ret; },
  QUANTUM_SIZE: 4,
  __dummy__: 0
}
//========================================
// Runtime essentials
//========================================
var __THREW__ = 0; // Used in checking for thrown exceptions.
var setjmpId = 1; // Used in setjmp/longjmp
var setjmpLabels = {};
var ABORT = false;
var undef = 0;
// tempInt is used for 32-bit signed values or smaller. tempBigInt is used
// for 32-bit unsigned values or more than 32 bits. TODO: audit all uses of tempInt
var tempValue, tempInt, tempBigInt, tempInt2, tempBigInt2, tempPair, tempBigIntI, tempBigIntR, tempBigIntS, tempBigIntP, tempBigIntD;
var tempI64, tempI64b;
var tempRet0, tempRet1, tempRet2, tempRet3, tempRet4, tempRet5, tempRet6, tempRet7, tempRet8, tempRet9;
function abort(text) {
  Module.print(text + ':\n' + (new Error).stack);
  ABORT = true;
  throw "Assertion: " + text;
}
function assert(condition, text) {
  if (!condition) {
    abort('Assertion failed: ' + text);
  }
}
var globalScope = this;
// C calling interface. A convenient way to call C functions (in C files, or
// defined with extern "C").
//
// Note: LLVM optimizations can inline and remove functions, after which you will not be
//       able to call them. Closure can also do so. To avoid that, add your function to
//       the exports using something like
//
//         -s EXPORTED_FUNCTIONS='["_main", "_myfunc"]'
//
// @param ident      The name of the C function (note that C++ functions will be name-mangled - use extern "C")
// @param returnType The return type of the function, one of the JS types 'number', 'string' or 'array' (use 'number' for any C pointer, and
//                   'array' for JavaScript arrays and typed arrays).
// @param argTypes   An array of the types of arguments for the function (if there are no arguments, this can be ommitted). Types are as in returnType,
//                   except that 'array' is not possible (there is no way for us to know the length of the array)
// @param args       An array of the arguments to the function, as native JS values (as in returnType)
//                   Note that string arguments will be stored on the stack (the JS string will become a C string on the stack).
// @return           The return value, as a native JS value (as in returnType)
function ccall(ident, returnType, argTypes, args) {
  return ccallFunc(getCFunc(ident), returnType, argTypes, args);
}
Module["ccall"] = ccall;
// Returns the C function with a specified identifier (for C++, you need to do manual name mangling)
function getCFunc(ident) {
  try {
    var func = globalScope['Module']['_' + ident]; // closure exported function
    if (!func) func = eval('_' + ident); // explicit lookup
  } catch(e) {
  }
  assert(func, 'Cannot call unknown function ' + ident + ' (perhaps LLVM optimizations or closure removed it?)');
  return func;
}
// Internal function that does a C call using a function, not an identifier
function ccallFunc(func, returnType, argTypes, args) {
  var stack = 0;
  function toC(value, type) {
    if (type == 'string') {
      if (value === null || value === undefined || value === 0) return 0; // null string
      if (!stack) stack = Runtime.stackSave();
      var ret = Runtime.stackAlloc(value.length+1);
      writeStringToMemory(value, ret);
      return ret;
    } else if (type == 'array') {
      if (!stack) stack = Runtime.stackSave();
      var ret = Runtime.stackAlloc(value.length);
      writeArrayToMemory(value, ret);
      return ret;
    }
    return value;
  }
  function fromC(value, type) {
    if (type == 'string') {
      return Pointer_stringify(value);
    }
    assert(type != 'array');
    return value;
  }
  var i = 0;
  var cArgs = args ? args.map(function(arg) {
    return toC(arg, argTypes[i++]);
  }) : [];
  var ret = fromC(func.apply(null, cArgs), returnType);
  if (stack) Runtime.stackRestore(stack);
  return ret;
}
// Returns a native JS wrapper for a C function. This is similar to ccall, but
// returns a function you can call repeatedly in a normal way. For example:
//
//   var my_function = cwrap('my_c_function', 'number', ['number', 'number']);
//   alert(my_function(5, 22));
//   alert(my_function(99, 12));
//
function cwrap(ident, returnType, argTypes) {
  var func = getCFunc(ident);
  return function() {
    return ccallFunc(func, returnType, argTypes, Array.prototype.slice.call(arguments));
  }
}
Module["cwrap"] = cwrap;
// Sets a value in memory in a dynamic way at run-time. Uses the
// type data. This is the same as makeSetValue, except that
// makeSetValue is done at compile-time and generates the needed
// code then, whereas this function picks the right code at
// run-time.
// Note that setValue and getValue only do *aligned* writes and reads!
// Note that ccall uses JS types as for defining types, while setValue and
// getValue need LLVM types ('i8', 'i32') - this is a lower-level operation
function setValue(ptr, value, type, noSafe) {
  type = type || 'i8';
  if (type.charAt(type.length-1) === '*') type = 'i32'; // pointers are 32-bit
    switch(type) {
      case 'i1': HEAP8[(ptr)]=value; break;
      case 'i8': HEAP8[(ptr)]=value; break;
      case 'i16': HEAP16[((ptr)>>1)]=value; break;
      case 'i32': HEAP32[((ptr)>>2)]=value; break;
      case 'i64': (tempI64 = [value>>>0,Math.min(Math.floor((value)/4294967296), 4294967295)>>>0],HEAP32[((ptr)>>2)]=tempI64[0],HEAP32[(((ptr)+(4))>>2)]=tempI64[1]); break;
      case 'float': HEAPF32[((ptr)>>2)]=value; break;
      case 'double': (HEAPF64[(tempDoublePtr)>>3]=value,HEAP32[((ptr)>>2)]=HEAP32[((tempDoublePtr)>>2)],HEAP32[(((ptr)+(4))>>2)]=HEAP32[(((tempDoublePtr)+(4))>>2)]); break;
      default: abort('invalid type for setValue: ' + type);
    }
}
Module['setValue'] = setValue;
// Parallel to setValue.
function getValue(ptr, type, noSafe) {
  type = type || 'i8';
  if (type.charAt(type.length-1) === '*') type = 'i32'; // pointers are 32-bit
    switch(type) {
      case 'i1': return HEAP8[(ptr)];
      case 'i8': return HEAP8[(ptr)];
      case 'i16': return HEAP16[((ptr)>>1)];
      case 'i32': return HEAP32[((ptr)>>2)];
      case 'i64': return HEAP32[((ptr)>>2)];
      case 'float': return HEAPF32[((ptr)>>2)];
      case 'double': return (HEAP32[((tempDoublePtr)>>2)]=HEAP32[((ptr)>>2)],HEAP32[(((tempDoublePtr)+(4))>>2)]=HEAP32[(((ptr)+(4))>>2)],HEAPF64[(tempDoublePtr)>>3]);
      default: abort('invalid type for setValue: ' + type);
    }
  return null;
}
Module['getValue'] = getValue;
var ALLOC_NORMAL = 0; // Tries to use _malloc()
var ALLOC_STACK = 1; // Lives for the duration of the current function call
var ALLOC_STATIC = 2; // Cannot be freed
var ALLOC_NONE = 3; // Do not allocate
Module['ALLOC_NORMAL'] = ALLOC_NORMAL;
Module['ALLOC_STACK'] = ALLOC_STACK;
Module['ALLOC_STATIC'] = ALLOC_STATIC;
Module['ALLOC_NONE'] = ALLOC_NONE;
// allocate(): This is for internal use. You can use it yourself as well, but the interface
//             is a little tricky (see docs right below). The reason is that it is optimized
//             for multiple syntaxes to save space in generated code. So you should
//             normally not use allocate(), and instead allocate memory using _malloc(),
//             initialize it with setValue(), and so forth.
// @slab: An array of data, or a number. If a number, then the size of the block to allocate,
//        in *bytes* (note that this is sometimes confusing: the next parameter does not
//        affect this!)
// @types: Either an array of types, one for each byte (or 0 if no type at that position),
//         or a single type which is used for the entire block. This only matters if there
//         is initial data - if @slab is a number, then this does not matter at all and is
//         ignored.
// @allocator: How to allocate memory, see ALLOC_*
function allocate(slab, types, allocator, ptr) {
  var zeroinit, size;
  if (typeof slab === 'number') {
    zeroinit = true;
    size = slab;
  } else {
    zeroinit = false;
    size = slab.length;
  }
  var singleType = typeof types === 'string' ? types : null;
  var ret;
  if (allocator == ALLOC_NONE) {
    ret = ptr;
  } else {
    ret = [_malloc, Runtime.stackAlloc, Runtime.staticAlloc][allocator === undefined ? ALLOC_STATIC : allocator](Math.max(size, singleType ? 1 : types.length));
  }
  if (zeroinit) {
    var ptr = ret, stop;
    assert((ret & 3) == 0);
    stop = ret + (size & ~3);
    for (; ptr < stop; ptr += 4) {
      HEAP32[((ptr)>>2)]=0;
    }
    stop = ret + size;
    while (ptr < stop) {
      HEAP8[((ptr++)|0)]=0;
    }
    return ret;
  }
  if (singleType === 'i8') {
    HEAPU8.set(new Uint8Array(slab), ret);
    return ret;
  }
  var i = 0, type, typeSize, previousType;
  while (i < size) {
    var curr = slab[i];
    if (typeof curr === 'function') {
      curr = Runtime.getFunctionIndex(curr);
    }
    type = singleType || types[i];
    if (type === 0) {
      i++;
      continue;
    }
    if (type == 'i64') type = 'i32'; // special case: we have one i32 here, and one i32 later
    setValue(ret+i, curr, type);
    // no need to look up size unless type changes, so cache it
    if (previousType !== type) {
      typeSize = Runtime.getNativeTypeSize(type);
      previousType = type;
    }
    i += typeSize;
  }
  return ret;
}
Module['allocate'] = allocate;
function Pointer_stringify(ptr, /* optional */ length) {
  // Find the length, and check for UTF while doing so
  var hasUtf = false;
  var t;
  var i = 0;
  while (1) {
    t = HEAPU8[(((ptr)+(i))|0)];
    if (t >= 128) hasUtf = true;
    else if (t == 0 && !length) break;
    i++;
    if (length && i == length) break;
  }
  if (!length) length = i;
  var ret = '';
  if (!hasUtf) {
    var MAX_CHUNK = 1024; // split up into chunks, because .apply on a huge string can overflow the stack
    var curr;
    while (length > 0) {
      curr = String.fromCharCode.apply(String, HEAPU8.subarray(ptr, ptr + Math.min(length, MAX_CHUNK)));
      ret = ret ? ret + curr : curr;
      ptr += MAX_CHUNK;
      length -= MAX_CHUNK;
    }
    return ret;
  }
  var utf8 = new Runtime.UTF8Processor();
  for (i = 0; i < length; i++) {
    t = HEAPU8[(((ptr)+(i))|0)];
    ret += utf8.processCChar(t);
  }
  return ret;
}
Module['Pointer_stringify'] = Pointer_stringify;
// Memory management
var PAGE_SIZE = 4096;
function alignMemoryPage(x) {
  return ((x+4095)>>12)<<12;
}
var HEAP;
var HEAP8, HEAPU8, HEAP16, HEAPU16, HEAP32, HEAPU32, HEAPF32, HEAPF64;
var STACK_ROOT, STACKTOP, STACK_MAX;
var STATICTOP;
function enlargeMemory() {
  abort('Cannot enlarge memory arrays. Either (1) compile with -s TOTAL_MEMORY=X with X higher than the current value, (2) compile with ALLOW_MEMORY_GROWTH which adjusts the size at runtime but prevents some optimizations, or (3) set Module.TOTAL_MEMORY before the program runs.');
}
var TOTAL_STACK = Module['TOTAL_STACK'] || 5242880;
var TOTAL_MEMORY = Module['TOTAL_MEMORY'] || 16777216;
var FAST_MEMORY = Module['FAST_MEMORY'] || 2097152;
// Initialize the runtime's memory
// check for full engine support (use string 'subarray' to avoid closure compiler confusion)
assert(!!Int32Array && !!Float64Array && !!(new Int32Array(1)['subarray']) && !!(new Int32Array(1)['set']),
       'Cannot fallback to non-typed array case: Code is too specialized');
var buffer = new ArrayBuffer(TOTAL_MEMORY);
HEAP8 = new Int8Array(buffer);
HEAP16 = new Int16Array(buffer);
HEAP32 = new Int32Array(buffer);
HEAPU8 = new Uint8Array(buffer);
HEAPU16 = new Uint16Array(buffer);
HEAPU32 = new Uint32Array(buffer);
HEAPF32 = new Float32Array(buffer);
HEAPF64 = new Float64Array(buffer);
// Endianness check (note: assumes compiler arch was little-endian)
HEAP32[0] = 255;
assert(HEAPU8[0] === 255 && HEAPU8[3] === 0, 'Typed arrays 2 must be run on a little-endian system');
Module['HEAP'] = HEAP;
Module['HEAP8'] = HEAP8;
Module['HEAP16'] = HEAP16;
Module['HEAP32'] = HEAP32;
Module['HEAPU8'] = HEAPU8;
Module['HEAPU16'] = HEAPU16;
Module['HEAPU32'] = HEAPU32;
Module['HEAPF32'] = HEAPF32;
Module['HEAPF64'] = HEAPF64;
STACK_ROOT = STACKTOP = Runtime.alignMemory(1);
STACK_MAX = TOTAL_STACK; // we lose a little stack here, but TOTAL_STACK is nice and round so use that as the max
var tempDoublePtr = Runtime.alignMemory(allocate(12, 'i8', ALLOC_STACK), 8);
assert(tempDoublePtr % 8 == 0);
function copyTempFloat(ptr) { // functions, because inlining this code increases code size too much
  HEAP8[tempDoublePtr] = HEAP8[ptr];
  HEAP8[tempDoublePtr+1] = HEAP8[ptr+1];
  HEAP8[tempDoublePtr+2] = HEAP8[ptr+2];
  HEAP8[tempDoublePtr+3] = HEAP8[ptr+3];
}
function copyTempDouble(ptr) {
  HEAP8[tempDoublePtr] = HEAP8[ptr];
  HEAP8[tempDoublePtr+1] = HEAP8[ptr+1];
  HEAP8[tempDoublePtr+2] = HEAP8[ptr+2];
  HEAP8[tempDoublePtr+3] = HEAP8[ptr+3];
  HEAP8[tempDoublePtr+4] = HEAP8[ptr+4];
  HEAP8[tempDoublePtr+5] = HEAP8[ptr+5];
  HEAP8[tempDoublePtr+6] = HEAP8[ptr+6];
  HEAP8[tempDoublePtr+7] = HEAP8[ptr+7];
}
STATICTOP = STACK_MAX;
assert(STATICTOP < TOTAL_MEMORY); // Stack must fit in TOTAL_MEMORY; allocations from here on may enlarge TOTAL_MEMORY
var nullString = allocate(intArrayFromString('(null)'), 'i8', ALLOC_STACK);
function callRuntimeCallbacks(callbacks) {
  while(callbacks.length > 0) {
    var callback = callbacks.shift();
    var func = callback.func;
    if (typeof func === 'number') {
      if (callback.arg === undefined) {
        Runtime.dynCall('v', func);
      } else {
        Runtime.dynCall('vi', func, [callback.arg]);
      }
    } else {
      func(callback.arg === undefined ? null : callback.arg);
    }
  }
}
var __ATINIT__ = []; // functions called during startup
var __ATMAIN__ = []; // functions called when main() is to be run
var __ATEXIT__ = []; // functions called during shutdown
function initRuntime() {
  callRuntimeCallbacks(__ATINIT__);
}
function preMain() {
  callRuntimeCallbacks(__ATMAIN__);
}
function exitRuntime() {
  callRuntimeCallbacks(__ATEXIT__);
}
// Tools
// This processes a JS string into a C-line array of numbers, 0-terminated.
// For LLVM-originating strings, see parser.js:parseLLVMString function
function intArrayFromString(stringy, dontAddNull, length /* optional */) {
  var ret = (new Runtime.UTF8Processor()).processJSString(stringy);
  if (length) {
    ret.length = length;
  }
  if (!dontAddNull) {
    ret.push(0);
  }
  return ret;
}
Module['intArrayFromString'] = intArrayFromString;
function intArrayToString(array) {
  var ret = [];
  for (var i = 0; i < array.length; i++) {
    var chr = array[i];
    if (chr > 0xFF) {
      chr &= 0xFF;
    }
    ret.push(String.fromCharCode(chr));
  }
  return ret.join('');
}
Module['intArrayToString'] = intArrayToString;
// Write a Javascript array to somewhere in the heap
function writeStringToMemory(string, buffer, dontAddNull) {
  var array = intArrayFromString(string, dontAddNull);
  var i = 0;
  while (i < array.length) {
    var chr = array[i];
    HEAP8[(((buffer)+(i))|0)]=chr
    i = i + 1;
  }
}
Module['writeStringToMemory'] = writeStringToMemory;
function writeArrayToMemory(array, buffer) {
  for (var i = 0; i < array.length; i++) {
    HEAP8[(((buffer)+(i))|0)]=array[i];
  }
}
Module['writeArrayToMemory'] = writeArrayToMemory;
function unSign(value, bits, ignore, sig) {
  if (value >= 0) {
    return value;
  }
  return bits <= 32 ? 2*Math.abs(1 << (bits-1)) + value // Need some trickery, since if bits == 32, we are right at the limit of the bits JS uses in bitshifts
                    : Math.pow(2, bits)         + value;
}
function reSign(value, bits, ignore, sig) {
  if (value <= 0) {
    return value;
  }
  var half = bits <= 32 ? Math.abs(1 << (bits-1)) // abs is needed if bits == 32
                        : Math.pow(2, bits-1);
  if (value >= half && (bits <= 32 || value > half)) { // for huge values, we can hit the precision limit and always get true here. so don't do that
                                                       // but, in general there is no perfect solution here. With 64-bit ints, we get rounding and errors
                                                       // TODO: In i64 mode 1, resign the two parts separately and safely
    value = -2*half + value; // Cannot bitshift half, as it may be at the limit of the bits JS uses in bitshifts
  }
  return value;
}
if (!Math.imul) Math.imul = function(a, b) {
  var ah  = a >>> 16;
  var al = a & 0xffff;
  var bh  = b >>> 16;
  var bl = b & 0xffff;
  return (al*bl + ((ah*bl + al*bh) << 16))|0;
};
// A counter of dependencies for calling run(). If we need to
// do asynchronous work before running, increment this and
// decrement it. Incrementing must happen in a place like
// PRE_RUN_ADDITIONS (used by emcc to add file preloading).
// Note that you can add dependencies in preRun, even though
// it happens right before run - run will be postponed until
// the dependencies are met.
var runDependencies = 0;
var runDependencyTracking = {};
var calledRun = false;
var runDependencyWatcher = null;
function addRunDependency(id) {
  runDependencies++;
  if (Module['monitorRunDependencies']) {
    Module['monitorRunDependencies'](runDependencies);
  }
  if (id) {
    assert(!runDependencyTracking[id]);
    runDependencyTracking[id] = 1;
    if (runDependencyWatcher === null && typeof setInterval !== 'undefined') {
      // Check for missing dependencies every few seconds
      runDependencyWatcher = setInterval(function() {
        var shown = false;
        for (var dep in runDependencyTracking) {
          if (!shown) {
            shown = true;
            Module.printErr('still waiting on run dependencies:');
          }
          Module.printErr('dependency: ' + dep);
        }
        if (shown) {
          Module.printErr('(end of list)');
        }
      }, 6000);
    }
  } else {
    Module.printErr('warning: run dependency added without ID');
  }
}
Module['addRunDependency'] = addRunDependency;
function removeRunDependency(id) {
  runDependencies--;
  if (Module['monitorRunDependencies']) {
    Module['monitorRunDependencies'](runDependencies);
  }
  if (id) {
    assert(runDependencyTracking[id]);
    delete runDependencyTracking[id];
  } else {
    Module.printErr('warning: run dependency removed without ID');
  }
  if (runDependencies == 0) {
    if (runDependencyWatcher !== null) {
      clearInterval(runDependencyWatcher);
      runDependencyWatcher = null;
    } 
    // If run has never been called, and we should call run (INVOKE_RUN is true, and Module.noInitialRun is not false)
    if (!calledRun && shouldRunNow) run();
  }
}
Module['removeRunDependency'] = removeRunDependency;
Module["preloadedImages"] = {}; // maps url to image data
Module["preloadedAudios"] = {}; // maps url to audio data
// === Body ===
assert(STATICTOP == STACK_MAX); assert(STACK_MAX == TOTAL_STACK);
STATICTOP += 12892;
assert(STATICTOP < TOTAL_MEMORY);
var _stdout;
var _stdin;
var _stderr;
__ATINIT__ = __ATINIT__.concat([
  { func: function() { __GLOBAL__I_a() } },
  { func: function() { __GLOBAL__I_a263() } }
]);
var ___dso_handle;
var __ZTVN10__cxxabiv120__si_class_type_infoE;
var __ZTVN10__cxxabiv117__class_type_infoE;
var __ZTISt9exception;
allocate([0,0,0,0,0,0,36,64,0,0,0,0,0,0,89,64,0,0,0,0,0,136,195,64,0,0,0,0,132,215,151,65,0,128,224,55,121,195,65,67,23,110,5,181,181,184,147,70,245,249,63,233,3,79,56,77,50,29,48,249,72,119,130,90,60,191,115,127,221,79,21,117], "i8", ALLOC_NONE, 5242880);
allocate(24, "i8", ALLOC_NONE, 5242952);
allocate([65,0,0,0,112,0,0,0,114,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5242976);
allocate([77,0,0,0,97,0,0,0,114,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5242992);
allocate([70,0,0,0,101,0,0,0,98,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243008);
allocate([74,0,0,0,97,0,0,0,110,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243024);
allocate([68,0,0,0,101,0,0,0,99,0,0,0,101,0,0,0,109,0,0,0,98,0,0,0,101,0,0,0,114,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243040);
allocate([78,0,0,0,111,0,0,0,118,0,0,0,101,0,0,0,109,0,0,0,98,0,0,0,101,0,0,0,114,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243076);
allocate([79,0,0,0,99,0,0,0,116,0,0,0,111,0,0,0,98,0,0,0,101,0,0,0,114,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243112);
allocate([117,110,115,117,112,112,111,114,116,101,100,32,108,111,99,97,108,101,32,102,111,114,32,115,116,97,110,100,97,114,100,32,105,110,112,117,116,0] /* unsupported locale f */, "i8", ALLOC_NONE, 5243144);
allocate([83,0,0,0,101,0,0,0,112,0,0,0,116,0,0,0,101,0,0,0,109,0,0,0,98,0,0,0,101,0,0,0,114,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243184);
allocate([65,0,0,0,117,0,0,0,103,0,0,0,117,0,0,0,115,0,0,0,116,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243224);
allocate([74,0,0,0,117,0,0,0,108,0,0,0,121,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243252);
allocate([74,0,0,0,117,0,0,0,110,0,0,0,101,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243272);
allocate([65,0,0,0,112,0,0,0,114,0,0,0,105,0,0,0,108,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243292);
allocate([77,0,0,0,97,0,0,0,114,0,0,0,99,0,0,0,104,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243316);
allocate([70,0,0,0,101,0,0,0,98,0,0,0,114,0,0,0,117,0,0,0,97,0,0,0,114,0,0,0,121,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243340);
allocate([74,0,0,0,97,0,0,0,110,0,0,0,117,0,0,0,97,0,0,0,114,0,0,0,121,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243376);
allocate([80,77,0] /* PM\00 */, "i8", ALLOC_NONE, 5243408);
allocate([65,77,0] /* AM\00 */, "i8", ALLOC_NONE, 5243412);
allocate([80,0,0,0,77,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243416);
allocate([65,0,0,0,77,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243428);
allocate([118,101,99,116,111,114,0] /* vector\00 */, "i8", ALLOC_NONE, 5243440);
allocate([37,46,48,76,102,0] /* %.0Lf\00 */, "i8", ALLOC_NONE, 5243448);
allocate([109,111,110,101,121,95,103,101,116,32,101,114,114,111,114,0] /* money_get error\00 */, "i8", ALLOC_NONE, 5243456);
allocate([37,76,102,0] /* %Lf\00 */, "i8", ALLOC_NONE, 5243472);
allocate([37,112,0] /* %p\00 */, "i8", ALLOC_NONE, 5243476);
allocate([108,111,99,97,108,101,32,110,111,116,32,115,117,112,112,111,114,116,101,100,0] /* locale not supported */, "i8", ALLOC_NONE, 5243480);
allocate([37,0,0,0,73,0,0,0,58,0,0,0,37,0,0,0,77,0,0,0,58,0,0,0,37,0,0,0,83,0,0,0,32,0,0,0,37,0,0,0,112,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243504);
allocate([37,73,58,37,77,58,37,83,32,37,112,0] /* %I:%M:%S %p\00 */, "i8", ALLOC_NONE, 5243552);
allocate([37,0,0,0,97,0,0,0,32,0,0,0,37,0,0,0,98,0,0,0,32,0,0,0,37,0,0,0,100,0,0,0,32,0,0,0,37,0,0,0,72,0,0,0,58,0,0,0,37,0,0,0,77,0,0,0,58,0,0,0,37,0,0,0,83,0,0,0,32,0,0,0,37,0,0,0,89,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243564);
allocate([115,116,100,58,58,98,97,100,95,99,97,115,116,0] /* std::bad_cast\00 */, "i8", ALLOC_NONE, 5243648);
allocate([37,97,32,37,98,32,37,100,32,37,72,58,37,77,58,37,83,32,37,89,0] /* %a %b %d %H:%M:%S %Y */, "i8", ALLOC_NONE, 5243664);
allocate([37,0,0,0,72,0,0,0,58,0,0,0,37,0,0,0,77,0,0,0,58,0,0,0,37,0,0,0,83,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243688);
allocate([37,0,0,0,109,0,0,0,47,0,0,0,37,0,0,0,100,0,0,0,47,0,0,0,37,0,0,0,121,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243724);
allocate([102,0,0,0,97,0,0,0,108,0,0,0,115,0,0,0,101,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243760);
allocate([102,97,108,115,101,0] /* false\00 */, "i8", ALLOC_NONE, 5243784);
allocate([116,0,0,0,114,0,0,0,117,0,0,0,101,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243792);
allocate([115,116,100,58,58,98,97,100,95,97,108,108,111,99,0] /* std::bad_alloc\00 */, "i8", ALLOC_NONE, 5243812);
allocate([98,97,115,105,99,95,115,116,114,105,110,103,0] /* basic_string\00 */, "i8", ALLOC_NONE, 5243828);
allocate([105,111,115,95,98,97,115,101,58,58,99,108,101,97,114,0] /* ios_base::clear\00 */, "i8", ALLOC_NONE, 5243844);
allocate([67,0] /* C\00 */, "i8", ALLOC_NONE, 5243860);
allocate([83,97,116,0] /* Sat\00 */, "i8", ALLOC_NONE, 5243864);
allocate([70,114,105,0] /* Fri\00 */, "i8", ALLOC_NONE, 5243868);
allocate([84,104,117,0] /* Thu\00 */, "i8", ALLOC_NONE, 5243872);
allocate([87,101,100,0] /* Wed\00 */, "i8", ALLOC_NONE, 5243876);
allocate([84,117,101,0] /* Tue\00 */, "i8", ALLOC_NONE, 5243880);
allocate([77,111,110,0] /* Mon\00 */, "i8", ALLOC_NONE, 5243884);
allocate([83,117,110,0] /* Sun\00 */, "i8", ALLOC_NONE, 5243888);
allocate([83,97,116,117,114,100,97,121,0] /* Saturday\00 */, "i8", ALLOC_NONE, 5243892);
allocate([70,114,105,100,97,121,0] /* Friday\00 */, "i8", ALLOC_NONE, 5243904);
allocate([84,104,117,114,115,100,97,121,0] /* Thursday\00 */, "i8", ALLOC_NONE, 5243912);
allocate([87,101,100,110,101,115,100,97,121,0] /* Wednesday\00 */, "i8", ALLOC_NONE, 5243924);
allocate([84,117,101,115,100,97,121,0] /* Tuesday\00 */, "i8", ALLOC_NONE, 5243936);
allocate([77,111,110,100,97,121,0] /* Monday\00 */, "i8", ALLOC_NONE, 5243944);
allocate([83,117,110,100,97,121,0] /* Sunday\00 */, "i8", ALLOC_NONE, 5243952);
allocate([83,0,0,0,97,0,0,0,116,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243960);
allocate([70,0,0,0,114,0,0,0,105,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243976);
allocate([84,0,0,0,104,0,0,0,117,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5243992);
allocate([87,0,0,0,101,0,0,0,100,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244008);
allocate([84,0,0,0,117,0,0,0,101,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244024);
allocate([77,0,0,0,111,0,0,0,110,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244040);
allocate([83,0,0,0,117,0,0,0,110,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244056);
allocate([83,0,0,0,97,0,0,0,116,0,0,0,117,0,0,0,114,0,0,0,100,0,0,0,97,0,0,0,121,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244072);
allocate([70,0,0,0,114,0,0,0,105,0,0,0,100,0,0,0,97,0,0,0,121,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244108);
allocate([84,0,0,0,104,0,0,0,117,0,0,0,114,0,0,0,115,0,0,0,100,0,0,0,97,0,0,0,121,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244136);
allocate([87,0,0,0,101,0,0,0,100,0,0,0,110,0,0,0,101,0,0,0,115,0,0,0,100,0,0,0,97,0,0,0,121,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244172);
allocate([105,111,115,116,114,101,97,109,0] /* iostream\00 */, "i8", ALLOC_NONE, 5244212);
allocate([84,0,0,0,117,0,0,0,101,0,0,0,115,0,0,0,100,0,0,0,97,0,0,0,121,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244224);
allocate([77,0,0,0,111,0,0,0,110,0,0,0,100,0,0,0,97,0,0,0,121,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244256);
allocate([83,0,0,0,117,0,0,0,110,0,0,0,100,0,0,0,97,0,0,0,121,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244284);
allocate([68,101,99,0] /* Dec\00 */, "i8", ALLOC_NONE, 5244312);
allocate([78,111,118,0] /* Nov\00 */, "i8", ALLOC_NONE, 5244316);
allocate([79,99,116,0] /* Oct\00 */, "i8", ALLOC_NONE, 5244320);
allocate([83,101,112,0] /* Sep\00 */, "i8", ALLOC_NONE, 5244324);
allocate([65,117,103,0] /* Aug\00 */, "i8", ALLOC_NONE, 5244328);
allocate([74,117,108,0] /* Jul\00 */, "i8", ALLOC_NONE, 5244332);
allocate([74,117,110,0] /* Jun\00 */, "i8", ALLOC_NONE, 5244336);
allocate([65,112,114,0] /* Apr\00 */, "i8", ALLOC_NONE, 5244340);
allocate([77,97,114,0] /* Mar\00 */, "i8", ALLOC_NONE, 5244344);
allocate([70,101,98,0] /* Feb\00 */, "i8", ALLOC_NONE, 5244348);
allocate([74,97,110,0] /* Jan\00 */, "i8", ALLOC_NONE, 5244352);
allocate([58,32,0] /* : \00 */, "i8", ALLOC_NONE, 5244356);
allocate([68,101,99,101,109,98,101,114,0] /* December\00 */, "i8", ALLOC_NONE, 5244360);
allocate([78,111,118,101,109,98,101,114,0] /* November\00 */, "i8", ALLOC_NONE, 5244372);
allocate([79,99,116,111,98,101,114,0] /* October\00 */, "i8", ALLOC_NONE, 5244384);
allocate([83,101,112,116,101,109,98,101,114,0] /* September\00 */, "i8", ALLOC_NONE, 5244392);
allocate([65,117,103,117,115,116,0] /* August\00 */, "i8", ALLOC_NONE, 5244404);
allocate([74,117,108,121,0] /* July\00 */, "i8", ALLOC_NONE, 5244412);
allocate([74,117,110,101,0] /* June\00 */, "i8", ALLOC_NONE, 5244420);
allocate([117,110,115,112,101,99,105,102,105,101,100,32,105,111,115,116,114,101,97,109,95,99,97,116,101,103,111,114,121,32,101,114,114,111,114,0] /* unspecified iostream */, "i8", ALLOC_NONE, 5244428);
allocate([77,97,121,0] /* May\00 */, "i8", ALLOC_NONE, 5244464);
allocate([65,112,114,105,108,0] /* April\00 */, "i8", ALLOC_NONE, 5244468);
allocate([77,97,114,99,104,0] /* March\00 */, "i8", ALLOC_NONE, 5244476);
allocate([70,101,98,114,117,97,114,121,0] /* February\00 */, "i8", ALLOC_NONE, 5244484);
allocate([74,97,110,117,97,114,121,0] /* January\00 */, "i8", ALLOC_NONE, 5244496);
allocate([68,0,0,0,101,0,0,0,99,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244504);
allocate([78,0,0,0,111,0,0,0,118,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244520);
allocate([79,0,0,0,99,0,0,0,116,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244536);
allocate([83,0,0,0,101,0,0,0,112,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244552);
allocate([65,0,0,0,117,0,0,0,103,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244568);
allocate([74,0,0,0,117,0,0,0,108,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244584);
allocate([74,0,0,0,117,0,0,0,110,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244600);
allocate([77,0,0,0,97,0,0,0,121,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5244616);
allocate(472, "i8", ALLOC_NONE, 5244632);
allocate(288, "i8", ALLOC_NONE, 5245104);
allocate(168, "i8", ALLOC_NONE, 5245392);
allocate(288, "i8", ALLOC_NONE, 5245560);
allocate(288, "i8", ALLOC_NONE, 5245848);
allocate(168, "i8", ALLOC_NONE, 5246136);
allocate(288, "i8", ALLOC_NONE, 5246304);
allocate(4, "i8", ALLOC_NONE, 5246592);
allocate(4, "i8", ALLOC_NONE, 5246596);
allocate(4, "i8", ALLOC_NONE, 5246600);
allocate(4, "i8", ALLOC_NONE, 5246604);
allocate(4, "i8", ALLOC_NONE, 5246608);
allocate(4, "i8", ALLOC_NONE, 5246612);
allocate(8, "i8", ALLOC_NONE, 5246616);
allocate(8, "i8", ALLOC_NONE, 5246624);
allocate(8, "i8", ALLOC_NONE, 5246632);
allocate(8, "i8", ALLOC_NONE, 5246640);
allocate(12, "i8", ALLOC_NONE, 5246648);
allocate(12, "i8", ALLOC_NONE, 5246660);
allocate(12, "i8", ALLOC_NONE, 5246672);
allocate(12, "i8", ALLOC_NONE, 5246684);
allocate(28, "i8", ALLOC_NONE, 5246696);
allocate(24, "i8", ALLOC_NONE, 5246724);
allocate(8, "i8", ALLOC_NONE, 5246748);
allocate(8, "i8", ALLOC_NONE, 5246756);
allocate(8, "i8", ALLOC_NONE, 5246764);
allocate(8, "i8", ALLOC_NONE, 5246772);
allocate(8, "i8", ALLOC_NONE, 5246780);
allocate(8, "i8", ALLOC_NONE, 5246788);
allocate(8, "i8", ALLOC_NONE, 5246796);
allocate(8, "i8", ALLOC_NONE, 5246804);
allocate(12, "i8", ALLOC_NONE, 5246812);
allocate(8, "i8", ALLOC_NONE, 5246824);
allocate(8, "i8", ALLOC_NONE, 5246832);
allocate(8, "i8", ALLOC_NONE, 5246840);
allocate(148, "i8", ALLOC_NONE, 5246848);
allocate(8, "i8", ALLOC_NONE, 5246996);
allocate(16, "i8", ALLOC_NONE, 5247004);
allocate(8, "i8", ALLOC_NONE, 5247020);
allocate(8, "i8", ALLOC_NONE, 5247028);
allocate(8, "i8", ALLOC_NONE, 5247036);
allocate(8, "i8", ALLOC_NONE, 5247044);
allocate([48,49,50,51,52,53,54,55,56,57,0] /* 0123456789\00 */, "i8", ALLOC_NONE, 5247052);
allocate([48,49,50,51,52,53,54,55,56,57,0] /* 0123456789\00 */, "i8", ALLOC_NONE, 5247064);
allocate([37,0,0,0,72,0,0,0,58,0,0,0,37,0,0,0,77,0,0,0,58,0,0,0,37,0,0,0,83,0,0,0], "i8", ALLOC_NONE, 5247076);
allocate([37,0,0,0,72,0,0,0,58,0,0,0,37,0,0,0,77,0,0,0], "i8", ALLOC_NONE, 5247108);
allocate([37,0,0,0,73,0,0,0,58,0,0,0,37,0,0,0,77,0,0,0,58,0,0,0,37,0,0,0,83,0,0,0,32,0,0,0,37,0,0,0,112,0,0,0], "i8", ALLOC_NONE, 5247128);
allocate([37,0,0,0,89,0,0,0,45,0,0,0,37,0,0,0,109,0,0,0,45,0,0,0,37,0,0,0,100,0,0,0], "i8", ALLOC_NONE, 5247172);
allocate([37,0,0,0,109,0,0,0,47,0,0,0,37,0,0,0,100,0,0,0,47,0,0,0,37,0,0,0,121,0,0,0], "i8", ALLOC_NONE, 5247204);
allocate([37,0,0,0,72,0,0,0,58,0,0,0,37,0,0,0,77,0,0,0,58,0,0,0,37,0,0,0,83,0,0,0], "i8", ALLOC_NONE, 5247236);
allocate([37,72,58,37,77,58,37,83] /* %H:%M:%S */, "i8", ALLOC_NONE, 5247268);
allocate([37,72,58,37,77] /* %H:%M */, "i8", ALLOC_NONE, 5247276);
allocate([37,73,58,37,77,58,37,83,32,37,112] /* %I:%M:%S %p */, "i8", ALLOC_NONE, 5247284);
allocate([37,89,45,37,109,45,37,100] /* %Y-%m-%d */, "i8", ALLOC_NONE, 5247296);
allocate([37,109,47,37,100,47,37,121] /* %m/%d/%y */, "i8", ALLOC_NONE, 5247304);
allocate([37,72,58,37,77,58,37,83] /* %H:%M:%S */, "i8", ALLOC_NONE, 5247312);
allocate([37,0,0,0,0,0] /* %\00\00\00\00\00 */, "i8", ALLOC_NONE, 5247320);
allocate([37,112,0,0,0,0] /* %p\00\00\00\00 */, "i8", ALLOC_NONE, 5247328);
allocate(4, "i8", ALLOC_NONE, 5247336);
allocate(4, "i8", ALLOC_NONE, 5247340);
allocate(4, "i8", ALLOC_NONE, 5247344);
allocate(12, "i8", ALLOC_NONE, 5247348);
allocate(12, "i8", ALLOC_NONE, 5247360);
allocate(12, "i8", ALLOC_NONE, 5247372);
allocate(12, "i8", ALLOC_NONE, 5247384);
allocate(4, "i8", ALLOC_NONE, 5247396);
allocate(4, "i8", ALLOC_NONE, 5247400);
allocate(4, "i8", ALLOC_NONE, 5247404);
allocate(12, "i8", ALLOC_NONE, 5247408);
allocate(12, "i8", ALLOC_NONE, 5247420);
allocate(12, "i8", ALLOC_NONE, 5247432);
allocate(12, "i8", ALLOC_NONE, 5247444);
allocate([0,0,0,0,80,38,80,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5247456);
allocate(1, "i8", ALLOC_NONE, 5247476);
allocate([0,0,0,0,92,38,80,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5247480);
allocate(1, "i8", ALLOC_NONE, 5247500);
allocate([0,0,0,0,104,38,80,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5247504);
allocate(1, "i8", ALLOC_NONE, 5247524);
allocate([0,0,0,0,116,38,80,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5247528);
allocate(1, "i8", ALLOC_NONE, 5247548);
allocate([0,0,0,0,128,38,80,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5247552);
allocate(1, "i8", ALLOC_NONE, 5247572);
allocate([0,0,0,0,148,38,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5247576);
allocate(1, "i8", ALLOC_NONE, 5247604);
allocate([0,0,0,0,180,38,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5247608);
allocate(1, "i8", ALLOC_NONE, 5247636);
allocate([0,0,0,0,212,38,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5247640);
allocate(1, "i8", ALLOC_NONE, 5247668);
allocate([0,0,0,0,244,38,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5247672);
allocate(1, "i8", ALLOC_NONE, 5247700);
allocate([0,0,0,0,140,39,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5247704);
allocate(1, "i8", ALLOC_NONE, 5247728);
allocate([0,0,0,0,172,39,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5247732);
allocate(1, "i8", ALLOC_NONE, 5247756);
allocate([0,0,0,0,204,39,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,248,255,255,255,204,39,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5247760);
allocate(1, "i8", ALLOC_NONE, 5247844);
allocate([0,0,0,0,244,39,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,248,255,255,255,244,39,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5247848);
allocate(1, "i8", ALLOC_NONE, 5247932);
allocate([0,0,0,0,28,40,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5247936);
allocate(1, "i8", ALLOC_NONE, 5247976);
allocate([0,0,0,0,40,40,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5247980);
allocate(1, "i8", ALLOC_NONE, 5248020);
allocate([0,0,0,0,52,40,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248024);
allocate(1, "i8", ALLOC_NONE, 5248056);
allocate([0,0,0,0,84,40,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248060);
allocate(1, "i8", ALLOC_NONE, 5248092);
allocate([0,0,0,0,116,40,80,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248096);
allocate(1, "i8", ALLOC_NONE, 5248112);
allocate([0,0,0,0,124,40,80,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248116);
allocate(1, "i8", ALLOC_NONE, 5248136);
allocate([0,0,0,0,136,40,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248140);
allocate(1, "i8", ALLOC_NONE, 5248192);
allocate([0,0,0,0,168,40,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248196);
allocate(1, "i8", ALLOC_NONE, 5248248);
allocate([0,0,0,0,200,40,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248252);
allocate(1, "i8", ALLOC_NONE, 5248316);
allocate([0,0,0,0,232,40,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248320);
allocate(1, "i8", ALLOC_NONE, 5248384);
allocate([0,0,0,0,8,41,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248388);
allocate(1, "i8", ALLOC_NONE, 5248420);
allocate([0,0,0,0,20,41,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248424);
allocate(1, "i8", ALLOC_NONE, 5248456);
allocate([0,0,0,0,32,41,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248460);
allocate(1, "i8", ALLOC_NONE, 5248508);
allocate([0,0,0,0,64,41,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248512);
allocate(1, "i8", ALLOC_NONE, 5248560);
allocate([0,0,0,0,96,41,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248564);
allocate(1, "i8", ALLOC_NONE, 5248612);
allocate([0,0,0,0,128,41,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248616);
allocate(1, "i8", ALLOC_NONE, 5248664);
allocate([0,0,0,0,160,41,80,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248668);
allocate(1, "i8", ALLOC_NONE, 5248688);
allocate([0,0,0,0,172,41,80,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248692);
allocate(1, "i8", ALLOC_NONE, 5248712);
allocate([0,0,0,0,184,41,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248716);
allocate(1, "i8", ALLOC_NONE, 5248784);
allocate([0,0,0,0,216,41,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248788);
allocate(1, "i8", ALLOC_NONE, 5248840);
allocate([0,0,0,0,8,42,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248844);
allocate(1, "i8", ALLOC_NONE, 5248880);
allocate([0,0,0,0,20,42,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248884);
allocate(1, "i8", ALLOC_NONE, 5248932);
allocate([0,0,0,0,32,42,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248936);
allocate(1, "i8", ALLOC_NONE, 5248984);
allocate([0,0,0,0,44,42,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5248988);
allocate(1, "i8", ALLOC_NONE, 5249052);
allocate([0,0,0,0,52,42,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5249056);
allocate(1, "i8", ALLOC_NONE, 5249120);
allocate([4,0,0,0,0,0,0,0,100,42,80,0,0,0,0,0,0,0,0,0,252,255,255,255,252,255,255,255,100,42,80,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5249124);
allocate(1, "i8", ALLOC_NONE, 5249164);
allocate([4,0,0,0,0,0,0,0,124,42,80,0,0,0,0,0,0,0,0,0,252,255,255,255,252,255,255,255,124,42,80,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5249168);
allocate(1, "i8", ALLOC_NONE, 5249208);
allocate([8,0,0,0,0,0,0,0,148,42,80,0,0,0,0,0,0,0,0,0,248,255,255,255,248,255,255,255,148,42,80,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5249212);
allocate(1, "i8", ALLOC_NONE, 5249252);
allocate([8,0,0,0,0,0,0,0,172,42,80,0,0,0,0,0,0,0,0,0,248,255,255,255,248,255,255,255,172,42,80,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5249256);
allocate(1, "i8", ALLOC_NONE, 5249296);
allocate([0,0,0,0,196,42,80,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5249300);
allocate(1, "i8", ALLOC_NONE, 5249320);
allocate([0,0,0,0,228,42,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5249324);
allocate(1, "i8", ALLOC_NONE, 5249388);
allocate([0,0,0,0,240,42,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5249392);
allocate(1, "i8", ALLOC_NONE, 5249456);
allocate([0,0,0,0,28,43,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5249460);
allocate(1, "i8", ALLOC_NONE, 5249516);
allocate([0,0,0,0,60,43,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5249520);
allocate(1, "i8", ALLOC_NONE, 5249576);
allocate([0,0,0,0,92,43,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5249580);
allocate(1, "i8", ALLOC_NONE, 5249636);
allocate([0,0,0,0,124,43,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5249640);
allocate(1, "i8", ALLOC_NONE, 5249696);
allocate([0,0,0,0,180,43,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5249700);
allocate(1, "i8", ALLOC_NONE, 5249764);
allocate([0,0,0,0,192,43,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5249768);
allocate(1, "i8", ALLOC_NONE, 5249832);
allocate([0,0,0,0,204,43,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, 5249836);
allocate(1, "i8", ALLOC_NONE, 5249876);
__ZTVN10__cxxabiv120__si_class_type_infoE=allocate([0,0,0,0,216,43,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_STATIC);
allocate(1, "i8", ALLOC_STATIC);
__ZTVN10__cxxabiv117__class_type_infoE=allocate([0,0,0,0,228,43,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_STATIC);
allocate(1, "i8", ALLOC_STATIC);
allocate([83,116,57,116,121,112,101,95,105,110,102,111,0] /* St9type_info\00 */, "i8", ALLOC_NONE, 5249880);
allocate([83,116,57,98,97,100,95,97,108,108,111,99,0] /* St9bad_alloc\00 */, "i8", ALLOC_NONE, 5249896);
allocate([83,116,56,98,97,100,95,99,97,115,116,0] /* St8bad_cast\00 */, "i8", ALLOC_NONE, 5249912);
allocate([83,116,49,51,114,117,110,116,105,109,101,95,101,114,114,111,114,0] /* St13runtime_error\00 */, "i8", ALLOC_NONE, 5249924);
allocate([83,116,49,50,108,101,110,103,116,104,95,101,114,114,111,114,0] /* St12length_error\00 */, "i8", ALLOC_NONE, 5249944);
allocate([83,116,49,49,108,111,103,105,99,95,101,114,114,111,114,0] /* St11logic_error\00 */, "i8", ALLOC_NONE, 5249964);
allocate([78,83,116,51,95,95,49,57,116,105,109,101,95,98,97,115,101,69,0] /* NSt3__19time_baseE\0 */, "i8", ALLOC_NONE, 5249980);
allocate([78,83,116,51,95,95,49,57,109,111,110,101,121,95,112,117,116,73,119,78,83,95,49,57,111,115,116,114,101,97,109,98,117,102,95,105,116,101,114,97,116,111,114,73,119,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,119,69,69,69,69,69,69,0] /* NSt3__19money_putIwN */, "i8", ALLOC_NONE, 5250000);
allocate([78,83,116,51,95,95,49,57,109,111,110,101,121,95,112,117,116,73,99,78,83,95,49,57,111,115,116,114,101,97,109,98,117,102,95,105,116,101,114,97,116,111,114,73,99,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,99,69,69,69,69,69,69,0] /* NSt3__19money_putIcN */, "i8", ALLOC_NONE, 5250072);
allocate([78,83,116,51,95,95,49,57,109,111,110,101,121,95,103,101,116,73,119,78,83,95,49,57,105,115,116,114,101,97,109,98,117,102,95,105,116,101,114,97,116,111,114,73,119,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,119,69,69,69,69,69,69,0] /* NSt3__19money_getIwN */, "i8", ALLOC_NONE, 5250144);
allocate([78,83,116,51,95,95,49,57,109,111,110,101,121,95,103,101,116,73,99,78,83,95,49,57,105,115,116,114,101,97,109,98,117,102,95,105,116,101,114,97,116,111,114,73,99,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,99,69,69,69,69,69,69,0] /* NSt3__19money_getIcN */, "i8", ALLOC_NONE, 5250216);
allocate([78,83,116,51,95,95,49,57,98,97,115,105,99,95,105,111,115,73,119,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,119,69,69,69,69,0] /* NSt3__19basic_iosIwN */, "i8", ALLOC_NONE, 5250288);
allocate([78,83,116,51,95,95,49,57,98,97,115,105,99,95,105,111,115,73,99,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,99,69,69,69,69,0] /* NSt3__19basic_iosIcN */, "i8", ALLOC_NONE, 5250332);
allocate([78,83,116,51,95,95,49,57,95,95,110,117,109,95,112,117,116,73,119,69,69,0] /* NSt3__19__num_putIwE */, "i8", ALLOC_NONE, 5250376);
allocate([78,83,116,51,95,95,49,57,95,95,110,117,109,95,112,117,116,73,99,69,69,0] /* NSt3__19__num_putIcE */, "i8", ALLOC_NONE, 5250400);
allocate([78,83,116,51,95,95,49,57,95,95,110,117,109,95,103,101,116,73,119,69,69,0] /* NSt3__19__num_getIwE */, "i8", ALLOC_NONE, 5250424);
allocate([78,83,116,51,95,95,49,57,95,95,110,117,109,95,103,101,116,73,99,69,69,0] /* NSt3__19__num_getIcE */, "i8", ALLOC_NONE, 5250448);
allocate([78,83,116,51,95,95,49,56,116,105,109,101,95,112,117,116,73,119,78,83,95,49,57,111,115,116,114,101,97,109,98,117,102,95,105,116,101,114,97,116,111,114,73,119,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,119,69,69,69,69,69,69,0] /* NSt3__18time_putIwNS */, "i8", ALLOC_NONE, 5250472);
allocate([78,83,116,51,95,95,49,56,116,105,109,101,95,112,117,116,73,99,78,83,95,49,57,111,115,116,114,101,97,109,98,117,102,95,105,116,101,114,97,116,111,114,73,99,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,99,69,69,69,69,69,69,0] /* NSt3__18time_putIcNS */, "i8", ALLOC_NONE, 5250544);
allocate([78,83,116,51,95,95,49,56,116,105,109,101,95,103,101,116,73,119,78,83,95,49,57,105,115,116,114,101,97,109,98,117,102,95,105,116,101,114,97,116,111,114,73,119,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,119,69,69,69,69,69,69,0] /* NSt3__18time_getIwNS */, "i8", ALLOC_NONE, 5250616);
allocate([78,83,116,51,95,95,49,56,116,105,109,101,95,103,101,116,73,99,78,83,95,49,57,105,115,116,114,101,97,109,98,117,102,95,105,116,101,114,97,116,111,114,73,99,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,99,69,69,69,69,69,69,0] /* NSt3__18time_getIcNS */, "i8", ALLOC_NONE, 5250688);
allocate([78,83,116,51,95,95,49,56,110,117,109,112,117,110,99,116,73,119,69,69,0] /* NSt3__18numpunctIwEE */, "i8", ALLOC_NONE, 5250760);
allocate([78,83,116,51,95,95,49,56,110,117,109,112,117,110,99,116,73,99,69,69,0] /* NSt3__18numpunctIcEE */, "i8", ALLOC_NONE, 5250784);
allocate([78,83,116,51,95,95,49,56,109,101,115,115,97,103,101,115,73,119,69,69,0] /* NSt3__18messagesIwEE */, "i8", ALLOC_NONE, 5250808);
allocate([78,83,116,51,95,95,49,56,109,101,115,115,97,103,101,115,73,99,69,69,0] /* NSt3__18messagesIcEE */, "i8", ALLOC_NONE, 5250832);
allocate([78,83,116,51,95,95,49,56,105,111,115,95,98,97,115,101,69,0] /* NSt3__18ios_baseE\00 */, "i8", ALLOC_NONE, 5250856);
allocate([78,83,116,51,95,95,49,56,105,111,115,95,98,97,115,101,55,102,97,105,108,117,114,101,69,0] /* NSt3__18ios_base7fai */, "i8", ALLOC_NONE, 5250876);
allocate([78,83,116,51,95,95,49,55,110,117,109,95,112,117,116,73,119,78,83,95,49,57,111,115,116,114,101,97,109,98,117,102,95,105,116,101,114,97,116,111,114,73,119,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,119,69,69,69,69,69,69,0] /* NSt3__17num_putIwNS_ */, "i8", ALLOC_NONE, 5250904);
allocate([78,83,116,51,95,95,49,55,110,117,109,95,112,117,116,73,99,78,83,95,49,57,111,115,116,114,101,97,109,98,117,102,95,105,116,101,114,97,116,111,114,73,99,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,99,69,69,69,69,69,69,0] /* NSt3__17num_putIcNS_ */, "i8", ALLOC_NONE, 5250972);
allocate([78,83,116,51,95,95,49,55,110,117,109,95,103,101,116,73,119,78,83,95,49,57,105,115,116,114,101,97,109,98,117,102,95,105,116,101,114,97,116,111,114,73,119,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,119,69,69,69,69,69,69,0] /* NSt3__17num_getIwNS_ */, "i8", ALLOC_NONE, 5251040);
allocate([78,83,116,51,95,95,49,55,110,117,109,95,103,101,116,73,99,78,83,95,49,57,105,115,116,114,101,97,109,98,117,102,95,105,116,101,114,97,116,111,114,73,99,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,99,69,69,69,69,69,69,0] /* NSt3__17num_getIcNS_ */, "i8", ALLOC_NONE, 5251108);
allocate([78,83,116,51,95,95,49,55,99,111,108,108,97,116,101,73,119,69,69,0] /* NSt3__17collateIwEE\ */, "i8", ALLOC_NONE, 5251176);
allocate([78,83,116,51,95,95,49,55,99,111,108,108,97,116,101,73,99,69,69,0] /* NSt3__17collateIcEE\ */, "i8", ALLOC_NONE, 5251196);
allocate([78,83,116,51,95,95,49,55,99,111,100,101,99,118,116,73,119,99,49,48,95,109,98,115,116,97,116,101,95,116,69,69,0] /* NSt3__17codecvtIwc10 */, "i8", ALLOC_NONE, 5251216);
allocate([78,83,116,51,95,95,49,55,99,111,100,101,99,118,116,73,99,99,49,48,95,109,98,115,116,97,116,101,95,116,69,69,0] /* NSt3__17codecvtIcc10 */, "i8", ALLOC_NONE, 5251252);
allocate([78,83,116,51,95,95,49,55,99,111,100,101,99,118,116,73,68,115,99,49,48,95,109,98,115,116,97,116,101,95,116,69,69,0] /* NSt3__17codecvtIDsc1 */, "i8", ALLOC_NONE, 5251288);
allocate([78,83,116,51,95,95,49,55,99,111,100,101,99,118,116,73,68,105,99,49,48,95,109,98,115,116,97,116,101,95,116,69,69,0] /* NSt3__17codecvtIDic1 */, "i8", ALLOC_NONE, 5251324);
allocate([78,83,116,51,95,95,49,54,108,111,99,97,108,101,53,102,97,99,101,116,69,0] /* NSt3__16locale5facet */, "i8", ALLOC_NONE, 5251360);
allocate([78,83,116,51,95,95,49,54,108,111,99,97,108,101,53,95,95,105,109,112,69,0] /* NSt3__16locale5__imp */, "i8", ALLOC_NONE, 5251384);
allocate([78,83,116,51,95,95,49,53,99,116,121,112,101,73,119,69,69,0] /* NSt3__15ctypeIwEE\00 */, "i8", ALLOC_NONE, 5251408);
allocate([78,83,116,51,95,95,49,53,99,116,121,112,101,73,99,69,69,0] /* NSt3__15ctypeIcEE\00 */, "i8", ALLOC_NONE, 5251428);
allocate([78,83,116,51,95,95,49,50,48,95,95,116,105,109,101,95,103,101,116,95,99,95,115,116,111,114,97,103,101,73,119,69,69,0] /* NSt3__120__time_get_ */, "i8", ALLOC_NONE, 5251448);
allocate([78,83,116,51,95,95,49,50,48,95,95,116,105,109,101,95,103,101,116,95,99,95,115,116,111,114,97,103,101,73,99,69,69,0] /* NSt3__120__time_get_ */, "i8", ALLOC_NONE, 5251484);
allocate([78,83,116,51,95,95,49,49,57,95,95,105,111,115,116,114,101,97,109,95,99,97,116,101,103,111,114,121,69,0] /* NSt3__119__iostream_ */, "i8", ALLOC_NONE, 5251520);
allocate([78,83,116,51,95,95,49,49,55,95,95,119,105,100,101,110,95,102,114,111,109,95,117,116,102,56,73,76,106,51,50,69,69,69,0] /* NSt3__117__widen_fro */, "i8", ALLOC_NONE, 5251552);
allocate([78,83,116,51,95,95,49,49,54,95,95,110,97,114,114,111,119,95,116,111,95,117,116,102,56,73,76,106,51,50,69,69,69,0] /* NSt3__116__narrow_to */, "i8", ALLOC_NONE, 5251588);
allocate([78,83,116,51,95,95,49,49,53,98,97,115,105,99,95,115,116,114,101,97,109,98,117,102,73,119,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,119,69,69,69,69,0] /* NSt3__115basic_strea */, "i8", ALLOC_NONE, 5251624);
allocate([78,83,116,51,95,95,49,49,53,98,97,115,105,99,95,115,116,114,101,97,109,98,117,102,73,99,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,99,69,69,69,69,0] /* NSt3__115basic_strea */, "i8", ALLOC_NONE, 5251676);
allocate([78,83,116,51,95,95,49,49,52,101,114,114,111,114,95,99,97,116,101,103,111,114,121,69,0] /* NSt3__114error_categ */, "i8", ALLOC_NONE, 5251728);
allocate([78,83,116,51,95,95,49,49,52,95,95,115,104,97,114,101,100,95,99,111,117,110,116,69,0] /* NSt3__114__shared_co */, "i8", ALLOC_NONE, 5251756);
allocate([78,83,116,51,95,95,49,49,52,95,95,110,117,109,95,112,117,116,95,98,97,115,101,69,0] /* NSt3__114__num_put_b */, "i8", ALLOC_NONE, 5251784);
allocate([78,83,116,51,95,95,49,49,52,95,95,110,117,109,95,103,101,116,95,98,97,115,101,69,0] /* NSt3__114__num_get_b */, "i8", ALLOC_NONE, 5251812);
allocate([78,83,116,51,95,95,49,49,51,109,101,115,115,97,103,101,115,95,98,97,115,101,69,0] /* NSt3__113messages_ba */, "i8", ALLOC_NONE, 5251840);
allocate([78,83,116,51,95,95,49,49,51,98,97,115,105,99,95,111,115,116,114,101,97,109,73,119,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,119,69,69,69,69,0] /* NSt3__113basic_ostre */, "i8", ALLOC_NONE, 5251864);
allocate([78,83,116,51,95,95,49,49,51,98,97,115,105,99,95,111,115,116,114,101,97,109,73,99,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,99,69,69,69,69,0] /* NSt3__113basic_ostre */, "i8", ALLOC_NONE, 5251912);
allocate([78,83,116,51,95,95,49,49,51,98,97,115,105,99,95,105,115,116,114,101,97,109,73,119,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,119,69,69,69,69,0] /* NSt3__113basic_istre */, "i8", ALLOC_NONE, 5251960);
allocate([78,83,116,51,95,95,49,49,51,98,97,115,105,99,95,105,115,116,114,101,97,109,73,99,78,83,95,49,49,99,104,97,114,95,116,114,97,105,116,115,73,99,69,69,69,69,0] /* NSt3__113basic_istre */, "i8", ALLOC_NONE, 5252008);
allocate([78,83,116,51,95,95,49,49,50,115,121,115,116,101,109,95,101,114,114,111,114,69,0] /* NSt3__112system_erro */, "i8", ALLOC_NONE, 5252056);
allocate([78,83,116,51,95,95,49,49,50,99,111,100,101,99,118,116,95,98,97,115,101,69,0] /* NSt3__112codecvt_bas */, "i8", ALLOC_NONE, 5252080);
allocate([78,83,116,51,95,95,49,49,50,95,95,100,111,95,109,101,115,115,97,103,101,69,0] /* NSt3__112__do_messag */, "i8", ALLOC_NONE, 5252104);
allocate([78,83,116,51,95,95,49,49,49,95,95,115,116,100,111,117,116,98,117,102,73,119,69,69,0] /* NSt3__111__stdoutbuf */, "i8", ALLOC_NONE, 5252128);
allocate([78,83,116,51,95,95,49,49,49,95,95,115,116,100,111,117,116,98,117,102,73,99,69,69,0] /* NSt3__111__stdoutbuf */, "i8", ALLOC_NONE, 5252156);
allocate([78,83,116,51,95,95,49,49,49,95,95,109,111,110,101,121,95,112,117,116,73,119,69,69,0] /* NSt3__111__money_put */, "i8", ALLOC_NONE, 5252184);
allocate([78,83,116,51,95,95,49,49,49,95,95,109,111,110,101,121,95,112,117,116,73,99,69,69,0] /* NSt3__111__money_put */, "i8", ALLOC_NONE, 5252212);
allocate([78,83,116,51,95,95,49,49,49,95,95,109,111,110,101,121,95,103,101,116,73,119,69,69,0] /* NSt3__111__money_get */, "i8", ALLOC_NONE, 5252240);
allocate([78,83,116,51,95,95,49,49,49,95,95,109,111,110,101,121,95,103,101,116,73,99,69,69,0] /* NSt3__111__money_get */, "i8", ALLOC_NONE, 5252268);
allocate([78,83,116,51,95,95,49,49,48,109,111,110,101,121,112,117,110,99,116,73,119,76,98,49,69,69,69,0] /* NSt3__110moneypunctI */, "i8", ALLOC_NONE, 5252296);
allocate([78,83,116,51,95,95,49,49,48,109,111,110,101,121,112,117,110,99,116,73,119,76,98,48,69,69,69,0] /* NSt3__110moneypunctI */, "i8", ALLOC_NONE, 5252324);
allocate([78,83,116,51,95,95,49,49,48,109,111,110,101,121,112,117,110,99,116,73,99,76,98,49,69,69,69,0] /* NSt3__110moneypunctI */, "i8", ALLOC_NONE, 5252352);
allocate([78,83,116,51,95,95,49,49,48,109,111,110,101,121,112,117,110,99,116,73,99,76,98,48,69,69,69,0] /* NSt3__110moneypunctI */, "i8", ALLOC_NONE, 5252380);
allocate([78,83,116,51,95,95,49,49,48,109,111,110,101,121,95,98,97,115,101,69,0] /* NSt3__110money_baseE */, "i8", ALLOC_NONE, 5252408);
allocate([78,83,116,51,95,95,49,49,48,99,116,121,112,101,95,98,97,115,101,69,0] /* NSt3__110ctype_baseE */, "i8", ALLOC_NONE, 5252432);
allocate([78,83,116,51,95,95,49,49,48,95,95,116,105,109,101,95,112,117,116,69,0] /* NSt3__110__time_putE */, "i8", ALLOC_NONE, 5252456);
allocate([78,83,116,51,95,95,49,49,48,95,95,115,116,100,105,110,98,117,102,73,119,69,69,0] /* NSt3__110__stdinbufI */, "i8", ALLOC_NONE, 5252480);
allocate([78,83,116,51,95,95,49,49,48,95,95,115,116,100,105,110,98,117,102,73,99,69,69,0] /* NSt3__110__stdinbufI */, "i8", ALLOC_NONE, 5252504);
allocate([78,49,48,95,95,99,120,120,97,98,105,118,49,50,49,95,95,118,109,105,95,99,108,97,115,115,95,116,121,112,101,95,105,110,102,111,69,0] /* N10__cxxabiv121__vmi */, "i8", ALLOC_NONE, 5252528);
allocate([78,49,48,95,95,99,120,120,97,98,105,118,49,50,48,95,95,115,105,95,99,108,97,115,115,95,116,121,112,101,95,105,110,102,111,69,0] /* N10__cxxabiv120__si_ */, "i8", ALLOC_NONE, 5252568);
allocate([78,49,48,95,95,99,120,120,97,98,105,118,49,49,55,95,95,99,108,97,115,115,95,116,121,112,101,95,105,110,102,111,69,0] /* N10__cxxabiv117__cla */, "i8", ALLOC_NONE, 5252608);
allocate([78,49,48,95,95,99,120,120,97,98,105,118,49,49,54,95,95,115,104,105,109,95,116,121,112,101,95,105,110,102,111,69,0] /* N10__cxxabiv116__shi */, "i8", ALLOC_NONE, 5252644);
allocate(8, "i8", ALLOC_NONE, 5252680);
allocate(12, "i8", ALLOC_NONE, 5252688);
allocate(12, "i8", ALLOC_NONE, 5252700);
allocate(12, "i8", ALLOC_NONE, 5252712);
allocate([0,0,0,0,0,0,0,0,128,38,80,0], "i8", ALLOC_NONE, 5252724);
allocate(12, "i8", ALLOC_NONE, 5252736);
allocate(8, "i8", ALLOC_NONE, 5252748);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,252,42,80,0,0,0,0,0], "i8", ALLOC_NONE, 5252756);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,4,43,80,0,0,0,0,0], "i8", ALLOC_NONE, 5252788);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,12,43,80,0,0,0,0,0], "i8", ALLOC_NONE, 5252820);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,20,43,80,0,0,0,0,0], "i8", ALLOC_NONE, 5252852);
allocate([0,0,0,0,0,0,0,0,116,40,80,0], "i8", ALLOC_NONE, 5252884);
allocate([0,0,0,0,0,0,0,0,116,40,80,0], "i8", ALLOC_NONE, 5252896);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,76,42,80,0,0,0,0,0], "i8", ALLOC_NONE, 5252908);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,76,42,80,0,0,0,0,0], "i8", ALLOC_NONE, 5252932);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,84,42,80,0,0,0,0,0], "i8", ALLOC_NONE, 5252956);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,84,42,80,0,0,0,0,0], "i8", ALLOC_NONE, 5252980);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,172,43,80,0,0,8,0,0], "i8", ALLOC_NONE, 5253004);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,172,43,80,0,0,8,0,0], "i8", ALLOC_NONE, 5253036);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,160,41,80,0,2,0,0,0,140,38,80,0,2,0,0,0,248,41,80,0,0,8,0,0], "i8", ALLOC_NONE, 5253068);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,160,41,80,0,2,0,0,0,140,38,80,0,2,0,0,0,0,42,80,0,0,8,0,0], "i8", ALLOC_NONE, 5253108);
allocate([0,0,0,0,0,0,0,0,160,41,80,0], "i8", ALLOC_NONE, 5253148);
allocate([0,0,0,0,0,0,0,0,160,41,80,0], "i8", ALLOC_NONE, 5253160);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,92,42,80,0,2,0,0,0], "i8", ALLOC_NONE, 5253172);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,92,42,80,0,2,0,0,0], "i8", ALLOC_NONE, 5253204);
allocate(8, "i8", ALLOC_NONE, 5253236);
allocate([0,0,0,0,0,0,0,0,196,42,80,0], "i8", ALLOC_NONE, 5253244);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,44,39,80,0,0,0,0,0], "i8", ALLOC_NONE, 5253256);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,68,39,80,0,0,0,0,0], "i8", ALLOC_NONE, 5253288);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,92,39,80,0,0,0,0,0], "i8", ALLOC_NONE, 5253320);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,116,39,80,0,0,0,0,0], "i8", ALLOC_NONE, 5253352);
allocate([0,0,0,0,0,0,0,0,160,41,80,0], "i8", ALLOC_NONE, 5253384);
allocate([0,0,0,0,0,0,0,0,160,41,80,0], "i8", ALLOC_NONE, 5253396);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,208,42,80,0,2,0,0,0], "i8", ALLOC_NONE, 5253408);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,208,42,80,0,2,0,0,0], "i8", ALLOC_NONE, 5253440);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,208,42,80,0,2,0,0,0], "i8", ALLOC_NONE, 5253472);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,208,42,80,0,2,0,0,0], "i8", ALLOC_NONE, 5253504);
allocate([0,0,0,0,0,0,0,0,68,42,80,0], "i8", ALLOC_NONE, 5253536);
allocate([0,0,0,0,0,0,0,0,160,41,80,0], "i8", ALLOC_NONE, 5253548);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,164,43,80,0,2,0,0,0], "i8", ALLOC_NONE, 5253560);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,164,43,80,0,2,0,0,0], "i8", ALLOC_NONE, 5253592);
allocate(8, "i8", ALLOC_NONE, 5253624);
allocate(8, "i8", ALLOC_NONE, 5253632);
allocate([0,0,0,0,0,0,0,0,216,42,80,0], "i8", ALLOC_NONE, 5253640);
allocate([0,0,0,0,0,0,0,0,128,41,80,0], "i8", ALLOC_NONE, 5253652);
allocate([0,0,0,0,0,0,0,0,128,41,80,0], "i8", ALLOC_NONE, 5253664);
allocate(8, "i8", ALLOC_NONE, 5253676);
allocate(8, "i8", ALLOC_NONE, 5253684);
allocate(8, "i8", ALLOC_NONE, 5253692);
allocate(8, "i8", ALLOC_NONE, 5253700);
allocate(8, "i8", ALLOC_NONE, 5253708);
allocate(8, "i8", ALLOC_NONE, 5253716);
allocate(8, "i8", ALLOC_NONE, 5253724);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,20,39,80,0,3,244,255,255], "i8", ALLOC_NONE, 5253732);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,32,39,80,0,3,244,255,255], "i8", ALLOC_NONE, 5253756);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,20,39,80,0,3,244,255,255], "i8", ALLOC_NONE, 5253780);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,32,39,80,0,3,244,255,255], "i8", ALLOC_NONE, 5253804);
allocate([0,0,0,0,0,0,0,0,104,38,80,0], "i8", ALLOC_NONE, 5253828);
allocate(8, "i8", ALLOC_NONE, 5253840);
allocate([0,0,0,0,0,0,0,0,60,42,80,0], "i8", ALLOC_NONE, 5253848);
allocate([0,0,0,0,0,0,0,0,44,42,80,0], "i8", ALLOC_NONE, 5253860);
allocate([0,0,0,0,0,0,0,0,52,42,80,0], "i8", ALLOC_NONE, 5253872);
allocate(8, "i8", ALLOC_NONE, 5253884);
allocate(8, "i8", ALLOC_NONE, 5253892);
allocate(8, "i8", ALLOC_NONE, 5253900);
allocate(8, "i8", ALLOC_NONE, 5253908);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,156,43,80,0,2,0,0,0], "i8", ALLOC_NONE, 5253916);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,156,43,80,0,2,0,0,0], "i8", ALLOC_NONE, 5253948);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,156,43,80,0,2,0,0,0], "i8", ALLOC_NONE, 5253980);
allocate([0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,160,41,80,0,2,0,0,0,156,43,80,0,2,0,0,0], "i8", ALLOC_NONE, 5254012);
allocate(8, "i8", ALLOC_NONE, 5254044);
allocate(8, "i8", ALLOC_NONE, 5254052);
allocate(8, "i8", ALLOC_NONE, 5254060);
allocate([0,0,0,0,0,0,0,0,44,42,80,0], "i8", ALLOC_NONE, 5254068);
allocate([0,0,0,0,0,0,0,0,52,42,80,0], "i8", ALLOC_NONE, 5254080);
allocate([0,0,0,0,0,0,0,0,228,43,80,0], "i8", ALLOC_NONE, 5254092);
allocate([0,0,0,0,0,0,0,0,228,43,80,0], "i8", ALLOC_NONE, 5254104);
allocate([0,0,0,0,0,0,0,0,240,43,80,0], "i8", ALLOC_NONE, 5254116);
allocate([0,0,0,0,0,0,0,0,72,38,80,0], "i8", ALLOC_NONE, 5254128);
allocate(52, "i8", ALLOC_NONE, 5254140);
allocate(52, "i8", ALLOC_NONE, 5254192);
allocate(56, "i8", ALLOC_NONE, 5254244);
allocate(52, "i8", ALLOC_NONE, 5254300);
allocate(52, "i8", ALLOC_NONE, 5254352);
allocate(56, "i8", ALLOC_NONE, 5254404);
allocate([255,255,255,255], "i8", ALLOC_NONE, 5254460);
allocate([255,255,255,255], "i8", ALLOC_NONE, 5254464);
allocate(8, "i8", ALLOC_NONE, 5254468);
allocate(8, "i8", ALLOC_NONE, 5254476);
allocate(8, "i8", ALLOC_NONE, 5254484);
allocate(8, "i8", ALLOC_NONE, 5254492);
allocate(8, "i8", ALLOC_NONE, 5254500);
allocate(8, "i8", ALLOC_NONE, 5254508);
allocate(8, "i8", ALLOC_NONE, 5254516);
allocate(8, "i8", ALLOC_NONE, 5254524);
allocate(8, "i8", ALLOC_NONE, 5254532);
allocate(8, "i8", ALLOC_NONE, 5254540);
allocate(8, "i8", ALLOC_NONE, 5254548);
allocate(8, "i8", ALLOC_NONE, 5254556);
allocate(8, "i8", ALLOC_NONE, 5254564);
allocate(8, "i8", ALLOC_NONE, 5254572);
allocate(8, "i8", ALLOC_NONE, 5254580);
allocate(8, "i8", ALLOC_NONE, 5254588);
allocate(8, "i8", ALLOC_NONE, 5254596);
allocate(8, "i8", ALLOC_NONE, 5254604);
allocate(8, "i8", ALLOC_NONE, 5254612);
allocate(8, "i8", ALLOC_NONE, 5254620);
allocate(8, "i8", ALLOC_NONE, 5254628);
allocate(8, "i8", ALLOC_NONE, 5254636);
allocate(4, "i8", ALLOC_NONE, 5254644);
allocate(84, "i8", ALLOC_NONE, 5254648);
allocate(84, "i8", ALLOC_NONE, 5254732);
allocate(84, "i8", ALLOC_NONE, 5254816);
allocate(8, "i8", ALLOC_NONE, 5254900);
allocate(8, "i8", ALLOC_NONE, 5254908);
allocate(88, "i8", ALLOC_NONE, 5254916);
allocate(84, "i8", ALLOC_NONE, 5255004);
allocate(84, "i8", ALLOC_NONE, 5255088);
allocate(84, "i8", ALLOC_NONE, 5255172);
allocate(88, "i8", ALLOC_NONE, 5255256);
allocate(1, "i8", ALLOC_NONE, 5255344);
allocate([48,49,50,51,52,53,54,55,56,57,97,98,99,100,101,102,65,66,67,68,69,70,120,88,43,45,112,80,105,73,110,78,0] /* 0123456789abcdefABCD */, "i8", ALLOC_NONE, 5255348);
allocate(8, "i8", ALLOC_NONE, 5255384);
allocate(8, "i8", ALLOC_NONE, 5255392);
allocate(8, "i8", ALLOC_NONE, 5255400);
allocate(8, "i8", ALLOC_NONE, 5255408);
allocate(4, "i8", ALLOC_NONE, 5255416);
allocate(8, "i8", ALLOC_NONE, 5255420);
allocate(8, "i8", ALLOC_NONE, 5255428);
allocate(8, "i8", ALLOC_NONE, 5255436);
allocate(8, "i8", ALLOC_NONE, 5255444);
allocate(8, "i8", ALLOC_NONE, 5255452);
allocate(8, "i8", ALLOC_NONE, 5255460);
allocate(8, "i8", ALLOC_NONE, 5255468);
allocate(8, "i8", ALLOC_NONE, 5255476);
allocate(8, "i8", ALLOC_NONE, 5255484);
allocate(8, "i8", ALLOC_NONE, 5255492);
allocate(8, "i8", ALLOC_NONE, 5255500);
allocate(8, "i8", ALLOC_NONE, 5255508);
allocate(8, "i8", ALLOC_NONE, 5255516);
allocate(8, "i8", ALLOC_NONE, 5255524);
allocate(8, "i8", ALLOC_NONE, 5255532);
allocate(8, "i8", ALLOC_NONE, 5255540);
allocate(8, "i8", ALLOC_NONE, 5255548);
allocate(8, "i8", ALLOC_NONE, 5255556);
allocate(8, "i8", ALLOC_NONE, 5255564);
allocate(8, "i8", ALLOC_NONE, 5255572);
allocate(8, "i8", ALLOC_NONE, 5255580);
allocate(8, "i8", ALLOC_NONE, 5255588);
allocate(8, "i8", ALLOC_NONE, 5255596);
allocate(8, "i8", ALLOC_NONE, 5255604);
allocate(8, "i8", ALLOC_NONE, 5255612);
allocate(8, "i8", ALLOC_NONE, 5255620);
allocate(8, "i8", ALLOC_NONE, 5255628);
allocate(8, "i8", ALLOC_NONE, 5255636);
allocate(8, "i8", ALLOC_NONE, 5255644);
allocate(8, "i8", ALLOC_NONE, 5255652);
allocate(8, "i8", ALLOC_NONE, 5255660);
allocate(8, "i8", ALLOC_NONE, 5255668);
allocate(8, "i8", ALLOC_NONE, 5255676);
allocate(8, "i8", ALLOC_NONE, 5255684);
allocate(8, "i8", ALLOC_NONE, 5255692);
allocate(8, "i8", ALLOC_NONE, 5255700);
allocate(8, "i8", ALLOC_NONE, 5255708);
allocate(8, "i8", ALLOC_NONE, 5255716);
allocate(8, "i8", ALLOC_NONE, 5255724);
allocate(8, "i8", ALLOC_NONE, 5255732);
allocate(8, "i8", ALLOC_NONE, 5255740);
allocate(8, "i8", ALLOC_NONE, 5255748);
allocate(8, "i8", ALLOC_NONE, 5255756);
allocate(8, "i8", ALLOC_NONE, 5255764);
HEAP32[((5247464)>>2)]=(66);
HEAP32[((5247468)>>2)]=(306);
HEAP32[((5247472)>>2)]=(542);
HEAP32[((5247488)>>2)]=(514);
HEAP32[((5247492)>>2)]=(408);
HEAP32[((5247496)>>2)]=(204);
HEAP32[((5247512)>>2)]=(186);
HEAP32[((5247516)>>2)]=(722);
HEAP32[((5247520)>>2)]=(216);
HEAP32[((5247536)>>2)]=(248);
HEAP32[((5247540)>>2)]=(16);
HEAP32[((5247544)>>2)]=(582);
HEAP32[((5247560)>>2)]=(248);
HEAP32[((5247564)>>2)]=(40);
HEAP32[((5247568)>>2)]=(582);
HEAP32[((5247584)>>2)]=(416);
HEAP32[((5247588)>>2)]=(220);
HEAP32[((5247592)>>2)]=(116);
HEAP32[((5247596)>>2)]=(456);
HEAP32[((5247600)>>2)]=(48);
HEAP32[((5247616)>>2)]=(650);
HEAP32[((5247620)>>2)]=(464);
HEAP32[((5247624)>>2)]=(116);
HEAP32[((5247628)>>2)]=(676);
HEAP32[((5247632)>>2)]=(100);
HEAP32[((5247648)>>2)]=(474);
HEAP32[((5247652)>>2)]=(468);
HEAP32[((5247656)>>2)]=(116);
HEAP32[((5247660)>>2)]=(458);
HEAP32[((5247664)>>2)]=(696);
HEAP32[((5247680)>>2)]=(714);
HEAP32[((5247684)>>2)]=(360);
HEAP32[((5247688)>>2)]=(116);
HEAP32[((5247692)>>2)]=(444);
HEAP32[((5247696)>>2)]=(522);
HEAP32[((5247712)>>2)]=(704);
HEAP32[((5247716)>>2)]=(36);
HEAP32[((5247720)>>2)]=(116);
HEAP32[((5247724)>>2)]=(120);
HEAP32[((5247740)>>2)]=(404);
HEAP32[((5247744)>>2)]=(288);
HEAP32[((5247748)>>2)]=(116);
HEAP32[((5247752)>>2)]=(170);
HEAP32[((5247768)>>2)]=(84);
HEAP32[((5247772)>>2)]=(290);
HEAP32[((5247776)>>2)]=(116);
HEAP32[((5247780)>>2)]=(620);
HEAP32[((5247784)>>2)]=(20);
HEAP32[((5247788)>>2)]=(470);
HEAP32[((5247792)>>2)]=(28);
HEAP32[((5247796)>>2)]=(200);
HEAP32[((5247800)>>2)]=(622);
HEAP32[((5247804)>>2)]=(226);
HEAP32[((5247816)>>2)]=(112);
HEAP32[((5247820)>>2)]=(44);
HEAP32[((5247824)>>2)]=(178);
HEAP32[((5247828)>>2)]=(72);
HEAP32[((5247832)>>2)]=(8);
HEAP32[((5247836)>>2)]=(164);
HEAP32[((5247840)>>2)]=(652);
HEAP32[((5247856)>>2)]=(688);
HEAP32[((5247860)>>2)]=(632);
HEAP32[((5247864)>>2)]=(116);
HEAP32[((5247868)>>2)]=(108);
HEAP32[((5247872)>>2)]=(126);
HEAP32[((5247876)>>2)]=(654);
HEAP32[((5247880)>>2)]=(372);
HEAP32[((5247884)>>2)]=(162);
HEAP32[((5247888)>>2)]=(14);
HEAP32[((5247892)>>2)]=(602);
HEAP32[((5247904)>>2)]=(350);
HEAP32[((5247908)>>2)]=(556);
HEAP32[((5247912)>>2)]=(604);
HEAP32[((5247916)>>2)]=(640);
HEAP32[((5247920)>>2)]=(312);
HEAP32[((5247924)>>2)]=(238);
HEAP32[((5247928)>>2)]=(270);
HEAP32[((5247944)>>2)]=(210);
HEAP32[((5247948)>>2)]=(476);
HEAP32[((5247952)>>2)]=(116);
HEAP32[((5247956)>>2)]=(254);
HEAP32[((5247960)>>2)]=(224);
HEAP32[((5247964)>>2)]=(114);
HEAP32[((5247968)>>2)]=(352);
HEAP32[((5247972)>>2)]=(428);
HEAP32[((5247988)>>2)]=(152);
HEAP32[((5247992)>>2)]=(172);
HEAP32[((5247996)>>2)]=(116);
HEAP32[((5248000)>>2)]=(232);
HEAP32[((5248004)>>2)]=(462);
HEAP32[((5248008)>>2)]=(158);
HEAP32[((5248012)>>2)]=(450);
HEAP32[((5248016)>>2)]=(6);
HEAP32[((5248032)>>2)]=(692);
HEAP32[((5248036)>>2)]=(2);
HEAP32[((5248040)>>2)]=(116);
HEAP32[((5248044)>>2)]=(384);
HEAP32[((5248048)>>2)]=(708);
HEAP32[((5248052)>>2)]=(538);
HEAP32[((5248068)>>2)]=(110);
HEAP32[((5248072)>>2)]=(598);
HEAP32[((5248076)>>2)]=(116);
HEAP32[((5248080)>>2)]=(630);
HEAP32[((5248084)>>2)]=(208);
HEAP32[((5248088)>>2)]=(182);
HEAP32[((5248104)>>2)]=(614);
HEAP32[((5248108)>>2)]=(302);
HEAP32[((5248124)>>2)]=(62);
HEAP32[((5248128)>>2)]=(358);
HEAP32[((5248132)>>2)]=(216);
HEAP32[((5248148)>>2)]=(12);
HEAP32[((5248152)>>2)]=(424);
HEAP32[((5248156)>>2)]=(116);
HEAP32[((5248160)>>2)]=(528);
HEAP32[((5248164)>>2)]=(82);
HEAP32[((5248168)>>2)]=(76);
HEAP32[((5248172)>>2)]=(80);
HEAP32[((5248176)>>2)]=(74);
HEAP32[((5248180)>>2)]=(90);
HEAP32[((5248184)>>2)]=(88);
HEAP32[((5248188)>>2)]=(150);
HEAP32[((5248204)>>2)]=(252);
HEAP32[((5248208)>>2)]=(38);
HEAP32[((5248212)>>2)]=(116);
HEAP32[((5248216)>>2)]=(508);
HEAP32[((5248220)>>2)]=(512);
HEAP32[((5248224)>>2)]=(502);
HEAP32[((5248228)>>2)]=(510);
HEAP32[((5248232)>>2)]=(500);
HEAP32[((5248236)>>2)]=(506);
HEAP32[((5248240)>>2)]=(504);
HEAP32[((5248244)>>2)]=(426);
HEAP32[((5248260)>>2)]=(86);
HEAP32[((5248264)>>2)]=(46);
HEAP32[((5248268)>>2)]=(116);
HEAP32[((5248272)>>2)]=(566);
HEAP32[((5248276)>>2)]=(564);
HEAP32[((5248280)>>2)]=(554);
HEAP32[((5248284)>>2)]=(558);
HEAP32[((5248288)>>2)]=(460);
HEAP32[((5248292)>>2)]=(562);
HEAP32[((5248296)>>2)]=(552);
HEAP32[((5248300)>>2)]=(572);
HEAP32[((5248304)>>2)]=(570);
HEAP32[((5248308)>>2)]=(568);
HEAP32[((5248312)>>2)]=(336);
HEAP32[((5248328)>>2)]=(130);
HEAP32[((5248332)>>2)]=(4);
HEAP32[((5248336)>>2)]=(116);
HEAP32[((5248340)>>2)]=(684);
HEAP32[((5248344)>>2)]=(674);
HEAP32[((5248348)>>2)]=(668);
HEAP32[((5248352)>>2)]=(670);
HEAP32[((5248356)>>2)]=(648);
HEAP32[((5248360)>>2)]=(672);
HEAP32[((5248364)>>2)]=(666);
HEAP32[((5248368)>>2)]=(412);
HEAP32[((5248372)>>2)]=(680);
HEAP32[((5248376)>>2)]=(678);
HEAP32[((5248380)>>2)]=(560);
HEAP32[((5248396)>>2)]=(198);
HEAP32[((5248400)>>2)]=(240);
HEAP32[((5248404)>>2)]=(116);
HEAP32[((5248408)>>2)]=(332);
HEAP32[((5248412)>>2)]=(498);
HEAP32[((5248416)>>2)]=(292);
HEAP32[((5248432)>>2)]=(60);
HEAP32[((5248436)>>2)]=(430);
HEAP32[((5248440)>>2)]=(116);
HEAP32[((5248444)>>2)]=(490);
HEAP32[((5248448)>>2)]=(592);
HEAP32[((5248452)>>2)]=(42);
HEAP32[((5248468)>>2)]=(642);
HEAP32[((5248472)>>2)]=(344);
HEAP32[((5248476)>>2)]=(116);
HEAP32[((5248480)>>2)]=(494);
HEAP32[((5248484)>>2)]=(136);
HEAP32[((5248488)>>2)]=(488);
HEAP32[((5248492)>>2)]=(94);
HEAP32[((5248496)>>2)]=(310);
HEAP32[((5248500)>>2)]=(106);
HEAP32[((5248504)>>2)]=(376);
HEAP32[((5248520)>>2)]=(438);
HEAP32[((5248524)>>2)]=(156);
HEAP32[((5248528)>>2)]=(116);
HEAP32[((5248532)>>2)]=(52);
HEAP32[((5248536)>>2)]=(284);
HEAP32[((5248540)>>2)]=(166);
HEAP32[((5248544)>>2)]=(574);
HEAP32[((5248548)>>2)]=(536);
HEAP32[((5248552)>>2)]=(446);
HEAP32[((5248556)>>2)]=(532);
HEAP32[((5248572)>>2)]=(438);
HEAP32[((5248576)>>2)]=(348);
HEAP32[((5248580)>>2)]=(116);
HEAP32[((5248584)>>2)]=(706);
HEAP32[((5248588)>>2)]=(138);
HEAP32[((5248592)>>2)]=(68);
HEAP32[((5248596)>>2)]=(710);
HEAP32[((5248600)>>2)]=(244);
HEAP32[((5248604)>>2)]=(246);
HEAP32[((5248608)>>2)]=(104);
HEAP32[((5248624)>>2)]=(438);
HEAP32[((5248628)>>2)]=(386);
HEAP32[((5248632)>>2)]=(116);
HEAP32[((5248636)>>2)]=(320);
HEAP32[((5248640)>>2)]=(324);
HEAP32[((5248644)>>2)]=(520);
HEAP32[((5248648)>>2)]=(192);
HEAP32[((5248652)>>2)]=(374);
HEAP32[((5248656)>>2)]=(144);
HEAP32[((5248660)>>2)]=(322);
HEAP32[((5248676)>>2)]=(438);
HEAP32[((5248680)>>2)]=(70);
HEAP32[((5248684)>>2)]=(116);
HEAP32[((5248700)>>2)]=(146);
HEAP32[((5248704)>>2)]=(392);
HEAP32[((5248708)>>2)]=(116);
HEAP32[((5248724)>>2)]=(438);
HEAP32[((5248728)>>2)]=(214);
HEAP32[((5248732)>>2)]=(116);
HEAP32[((5248736)>>2)]=(364);
HEAP32[((5248740)>>2)]=(184);
HEAP32[((5248744)>>2)]=(318);
HEAP32[((5248748)>>2)]=(700);
HEAP32[((5248752)>>2)]=(188);
HEAP32[((5248756)>>2)]=(524);
HEAP32[((5248760)>>2)]=(480);
HEAP32[((5248764)>>2)]=(56);
HEAP32[((5248768)>>2)]=(118);
HEAP32[((5248772)>>2)]=(608);
HEAP32[((5248776)>>2)]=(272);
HEAP32[((5248780)>>2)]=(190);
HEAP32[((5248796)>>2)]=(720);
HEAP32[((5248800)>>2)]=(78);
HEAP32[((5248804)>>2)]=(116);
HEAP32[((5248808)>>2)]=(22);
HEAP32[((5248812)>>2)]=(50);
HEAP32[((5248816)>>2)]=(338);
HEAP32[((5248820)>>2)]=(600);
HEAP32[((5248824)>>2)]=(140);
HEAP32[((5248828)>>2)]=(342);
HEAP32[((5248832)>>2)]=(410);
HEAP32[((5248836)>>2)]=(400);
HEAP32[((5248852)>>2)]=(176);
HEAP32[((5248856)>>2)]=(624);
HEAP32[((5248860)>>2)]=(396);
HEAP32[((5248864)>>2)]=(530);
HEAP32[((5248868)>>2)]=(314);
HEAP32[((5248872)>>2)]=(594);
HEAP32[((5248876)>>2)]=(584);
HEAP32[((5248892)>>2)]=(438);
HEAP32[((5248896)>>2)]=(222);
HEAP32[((5248900)>>2)]=(116);
HEAP32[((5248904)>>2)]=(320);
HEAP32[((5248908)>>2)]=(324);
HEAP32[((5248912)>>2)]=(520);
HEAP32[((5248916)>>2)]=(192);
HEAP32[((5248920)>>2)]=(374);
HEAP32[((5248924)>>2)]=(144);
HEAP32[((5248928)>>2)]=(322);
HEAP32[((5248944)>>2)]=(438);
HEAP32[((5248948)>>2)]=(682);
HEAP32[((5248952)>>2)]=(116);
HEAP32[((5248956)>>2)]=(320);
HEAP32[((5248960)>>2)]=(324);
HEAP32[((5248964)>>2)]=(520);
HEAP32[((5248968)>>2)]=(192);
HEAP32[((5248972)>>2)]=(374);
HEAP32[((5248976)>>2)]=(144);
HEAP32[((5248980)>>2)]=(322);
HEAP32[((5248996)>>2)]=(328);
HEAP32[((5249000)>>2)]=(658);
HEAP32[((5249004)>>2)]=(194);
HEAP32[((5249008)>>2)]=(380);
HEAP32[((5249012)>>2)]=(250);
HEAP32[((5249016)>>2)]=(454);
HEAP32[((5249020)>>2)]=(482);
HEAP32[((5249024)>>2)]=(546);
HEAP32[((5249028)>>2)]=(578);
HEAP32[((5249032)>>2)]=(148);
HEAP32[((5249036)>>2)]=(132);
HEAP32[((5249040)>>2)]=(124);
HEAP32[((5249044)>>2)]=(716);
HEAP32[((5249048)>>2)]=(472);
HEAP32[((5249064)>>2)]=(18);
HEAP32[((5249068)>>2)]=(308);
HEAP32[((5249072)>>2)]=(478);
HEAP32[((5249076)>>2)]=(644);
HEAP32[((5249080)>>2)]=(638);
HEAP32[((5249084)>>2)]=(294);
HEAP32[((5249088)>>2)]=(256);
HEAP32[((5249092)>>2)]=(466);
HEAP32[((5249096)>>2)]=(334);
HEAP32[((5249100)>>2)]=(32);
HEAP32[((5249104)>>2)]=(58);
HEAP32[((5249108)>>2)]=(660);
HEAP32[((5249112)>>2)]=(316);
HEAP32[((5249116)>>2)]=(154);
HEAP32[((5249136)>>2)]=(102);
HEAP32[((5249140)>>2)]=(588);
HEAP32[((5249156)>>2)]=(368);
HEAP32[((5249160)>>2)]=(326);
HEAP32[((5249180)>>2)]=(616);
HEAP32[((5249184)>>2)]=(662);
HEAP32[((5249200)>>2)]=(286);
HEAP32[((5249204)>>2)]=(526);
HEAP32[((5249224)>>2)]=(228);
HEAP32[((5249228)>>2)]=(724);
HEAP32[((5249244)>>2)]=(440);
HEAP32[((5249248)>>2)]=(656);
HEAP32[((5249268)>>2)]=(282);
HEAP32[((5249272)>>2)]=(550);
HEAP32[((5249288)>>2)]=(354);
HEAP32[((5249292)>>2)]=(128);
HEAP32[((5249308)>>2)]=(540);
HEAP32[((5249312)>>2)]=(442);
HEAP32[((5249316)>>2)]=(216);
HEAP32[((5249332)>>2)]=(694);
HEAP32[((5249336)>>2)]=(636);
HEAP32[((5249340)>>2)]=(34);
HEAP32[((5249344)>>2)]=(380);
HEAP32[((5249348)>>2)]=(250);
HEAP32[((5249352)>>2)]=(454);
HEAP32[((5249356)>>2)]=(276);
HEAP32[((5249360)>>2)]=(546);
HEAP32[((5249364)>>2)]=(578);
HEAP32[((5249368)>>2)]=(148);
HEAP32[((5249372)>>2)]=(132);
HEAP32[((5249376)>>2)]=(124);
HEAP32[((5249380)>>2)]=(716);
HEAP32[((5249384)>>2)]=(664);
HEAP32[((5249400)>>2)]=(388);
HEAP32[((5249404)>>2)]=(436);
HEAP32[((5249408)>>2)]=(296);
HEAP32[((5249412)>>2)]=(644);
HEAP32[((5249416)>>2)]=(638);
HEAP32[((5249420)>>2)]=(294);
HEAP32[((5249424)>>2)]=(484);
HEAP32[((5249428)>>2)]=(466);
HEAP32[((5249432)>>2)]=(334);
HEAP32[((5249436)>>2)]=(32);
HEAP32[((5249440)>>2)]=(58);
HEAP32[((5249444)>>2)]=(660);
HEAP32[((5249448)>>2)]=(316);
HEAP32[((5249452)>>2)]=(174);
HEAP32[((5249468)>>2)]=(626);
HEAP32[((5249472)>>2)]=(366);
HEAP32[((5249476)>>2)]=(116);
HEAP32[((5249480)>>2)]=(346);
HEAP32[((5249484)>>2)]=(610);
HEAP32[((5249488)>>2)]=(370);
HEAP32[((5249492)>>2)]=(702);
HEAP32[((5249496)>>2)]=(54);
HEAP32[((5249500)>>2)]=(266);
HEAP32[((5249504)>>2)]=(264);
HEAP32[((5249508)>>2)]=(212);
HEAP32[((5249512)>>2)]=(340);
HEAP32[((5249528)>>2)]=(278);
HEAP32[((5249532)>>2)]=(142);
HEAP32[((5249536)>>2)]=(116);
HEAP32[((5249540)>>2)]=(590);
HEAP32[((5249544)>>2)]=(10);
HEAP32[((5249548)>>2)]=(544);
HEAP32[((5249552)>>2)]=(628);
HEAP32[((5249556)>>2)]=(646);
HEAP32[((5249560)>>2)]=(234);
HEAP32[((5249564)>>2)]=(596);
HEAP32[((5249568)>>2)]=(432);
HEAP32[((5249572)>>2)]=(134);
HEAP32[((5249588)>>2)]=(634);
HEAP32[((5249592)>>2)]=(304);
HEAP32[((5249596)>>2)]=(116);
HEAP32[((5249600)>>2)]=(92);
HEAP32[((5249604)>>2)]=(300);
HEAP32[((5249608)>>2)]=(402);
HEAP32[((5249612)>>2)]=(390);
HEAP32[((5249616)>>2)]=(712);
HEAP32[((5249620)>>2)]=(434);
HEAP32[((5249624)>>2)]=(518);
HEAP32[((5249628)>>2)]=(452);
HEAP32[((5249632)>>2)]=(280);
HEAP32[((5249648)>>2)]=(206);
HEAP32[((5249652)>>2)]=(422);
HEAP32[((5249656)>>2)]=(116);
HEAP32[((5249660)>>2)]=(548);
HEAP32[((5249664)>>2)]=(576);
HEAP32[((5249668)>>2)]=(260);
HEAP32[((5249672)>>2)]=(606);
HEAP32[((5249676)>>2)]=(242);
HEAP32[((5249680)>>2)]=(196);
HEAP32[((5249684)>>2)]=(398);
HEAP32[((5249688)>>2)]=(586);
HEAP32[((5249692)>>2)]=(580);
HEAP32[((5249708)>>2)]=(230);
HEAP32[((5249712)>>2)]=(180);
HEAP32[((5249716)>>2)]=(96);
HEAP32[((5249720)>>2)]=(380);
HEAP32[((5249724)>>2)]=(250);
HEAP32[((5249728)>>2)]=(454);
HEAP32[((5249732)>>2)]=(482);
HEAP32[((5249736)>>2)]=(546);
HEAP32[((5249740)>>2)]=(578);
HEAP32[((5249744)>>2)]=(356);
HEAP32[((5249748)>>2)]=(448);
HEAP32[((5249752)>>2)]=(168);
HEAP32[((5249756)>>2)]=(716);
HEAP32[((5249760)>>2)]=(472);
HEAP32[((5249776)>>2)]=(26);
HEAP32[((5249780)>>2)]=(618);
HEAP32[((5249784)>>2)]=(496);
HEAP32[((5249788)>>2)]=(644);
HEAP32[((5249792)>>2)]=(638);
HEAP32[((5249796)>>2)]=(294);
HEAP32[((5249800)>>2)]=(256);
HEAP32[((5249804)>>2)]=(466);
HEAP32[((5249808)>>2)]=(334);
HEAP32[((5249812)>>2)]=(274);
HEAP32[((5249816)>>2)]=(122);
HEAP32[((5249820)>>2)]=(30);
HEAP32[((5249824)>>2)]=(316);
HEAP32[((5249828)>>2)]=(154);
HEAP32[((5249844)>>2)]=(698);
HEAP32[((5249848)>>2)]=(516);
HEAP32[((5249852)>>2)]=(160);
HEAP32[((5249856)>>2)]=(378);
HEAP32[((5249860)>>2)]=(202);
HEAP32[((5249864)>>2)]=(64);
HEAP32[((5249868)>>2)]=(612);
HEAP32[((5249872)>>2)]=(268);
HEAP32[(((__ZTVN10__cxxabiv120__si_class_type_infoE)+(8))>>2)]=(698);
HEAP32[(((__ZTVN10__cxxabiv120__si_class_type_infoE)+(12))>>2)]=(330);
HEAP32[(((__ZTVN10__cxxabiv120__si_class_type_infoE)+(16))>>2)]=(160);
HEAP32[(((__ZTVN10__cxxabiv120__si_class_type_infoE)+(20))>>2)]=(378);
HEAP32[(((__ZTVN10__cxxabiv120__si_class_type_infoE)+(24))>>2)]=(202);
HEAP32[(((__ZTVN10__cxxabiv120__si_class_type_infoE)+(28))>>2)]=(98);
HEAP32[(((__ZTVN10__cxxabiv120__si_class_type_infoE)+(32))>>2)]=(236);
HEAP32[(((__ZTVN10__cxxabiv120__si_class_type_infoE)+(36))>>2)]=(262);
HEAP32[(((__ZTVN10__cxxabiv117__class_type_infoE)+(8))>>2)]=(698);
HEAP32[(((__ZTVN10__cxxabiv117__class_type_infoE)+(12))>>2)]=(690);
HEAP32[(((__ZTVN10__cxxabiv117__class_type_infoE)+(16))>>2)]=(160);
HEAP32[(((__ZTVN10__cxxabiv117__class_type_infoE)+(20))>>2)]=(378);
HEAP32[(((__ZTVN10__cxxabiv117__class_type_infoE)+(24))>>2)]=(202);
HEAP32[(((__ZTVN10__cxxabiv117__class_type_infoE)+(28))>>2)]=(534);
HEAP32[(((__ZTVN10__cxxabiv117__class_type_infoE)+(32))>>2)]=(258);
HEAP32[(((__ZTVN10__cxxabiv117__class_type_infoE)+(36))>>2)]=(394);
HEAP32[((5252680)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5252684)>>2)]=((5249880)|0);
HEAP32[((5252688)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5252692)>>2)]=((5249896)|0);
HEAP32[((5252696)>>2)]=__ZTISt9exception;
HEAP32[((5252700)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5252704)>>2)]=((5249912)|0);
HEAP32[((5252708)>>2)]=__ZTISt9exception;
HEAP32[((5252712)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5252716)>>2)]=((5249924)|0);
HEAP32[((5252720)>>2)]=__ZTISt9exception;
HEAP32[((5252724)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5252728)>>2)]=((5249944)|0);
HEAP32[((5252736)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5252740)>>2)]=((5249964)|0);
HEAP32[((5252744)>>2)]=__ZTISt9exception;
HEAP32[((5252748)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5252752)>>2)]=((5249980)|0);
HEAP32[((5252756)>>2)]=(((5249844)|0));
HEAP32[((5252760)>>2)]=((5250000)|0);
HEAP32[((5252788)>>2)]=(((5249844)|0));
HEAP32[((5252792)>>2)]=((5250072)|0);
HEAP32[((5252820)>>2)]=(((5249844)|0));
HEAP32[((5252824)>>2)]=((5250144)|0);
HEAP32[((5252852)>>2)]=(((5249844)|0));
HEAP32[((5252856)>>2)]=((5250216)|0);
HEAP32[((5252884)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5252888)>>2)]=((5250288)|0);
HEAP32[((5252896)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5252900)>>2)]=((5250332)|0);
HEAP32[((5252908)>>2)]=(((5249844)|0));
HEAP32[((5252912)>>2)]=((5250376)|0);
HEAP32[((5252932)>>2)]=(((5249844)|0));
HEAP32[((5252936)>>2)]=((5250400)|0);
HEAP32[((5252956)>>2)]=(((5249844)|0));
HEAP32[((5252960)>>2)]=((5250424)|0);
HEAP32[((5252980)>>2)]=(((5249844)|0));
HEAP32[((5252984)>>2)]=((5250448)|0);
HEAP32[((5253004)>>2)]=(((5249844)|0));
HEAP32[((5253008)>>2)]=((5250472)|0);
HEAP32[((5253036)>>2)]=(((5249844)|0));
HEAP32[((5253040)>>2)]=((5250544)|0);
HEAP32[((5253068)>>2)]=(((5249844)|0));
HEAP32[((5253072)>>2)]=((5250616)|0);
HEAP32[((5253108)>>2)]=(((5249844)|0));
HEAP32[((5253112)>>2)]=((5250688)|0);
HEAP32[((5253148)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5253152)>>2)]=((5250760)|0);
HEAP32[((5253160)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5253164)>>2)]=((5250784)|0);
HEAP32[((5253172)>>2)]=(((5249844)|0));
HEAP32[((5253176)>>2)]=((5250808)|0);
HEAP32[((5253204)>>2)]=(((5249844)|0));
HEAP32[((5253208)>>2)]=((5250832)|0);
HEAP32[((5253236)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5253240)>>2)]=((5250856)|0);
HEAP32[((5253244)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5253248)>>2)]=((5250876)|0);
HEAP32[((5253256)>>2)]=(((5249844)|0));
HEAP32[((5253260)>>2)]=((5250904)|0);
HEAP32[((5253288)>>2)]=(((5249844)|0));
HEAP32[((5253292)>>2)]=((5250972)|0);
HEAP32[((5253320)>>2)]=(((5249844)|0));
HEAP32[((5253324)>>2)]=((5251040)|0);
HEAP32[((5253352)>>2)]=(((5249844)|0));
HEAP32[((5253356)>>2)]=((5251108)|0);
HEAP32[((5253384)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5253388)>>2)]=((5251176)|0);
HEAP32[((5253396)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5253400)>>2)]=((5251196)|0);
HEAP32[((5253408)>>2)]=(((5249844)|0));
HEAP32[((5253412)>>2)]=((5251216)|0);
HEAP32[((5253440)>>2)]=(((5249844)|0));
HEAP32[((5253444)>>2)]=((5251252)|0);
HEAP32[((5253472)>>2)]=(((5249844)|0));
HEAP32[((5253476)>>2)]=((5251288)|0);
HEAP32[((5253504)>>2)]=(((5249844)|0));
HEAP32[((5253508)>>2)]=((5251324)|0);
HEAP32[((5253536)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5253540)>>2)]=((5251360)|0);
HEAP32[((5253548)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5253552)>>2)]=((5251384)|0);
HEAP32[((5253560)>>2)]=(((5249844)|0));
HEAP32[((5253564)>>2)]=((5251408)|0);
HEAP32[((5253592)>>2)]=(((5249844)|0));
HEAP32[((5253596)>>2)]=((5251428)|0);
HEAP32[((5253624)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5253628)>>2)]=((5251448)|0);
HEAP32[((5253632)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5253636)>>2)]=((5251484)|0);
HEAP32[((5253640)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5253644)>>2)]=((5251520)|0);
HEAP32[((5253652)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5253656)>>2)]=((5251552)|0);
HEAP32[((5253664)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5253668)>>2)]=((5251588)|0);
HEAP32[((5253676)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5253680)>>2)]=((5251624)|0);
HEAP32[((5253684)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5253688)>>2)]=((5251676)|0);
HEAP32[((5253692)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5253696)>>2)]=((5251728)|0);
HEAP32[((5253700)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5253704)>>2)]=((5251756)|0);
HEAP32[((5253708)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5253712)>>2)]=((5251784)|0);
HEAP32[((5253716)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5253720)>>2)]=((5251812)|0);
HEAP32[((5253724)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5253728)>>2)]=((5251840)|0);
HEAP32[((5253732)>>2)]=(((5249844)|0));
HEAP32[((5253736)>>2)]=((5251864)|0);
HEAP32[((5253756)>>2)]=(((5249844)|0));
HEAP32[((5253760)>>2)]=((5251912)|0);
HEAP32[((5253780)>>2)]=(((5249844)|0));
HEAP32[((5253784)>>2)]=((5251960)|0);
HEAP32[((5253804)>>2)]=(((5249844)|0));
HEAP32[((5253808)>>2)]=((5252008)|0);
HEAP32[((5253828)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5253832)>>2)]=((5252056)|0);
HEAP32[((5253840)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5253844)>>2)]=((5252080)|0);
HEAP32[((5253848)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5253852)>>2)]=((5252104)|0);
HEAP32[((5253860)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5253864)>>2)]=((5252128)|0);
HEAP32[((5253872)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5253876)>>2)]=((5252156)|0);
HEAP32[((5253884)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5253888)>>2)]=((5252184)|0);
HEAP32[((5253892)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5253896)>>2)]=((5252212)|0);
HEAP32[((5253900)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5253904)>>2)]=((5252240)|0);
HEAP32[((5253908)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5253912)>>2)]=((5252268)|0);
HEAP32[((5253916)>>2)]=(((5249844)|0));
HEAP32[((5253920)>>2)]=((5252296)|0);
HEAP32[((5253948)>>2)]=(((5249844)|0));
HEAP32[((5253952)>>2)]=((5252324)|0);
HEAP32[((5253980)>>2)]=(((5249844)|0));
HEAP32[((5253984)>>2)]=((5252352)|0);
HEAP32[((5254012)>>2)]=(((5249844)|0));
HEAP32[((5254016)>>2)]=((5252380)|0);
HEAP32[((5254044)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5254048)>>2)]=((5252408)|0);
HEAP32[((5254052)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5254056)>>2)]=((5252432)|0);
HEAP32[((5254060)>>2)]=(((__ZTVN10__cxxabiv117__class_type_infoE+8)|0));
HEAP32[((5254064)>>2)]=((5252456)|0);
HEAP32[((5254068)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5254072)>>2)]=((5252480)|0);
HEAP32[((5254080)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5254084)>>2)]=((5252504)|0);
HEAP32[((5254092)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5254096)>>2)]=((5252528)|0);
HEAP32[((5254104)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5254108)>>2)]=((5252568)|0);
HEAP32[((5254116)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5254120)>>2)]=((5252608)|0);
HEAP32[((5254128)>>2)]=(((__ZTVN10__cxxabiv120__si_class_type_infoE+8)|0));
HEAP32[((5254132)>>2)]=((5252644)|0);
  function _memcpy(dest, src, num) {
      dest = dest|0; src = src|0; num = num|0;
      var ret = 0;
      ret = dest|0;
      if ((dest&3) == (src&3)) {
        while (dest & 3) {
          if ((num|0) == 0) return ret|0;
          HEAP8[(dest)]=HEAP8[(src)];
          dest = (dest+1)|0;
          src = (src+1)|0;
          num = (num-1)|0;
        }
        while ((num|0) >= 4) {
          HEAP32[((dest)>>2)]=HEAP32[((src)>>2)];
          dest = (dest+4)|0;
          src = (src+4)|0;
          num = (num-4)|0;
        }
      }
      while ((num|0) > 0) {
        HEAP8[(dest)]=HEAP8[(src)];
        dest = (dest+1)|0;
        src = (src+1)|0;
        num = (num-1)|0;
      }
      return ret|0;
    }var _llvm_memcpy_p0i8_p0i8_i32=_memcpy;
  var ERRNO_CODES={E2BIG:7,EACCES:13,EADDRINUSE:98,EADDRNOTAVAIL:99,EAFNOSUPPORT:97,EAGAIN:11,EALREADY:114,EBADF:9,EBADMSG:74,EBUSY:16,ECANCELED:125,ECHILD:10,ECONNABORTED:103,ECONNREFUSED:111,ECONNRESET:104,EDEADLK:35,EDESTADDRREQ:89,EDOM:33,EDQUOT:122,EEXIST:17,EFAULT:14,EFBIG:27,EHOSTUNREACH:113,EIDRM:43,EILSEQ:84,EINPROGRESS:115,EINTR:4,EINVAL:22,EIO:5,EISCONN:106,EISDIR:21,ELOOP:40,EMFILE:24,EMLINK:31,EMSGSIZE:90,EMULTIHOP:72,ENAMETOOLONG:36,ENETDOWN:100,ENETRESET:102,ENETUNREACH:101,ENFILE:23,ENOBUFS:105,ENODATA:61,ENODEV:19,ENOENT:2,ENOEXEC:8,ENOLCK:37,ENOLINK:67,ENOMEM:12,ENOMSG:42,ENOPROTOOPT:92,ENOSPC:28,ENOSR:63,ENOSTR:60,ENOSYS:38,ENOTCONN:107,ENOTDIR:20,ENOTEMPTY:39,ENOTRECOVERABLE:131,ENOTSOCK:88,ENOTSUP:95,ENOTTY:25,ENXIO:6,EOVERFLOW:75,EOWNERDEAD:130,EPERM:1,EPIPE:32,EPROTO:71,EPROTONOSUPPORT:93,EPROTOTYPE:91,ERANGE:34,EROFS:30,ESPIPE:29,ESRCH:3,ESTALE:116,ETIME:62,ETIMEDOUT:110,ETXTBSY:26,EWOULDBLOCK:11,EXDEV:18};
  function ___setErrNo(value) {
      // For convenient setting and returning of errno.
      if (!___setErrNo.ret) ___setErrNo.ret = allocate([0], 'i32', ALLOC_STATIC);
      HEAP32[((___setErrNo.ret)>>2)]=value
      return value;
    }
  var _stdin=allocate(1, "i32*", ALLOC_STACK);
  var _stdout=allocate(1, "i32*", ALLOC_STACK);
  var _stderr=allocate(1, "i32*", ALLOC_STACK);
  var __impure_ptr=allocate(1, "i32*", ALLOC_STACK);var FS={currentPath:"/",nextInode:2,streams:[null],ignorePermissions:true,joinPath:function (parts, forceRelative) {
        var ret = parts[0];
        for (var i = 1; i < parts.length; i++) {
          if (ret[ret.length-1] != '/') ret += '/';
          ret += parts[i];
        }
        if (forceRelative && ret[0] == '/') ret = ret.substr(1);
        return ret;
      },absolutePath:function (relative, base) {
        if (typeof relative !== 'string') return null;
        if (base === undefined) base = FS.currentPath;
        if (relative && relative[0] == '/') base = '';
        var full = base + '/' + relative;
        var parts = full.split('/').reverse();
        var absolute = [''];
        while (parts.length) {
          var part = parts.pop();
          if (part == '' || part == '.') {
            // Nothing.
          } else if (part == '..') {
            if (absolute.length > 1) absolute.pop();
          } else {
            absolute.push(part);
          }
        }
        return absolute.length == 1 ? '/' : absolute.join('/');
      },analyzePath:function (path, dontResolveLastLink, linksVisited) {
        var ret = {
          isRoot: false,
          exists: false,
          error: 0,
          name: null,
          path: null,
          object: null,
          parentExists: false,
          parentPath: null,
          parentObject: null
        };
        path = FS.absolutePath(path);
        if (path == '/') {
          ret.isRoot = true;
          ret.exists = ret.parentExists = true;
          ret.name = '/';
          ret.path = ret.parentPath = '/';
          ret.object = ret.parentObject = FS.root;
        } else if (path !== null) {
          linksVisited = linksVisited || 0;
          path = path.slice(1).split('/');
          var current = FS.root;
          var traversed = [''];
          while (path.length) {
            if (path.length == 1 && current.isFolder) {
              ret.parentExists = true;
              ret.parentPath = traversed.length == 1 ? '/' : traversed.join('/');
              ret.parentObject = current;
              ret.name = path[0];
            }
            var target = path.shift();
            if (!current.isFolder) {
              ret.error = ERRNO_CODES.ENOTDIR;
              break;
            } else if (!current.read) {
              ret.error = ERRNO_CODES.EACCES;
              break;
            } else if (!current.contents.hasOwnProperty(target)) {
              ret.error = ERRNO_CODES.ENOENT;
              break;
            }
            current = current.contents[target];
            if (current.link && !(dontResolveLastLink && path.length == 0)) {
              if (linksVisited > 40) { // Usual Linux SYMLOOP_MAX.
                ret.error = ERRNO_CODES.ELOOP;
                break;
              }
              var link = FS.absolutePath(current.link, traversed.join('/'));
              ret = FS.analyzePath([link].concat(path).join('/'),
                                   dontResolveLastLink, linksVisited + 1);
              return ret;
            }
            traversed.push(target);
            if (path.length == 0) {
              ret.exists = true;
              ret.path = traversed.join('/');
              ret.object = current;
            }
          }
        }
        return ret;
      },findObject:function (path, dontResolveLastLink) {
        FS.ensureRoot();
        var ret = FS.analyzePath(path, dontResolveLastLink);
        if (ret.exists) {
          return ret.object;
        } else {
          ___setErrNo(ret.error);
          return null;
        }
      },createObject:function (parent, name, properties, canRead, canWrite) {
        if (!parent) parent = '/';
        if (typeof parent === 'string') parent = FS.findObject(parent);
        if (!parent) {
          ___setErrNo(ERRNO_CODES.EACCES);
          throw new Error('Parent path must exist.');
        }
        if (!parent.isFolder) {
          ___setErrNo(ERRNO_CODES.ENOTDIR);
          throw new Error('Parent must be a folder.');
        }
        if (!parent.write && !FS.ignorePermissions) {
          ___setErrNo(ERRNO_CODES.EACCES);
          throw new Error('Parent folder must be writeable.');
        }
        if (!name || name == '.' || name == '..') {
          ___setErrNo(ERRNO_CODES.ENOENT);
          throw new Error('Name must not be empty.');
        }
        if (parent.contents.hasOwnProperty(name)) {
          ___setErrNo(ERRNO_CODES.EEXIST);
          throw new Error("Can't overwrite object.");
        }
        parent.contents[name] = {
          read: canRead === undefined ? true : canRead,
          write: canWrite === undefined ? false : canWrite,
          timestamp: Date.now(),
          inodeNumber: FS.nextInode++
        };
        for (var key in properties) {
          if (properties.hasOwnProperty(key)) {
            parent.contents[name][key] = properties[key];
          }
        }
        return parent.contents[name];
      },createFolder:function (parent, name, canRead, canWrite) {
        var properties = {isFolder: true, isDevice: false, contents: {}};
        return FS.createObject(parent, name, properties, canRead, canWrite);
      },createPath:function (parent, path, canRead, canWrite) {
        var current = FS.findObject(parent);
        if (current === null) throw new Error('Invalid parent.');
        path = path.split('/').reverse();
        while (path.length) {
          var part = path.pop();
          if (!part) continue;
          if (!current.contents.hasOwnProperty(part)) {
            FS.createFolder(current, part, canRead, canWrite);
          }
          current = current.contents[part];
        }
        return current;
      },createFile:function (parent, name, properties, canRead, canWrite) {
        properties.isFolder = false;
        return FS.createObject(parent, name, properties, canRead, canWrite);
      },createDataFile:function (parent, name, data, canRead, canWrite) {
        if (typeof data === 'string') {
          var dataArray = new Array(data.length);
          for (var i = 0, len = data.length; i < len; ++i) dataArray[i] = data.charCodeAt(i);
          data = dataArray;
        }
        var properties = {
          isDevice: false,
          contents: data.subarray ? data.subarray(0) : data // as an optimization, create a new array wrapper (not buffer) here, to help JS engines understand this object
        };
        return FS.createFile(parent, name, properties, canRead, canWrite);
      },createLazyFile:function (parent, name, url, canRead, canWrite) {
        if (typeof XMLHttpRequest !== 'undefined') {
          if (!ENVIRONMENT_IS_WORKER) throw 'Cannot do synchronous binary XHRs outside webworkers in modern browsers. Use --embed-file or --preload-file in emcc';
          // Lazy chunked Uint8Array (implements get and length from Uint8Array). Actual getting is abstracted away for eventual reuse.
          var LazyUint8Array = function(chunkSize, length) {
            this.length = length;
            this.chunkSize = chunkSize;
            this.chunks = []; // Loaded chunks. Index is the chunk number
          }
          LazyUint8Array.prototype.get = function(idx) {
            if (idx > this.length-1 || idx < 0) {
              return undefined;
            }
            var chunkOffset = idx % chunkSize;
            var chunkNum = Math.floor(idx / chunkSize);
            return this.getter(chunkNum)[chunkOffset];
          }
          LazyUint8Array.prototype.setDataGetter = function(getter) {
            this.getter = getter;
          }
          // Find length
          var xhr = new XMLHttpRequest();
          xhr.open('HEAD', url, false);
          xhr.send(null);
          if (!(xhr.status >= 200 && xhr.status < 300 || xhr.status === 304)) throw new Error("Couldn't load " + url + ". Status: " + xhr.status);
          var datalength = Number(xhr.getResponseHeader("Content-length"));
          var header;
          var hasByteServing = (header = xhr.getResponseHeader("Accept-Ranges")) && header === "bytes";
          var chunkSize = 1024*1024; // Chunk size in bytes
          if (!hasByteServing) chunkSize = datalength;
          // Function to get a range from the remote URL.
          var doXHR = (function(from, to) {
            if (from > to) throw new Error("invalid range (" + from + ", " + to + ") or no bytes requested!");
            if (to > datalength-1) throw new Error("only " + datalength + " bytes available! programmer error!");
            // TODO: Use mozResponseArrayBuffer, responseStream, etc. if available.
            var xhr = new XMLHttpRequest();
            xhr.open('GET', url, false);
            if (datalength !== chunkSize) xhr.setRequestHeader("Range", "bytes=" + from + "-" + to);
            // Some hints to the browser that we want binary data.
            if (typeof Uint8Array != 'undefined') xhr.responseType = 'arraybuffer';
            if (xhr.overrideMimeType) {
              xhr.overrideMimeType('text/plain; charset=x-user-defined');
            }
            xhr.send(null);
            if (!(xhr.status >= 200 && xhr.status < 300 || xhr.status === 304)) throw new Error("Couldn't load " + url + ". Status: " + xhr.status);
            if (xhr.response !== undefined) {
              return new Uint8Array(xhr.response || []);
            } else {
              return intArrayFromString(xhr.responseText || '', true);
            }
          });
          var lazyArray = new LazyUint8Array(chunkSize, datalength);
          lazyArray.setDataGetter(function(chunkNum) {
            var start = chunkNum * lazyArray.chunkSize;
            var end = (chunkNum+1) * lazyArray.chunkSize - 1; // including this byte
            end = Math.min(end, datalength-1); // if datalength-1 is selected, this is the last block
            if (typeof(lazyArray.chunks[chunkNum]) === "undefined") {
              lazyArray.chunks[chunkNum] = doXHR(start, end);
            }
            if (typeof(lazyArray.chunks[chunkNum]) === "undefined") throw new Error("doXHR failed!");
            return lazyArray.chunks[chunkNum];
          });
          var properties = { isDevice: false, contents: lazyArray };
        } else {
          var properties = { isDevice: false, url: url };
        }
        return FS.createFile(parent, name, properties, canRead, canWrite);
      },createPreloadedFile:function (parent, name, url, canRead, canWrite, onload, onerror, dontCreateFile) {
        Browser.init();
        var fullname = FS.joinPath([parent, name], true);
        function processData(byteArray) {
          function finish(byteArray) {
            if (!dontCreateFile) {
              FS.createDataFile(parent, name, byteArray, canRead, canWrite);
            }
            if (onload) onload();
            removeRunDependency('cp ' + fullname);
          }
          var handled = false;
          Module['preloadPlugins'].forEach(function(plugin) {
            if (handled) return;
            if (plugin['canHandle'](fullname)) {
              plugin['handle'](byteArray, fullname, finish, function() {
                if (onerror) onerror();
                removeRunDependency('cp ' + fullname);
              });
              handled = true;
            }
          });
          if (!handled) finish(byteArray);
        }
        addRunDependency('cp ' + fullname);
        if (typeof url == 'string') {
          Browser.asyncLoad(url, function(byteArray) {
            processData(byteArray);
          }, onerror);
        } else {
          processData(url);
        }
      },createLink:function (parent, name, target, canRead, canWrite) {
        var properties = {isDevice: false, link: target};
        return FS.createFile(parent, name, properties, canRead, canWrite);
      },createDevice:function (parent, name, input, output) {
        if (!(input || output)) {
          throw new Error('A device must have at least one callback defined.');
        }
        var ops = {isDevice: true, input: input, output: output};
        return FS.createFile(parent, name, ops, Boolean(input), Boolean(output));
      },forceLoadFile:function (obj) {
        if (obj.isDevice || obj.isFolder || obj.link || obj.contents) return true;
        var success = true;
        if (typeof XMLHttpRequest !== 'undefined') {
          throw new Error("Lazy loading should have been performed (contents set) in createLazyFile, but it was not. Lazy loading only works in web workers. Use --embed-file or --preload-file in emcc on the main thread.");
        } else if (Module['read']) {
          // Command-line.
          try {
            // WARNING: Can't read binary files in V8's d8 or tracemonkey's js, as
            //          read() will try to parse UTF8.
            obj.contents = intArrayFromString(Module['read'](obj.url), true);
          } catch (e) {
            success = false;
          }
        } else {
          throw new Error('Cannot load without read() or XMLHttpRequest.');
        }
        if (!success) ___setErrNo(ERRNO_CODES.EIO);
        return success;
      },ensureRoot:function () {
        if (FS.root) return;
        // The main file system tree. All the contents are inside this.
        FS.root = {
          read: true,
          write: true,
          isFolder: true,
          isDevice: false,
          timestamp: Date.now(),
          inodeNumber: 1,
          contents: {}
        };
      },init:function (input, output, error) {
        // Make sure we initialize only once.
        assert(!FS.init.initialized, 'FS.init was previously called. If you want to initialize later with custom parameters, remove any earlier calls (note that one is automatically added to the generated code)');
        FS.init.initialized = true;
        FS.ensureRoot();
        // Allow Module.stdin etc. to provide defaults, if none explicitly passed to us here
        input = input || Module['stdin'];
        output = output || Module['stdout'];
        error = error || Module['stderr'];
        // Default handlers.
        var stdinOverridden = true, stdoutOverridden = true, stderrOverridden = true;
        if (!input) {
          stdinOverridden = false;
          input = function() {
            if (!input.cache || !input.cache.length) {
              var result;
              if (typeof window != 'undefined' &&
                  typeof window.prompt == 'function') {
                // Browser.
                result = window.prompt('Input: ');
                if (result === null) result = String.fromCharCode(0); // cancel ==> EOF
              } else if (typeof readline == 'function') {
                // Command line.
                result = readline();
              }
              if (!result) result = '';
              input.cache = intArrayFromString(result + '\n', true);
            }
            return input.cache.shift();
          };
        }
        var utf8 = new Runtime.UTF8Processor();
        function simpleOutput(val) {
          if (val === null || val === 10) {
            output.printer(output.buffer.join(''));
            output.buffer = [];
          } else {
            output.buffer.push(utf8.processCChar(val));
          }
        }
        if (!output) {
          stdoutOverridden = false;
          output = simpleOutput;
        }
        if (!output.printer) output.printer = Module['print'];
        if (!output.buffer) output.buffer = [];
        if (!error) {
          stderrOverridden = false;
          error = simpleOutput;
        }
        if (!error.printer) error.printer = Module['print'];
        if (!error.buffer) error.buffer = [];
        // Create the temporary folder, if not already created
        try {
          FS.createFolder('/', 'tmp', true, true);
        } catch(e) {}
        // Create the I/O devices.
        var devFolder = FS.createFolder('/', 'dev', true, true);
        var stdin = FS.createDevice(devFolder, 'stdin', input);
        var stdout = FS.createDevice(devFolder, 'stdout', null, output);
        var stderr = FS.createDevice(devFolder, 'stderr', null, error);
        FS.createDevice(devFolder, 'tty', input, output);
        // Create default streams.
        FS.streams[1] = {
          path: '/dev/stdin',
          object: stdin,
          position: 0,
          isRead: true,
          isWrite: false,
          isAppend: false,
          isTerminal: !stdinOverridden,
          error: false,
          eof: false,
          ungotten: []
        };
        FS.streams[2] = {
          path: '/dev/stdout',
          object: stdout,
          position: 0,
          isRead: false,
          isWrite: true,
          isAppend: false,
          isTerminal: !stdoutOverridden,
          error: false,
          eof: false,
          ungotten: []
        };
        FS.streams[3] = {
          path: '/dev/stderr',
          object: stderr,
          position: 0,
          isRead: false,
          isWrite: true,
          isAppend: false,
          isTerminal: !stderrOverridden,
          error: false,
          eof: false,
          ungotten: []
        };
        assert(Math.max(_stdin, _stdout, _stderr) < 128); // make sure these are low, we flatten arrays with these
        HEAP32[((_stdin)>>2)]=1;
        HEAP32[((_stdout)>>2)]=2;
        HEAP32[((_stderr)>>2)]=3;
        // Other system paths
        FS.createPath('/', 'dev/shm/tmp', true, true); // temp files
        // Newlib initialization
        for (var i = FS.streams.length; i < Math.max(_stdin, _stdout, _stderr) + 4; i++) {
          FS.streams[i] = null; // Make sure to keep FS.streams dense
        }
        FS.streams[_stdin] = FS.streams[1];
        FS.streams[_stdout] = FS.streams[2];
        FS.streams[_stderr] = FS.streams[3];
        allocate([ allocate(
          [0, 0, 0, 0, _stdin, 0, 0, 0, _stdout, 0, 0, 0, _stderr, 0, 0, 0],
          'void*', ALLOC_STATIC) ], 'void*', ALLOC_NONE, __impure_ptr);
      },quit:function () {
        if (!FS.init.initialized) return;
        // Flush any partially-printed lines in stdout and stderr. Careful, they may have been closed
        if (FS.streams[2] && FS.streams[2].object.output.buffer.length > 0) FS.streams[2].object.output(10);
        if (FS.streams[3] && FS.streams[3].object.output.buffer.length > 0) FS.streams[3].object.output(10);
      },standardizePath:function (path) {
        if (path.substr(0, 2) == './') path = path.substr(2);
        return path;
      },deleteFile:function (path) {
        path = FS.analyzePath(path);
        if (!path.parentExists || !path.exists) {
          throw 'Invalid path ' + path;
        }
        delete path.parentObject.contents[path.name];
      }};
  function _pread(fildes, buf, nbyte, offset) {
      // ssize_t pread(int fildes, void *buf, size_t nbyte, off_t offset);
      // http://pubs.opengroup.org/onlinepubs/000095399/functions/read.html
      var stream = FS.streams[fildes];
      if (!stream || stream.object.isDevice) {
        ___setErrNo(ERRNO_CODES.EBADF);
        return -1;
      } else if (!stream.isRead) {
        ___setErrNo(ERRNO_CODES.EACCES);
        return -1;
      } else if (stream.object.isFolder) {
        ___setErrNo(ERRNO_CODES.EISDIR);
        return -1;
      } else if (nbyte < 0 || offset < 0) {
        ___setErrNo(ERRNO_CODES.EINVAL);
        return -1;
      } else {
        var bytesRead = 0;
        while (stream.ungotten.length && nbyte > 0) {
          HEAP8[((buf++)|0)]=stream.ungotten.pop()
          nbyte--;
          bytesRead++;
        }
        var contents = stream.object.contents;
        var size = Math.min(contents.length - offset, nbyte);
        if (contents.subarray) { // typed array
          HEAPU8.set(contents.subarray(offset, offset+size), buf);
        } else
        if (contents.slice) { // normal array
          for (var i = 0; i < size; i++) {
            HEAP8[(((buf)+(i))|0)]=contents[offset + i]
          }
        } else {
          for (var i = 0; i < size; i++) { // LazyUint8Array from sync binary XHR
            HEAP8[(((buf)+(i))|0)]=contents.get(offset + i)
          }
        }
        bytesRead += size;
        return bytesRead;
      }
    }function _read(fildes, buf, nbyte) {
      // ssize_t read(int fildes, void *buf, size_t nbyte);
      // http://pubs.opengroup.org/onlinepubs/000095399/functions/read.html
      var stream = FS.streams[fildes];
      if (!stream) {
        ___setErrNo(ERRNO_CODES.EBADF);
        return -1;
      } else if (!stream.isRead) {
        ___setErrNo(ERRNO_CODES.EACCES);
        return -1;
      } else if (nbyte < 0) {
        ___setErrNo(ERRNO_CODES.EINVAL);
        return -1;
      } else {
        var bytesRead;
        if (stream.object.isDevice) {
          if (stream.object.input) {
            bytesRead = 0;
            while (stream.ungotten.length && nbyte > 0) {
              HEAP8[((buf++)|0)]=stream.ungotten.pop()
              nbyte--;
              bytesRead++;
            }
            for (var i = 0; i < nbyte; i++) {
              try {
                var result = stream.object.input();
              } catch (e) {
                ___setErrNo(ERRNO_CODES.EIO);
                return -1;
              }
              if (result === null || result === undefined) break;
              bytesRead++;
              HEAP8[(((buf)+(i))|0)]=result
            }
            return bytesRead;
          } else {
            ___setErrNo(ERRNO_CODES.ENXIO);
            return -1;
          }
        } else {
          var ungotSize = stream.ungotten.length;
          bytesRead = _pread(fildes, buf, nbyte, stream.position);
          if (bytesRead != -1) {
            stream.position += (stream.ungotten.length - ungotSize) + bytesRead;
          }
          return bytesRead;
        }
      }
    }function _fgetc(stream) {
      // int fgetc(FILE *stream);
      // http://pubs.opengroup.org/onlinepubs/000095399/functions/fgetc.html
      if (!FS.streams[stream]) return -1;
      var streamObj = FS.streams[stream];
      if (streamObj.eof || streamObj.error) return -1;
      var ret = _read(stream, _fgetc.ret, 1);
      if (ret == 0) {
        streamObj.eof = true;
        return -1;
      } else if (ret == -1) {
        streamObj.error = true;
        return -1;
      } else {
        return HEAPU8[((_fgetc.ret)|0)];
      }
    }var _getc=_fgetc;
  function ___errno_location() {
      return ___setErrNo.ret;
    }var ___errno=___errno_location;
  function _strlen(ptr) {
      ptr = ptr|0;
      var curr = 0;
      curr = ptr;
      while (HEAP8[(curr)]|0 != 0) {
        curr = (curr + 1)|0;
      }
      return (curr - ptr)|0;
    }
  function ___gxx_personality_v0() {
    }
  function __exit(status) {
      // void _exit(int status);
      // http://pubs.opengroup.org/onlinepubs/000095399/functions/exit.html
      function ExitStatus() {
        this.name = "ExitStatus";
        this.message = "Program terminated with exit(" + status + ")";
        this.status = status;
        Module.print('Exit Status: ' + status);
      };
      ExitStatus.prototype = new Error();
      ExitStatus.prototype.constructor = ExitStatus;
      exitRuntime();
      ABORT = true;
      throw new ExitStatus();
    }function _exit(status) {
      __exit(status);
    }function __ZSt9terminatev() {
      _exit(-1234);
    }
  function _wcslen() { throw 'wcslen not implemented' }
  function _fflush(stream) {
      // int fflush(FILE *stream);
      // http://pubs.opengroup.org/onlinepubs/000095399/functions/fflush.html
      var flush = function(filedes) {
        // Right now we write all data directly, except for output devices.
        if (FS.streams[filedes] && FS.streams[filedes].object.output) {
          if (!FS.streams[filedes].isTerminal) { // don't flush terminals, it would cause a \n to also appear
            FS.streams[filedes].object.output(null);
          }
        }
      };
      try {
        if (stream === 0) {
          for (var i = 0; i < FS.streams.length; i++) if (FS.streams[i]) flush(i);
        } else {
          flush(stream);
        }
        return 0;
      } catch (e) {
        ___setErrNo(ERRNO_CODES.EIO);
        return -1;
      }
    }
  function _pwrite(fildes, buf, nbyte, offset) {
      // ssize_t pwrite(int fildes, const void *buf, size_t nbyte, off_t offset);
      // http://pubs.opengroup.org/onlinepubs/000095399/functions/write.html
      var stream = FS.streams[fildes];
      if (!stream || stream.object.isDevice) {
        ___setErrNo(ERRNO_CODES.EBADF);
        return -1;
      } else if (!stream.isWrite) {
        ___setErrNo(ERRNO_CODES.EACCES);
        return -1;
      } else if (stream.object.isFolder) {
        ___setErrNo(ERRNO_CODES.EISDIR);
        return -1;
      } else if (nbyte < 0 || offset < 0) {
        ___setErrNo(ERRNO_CODES.EINVAL);
        return -1;
      } else {
        var contents = stream.object.contents;
        while (contents.length < offset) contents.push(0);
        for (var i = 0; i < nbyte; i++) {
          contents[offset + i] = HEAPU8[(((buf)+(i))|0)];
        }
        stream.object.timestamp = Date.now();
        return i;
      }
    }function _write(fildes, buf, nbyte) {
      // ssize_t write(int fildes, const void *buf, size_t nbyte);
      // http://pubs.opengroup.org/onlinepubs/000095399/functions/write.html
      var stream = FS.streams[fildes];
      if (!stream) {
        ___setErrNo(ERRNO_CODES.EBADF);
        return -1;
      } else if (!stream.isWrite) {
        ___setErrNo(ERRNO_CODES.EACCES);
        return -1;
      } else if (nbyte < 0) {
        ___setErrNo(ERRNO_CODES.EINVAL);
        return -1;
      } else {
        if (stream.object.isDevice) {
          if (stream.object.output) {
            for (var i = 0; i < nbyte; i++) {
              try {
                stream.object.output(HEAP8[(((buf)+(i))|0)]);
              } catch (e) {
                ___setErrNo(ERRNO_CODES.EIO);
                return -1;
              }
            }
            stream.object.timestamp = Date.now();
            return i;
          } else {
            ___setErrNo(ERRNO_CODES.ENXIO);
            return -1;
          }
        } else {
          var bytesWritten = _pwrite(fildes, buf, nbyte, stream.position);
          if (bytesWritten != -1) stream.position += bytesWritten;
          return bytesWritten;
        }
      }
    }function _fwrite(ptr, size, nitems, stream) {
      // size_t fwrite(const void *restrict ptr, size_t size, size_t nitems, FILE *restrict stream);
      // http://pubs.opengroup.org/onlinepubs/000095399/functions/fwrite.html
      var bytesToWrite = nitems * size;
      if (bytesToWrite == 0) return 0;
      var bytesWritten = _write(stream, ptr, bytesToWrite);
      if (bytesWritten == -1) {
        if (FS.streams[stream]) FS.streams[stream].error = true;
        return 0;
      } else {
        return Math.floor(bytesWritten / size);
      }
    }
  function _pthread_mutex_lock() {}
  function _pthread_mutex_unlock() {}
  function ___cxa_guard_acquire(variable) {
      if (!HEAP8[(variable)]) { // ignore SAFE_HEAP stuff because llvm mixes i64 and i8 here
        HEAP8[(variable)]=1;
        return 1;
      }
      return 0;
    }
  function ___cxa_guard_abort() {}
  function ___cxa_guard_release() {}
  function ___cxa_call_unexpected(exception) {
      Module.printErr('Unexpected exception thrown, this is not properly supported - aborting');
      ABORT = true;
      throw exception;
    }
  function _pthread_cond_broadcast() {
      return 0;
    }
  function _pthread_cond_wait() {
      return 0;
    }
  function _atexit(func, arg) {
      __ATEXIT__.unshift({ func: func, arg: arg });
    }var ___cxa_atexit=_atexit;
  function ___cxa_allocate_exception(size) {
      return _malloc(size);
    }
  function ___cxa_free_exception(ptr) {
      return _free(ptr);
    }
  function _llvm_eh_exception() {
      return HEAP32[((_llvm_eh_exception.buf)>>2)];
    }
  function __ZSt18uncaught_exceptionv() { // std::uncaught_exception()
      return !!__ZSt18uncaught_exceptionv.uncaught_exception;
    }
  function ___cxa_is_number_type(type) {
      var isNumber = false;
      try { if (type == __ZTIi) isNumber = true } catch(e){}
      try { if (type == __ZTIj) isNumber = true } catch(e){}
      try { if (type == __ZTIl) isNumber = true } catch(e){}
      try { if (type == __ZTIm) isNumber = true } catch(e){}
      try { if (type == __ZTIx) isNumber = true } catch(e){}
      try { if (type == __ZTIy) isNumber = true } catch(e){}
      try { if (type == __ZTIf) isNumber = true } catch(e){}
      try { if (type == __ZTId) isNumber = true } catch(e){}
      try { if (type == __ZTIe) isNumber = true } catch(e){}
      try { if (type == __ZTIc) isNumber = true } catch(e){}
      try { if (type == __ZTIa) isNumber = true } catch(e){}
      try { if (type == __ZTIh) isNumber = true } catch(e){}
      try { if (type == __ZTIs) isNumber = true } catch(e){}
      try { if (type == __ZTIt) isNumber = true } catch(e){}
      return isNumber;
    }function ___cxa_does_inherit(definiteType, possibilityType, possibility) {
      if (possibility == 0) return false;
      if (possibilityType == 0 || possibilityType == definiteType)
        return true;
      var possibility_type_info;
      if (___cxa_is_number_type(possibilityType)) {
        possibility_type_info = possibilityType;
      } else {
        var possibility_type_infoAddr = HEAP32[((possibilityType)>>2)] - 8;
        possibility_type_info = HEAP32[((possibility_type_infoAddr)>>2)];
      }
      switch (possibility_type_info) {
      case 0: // possibility is a pointer
        // See if definite type is a pointer
        var definite_type_infoAddr = HEAP32[((definiteType)>>2)] - 8;
        var definite_type_info = HEAP32[((definite_type_infoAddr)>>2)];
        if (definite_type_info == 0) {
          // Also a pointer; compare base types of pointers
          var defPointerBaseAddr = definiteType+8;
          var defPointerBaseType = HEAP32[((defPointerBaseAddr)>>2)];
          var possPointerBaseAddr = possibilityType+8;
          var possPointerBaseType = HEAP32[((possPointerBaseAddr)>>2)];
          return ___cxa_does_inherit(defPointerBaseType, possPointerBaseType, possibility);
        } else
          return false; // one pointer and one non-pointer
      case 1: // class with no base class
        return false;
      case 2: // class with base class
        var parentTypeAddr = possibilityType + 8;
        var parentType = HEAP32[((parentTypeAddr)>>2)];
        return ___cxa_does_inherit(definiteType, parentType, possibility);
      default:
        return false; // some unencountered type
      }
    }function ___cxa_find_matching_catch(thrown, throwntype, typeArray) {
      // If throwntype is a pointer, this means a pointer has been
      // thrown. When a pointer is thrown, actually what's thrown
      // is a pointer to the pointer. We'll dereference it.
      if (throwntype != 0 && !___cxa_is_number_type(throwntype)) {
        var throwntypeInfoAddr= HEAP32[((throwntype)>>2)] - 8;
        var throwntypeInfo= HEAP32[((throwntypeInfoAddr)>>2)];
        if (throwntypeInfo == 0)
          thrown = HEAP32[((thrown)>>2)];
      }
      // The different catch blocks are denoted by different types.
      // Due to inheritance, those types may not precisely match the
      // type of the thrown object. Find one which matches, and
      // return the type of the catch block which should be called.
      for (var i = 0; i < typeArray.length; i++) {
        if (___cxa_does_inherit(typeArray[i], throwntype, thrown))
          return tempRet0 = typeArray[i],thrown;
      }
      // Shouldn't happen unless we have bogus data in typeArray
      // or encounter a type for which emscripten doesn't have suitable
      // typeinfo defined. Best-efforts match just in case.
      return tempRet0 = throwntype,thrown;
    }function ___cxa_throw(ptr, type, destructor) {
      if (!___cxa_throw.initialized) {
        try {
          HEAP32[((__ZTVN10__cxxabiv119__pointer_type_infoE)>>2)]=0; // Workaround for libcxxabi integration bug
        } catch(e){}
        try {
          HEAP32[((__ZTVN10__cxxabiv117__class_type_infoE)>>2)]=1; // Workaround for libcxxabi integration bug
        } catch(e){}
        try {
          HEAP32[((__ZTVN10__cxxabiv120__si_class_type_infoE)>>2)]=2; // Workaround for libcxxabi integration bug
        } catch(e){}
        ___cxa_throw.initialized = true;
      }
      HEAP32[((_llvm_eh_exception.buf)>>2)]=ptr
      HEAP32[(((_llvm_eh_exception.buf)+(4))>>2)]=type
      HEAP32[(((_llvm_eh_exception.buf)+(8))>>2)]=destructor
      if (!("uncaught_exception" in __ZSt18uncaught_exceptionv)) {
        __ZSt18uncaught_exceptionv.uncaught_exception = 1;
      } else {
        __ZSt18uncaught_exceptionv.uncaught_exception++;
      }
      throw ptr + " - Exception catching is disabled, this exception cannot be caught. Compile with -s DISABLE_EXCEPTION_CATCHING=0 or DISABLE_EXCEPTION_CATCHING=2 to catch.";;
    }
  function ___cxa_begin_catch(ptr) {
      __ZSt18uncaught_exceptionv.uncaught_exception--;
      return ptr;
    }
  function ___cxa_end_catch() {
      if (___cxa_end_catch.rethrown) {
        ___cxa_end_catch.rethrown = false;
        return;
      }
      // Clear state flag.
      __THREW__ = 0;
      // Clear type.
      HEAP32[(((_llvm_eh_exception.buf)+(4))>>2)]=0
      // Call destructor if one is registered then clear it.
      var ptr = HEAP32[((_llvm_eh_exception.buf)>>2)];
      var destructor = HEAP32[(((_llvm_eh_exception.buf)+(8))>>2)];
      if (destructor) {
        Runtime.dynCall('vi', destructor, [ptr]);
        HEAP32[(((_llvm_eh_exception.buf)+(8))>>2)]=0
      }
      // Free ptr if it isn't null.
      if (ptr) {
        ___cxa_free_exception(ptr);
        HEAP32[((_llvm_eh_exception.buf)>>2)]=0
      }
    }
  function _memset(ptr, value, num) {
      ptr = ptr|0; value = value|0; num = num|0;
      var stop = 0, value4 = 0, stop4 = 0, unaligned = 0;
      stop = (ptr + num)|0;
      if ((num|0) >= 20) {
        // This is unaligned, but quite large, so work hard to get to aligned settings
        value = value & 0xff;
        unaligned = ptr & 3;
        value4 = value | (value << 8) | (value << 16) | (value << 24);
        stop4 = stop & ~3;
        if (unaligned) {
          unaligned = (ptr + 4 - unaligned)|0;
          while ((ptr|0) < (unaligned|0)) { // no need to check for stop, since we have large num
            HEAP8[(ptr)]=value;
            ptr = (ptr+1)|0;
          }
        }
        while ((ptr|0) < (stop4|0)) {
          HEAP32[((ptr)>>2)]=value4;
          ptr = (ptr+4)|0;
        }
      }
      while ((ptr|0) < (stop|0)) {
        HEAP8[(ptr)]=value;
        ptr = (ptr+1)|0;
      }
    }var _llvm_memset_p0i8_i32=_memset;
  function _ungetc(c, stream) {
      // int ungetc(int c, FILE *stream);
      // http://pubs.opengroup.org/onlinepubs/000095399/functions/ungetc.html
      if (FS.streams[stream]) {
        c = unSign(c & 0xFF);
        FS.streams[stream].ungotten.push(c);
        return c;
      } else {
        return -1;
      }
    }
  function __ZNSt9exceptionD2Ev(){}
  function _strcpy(pdest, psrc) {
      pdest = pdest|0; psrc = psrc|0;
      var i = 0;
      do {
        HEAP8[(((pdest+i)|0)|0)]=HEAP8[(((psrc+i)|0)|0)];
        i = (i+1)|0;
      } while ((HEAP8[(((psrc)+(i-1))|0)])|0 != 0);
      return pdest|0;
    }
  var ERRNO_MESSAGES={1:"Operation not permitted",2:"No such file or directory",3:"No such process",4:"Interrupted system call",5:"Input/output error",6:"No such device or address",8:"Exec format error",9:"Bad file descriptor",10:"No child processes",11:"Resource temporarily unavailable",12:"Cannot allocate memory",13:"Permission denied",14:"Bad address",16:"Device or resource busy",17:"File exists",18:"Invalid cross-device link",19:"No such device",20:"Not a directory",21:"Is a directory",22:"Invalid argument",23:"Too many open files in system",24:"Too many open files",25:"Inappropriate ioctl for device",26:"Text file busy",27:"File too large",28:"No space left on device",29:"Illegal seek",30:"Read-only file system",31:"Too many links",32:"Broken pipe",33:"Numerical argument out of domain",34:"Numerical result out of range",35:"Resource deadlock avoided",36:"File name too long",37:"No locks available",38:"Function not implemented",39:"Directory not empty",40:"Too many levels of symbolic links",42:"No message of desired type",43:"Identifier removed",60:"Device not a stream",61:"No data available",62:"Timer expired",63:"Out of streams resources",67:"Link has been severed",71:"Protocol error",72:"Multihop attempted",74:"Bad message",75:"Value too large for defined data type",84:"Invalid or incomplete multibyte or wide character",88:"Socket operation on non-socket",89:"Destination address required",90:"Message too long",91:"Protocol wrong type for socket",92:"Protocol not available",93:"Protocol not supported",95:"Operation not supported",97:"Address family not supported by protocol",98:"Address already in use",99:"Cannot assign requested address",100:"Network is down",101:"Network is unreachable",102:"Network dropped connection on reset",103:"Software caused connection abort",104:"Connection reset by peer",105:"No buffer space available",106:"Transport endpoint is already connected",107:"Transport endpoint is not connected",110:"Connection timed out",111:"Connection refused",113:"No route to host",114:"Operation already in progress",115:"Operation now in progress",116:"Stale NFS file handle",122:"Disk quota exceeded",125:"Operation canceled",130:"Owner died",131:"State not recoverable"};function _strerror_r(errnum, strerrbuf, buflen) {
      if (errnum in ERRNO_MESSAGES) {
        if (ERRNO_MESSAGES[errnum].length > buflen - 1) {
          return ___setErrNo(ERRNO_CODES.ERANGE);
        } else {
          var msg = ERRNO_MESSAGES[errnum];
          for (var i = 0; i < msg.length; i++) {
            HEAP8[(((strerrbuf)+(i))|0)]=msg.charCodeAt(i)
          }
          HEAP8[(((strerrbuf)+(i))|0)]=0
          return 0;
        }
      } else {
        return ___setErrNo(ERRNO_CODES.EINVAL);
      }
    }function _strerror(errnum) {
      if (!_strerror.buffer) _strerror.buffer = _malloc(256);
      _strerror_r(errnum, _strerror.buffer, 256);
      return _strerror.buffer;
    }
  function _abort() {
      ABORT = true;
      throw 'abort() at ' + (new Error().stack);
    }
  function _memmove(dest, src, num) {
      dest = dest|0; src = src|0; num = num|0;
      if (((src|0) < (dest|0)) & ((dest|0) < ((src + num)|0))) {
        // Unlikely case: Copy backwards in a safe manner
        src = (src + num)|0;
        dest = (dest + num)|0;
        while ((num|0) > 0) {
          dest = (dest - 1)|0;
          src = (src - 1)|0;
          num = (num - 1)|0;
          HEAP8[(dest)]=HEAP8[(src)];
        }
      } else {
        _memcpy(dest, src, num);
      }
    }var _llvm_memmove_p0i8_p0i8_i32=_memmove;
  function ___cxa_rethrow() {
      ___cxa_end_catch.rethrown = true;
      throw HEAP32[((_llvm_eh_exception.buf)>>2)] + " - Exception catching is disabled, this exception cannot be caught. Compile with -s DISABLE_EXCEPTION_CATCHING=0 or DISABLE_EXCEPTION_CATCHING=2 to catch.";;
    }
  function __reallyNegative(x) {
      return x < 0 || (x === 0 && (1/x) === -Infinity);
    }function __formatString(format, varargs) {
      var textIndex = format;
      var argIndex = 0;
      function getNextArg(type) {
        // NOTE: Explicitly ignoring type safety. Otherwise this fails:
        //       int x = 4; printf("%c\n", (char)x);
        var ret;
        if (type === 'double') {
          ret = (HEAP32[((tempDoublePtr)>>2)]=HEAP32[(((varargs)+(argIndex))>>2)],HEAP32[(((tempDoublePtr)+(4))>>2)]=HEAP32[(((varargs)+((argIndex)+(4)))>>2)],HEAPF64[(tempDoublePtr)>>3]);
        } else if (type == 'i64') {
          ret = [HEAP32[(((varargs)+(argIndex))>>2)],
                 HEAP32[(((varargs)+(argIndex+4))>>2)]];
        } else {
          type = 'i32'; // varargs are always i32, i64, or double
          ret = HEAP32[(((varargs)+(argIndex))>>2)];
        }
        argIndex += Runtime.getNativeFieldSize(type);
        return ret;
      }
      var ret = [];
      var curr, next, currArg;
      while(1) {
        var startTextIndex = textIndex;
        curr = HEAP8[(textIndex)];
        if (curr === 0) break;
        next = HEAP8[((textIndex+1)|0)];
        if (curr == 37) {
          // Handle flags.
          var flagAlwaysSigned = false;
          var flagLeftAlign = false;
          var flagAlternative = false;
          var flagZeroPad = false;
          flagsLoop: while (1) {
            switch (next) {
              case 43:
                flagAlwaysSigned = true;
                break;
              case 45:
                flagLeftAlign = true;
                break;
              case 35:
                flagAlternative = true;
                break;
              case 48:
                if (flagZeroPad) {
                  break flagsLoop;
                } else {
                  flagZeroPad = true;
                  break;
                }
              default:
                break flagsLoop;
            }
            textIndex++;
            next = HEAP8[((textIndex+1)|0)];
          }
          // Handle width.
          var width = 0;
          if (next == 42) {
            width = getNextArg('i32');
            textIndex++;
            next = HEAP8[((textIndex+1)|0)];
          } else {
            while (next >= 48 && next <= 57) {
              width = width * 10 + (next - 48);
              textIndex++;
              next = HEAP8[((textIndex+1)|0)];
            }
          }
          // Handle precision.
          var precisionSet = false;
          if (next == 46) {
            var precision = 0;
            precisionSet = true;
            textIndex++;
            next = HEAP8[((textIndex+1)|0)];
            if (next == 42) {
              precision = getNextArg('i32');
              textIndex++;
            } else {
              while(1) {
                var precisionChr = HEAP8[((textIndex+1)|0)];
                if (precisionChr < 48 ||
                    precisionChr > 57) break;
                precision = precision * 10 + (precisionChr - 48);
                textIndex++;
              }
            }
            next = HEAP8[((textIndex+1)|0)];
          } else {
            var precision = 6; // Standard default.
          }
          // Handle integer sizes. WARNING: These assume a 32-bit architecture!
          var argSize;
          switch (String.fromCharCode(next)) {
            case 'h':
              var nextNext = HEAP8[((textIndex+2)|0)];
              if (nextNext == 104) {
                textIndex++;
                argSize = 1; // char (actually i32 in varargs)
              } else {
                argSize = 2; // short (actually i32 in varargs)
              }
              break;
            case 'l':
              var nextNext = HEAP8[((textIndex+2)|0)];
              if (nextNext == 108) {
                textIndex++;
                argSize = 8; // long long
              } else {
                argSize = 4; // long
              }
              break;
            case 'L': // long long
            case 'q': // int64_t
            case 'j': // intmax_t
              argSize = 8;
              break;
            case 'z': // size_t
            case 't': // ptrdiff_t
            case 'I': // signed ptrdiff_t or unsigned size_t
              argSize = 4;
              break;
            default:
              argSize = null;
          }
          if (argSize) textIndex++;
          next = HEAP8[((textIndex+1)|0)];
          // Handle type specifier.
          switch (String.fromCharCode(next)) {
            case 'd': case 'i': case 'u': case 'o': case 'x': case 'X': case 'p': {
              // Integer.
              var signed = next == 100 || next == 105;
              argSize = argSize || 4;
              var currArg = getNextArg('i' + (argSize * 8));
              var origArg = currArg;
              var argText;
              // Flatten i64-1 [low, high] into a (slightly rounded) double
              if (argSize == 8) {
                currArg = Runtime.makeBigInt(currArg[0], currArg[1], next == 117);
              }
              // Truncate to requested size.
              if (argSize <= 4) {
                var limit = Math.pow(256, argSize) - 1;
                currArg = (signed ? reSign : unSign)(currArg & limit, argSize * 8);
              }
              // Format the number.
              var currAbsArg = Math.abs(currArg);
              var prefix = '';
              if (next == 100 || next == 105) {
                if (argSize == 8 && i64Math) argText = i64Math.stringify(origArg[0], origArg[1], null); else
                argText = reSign(currArg, 8 * argSize, 1).toString(10);
              } else if (next == 117) {
                if (argSize == 8 && i64Math) argText = i64Math.stringify(origArg[0], origArg[1], true); else
                argText = unSign(currArg, 8 * argSize, 1).toString(10);
                currArg = Math.abs(currArg);
              } else if (next == 111) {
                argText = (flagAlternative ? '0' : '') + currAbsArg.toString(8);
              } else if (next == 120 || next == 88) {
                prefix = flagAlternative ? '0x' : '';
                if (argSize == 8 && i64Math) {
                  if (origArg[1]) {
                    argText = (origArg[1]>>>0).toString(16);
                    var lower = (origArg[0]>>>0).toString(16);
                    while (lower.length < 8) lower = '0' + lower;
                    argText += lower;
                  } else {
                    argText = (origArg[0]>>>0).toString(16);
                  }
                } else
                if (currArg < 0) {
                  // Represent negative numbers in hex as 2's complement.
                  currArg = -currArg;
                  argText = (currAbsArg - 1).toString(16);
                  var buffer = [];
                  for (var i = 0; i < argText.length; i++) {
                    buffer.push((0xF - parseInt(argText[i], 16)).toString(16));
                  }
                  argText = buffer.join('');
                  while (argText.length < argSize * 2) argText = 'f' + argText;
                } else {
                  argText = currAbsArg.toString(16);
                }
                if (next == 88) {
                  prefix = prefix.toUpperCase();
                  argText = argText.toUpperCase();
                }
              } else if (next == 112) {
                if (currAbsArg === 0) {
                  argText = '(nil)';
                } else {
                  prefix = '0x';
                  argText = currAbsArg.toString(16);
                }
              }
              if (precisionSet) {
                while (argText.length < precision) {
                  argText = '0' + argText;
                }
              }
              // Add sign if needed
              if (flagAlwaysSigned) {
                if (currArg < 0) {
                  prefix = '-' + prefix;
                } else {
                  prefix = '+' + prefix;
                }
              }
              // Add padding.
              while (prefix.length + argText.length < width) {
                if (flagLeftAlign) {
                  argText += ' ';
                } else {
                  if (flagZeroPad) {
                    argText = '0' + argText;
                  } else {
                    prefix = ' ' + prefix;
                  }
                }
              }
              // Insert the result into the buffer.
              argText = prefix + argText;
              argText.split('').forEach(function(chr) {
                ret.push(chr.charCodeAt(0));
              });
              break;
            }
            case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': {
              // Float.
              var currArg = getNextArg('double');
              var argText;
              if (isNaN(currArg)) {
                argText = 'nan';
                flagZeroPad = false;
              } else if (!isFinite(currArg)) {
                argText = (currArg < 0 ? '-' : '') + 'inf';
                flagZeroPad = false;
              } else {
                var isGeneral = false;
                var effectivePrecision = Math.min(precision, 20);
                // Convert g/G to f/F or e/E, as per:
                // http://pubs.opengroup.org/onlinepubs/9699919799/functions/printf.html
                if (next == 103 || next == 71) {
                  isGeneral = true;
                  precision = precision || 1;
                  var exponent = parseInt(currArg.toExponential(effectivePrecision).split('e')[1], 10);
                  if (precision > exponent && exponent >= -4) {
                    next = ((next == 103) ? 'f' : 'F').charCodeAt(0);
                    precision -= exponent + 1;
                  } else {
                    next = ((next == 103) ? 'e' : 'E').charCodeAt(0);
                    precision--;
                  }
                  effectivePrecision = Math.min(precision, 20);
                }
                if (next == 101 || next == 69) {
                  argText = currArg.toExponential(effectivePrecision);
                  // Make sure the exponent has at least 2 digits.
                  if (/[eE][-+]\d$/.test(argText)) {
                    argText = argText.slice(0, -1) + '0' + argText.slice(-1);
                  }
                } else if (next == 102 || next == 70) {
                  argText = currArg.toFixed(effectivePrecision);
                  if (currArg === 0 && __reallyNegative(currArg)) {
                    argText = '-' + argText;
                  }
                }
                var parts = argText.split('e');
                if (isGeneral && !flagAlternative) {
                  // Discard trailing zeros and periods.
                  while (parts[0].length > 1 && parts[0].indexOf('.') != -1 &&
                         (parts[0].slice(-1) == '0' || parts[0].slice(-1) == '.')) {
                    parts[0] = parts[0].slice(0, -1);
                  }
                } else {
                  // Make sure we have a period in alternative mode.
                  if (flagAlternative && argText.indexOf('.') == -1) parts[0] += '.';
                  // Zero pad until required precision.
                  while (precision > effectivePrecision++) parts[0] += '0';
                }
                argText = parts[0] + (parts.length > 1 ? 'e' + parts[1] : '');
                // Capitalize 'E' if needed.
                if (next == 69) argText = argText.toUpperCase();
                // Add sign.
                if (flagAlwaysSigned && currArg >= 0) {
                  argText = '+' + argText;
                }
              }
              // Add padding.
              while (argText.length < width) {
                if (flagLeftAlign) {
                  argText += ' ';
                } else {
                  if (flagZeroPad && (argText[0] == '-' || argText[0] == '+')) {
                    argText = argText[0] + '0' + argText.slice(1);
                  } else {
                    argText = (flagZeroPad ? '0' : ' ') + argText;
                  }
                }
              }
              // Adjust case.
              if (next < 97) argText = argText.toUpperCase();
              // Insert the result into the buffer.
              argText.split('').forEach(function(chr) {
                ret.push(chr.charCodeAt(0));
              });
              break;
            }
            case 's': {
              // String.
              var arg = getNextArg('i8*') || nullString;
              var argLength = _strlen(arg);
              if (precisionSet) argLength = Math.min(argLength, precision);
              if (!flagLeftAlign) {
                while (argLength < width--) {
                  ret.push(32);
                }
              }
              for (var i = 0; i < argLength; i++) {
                ret.push(HEAPU8[((arg++)|0)]);
              }
              if (flagLeftAlign) {
                while (argLength < width--) {
                  ret.push(32);
                }
              }
              break;
            }
            case 'c': {
              // Character.
              if (flagLeftAlign) ret.push(getNextArg('i8'));
              while (--width > 0) {
                ret.push(32);
              }
              if (!flagLeftAlign) ret.push(getNextArg('i8'));
              break;
            }
            case 'n': {
              // Write the length written so far to the next parameter.
              var ptr = getNextArg('i32*');
              HEAP32[((ptr)>>2)]=ret.length
              break;
            }
            case '%': {
              // Literal percent sign.
              ret.push(curr);
              break;
            }
            default: {
              // Unknown specifiers remain untouched.
              for (var i = startTextIndex; i < textIndex + 2; i++) {
                ret.push(HEAP8[(i)]);
              }
            }
          }
          textIndex += 2;
          // TODO: Support a/A (hex float) and m (last error) specifiers.
          // TODO: Support %1${specifier} for arg selection.
        } else {
          ret.push(curr);
          textIndex += 1;
        }
      }
      return ret;
    }function _snprintf(s, n, format, varargs) {
      // int snprintf(char *restrict s, size_t n, const char *restrict format, ...);
      // http://pubs.opengroup.org/onlinepubs/000095399/functions/printf.html
      var result = __formatString(format, varargs);
      var limit = (n === undefined) ? result.length
                                    : Math.min(result.length, Math.max(n - 1, 0));
      if (s < 0) {
        s = -s;
        var buf = _malloc(limit+1);
        HEAP32[((s)>>2)]=buf;
        s = buf;
      }
      for (var i = 0; i < limit; i++) {
        HEAP8[(((s)+(i))|0)]=result[i];
      }
      if (limit < n || (n === undefined)) HEAP8[(((s)+(i))|0)]=0;
      return result.length;
    }
  function _wmemmove() { throw 'wmemmove not implemented' }
  function _wmemset() { throw 'wmemset not implemented' }
  function _wmemcpy() { throw 'wmemcpy not implemented' }
  function _sysconf(name) {
      // long sysconf(int name);
      // http://pubs.opengroup.org/onlinepubs/009695399/functions/sysconf.html
      switch(name) {
        case 8: return PAGE_SIZE;
        case 54:
        case 56:
        case 21:
        case 61:
        case 63:
        case 22:
        case 67:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 69:
        case 28:
        case 101:
        case 70:
        case 71:
        case 29:
        case 30:
        case 199:
        case 75:
        case 76:
        case 32:
        case 43:
        case 44:
        case 80:
        case 46:
        case 47:
        case 45:
        case 48:
        case 49:
        case 42:
        case 82:
        case 33:
        case 7:
        case 108:
        case 109:
        case 107:
        case 112:
        case 119:
        case 121:
          return 200809;
        case 13:
        case 104:
        case 94:
        case 95:
        case 34:
        case 35:
        case 77:
        case 81:
        case 83:
        case 84:
        case 85:
        case 86:
        case 87:
        case 88:
        case 89:
        case 90:
        case 91:
        case 94:
        case 95:
        case 110:
        case 111:
        case 113:
        case 114:
        case 115:
        case 116:
        case 117:
        case 118:
        case 120:
        case 40:
        case 16:
        case 79:
        case 19:
          return -1;
        case 92:
        case 93:
        case 5:
        case 72:
        case 6:
        case 74:
        case 92:
        case 93:
        case 96:
        case 97:
        case 98:
        case 99:
        case 102:
        case 103:
        case 105:
          return 1;
        case 38:
        case 66:
        case 50:
        case 51:
        case 4:
          return 1024;
        case 15:
        case 64:
        case 41:
          return 32;
        case 55:
        case 37:
        case 17:
          return 2147483647;
        case 18:
        case 1:
          return 47839;
        case 59:
        case 57:
          return 99;
        case 68:
        case 58:
          return 2048;
        case 0: return 2097152;
        case 3: return 65536;
        case 14: return 32768;
        case 73: return 32767;
        case 39: return 16384;
        case 60: return 1000;
        case 106: return 700;
        case 52: return 256;
        case 62: return 255;
        case 2: return 100;
        case 65: return 64;
        case 36: return 20;
        case 100: return 16;
        case 20: return 6;
        case 53: return 4;
        case 10: return 1;
      }
      ___setErrNo(ERRNO_CODES.EINVAL);
      return -1;
    }
  function _newlocale(mask, locale, base) {
      return 0;
    }
  function _freelocale(locale) {}
  function ___ctype_b_loc() {
      // http://refspecs.freestandards.org/LSB_3.0.0/LSB-Core-generic/LSB-Core-generic/baselib---ctype-b-loc.html
      var me = ___ctype_b_loc;
      if (!me.ret) {
        var values = [
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,8195,8194,8194,8194,8194,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,24577,49156,49156,49156,
          49156,49156,49156,49156,49156,49156,49156,49156,49156,49156,49156,49156,55304,55304,55304,55304,55304,55304,55304,55304,
          55304,55304,49156,49156,49156,49156,49156,49156,49156,54536,54536,54536,54536,54536,54536,50440,50440,50440,50440,50440,
          50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,49156,49156,49156,49156,49156,
          49156,54792,54792,54792,54792,54792,54792,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,
          50696,50696,50696,50696,50696,50696,50696,49156,49156,49156,49156,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        ];
        var i16size = 2;
        var arr = _malloc(values.length * i16size);
        for (var i = 0; i < values.length; i++) {
          HEAP16[(((arr)+(i * i16size))>>1)]=values[i]
        }
        me.ret = allocate([arr + 128 * i16size], 'i16*', ALLOC_NORMAL);
      }
      return me.ret;
    }
  function ___ctype_tolower_loc() {
      // http://refspecs.freestandards.org/LSB_3.1.1/LSB-Core-generic/LSB-Core-generic/libutil---ctype-tolower-loc.html
      var me = ___ctype_tolower_loc;
      if (!me.ret) {
        var values = [
          128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,
          158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,
          188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
          218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,
          248,249,250,251,252,253,254,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
          33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,97,98,99,100,101,102,103,
          104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,91,92,93,94,95,96,97,98,99,100,101,102,103,
          104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,
          134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,
          164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,
          194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
          224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,
          254,255
        ];
        var i32size = 4;
        var arr = _malloc(values.length * i32size);
        for (var i = 0; i < values.length; i++) {
          HEAP32[(((arr)+(i * i32size))>>2)]=values[i]
        }
        me.ret = allocate([arr + 128 * i32size], 'i32*', ALLOC_NORMAL);
      }
      return me.ret;
    }
  function ___ctype_toupper_loc() {
      // http://refspecs.freestandards.org/LSB_3.1.1/LSB-Core-generic/LSB-Core-generic/libutil---ctype-toupper-loc.html
      var me = ___ctype_toupper_loc;
      if (!me.ret) {
        var values = [
          128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,
          158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,
          188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
          218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,
          248,249,250,251,252,253,254,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
          33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,
          73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,
          81,82,83,84,85,86,87,88,89,90,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,
          145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,
          175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,
          205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,
          235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
        ];
        var i32size = 4;
        var arr = _malloc(values.length * i32size);
        for (var i = 0; i < values.length; i++) {
          HEAP32[(((arr)+(i * i32size))>>2)]=values[i]
        }
        me.ret = allocate([arr + 128 * i32size], 'i32*', ALLOC_NORMAL);
      }
      return me.ret;
    }
  function _strftime(s, maxsize, format, timeptr) {
      // size_t strftime(char *restrict s, size_t maxsize, const char *restrict format, const struct tm *restrict timeptr);
      // http://pubs.opengroup.org/onlinepubs/009695399/functions/strftime.html
      // TODO: Implement.
      return 0;
    }var _strftime_l=_strftime;
  function _isxdigit(chr) {
      return (chr >= 48 && chr <= 57) ||
             (chr >= 97 && chr <= 102) ||
             (chr >= 65 && chr <= 70);
    }var _isxdigit_l=_isxdigit;
  function _isdigit(chr) {
      return chr >= 48 && chr <= 57;
    }var _isdigit_l=_isdigit;
  function __isFloat(text) {
      return !!(/^[+-]?[0-9]*\.?[0-9]+([eE][+-]?[0-9]+)?$/.exec(text));
    }function __scanString(format, get, unget, varargs) {
      if (!__scanString.whiteSpace) {
        __scanString.whiteSpace = {};
        __scanString.whiteSpace[32] = 1;
        __scanString.whiteSpace[9] = 1;
        __scanString.whiteSpace[10] = 1;
        __scanString.whiteSpace[' '] = 1;
        __scanString.whiteSpace['\t'] = 1;
        __scanString.whiteSpace['\n'] = 1;
      }
      // Supports %x, %4x, %d.%d, %lld, %s, %f, %lf.
      // TODO: Support all format specifiers.
      format = Pointer_stringify(format);
      var soFar = 0;
      if (format.indexOf('%n') >= 0) {
        // need to track soFar
        var _get = get;
        get = function() {
          soFar++;
          return _get();
        }
        var _unget = unget;
        unget = function() {
          soFar--;
          return _unget();
        }
      }
      var formatIndex = 0;
      var argsi = 0;
      var fields = 0;
      var argIndex = 0;
      var next;
      mainLoop:
      for (var formatIndex = 0; formatIndex < format.length;) {
        if (format[formatIndex] === '%' && format[formatIndex+1] == 'n') {
          var argPtr = HEAP32[(((varargs)+(argIndex))>>2)];
          argIndex += Runtime.getNativeFieldSize('void*');
          HEAP32[((argPtr)>>2)]=soFar;
          formatIndex += 2;
          continue;
        }
        // TODO: Support strings like "%5c" etc.
        if (format[formatIndex] === '%' && format[formatIndex+1] == 'c') {
          var argPtr = HEAP32[(((varargs)+(argIndex))>>2)];
          argIndex += Runtime.getNativeFieldSize('void*');
          fields++;
          next = get();
          HEAP8[(argPtr)]=next
          formatIndex += 2;
          continue;
        }
        // remove whitespace
        while (1) {
          next = get();
          if (next == 0) return fields;
          if (!(next in __scanString.whiteSpace)) break;
        }
        unget();
        if (format[formatIndex] === '%') {
          formatIndex++;
          var maxSpecifierStart = formatIndex;
          while (format[formatIndex].charCodeAt(0) >= 48 &&
                 format[formatIndex].charCodeAt(0) <= 57) {
            formatIndex++;
          }
          var max_;
          if (formatIndex != maxSpecifierStart) {
            max_ = parseInt(format.slice(maxSpecifierStart, formatIndex), 10);
          }
          var long_ = false;
          var half = false;
          var longLong = false;
          if (format[formatIndex] == 'l') {
            long_ = true;
            formatIndex++;
            if(format[formatIndex] == 'l') {
              longLong = true;
              formatIndex++;
            }
          } else if (format[formatIndex] == 'h') {
            half = true;
            formatIndex++;
          }
          var type = format[formatIndex];
          formatIndex++;
          var curr = 0;
          var buffer = [];
          // Read characters according to the format. floats are trickier, they may be in an unfloat state in the middle, then be a valid float later
          if (type == 'f' || type == 'e' || type == 'g' || type == 'E') {
            var last = 0;
            next = get();
            while (next > 0) {
              buffer.push(String.fromCharCode(next));
              if (__isFloat(buffer.join(''))) {
                last = buffer.length;
              }
              next = get();
            }
            for (var i = 0; i < buffer.length - last + 1; i++) {
              unget();
            }
            buffer.length = last;
          } else {
            next = get();
            var first = true;
            while ((curr < max_ || isNaN(max_)) && next > 0) {
              if (!(next in __scanString.whiteSpace) && // stop on whitespace
                  (type == 's' ||
                   ((type === 'd' || type == 'u' || type == 'i') && ((next >= 48 && next <= 57) ||
                                                                     (first && next == 45))) ||
                   (type === 'x' && (next >= 48 && next <= 57 ||
                                     next >= 97 && next <= 102 ||
                                     next >= 65 && next <= 70))) &&
                  (formatIndex >= format.length || next !== format[formatIndex].charCodeAt(0))) { // Stop when we read something that is coming up
                buffer.push(String.fromCharCode(next));
                next = get();
                curr++;
                first = false;
              } else {
                break;
              }
            }
            unget();
          }
          if (buffer.length === 0) return 0;  // Failure.
          var text = buffer.join('');
          var argPtr = HEAP32[(((varargs)+(argIndex))>>2)];
          argIndex += Runtime.getNativeFieldSize('void*');
          switch (type) {
            case 'd': case 'u': case 'i':
              if (half) {
                HEAP16[((argPtr)>>1)]=parseInt(text, 10);
              } else if(longLong) {
                (tempI64 = [parseInt(text, 10)>>>0,Math.min(Math.floor((parseInt(text, 10))/4294967296), 4294967295)>>>0],HEAP32[((argPtr)>>2)]=tempI64[0],HEAP32[(((argPtr)+(4))>>2)]=tempI64[1]);
              } else {
                HEAP32[((argPtr)>>2)]=parseInt(text, 10);
              }
              break;
            case 'x':
              HEAP32[((argPtr)>>2)]=parseInt(text, 16)
              break;
            case 'f':
            case 'e':
            case 'g':
            case 'E':
              // fallthrough intended
              if (long_) {
                (HEAPF64[(tempDoublePtr)>>3]=parseFloat(text),HEAP32[((argPtr)>>2)]=HEAP32[((tempDoublePtr)>>2)],HEAP32[(((argPtr)+(4))>>2)]=HEAP32[(((tempDoublePtr)+(4))>>2)])
              } else {
                HEAPF32[((argPtr)>>2)]=parseFloat(text)
              }
              break;
            case 's':
              var array = intArrayFromString(text);
              for (var j = 0; j < array.length; j++) {
                HEAP8[(((argPtr)+(j))|0)]=array[j]
              }
              break;
          }
          fields++;
        } else if (format[formatIndex] in __scanString.whiteSpace) {
          next = get();
          while (next in __scanString.whiteSpace) {
            if (next <= 0) break mainLoop;  // End of input.
            next = get();
          }
          unget(next);
          formatIndex++;
        } else {
          // Not a specifier.
          next = get();
          if (format[formatIndex].charCodeAt(0) !== next) {
            unget(next);
            break mainLoop;
          }
          formatIndex++;
        }
      }
      return fields;
    }function _sscanf(s, format, varargs) {
      // int sscanf(const char *restrict s, const char *restrict format, ... );
      // http://pubs.opengroup.org/onlinepubs/000095399/functions/scanf.html
      var index = 0;
      var get = function() { return HEAP8[(((s)+(index++))|0)]; };
      var unget = function() { index--; };
      return __scanString(format, get, unget, varargs);
    }
  function __Z7catopenPKci() { throw 'catopen not implemented' }
  function __Z7catgetsP8_nl_catdiiPKc() { throw 'catgets not implemented' }
  function __Z8catcloseP8_nl_catd() { throw 'catclose not implemented' }
  function _uselocale(locale) {
      return 0;
    }
  function _sprintf(s, format, varargs) {
      // int sprintf(char *restrict s, const char *restrict format, ...);
      // http://pubs.opengroup.org/onlinepubs/000095399/functions/printf.html
      return _snprintf(s, undefined, format, varargs);
    }function _asprintf(s, format, varargs) {
      return _sprintf(-s, format, varargs);
    }var _vasprintf=_asprintf;
  function _llvm_va_end() {}
  var _vsnprintf=_snprintf;
  var _vsprintf=_sprintf;
  var _vsscanf=_sscanf;
  function _isspace(chr) {
      return chr in { 32: 0, 9: 0, 10: 0, 11: 0, 12: 0, 13: 0 };
    }
  function __parseInt64(str, endptr, base, min, max, unsign) {
      var start = str;
      // Skip space.
      while (_isspace(HEAP8[(str)])) str++;
      // Check for a plus/minus sign.
      if (HEAP8[(str)] == 45) {
        str++;
      } else if (HEAP8[(str)] == 43) {
        str++;
      }
      // Find base.
      var ok = false;
      var finalBase = base;
      if (!finalBase) {
        if (HEAP8[(str)] == 48) {
          if (HEAP8[((str+1)|0)] == 120 ||
              HEAP8[((str+1)|0)] == 88) {
            finalBase = 16;
            str += 2;
          } else {
            finalBase = 8;
            str++;
            ok = true; // we saw an initial zero, perhaps the entire thing is just "0"
          }
        }
      }
      if (!finalBase) finalBase = 10;
      // Get digits.
      var chr;
      while ((chr = HEAP8[(str)]) != 0) {
        var digit = parseInt(String.fromCharCode(chr), finalBase);
        if (isNaN(digit)) {
          break;
        } else {
          str++;
          ok = true;
        }
      }
      if (!ok) {
        ___setErrNo(ERRNO_CODES.EINVAL);
        return tempRet0 = 0,0;
      }
      // Set end pointer.
      if (endptr) {
        HEAP32[((endptr)>>2)]=str
      }
      try {
        i64Math.fromString(Pointer_stringify(start, str - start), finalBase, min, max, unsign);
      } catch(e) {
        ___setErrNo(ERRNO_CODES.ERANGE); // not quite correct
      }
      return tempRet0 = HEAP32[(((tempDoublePtr)+(4))>>2)],HEAP32[((tempDoublePtr)>>2)];
    }function _strtoull(str, endptr, base) {
      return __parseInt64(str, endptr, base, 0, '18446744073709551615', true);  // ULONG_MAX.
    }var _strtoull_l=_strtoull;
  function _strtoll(str, endptr, base) {
      return __parseInt64(str, endptr, base, '-9223372036854775808', '9223372036854775807');  // LLONG_MIN, LLONG_MAX.
    }var _strtoll_l=_strtoll;
  function _mbsrtowcs() { throw 'mbsrtowcs not implemented' }
  function _mbrlen() { throw 'mbrlen not implemented' }
  function ___locale_mb_cur_max() { throw '__locale_mb_cur_max not implemented' }
  function _mbtowc(pwc, pmb, maxx) {
      // XXX doesn't really handle multibyte at all
      if (!pmb) return 0;
      maxx = Math.min(85, maxx);
      var i;
      for (i = 0; i < maxx; i++) {
        var curr = HEAP8[(pmb)];
        if (pwc) {
          HEAP8[(pwc)]=curr;
          HEAP8[(((pwc)+(1))|0)]=0;
          pwc += 2;
        }
        pmb++;
        if (!curr) break;
      }
      return i;
    }
  function _mbrtowc() { throw 'mbrtowc not implemented' }
  function _mbsnrtowcs() { throw 'mbsnrtowcs not implemented' }
  function _wcrtomb(s, wc, ps) {
      // XXX doesn't really handle multibyte at all
      if (s) {
        HEAP8[(s)]=wc;
      }
      return 1;
    }
  function _wcsnrtombs() { throw 'wcsnrtombs not implemented' }
  var _llvm_memset_p0i8_i64=_memset;
  function _time(ptr) {
      var ret = Math.floor(Date.now()/1000);
      if (ptr) {
        HEAP32[((ptr)>>2)]=ret
      }
      return ret;
    }
  function _sbrk(bytes) {
      // Implement a Linux-like 'memory area' for our 'process'.
      // Changes the size of the memory area by |bytes|; returns the
      // address of the previous top ('break') of the memory area
      // We need to make sure no one else allocates unfreeable memory!
      // We must control this entirely. So we don't even need to do
      // unfreeable allocations - the HEAP is ours, from STATICTOP up.
      // TODO: We could in theory slice off the top of the HEAP when
      //       sbrk gets a negative increment in |bytes|...
      var self = _sbrk;
      if (!self.called) {
        STATICTOP = alignMemoryPage(STATICTOP); // make sure we start out aligned
        self.called = true;
        _sbrk.DYNAMIC_START = STATICTOP;
      }
      var ret = STATICTOP;
      if (bytes != 0) Runtime.staticAlloc(bytes);
      return ret;  // Previous break location.
    }
  function _llvm_lifetime_start() {}
  function _llvm_lifetime_end() {}
  var Browser={mainLoop:{scheduler:null,shouldPause:false,paused:false,queue:[],pause:function () {
          Browser.mainLoop.shouldPause = true;
        },resume:function () {
          if (Browser.mainLoop.paused) {
            Browser.mainLoop.paused = false;
            Browser.mainLoop.scheduler();
          }
          Browser.mainLoop.shouldPause = false;
        },updateStatus:function () {
          if (Module['setStatus']) {
            var message = Module['statusMessage'] || 'Please wait...';
            var remaining = Browser.mainLoop.remainingBlockers;
            var expected = Browser.mainLoop.expectedBlockers;
            if (remaining) {
              if (remaining < expected) {
                Module['setStatus'](message + ' (' + (expected - remaining) + '/' + expected + ')');
              } else {
                Module['setStatus'](message);
              }
            } else {
              Module['setStatus']('');
            }
          }
        }},isFullScreen:false,pointerLock:false,moduleContextCreatedCallbacks:[],workers:[],init:function () {
        if (Browser.initted) return;
        Browser.initted = true;
        try {
          new Blob();
          Browser.hasBlobConstructor = true;
        } catch(e) {
          Browser.hasBlobConstructor = false;
          console.log("warning: no blob constructor, cannot create blobs with mimetypes");
        }
        Browser.BlobBuilder = typeof MozBlobBuilder != "undefined" ? MozBlobBuilder : (typeof WebKitBlobBuilder != "undefined" ? WebKitBlobBuilder : (!Browser.hasBlobConstructor ? console.log("warning: no BlobBuilder") : null));
        Browser.URLObject = typeof window != "undefined" ? (window.URL ? window.URL : window.webkitURL) : console.log("warning: cannot create object URLs");
        // Support for plugins that can process preloaded files. You can add more of these to
        // your app by creating and appending to Module.preloadPlugins.
        //
        // Each plugin is asked if it can handle a file based on the file's name. If it can,
        // it is given the file's raw data. When it is done, it calls a callback with the file's
        // (possibly modified) data. For example, a plugin might decompress a file, or it
        // might create some side data structure for use later (like an Image element, etc.).
        function getMimetype(name) {
          return {
            'jpg': 'image/jpeg',
            'jpeg': 'image/jpeg',
            'png': 'image/png',
            'bmp': 'image/bmp',
            'ogg': 'audio/ogg',
            'wav': 'audio/wav',
            'mp3': 'audio/mpeg'
          }[name.substr(-3)];
          return ret;
        }
        if (!Module["preloadPlugins"]) Module["preloadPlugins"] = [];
        var imagePlugin = {};
        imagePlugin['canHandle'] = function(name) {
          return !Module.noImageDecoding && /\.(jpg|jpeg|png|bmp)$/.exec(name);
        };
        imagePlugin['handle'] = function(byteArray, name, onload, onerror) {
          var b = null;
          if (Browser.hasBlobConstructor) {
            try {
              b = new Blob([byteArray], { type: getMimetype(name) });
            } catch(e) {
              Runtime.warnOnce('Blob constructor present but fails: ' + e + '; falling back to blob builder');
            }
          }
          if (!b) {
            var bb = new Browser.BlobBuilder();
            bb.append((new Uint8Array(byteArray)).buffer); // we need to pass a buffer, and must copy the array to get the right data range
            b = bb.getBlob();
          }
          var url = Browser.URLObject.createObjectURL(b);
          var img = new Image();
          img.onload = function() {
            assert(img.complete, 'Image ' + name + ' could not be decoded');
            var canvas = document.createElement('canvas');
            canvas.width = img.width;
            canvas.height = img.height;
            var ctx = canvas.getContext('2d');
            ctx.drawImage(img, 0, 0);
            Module["preloadedImages"][name] = canvas;
            Browser.URLObject.revokeObjectURL(url);
            if (onload) onload(byteArray);
          };
          img.onerror = function(event) {
            console.log('Image ' + url + ' could not be decoded');
            if (onerror) onerror();
          };
          img.src = url;
        };
        Module['preloadPlugins'].push(imagePlugin);
        var audioPlugin = {};
        audioPlugin['canHandle'] = function(name) {
          return !Module.noAudioDecoding && name.substr(-4) in { '.ogg': 1, '.wav': 1, '.mp3': 1 };
        };
        audioPlugin['handle'] = function(byteArray, name, onload, onerror) {
          var done = false;
          function finish(audio) {
            if (done) return;
            done = true;
            Module["preloadedAudios"][name] = audio;
            if (onload) onload(byteArray);
          }
          function fail() {
            if (done) return;
            done = true;
            Module["preloadedAudios"][name] = new Audio(); // empty shim
            if (onerror) onerror();
          }
          if (Browser.hasBlobConstructor) {
            try {
              var b = new Blob([byteArray], { type: getMimetype(name) });
            } catch(e) {
              return fail();
            }
            var url = Browser.URLObject.createObjectURL(b); // XXX we never revoke this!
            var audio = new Audio();
            audio.addEventListener('canplaythrough', function() { finish(audio) }, false); // use addEventListener due to chromium bug 124926
            audio.onerror = function(event) {
              if (done) return;
              console.log('warning: browser could not fully decode audio ' + name + ', trying slower base64 approach');
              function encode64(data) {
                var BASE = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
                var PAD = '=';
                var ret = '';
                var leftchar = 0;
                var leftbits = 0;
                for (var i = 0; i < data.length; i++) {
                  leftchar = (leftchar << 8) | data[i];
                  leftbits += 8;
                  while (leftbits >= 6) {
                    var curr = (leftchar >> (leftbits-6)) & 0x3f;
                    leftbits -= 6;
                    ret += BASE[curr];
                  }
                }
                if (leftbits == 2) {
                  ret += BASE[(leftchar&3) << 4];
                  ret += PAD + PAD;
                } else if (leftbits == 4) {
                  ret += BASE[(leftchar&0xf) << 2];
                  ret += PAD;
                }
                return ret;
              }
              audio.src = 'data:audio/x-' + name.substr(-3) + ';base64,' + encode64(byteArray);
              finish(audio); // we don't wait for confirmation this worked - but it's worth trying
            };
            audio.src = url;
            // workaround for chrome bug 124926 - we do not always get oncanplaythrough or onerror
            setTimeout(function() {
              finish(audio); // try to use it even though it is not necessarily ready to play
            }, 10000);
          } else {
            return fail();
          }
        };
        Module['preloadPlugins'].push(audioPlugin);
        // Canvas event setup
        var canvas = Module['canvas'];
        canvas.requestPointerLock = canvas['requestPointerLock'] ||
                                    canvas['mozRequestPointerLock'] ||
                                    canvas['webkitRequestPointerLock'];
        canvas.exitPointerLock = document['exitPointerLock'] ||
                                 document['mozExitPointerLock'] ||
                                 document['webkitExitPointerLock'];
        canvas.exitPointerLock = canvas.exitPointerLock.bind(document);
        function pointerLockChange() {
          Browser.pointerLock = document['pointerLockElement'] === canvas ||
                                document['mozPointerLockElement'] === canvas ||
                                document['webkitPointerLockElement'] === canvas;
        }
        document.addEventListener('pointerlockchange', pointerLockChange, false);
        document.addEventListener('mozpointerlockchange', pointerLockChange, false);
        document.addEventListener('webkitpointerlockchange', pointerLockChange, false);
        if (Module['elementPointerLock']) {
          canvas.addEventListener("click", function(ev) {
            if (!Browser.pointerLock && canvas.requestPointerLock) {
              canvas.requestPointerLock();
              ev.preventDefault();
            }
          }, false);
        }
      },createContext:function (canvas, useWebGL, setInModule) {
        var ctx;
        try {
          if (useWebGL) {
            ctx = canvas.getContext('experimental-webgl', {
              alpha: false
            });
          } else {
            ctx = canvas.getContext('2d');
          }
          if (!ctx) throw ':(';
        } catch (e) {
          Module.print('Could not create canvas - ' + e);
          return null;
        }
        if (useWebGL) {
          // Set the background of the WebGL canvas to black
          canvas.style.backgroundColor = "black";
          // Warn on context loss
          canvas.addEventListener('webglcontextlost', function(event) {
            alert('WebGL context lost. You will need to reload the page.');
          }, false);
        }
        if (setInModule) {
          Module.ctx = ctx;
          Module.useWebGL = useWebGL;
          Browser.moduleContextCreatedCallbacks.forEach(function(callback) { callback() });
          Browser.init();
        }
        return ctx;
      },destroyContext:function (canvas, useWebGL, setInModule) {},fullScreenHandlersInstalled:false,lockPointer:undefined,resizeCanvas:undefined,requestFullScreen:function (lockPointer, resizeCanvas) {
        this.lockPointer = lockPointer;
        this.resizeCanvas = resizeCanvas;
        if (typeof this.lockPointer === 'undefined') this.lockPointer = true;
        if (typeof this.resizeCanvas === 'undefined') this.resizeCanvas = false;
        var canvas = Module['canvas'];
        function fullScreenChange() {
          Browser.isFullScreen = false;
          if ((document['webkitFullScreenElement'] || document['webkitFullscreenElement'] ||
               document['mozFullScreenElement'] || document['mozFullscreenElement'] ||
               document['fullScreenElement'] || document['fullscreenElement']) === canvas) {
            canvas.cancelFullScreen = document['cancelFullScreen'] ||
                                      document['mozCancelFullScreen'] ||
                                      document['webkitCancelFullScreen'];
            canvas.cancelFullScreen = canvas.cancelFullScreen.bind(document);
            if (Browser.lockPointer) canvas.requestPointerLock();
            Browser.isFullScreen = true;
            if (Browser.resizeCanvas) Browser.setFullScreenCanvasSize();
          } else if (Browser.resizeCanvas){
            Browser.setWindowedCanvasSize();
          }
          if (Module['onFullScreen']) Module['onFullScreen'](Browser.isFullScreen);
        }
        if (!this.fullScreenHandlersInstalled) {
          this.fullScreenHandlersInstalled = true;
          document.addEventListener('fullscreenchange', fullScreenChange, false);
          document.addEventListener('mozfullscreenchange', fullScreenChange, false);
          document.addEventListener('webkitfullscreenchange', fullScreenChange, false);
        }
        canvas.requestFullScreen = canvas['requestFullScreen'] ||
                                   canvas['mozRequestFullScreen'] ||
                                   (canvas['webkitRequestFullScreen'] ? function() { canvas['webkitRequestFullScreen'](Element['ALLOW_KEYBOARD_INPUT']) } : null);
        canvas.requestFullScreen(); 
      },requestAnimationFrame:function (func) {
        if (!window.requestAnimationFrame) {
          window.requestAnimationFrame = window['requestAnimationFrame'] ||
                                         window['mozRequestAnimationFrame'] ||
                                         window['webkitRequestAnimationFrame'] ||
                                         window['msRequestAnimationFrame'] ||
                                         window['oRequestAnimationFrame'] ||
                                         window['setTimeout'];
        }
        window.requestAnimationFrame(func);
      },getMovementX:function (event) {
        return event['movementX'] ||
               event['mozMovementX'] ||
               event['webkitMovementX'] ||
               0;
      },getMovementY:function (event) {
        return event['movementY'] ||
               event['mozMovementY'] ||
               event['webkitMovementY'] ||
               0;
      },xhrLoad:function (url, onload, onerror) {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', url, true);
        xhr.responseType = 'arraybuffer';
        xhr.onload = function() {
          if (xhr.status == 200) {
            onload(xhr.response);
          } else {
            onerror();
          }
        };
        xhr.onerror = onerror;
        xhr.send(null);
      },asyncLoad:function (url, onload, onerror, noRunDep) {
        Browser.xhrLoad(url, function(arrayBuffer) {
          assert(arrayBuffer, 'Loading data file "' + url + '" failed (no arrayBuffer).');
          onload(new Uint8Array(arrayBuffer));
          if (!noRunDep) removeRunDependency('al ' + url);
        }, function(event) {
          if (onerror) {
            onerror();
          } else {
            throw 'Loading data file "' + url + '" failed.';
          }
        });
        if (!noRunDep) addRunDependency('al ' + url);
      },resizeListeners:[],updateResizeListeners:function () {
        var canvas = Module['canvas'];
        Browser.resizeListeners.forEach(function(listener) {
          listener(canvas.width, canvas.height);
        });
      },setCanvasSize:function (width, height, noUpdates) {
        var canvas = Module['canvas'];
        canvas.width = width;
        canvas.height = height;
        if (!noUpdates) Browser.updateResizeListeners();
      },windowedWidth:0,windowedHeight:0,setFullScreenCanvasSize:function () {
        var canvas = Module['canvas'];
        this.windowedWidth = canvas.width;
        this.windowedHeight = canvas.height;
        canvas.width = screen.width;
        canvas.height = screen.height;
        var flags = HEAPU32[((SDL.screen+Runtime.QUANTUM_SIZE*0)>>2)];
        flags = flags | 0x00800000; // set SDL_FULLSCREEN flag
        HEAP32[((SDL.screen+Runtime.QUANTUM_SIZE*0)>>2)]=flags
        Browser.updateResizeListeners();
      },setWindowedCanvasSize:function () {
        var canvas = Module['canvas'];
        canvas.width = this.windowedWidth;
        canvas.height = this.windowedHeight;
        var flags = HEAPU32[((SDL.screen+Runtime.QUANTUM_SIZE*0)>>2)];
        flags = flags & ~0x00800000; // clear SDL_FULLSCREEN flag
        HEAP32[((SDL.screen+Runtime.QUANTUM_SIZE*0)>>2)]=flags
        Browser.updateResizeListeners();
      }};
_fgetc.ret = allocate([0], "i8", ALLOC_STATIC);
__ATINIT__.unshift({ func: function() { if (!Module["noFSInit"] && !FS.init.initialized) FS.init() } });__ATMAIN__.push({ func: function() { FS.ignorePermissions = false } });__ATEXIT__.push({ func: function() { FS.quit() } });Module["FS_createFolder"] = FS.createFolder;Module["FS_createPath"] = FS.createPath;Module["FS_createDataFile"] = FS.createDataFile;Module["FS_createPreloadedFile"] = FS.createPreloadedFile;Module["FS_createLazyFile"] = FS.createLazyFile;Module["FS_createLink"] = FS.createLink;Module["FS_createDevice"] = FS.createDevice;
___setErrNo(0);
_llvm_eh_exception.buf = allocate(12, "void*", ALLOC_STATIC);
Module["requestFullScreen"] = function(lockPointer, resizeCanvas) { Browser.requestFullScreen(lockPointer, resizeCanvas) };
  Module["requestAnimationFrame"] = function(func) { Browser.requestAnimationFrame(func) };
  Module["pauseMainLoop"] = function() { Browser.mainLoop.pause() };
  Module["resumeMainLoop"] = function() { Browser.mainLoop.resume() };
var FUNCTION_TABLE = [0,0,__ZNSt3__18messagesIwED0Ev,0,__ZNSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEED0Ev,0,__ZNKSt3__18numpunctIcE12do_falsenameEv,0,__ZNKSt3__120__time_get_c_storageIwE3__rEv,0,__ZNKSt3__110moneypunctIwLb0EE16do_thousands_sepEv
,0,__ZNSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEED1Ev,0,__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE11do_get_yearES4_S4_RNS_8ios_baseERjP2tm,0,__ZNSt12length_errorD0Ev,0,__ZNSt3__115basic_streambufIcNS_11char_traitsIcEEED1Ev,0,__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE11do_get_timeES4_S4_RNS_8ios_baseERjP2tm
,0,__ZNKSt3__15ctypeIcE10do_toupperEc,0,__ZNSt3__16locale2id6__initEv,0,__ZNSt3__110__stdinbufIcED1Ev,0,__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE14do_get_weekdayES4_S4_RNS_8ios_baseERjP2tm,0,__ZNSt3__110__stdinbufIcE9pbackfailEi
,0,__ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE9underflowEv,0,__ZNSt3__111__stdoutbufIwE5imbueERKNS_6localeE,0,__ZNSt3__18time_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEED0Ev,0,__ZNSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEED0Ev,0,__ZNSt11logic_errorD0Ev
,0,__ZNKSt3__17collateIcE7do_hashEPKcS3_,0,__ZNKSt3__120__time_get_c_storageIwE8__monthsEv,0,__ZNSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEED0Ev,0,__ZNKSt3__19money_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_bRNS_8ios_baseEwRKNS_12basic_stringIwS3_NS_9allocatorIwEEEE,0,__ZNKSt3__15ctypeIcE10do_toupperEPcPKc
,0,__ZNKSt3__17codecvtIcc10_mbstate_tE6do_outERS1_PKcS5_RS5_PcS7_RS7_,0,__ZNKSt3__110moneypunctIwLb1EE16do_positive_signEv,0,__ZNKSt3__15ctypeIwE10do_tolowerEPwPKw,0,__ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE5uflowEv,0,__ZNSt3__17collateIcED1Ev
,0,__ZNSt3__18ios_base7failureD2Ev,0,__ZNK10__cxxabiv121__vmi_class_type_info16search_above_dstEPNS_19__dynamic_cast_infoEPKvS4_ib,0,__ZNSt9bad_allocD2Ev,0,__ZNKSt3__17codecvtIDsc10_mbstate_tE10do_unshiftERS1_PcS4_RS4_,0,__ZNSt3__16locale5facetD0Ev
,0,__ZNKSt3__120__time_get_c_storageIwE3__cEv,0,__ZNKSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwy,0,__ZNKSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwx,0,__ZNSt3__15ctypeIcED0Ev,0,__ZNKSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwm
,0,__ZNKSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwl,0,__ZNSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEED1Ev,0,__ZNSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEED1Ev,0,__ZNKSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwe,0,__ZNKSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwd
,0,__ZNKSt3__110moneypunctIcLb1EE16do_decimal_pointEv,0,__ZNKSt3__17codecvtIwc10_mbstate_tE11do_encodingEv,0,__ZNSt3__110__stdinbufIwE5imbueERKNS_6localeE,0,__ZNK10__cxxabiv120__si_class_type_info16search_above_dstEPNS_19__dynamic_cast_infoEPKvS4_ib,0,__ZNKSt3__19money_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_bRNS_8ios_baseEcRKNS_12basic_stringIcS3_NS_9allocatorIcEEEE
,0,__ZNSt3__113basic_ostreamIwNS_11char_traitsIwEEED1Ev,0,__ZNKSt3__17codecvtIDsc10_mbstate_tE13do_max_lengthEv,0,__ZNKSt3__17codecvtIwc10_mbstate_tE9do_lengthERS1_PKcS5_j,0,__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE13do_date_orderEv,0,__ZNSt3__18messagesIcED1Ev
,0,__ZNKSt3__120__time_get_c_storageIwE7__weeksEv,0,__ZNKSt3__18numpunctIwE11do_groupingEv,0,__ZNSt3__16locale5facet16__on_zero_sharedEv,0,__ZNKSt3__15ctypeIwE8do_widenEc,0,__ZNKSt3__18time_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwPK2tmcc
,0,__ZNSt3__110__stdinbufIcE5uflowEv,0,__ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE9pbackfailEj,0,__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE11do_get_timeES4_S4_RNS_8ios_baseERjP2tm,0,__ZTv0_n12_NSt3__113basic_istreamIcNS_11char_traitsIcEEED0Ev,0,__ZNSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEED1Ev
,0,__ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE5uflowEv,0,__ZNKSt3__110moneypunctIwLb0EE13do_neg_formatEv,0,__ZNKSt3__17codecvtIwc10_mbstate_tE5do_inERS1_PKcS5_RS5_PwS7_RS7_,0,__ZNKSt3__17codecvtIDsc10_mbstate_tE5do_inERS1_PKcS5_RS5_PDsS7_RS7_,0,__ZNKSt3__15ctypeIcE8do_widenEc
,0,__ZNSt3__110moneypunctIwLb0EED0Ev,0,__ZNKSt3__17codecvtIDic10_mbstate_tE9do_lengthERS1_PKcS5_j,0,__ZNSt3__16locale5__impD2Ev,0,__ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE9underflowEv,0,__ZNKSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwPKv
,0,__ZNSt3__18numpunctIcED2Ev,0,__ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE8overflowEi,0,__ZNSt3__17codecvtIcc10_mbstate_tED0Ev,0,__ZNKSt3__18numpunctIcE11do_groupingEv,0,__ZNK10__cxxabiv116__shim_type_info5noop1Ev
,0,__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE16do_get_monthnameES4_S4_RNS_8ios_baseERjP2tm,0,__ZNKSt3__120__time_get_c_storageIwE3__xEv,0,__ZNKSt3__17codecvtIcc10_mbstate_tE10do_unshiftERS1_PcS4_RS4_,0,__ZNSt3__110__stdinbufIwE9pbackfailEj,0,__ZNKSt3__18time_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEcPK2tmcc
,0,__ZNSt3__18numpunctIcED0Ev,0,__ZNSt3__111__stdoutbufIcE8overflowEi,0,__ZNSt3__119__iostream_categoryD1Ev,0,__ZNKSt3__120__time_get_c_storageIwE7__am_pmEv,0,__ZNSt3__110__stdinbufIwED0Ev
,0,__ZNKSt3__18messagesIcE8do_closeEi,0,__ZNKSt3__15ctypeIwE5do_isEPKwS3_Pt,0,__ZNSt13runtime_errorD2Ev,0,__ZNKSt3__15ctypeIwE10do_toupperEw,0,__ZNKSt3__15ctypeIwE9do_narrowEPKwS3_cPc
,0,__ZNKSt3__17codecvtIDic10_mbstate_tE11do_encodingEv,0,__ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE5imbueERKNS_6localeE,0,__ZNKSt3__110moneypunctIcLb0EE16do_negative_signEv,0,__ZNSt3__17collateIwED1Ev,0,__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE16do_get_monthnameES4_S4_RNS_8ios_baseERjP2tm
,0,__ZNK10__cxxabiv117__class_type_info9can_catchEPKNS_16__shim_type_infoERPv,0,__ZNKSt8bad_cast4whatEv,0,__ZNSt3__110moneypunctIcLb0EED1Ev,0,__ZNKSt3__18messagesIcE6do_getEiiiRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEE,0,__ZNSt3__18numpunctIwED2Ev
,0,__ZNKSt3__110moneypunctIwLb1EE13do_pos_formatEv,0,__ZNSt3__15ctypeIwED0Ev,0,__ZNKSt13runtime_error4whatEv,0,_free,0,__ZNSt3__19money_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEED0Ev
,0,__ZNSt3__117__widen_from_utf8ILj32EED0Ev,0,__ZNKSt3__18numpunctIwE16do_thousands_sepEv,0,__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjP2tmcc,0,__ZNSt3__113basic_istreamIwNS_11char_traitsIwEEED1Ev,0,__ZNSt3__110__stdinbufIwED1Ev
,0,__ZNKSt3__18numpunctIcE16do_decimal_pointEv,0,__ZNKSt3__110moneypunctIwLb0EE16do_negative_signEv,0,__ZNK10__cxxabiv120__si_class_type_info16search_below_dstEPNS_19__dynamic_cast_infoEPKvib,0,__ZNKSt3__120__time_get_c_storageIcE3__xEv,0,__ZNSt3__17collateIwED0Ev
,0,__ZNKSt3__110moneypunctIcLb0EE16do_positive_signEv,0,__ZNKSt3__17codecvtIDsc10_mbstate_tE16do_always_noconvEv,0,__ZNKSt3__17codecvtIDsc10_mbstate_tE9do_lengthERS1_PKcS5_j,0,__ZNSt11logic_errorD2Ev,0,__ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE7seekoffExNS_8ios_base7seekdirEj
,0,__ZNSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEED1Ev,0,__ZNKSt3__18numpunctIwE16do_decimal_pointEv,0,__ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE4syncEv,0,__ZNK10__cxxabiv117__class_type_info16search_below_dstEPNS_19__dynamic_cast_infoEPKvib,0,__ZNKSt3__110moneypunctIcLb0EE11do_groupingEv
,0,__ZNK10__cxxabiv120__si_class_type_info27has_unambiguous_public_baseEPNS_19__dynamic_cast_infoEPvi,0,__ZNKSt3__110moneypunctIwLb1EE14do_frac_digitsEv,0,__ZNKSt3__110moneypunctIwLb1EE16do_negative_signEv,0,__ZNK10__cxxabiv121__vmi_class_type_info27has_unambiguous_public_baseEPNS_19__dynamic_cast_infoEPvi,0,__ZNKSt3__120__time_get_c_storageIcE3__XEv
,0,__ZNKSt3__15ctypeIwE9do_narrowEwc,0,__ZNSt3__110__stdinbufIcE9underflowEv,0,__ZNSt3__111__stdoutbufIwE4syncEv,0,__ZNSt3__110moneypunctIwLb0EED1Ev,0,__ZNKSt3__110moneypunctIcLb1EE13do_neg_formatEv
,0,__ZNSt3__113basic_istreamIcNS_11char_traitsIcEEED1Ev,0,__ZNKSt3__17codecvtIcc10_mbstate_tE5do_inERS1_PKcS5_RS5_PcS7_RS7_,0,__ZTv0_n12_NSt3__113basic_ostreamIcNS_11char_traitsIcEEED1Ev,0,__ZNSt3__18time_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEED0Ev,0,__ZNSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEED0Ev
,0,__ZNKSt3__17collateIwE7do_hashEPKwS3_,0,__ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE7seekposENS_4fposI10_mbstate_tEEj,0,__ZNSt3__111__stdoutbufIcE5imbueERKNS_6localeE,0,___cxx_global_array_dtor147,0,__ZNKSt3__110moneypunctIcLb1EE16do_thousands_sepEv
,0,__ZNSt3__18ios_baseD0Ev,0,__ZNSt3__110moneypunctIcLb1EED0Ev,0,__ZNSt9bad_allocD0Ev,0,__ZNSt3__115basic_streambufIcNS_11char_traitsIcEEED0Ev,0,__ZNKSt3__17codecvtIwc10_mbstate_tE16do_always_noconvEv
,0,__ZNKSt3__120__time_get_c_storageIcE3__rEv,0,__ZNKSt3__114error_category10equivalentEiRKNS_15error_conditionE,0,__ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE6xsputnEPKci,0,__ZNKSt3__15ctypeIwE10do_scan_isEtPKwS3_,0,__ZNKSt3__17codecvtIDic10_mbstate_tE6do_outERS1_PKDiS5_RS5_PcS7_RS7_
,0,__ZNKSt3__17codecvtIDic10_mbstate_tE13do_max_lengthEv,0,__ZNKSt3__17codecvtIDic10_mbstate_tE5do_inERS1_PKcS5_RS5_PDiS7_RS7_,0,__ZTv0_n12_NSt3__113basic_ostreamIwNS_11char_traitsIwEEED0Ev,0,__ZNSt3__115basic_streambufIwNS_11char_traitsIwEEED1Ev,0,__ZN10__cxxabiv120__si_class_type_infoD0Ev
,0,__ZNKSt3__17collateIwE10do_compareEPKwS3_S3_S3_,0,__ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE6xsgetnEPci,0,__ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRPv,0,__ZNKSt3__15ctypeIcE10do_tolowerEc,0,__ZNKSt3__110moneypunctIwLb1EE13do_neg_formatEv
,0,__ZNKSt3__15ctypeIcE8do_widenEPKcS3_Pc,0,__ZNSt3__17codecvtIwc10_mbstate_tED0Ev,0,__ZNKSt3__110moneypunctIwLb1EE16do_decimal_pointEv,0,__ZNSt3__17codecvtIDsc10_mbstate_tED0Ev,0,__ZNKSt3__120__time_get_c_storageIcE7__weeksEv
,0,__ZNKSt3__18numpunctIwE11do_truenameEv,0,__ZTv0_n12_NSt3__113basic_istreamIcNS_11char_traitsIcEEED1Ev,0,__ZNSt3__110__stdinbufIwE9underflowEv,0,__ZNSt3__18ios_base7failureD0Ev,0,__ZNSt3__19money_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEED0Ev
,0,__ZNSt3__18ios_base4InitD2Ev,0,__ZNKSt3__15ctypeIwE5do_isEtw,0,__ZNSt3__110moneypunctIwLb1EED0Ev,0,__ZTv0_n12_NSt3__113basic_ostreamIwNS_11char_traitsIwEEED1Ev,0,__ZNKSt3__110moneypunctIwLb1EE11do_groupingEv
,0,__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE14do_get_weekdayES4_S4_RNS_8ios_baseERjP2tm,0,__ZNKSt3__17codecvtIDic10_mbstate_tE16do_always_noconvEv,0,__ZNKSt3__17codecvtIwc10_mbstate_tE13do_max_lengthEv,0,__ZNK10__cxxabiv116__shim_type_info5noop2Ev,0,__ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE6setbufEPwi
,0,___cxx_global_array_dtor108,0,__ZNKSt3__18messagesIwE7do_openERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEERKNS_6localeE,0,__ZNSt3__17codecvtIDic10_mbstate_tED0Ev,0,__ZNSt3__111__stdoutbufIcED1Ev,0,__ZNKSt3__110moneypunctIcLb1EE14do_curr_symbolEv
,0,__ZNSt3__16locale5__impD0Ev,0,__ZNK10__cxxabiv117__class_type_info27has_unambiguous_public_baseEPNS_19__dynamic_cast_infoEPvi,0,__ZNKSt3__119__iostream_category4nameEv,0,__ZNKSt3__110moneypunctIcLb0EE14do_frac_digitsEv,0,__ZNKSt3__15ctypeIcE9do_narrowEPKcS3_cPc
,0,__ZNKSt3__110moneypunctIcLb1EE11do_groupingEv,0,__ZNSt3__18time_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEED1Ev,0,__ZNSt3__117__call_once_proxyINS_12_GLOBAL__N_111__fake_bindEEEvPv,0,__ZNSt8bad_castD0Ev,0,__ZNKSt3__15ctypeIcE9do_narrowEcc
,0,__ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRf,0,__ZNSt3__112__do_nothingEPv,0,__ZNSt3__19money_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEED1Ev,0,___cxx_global_array_dtor83,0,___cxx_global_array_dtor80
,0,__ZNSt3__110moneypunctIcLb0EED0Ev,0,__ZNSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEED0Ev,0,__ZNKSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEcPKv,0,__ZNKSt3__18numpunctIwE12do_falsenameEv,0,__ZNSt3__17collateIcED0Ev
,0,__ZNKSt3__110moneypunctIwLb0EE13do_pos_formatEv,0,__ZNKSt3__110moneypunctIcLb1EE16do_negative_signEv,0,__ZNSt3__111__stdoutbufIcED0Ev,0,__ZNSt3__16locale5facetD2Ev,0,__ZTv0_n12_NSt3__113basic_istreamIwNS_11char_traitsIwEEED1Ev
,0,__ZNSt3__112system_errorD0Ev,0,__ZNKSt3__19money_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_bRNS_8ios_baseERjRe,0,__ZNKSt3__17codecvtIcc10_mbstate_tE9do_lengthERS1_PKcS5_j,0,__ZNSt3__110__stdinbufIwE5uflowEv,0,__ZNKSt3__18numpunctIcE11do_truenameEv
,0,__ZNKSt3__110moneypunctIcLb1EE13do_pos_formatEv,0,__ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE7seekposENS_4fposI10_mbstate_tEEj,0,__ZNKSt3__19money_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_bRNS_8ios_baseEwe,0,__ZNKSt3__19money_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_bRNS_8ios_baseERjRe,0,__ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjS8_
,0,__ZNKSt3__18numpunctIcE16do_thousands_sepEv,0,__ZNSt3__19money_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEED0Ev,0,__ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE9showmanycEv,0,__ZNSt3__19money_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEED0Ev,0,__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE11do_get_dateES4_S4_RNS_8ios_baseERjP2tm
,0,__ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE8overflowEj,0,__ZNSt3__19money_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEED1Ev,0,__ZNSt3__18numpunctIwED0Ev,0,__ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE5imbueERKNS_6localeE,0,__ZNKSt3__15ctypeIwE10do_tolowerEw
,0,__ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE4syncEv,0,__ZNSt3__111__stdoutbufIcE4syncEv,0,__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEED1Ev,0,__ZNKSt3__17codecvtIwc10_mbstate_tE10do_unshiftERS1_PcS4_RS4_,0,__ZNKSt3__17collateIcE10do_compareEPKcS3_S3_S3_
,0,___cxx_global_array_dtor132,0,__ZNKSt3__17codecvtIwc10_mbstate_tE6do_outERS1_PKwS5_RS5_PcS7_RS7_,0,__ZNSt3__110__stdinbufIcE5imbueERKNS_6localeE,0,__ZNKSt3__17collateIwE12do_transformEPKwS3_,0,__ZNKSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEcy
,0,__ZNKSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEcx,0,__ZNKSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEce,0,__ZNKSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEcd,0,__ZNKSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEcb,0,__ZNKSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEcm
,0,__ZNKSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEcl,0,__ZNSt8bad_castD2Ev,0,__ZN10__cxxabiv121__vmi_class_type_infoD0Ev,0,__ZNKSt3__110moneypunctIcLb1EE14do_frac_digitsEv,0,__ZNKSt3__17codecvtIDic10_mbstate_tE10do_unshiftERS1_PcS4_RS4_
,0,__ZNKSt3__19money_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_bRNS_8ios_baseERjRNS_12basic_stringIcS3_NS_9allocatorIcEEEE,0,__ZNKSt3__15ctypeIwE10do_toupperEPwPKw,0,__ZTv0_n12_NSt3__113basic_ostreamIcNS_11char_traitsIcEEED0Ev,0,__ZNKSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwb,0,__ZNKSt3__114error_category23default_error_conditionEi
,0,__ZNKSt3__17codecvtIcc10_mbstate_tE13do_max_lengthEv,0,__ZNK10__cxxabiv117__class_type_info16search_above_dstEPNS_19__dynamic_cast_infoEPKvS4_ib,0,__ZNKSt3__17codecvtIcc10_mbstate_tE16do_always_noconvEv,0,__ZNKSt3__18messagesIwE8do_closeEi,0,__ZNSt3__112system_errorD2Ev
,0,__ZNKSt9bad_alloc4whatEv,0,__ZNKSt3__110moneypunctIwLb0EE11do_groupingEv,0,__ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE9showmanycEv,0,__ZNKSt3__110moneypunctIcLb0EE16do_decimal_pointEv,0,__ZNSt3__113basic_istreamIcNS_11char_traitsIcEEED0Ev
,0,__ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRy,0,__ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRx,0,__ZNKSt3__120__time_get_c_storageIcE8__monthsEv,0,__ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRt,0,__ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRPv
,0,__ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRm,0,__ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRl,0,__ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRb,0,__ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRe,0,__ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRd
,0,__ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRf,0,__ZNKSt3__17codecvtIcc10_mbstate_tE11do_encodingEv,0,__ZNKSt3__110moneypunctIcLb0EE16do_thousands_sepEv,0,__ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE6xsgetnEPwi,0,__ZNKSt3__110moneypunctIcLb0EE13do_neg_formatEv
,0,__ZNKSt11logic_error4whatEv,0,__ZNKSt3__119__iostream_category7messageEi,0,__ZNKSt3__110moneypunctIcLb0EE13do_pos_formatEv,0,__ZNSt3__113basic_ostreamIwNS_11char_traitsIwEEED0Ev,0,__ZNKSt3__110moneypunctIwLb0EE16do_decimal_pointEv
,0,__ZNKSt3__17collateIcE12do_transformEPKcS3_,0,__ZNKSt3__114error_category10equivalentERKNS_10error_codeEi,0,__ZNKSt3__110moneypunctIwLb0EE14do_frac_digitsEv,0,__ZNSt3__18messagesIcED0Ev,0,__ZNKSt3__15ctypeIcE10do_tolowerEPcPKc
,0,__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjP2tmcc,0,__ZNKSt3__120__time_get_c_storageIcE7__am_pmEv,0,__ZNKSt3__110moneypunctIcLb0EE14do_curr_symbolEv,0,__ZNKSt3__15ctypeIwE8do_widenEPKcS3_Pw,0,__ZNKSt3__110moneypunctIwLb1EE16do_thousands_sepEv
,0,__ZNK10__cxxabiv121__vmi_class_type_info16search_below_dstEPNS_19__dynamic_cast_infoEPKvib,0,__ZNSt3__18ios_baseD2Ev,0,__ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEED1Ev,0,__ZNSt3__110__stdinbufIcED0Ev,0,__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE13do_date_orderEv
,0,__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE11do_get_yearES4_S4_RNS_8ios_baseERjP2tm,0,__ZNSt3__119__iostream_categoryD0Ev,0,__ZNSt3__110moneypunctIwLb1EED1Ev,0,__ZNKSt3__110moneypunctIwLb0EE14do_curr_symbolEv,0,__ZNKSt3__18messagesIcE7do_openERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEERKNS_6localeE
,0,__ZNSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEED0Ev,0,__ZNSt3__110moneypunctIcLb1EED1Ev,0,__ZNSt3__111__stdoutbufIwED0Ev,0,__ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE7seekoffExNS_8ios_base7seekdirEj,0,__ZNKSt3__120__time_get_c_storageIcE3__cEv
,0,__ZNSt3__17codecvtIwc10_mbstate_tED2Ev,0,__ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE6setbufEPci,0,__ZNKSt3__110moneypunctIwLb0EE16do_positive_signEv,0,__ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjS8_,0,__ZNSt3__19money_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEED1Ev
,0,__ZNKSt3__120__time_get_c_storageIwE3__XEv,0,__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE11do_get_dateES4_S4_RNS_8ios_baseERjP2tm,0,__ZTv0_n12_NSt3__113basic_istreamIwNS_11char_traitsIwEEED0Ev,0,__ZNSt3__115basic_streambufIwNS_11char_traitsIwEEED0Ev,0,__ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE9pbackfailEi
,0,__ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEED0Ev,0,__ZNSt3__111__stdoutbufIwE8overflowEj,0,__ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRy,0,__ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRx,0,__ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRt
,0,__ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRm,0,__ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRl,0,__ZNKSt3__19money_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_bRNS_8ios_baseEce,0,__ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRe,0,__ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRd
,0,__ZNSt3__116__narrow_to_utf8ILj32EED0Ev,0,__ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRb,0,___cxx_global_array_dtor,0,__ZNSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEED1Ev,0,__ZN10__cxxabiv117__class_type_infoD0Ev
,0,__ZNSt3__18messagesIwED1Ev,0,__ZNSt3__111__stdoutbufIwED1Ev,0,__ZNKSt3__19money_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_bRNS_8ios_baseERjRNS_12basic_stringIwS3_NS_9allocatorIwEEEE,0,__ZN10__cxxabiv116__shim_type_infoD2Ev,0,__ZNKSt3__15ctypeIwE11do_scan_notEtPKwS3_
,0,__ZNKSt3__110moneypunctIwLb1EE14do_curr_symbolEv,0,__ZNSt3__18time_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEED1Ev,0,__ZNKSt3__17codecvtIDsc10_mbstate_tE6do_outERS1_PKDsS5_RS5_PcS7_RS7_,0,__ZNKSt3__18messagesIwE6do_getEiiiRKNS_12basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEEE,0,__ZNKSt3__17codecvtIDsc10_mbstate_tE11do_encodingEv
,0,__ZNKSt3__110moneypunctIcLb1EE16do_positive_signEv,0,__ZNSt3__19money_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEED1Ev,0,__ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE6xsputnEPKwi,0,__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEED1Ev,0,__ZNSt3__15ctypeIcED2Ev,0,__ZNSt13runtime_errorD0Ev,0,__ZNSt3__113basic_istreamIwNS_11char_traitsIwEEED0Ev,0];
// EMSCRIPTEN_START_FUNCS
function __ZNSt3__18ios_base4InitD2Ev(r1){__ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEE5flushEv(5255004);__ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEE5flushEv(5255088);__ZNSt3__113basic_ostreamIwNS_11char_traitsIwEEE5flushEv(5254648);__ZNSt3__113basic_ostreamIwNS_11char_traitsIwEEE5flushEv(5254732);return}function __ZNSt3__111__stdoutbufIwED1Ev(r1){var r2;HEAP32[r1>>2]=5248996;r2=HEAP32[r1+4>>2];r1=r2+4|0;if(((tempValue=HEAP32[r1>>2],HEAP32[r1>>2]=tempValue+ -1,tempValue)|0)!=0){return}FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+8>>2]](r2|0);return}function __ZNSt3__111__stdoutbufIwED0Ev(r1){var r2,r3,r4;HEAP32[r1>>2]=5248996;r2=HEAP32[r1+4>>2];r3=r2+4|0;if(((tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+ -1,tempValue)|0)!=0){r4=r1;__ZdlPv(r4);return}FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+8>>2]](r2|0);r4=r1;__ZdlPv(r4);return}function __ZNSt3__111__stdoutbufIwE4syncEv(r1){var r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12;r2=0;r3=STACKTOP;STACKTOP=STACKTOP+12|0;r4=r3;r5=r3+8;r6=r1+36|0;r7=r1+40|0;r8=r4|0;r9=r4+8|0;r10=r4;r4=r1+32|0;while(1){r1=HEAP32[r6>>2];r11=FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+20>>2]](r1,r7,r8,r9,r5);r1=HEAP32[r5>>2]-r10|0;if((_fwrite(r8,1,r1,HEAP32[r4>>2])|0)!=(r1|0)){r12=-1;r2=17;break}if((r11|0)==2){r12=-1;r2=18;break}else if((r11|0)!=1){r2=15;break}}if(r2==15){r12=((_fflush(HEAP32[r4>>2])|0)!=0)<<31>>31;STACKTOP=r3;return r12}else if(r2==17){STACKTOP=r3;return r12}else if(r2==18){STACKTOP=r3;return r12}}function __ZNSt3__111__stdoutbufIwE8overflowEj(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26;r3=0;r4=STACKTOP;STACKTOP=STACKTOP+20|0;r5=r4;r6=r4+8;r7=r4+12;r8=r4+16;r9=(r2|0)==-1;if(!r9){r10=r6+4|0;r11=(r1+24|0)>>2;r12=(r1+20|0)>>2;HEAP32[r12]=r6;r13=(r1+28|0)>>2;HEAP32[r13]=r10;HEAP32[r6>>2]=r2;HEAP32[r11]=r10;L23:do{if((HEAP8[r1+48|0]&1)<<24>>24==0){r14=r5|0;HEAP32[r7>>2]=r14;r15=r1+36|0;r16=r1+40|0;r17=r5+8|0;r18=r5;r19=r1+32|0;r20=r6;r21=r10;while(1){r22=HEAP32[r15>>2];r23=FUNCTION_TABLE[HEAP32[HEAP32[r22>>2]+12>>2]](r22,r16,r20,r21,r8,r14,r17,r7);r24=HEAP32[r12];if((HEAP32[r8>>2]|0)==(r24|0)){r25=-1;r3=36;break}if((r23|0)==3){r3=26;break}if(r23>>>0>=2){r25=-1;r3=37;break}r22=HEAP32[r7>>2]-r18|0;if((_fwrite(r14,1,r22,HEAP32[r19>>2])|0)!=(r22|0)){r25=-1;r3=38;break}if((r23|0)!=1){break L23}r23=HEAP32[r8>>2];r22=HEAP32[r11];HEAP32[r12]=r23;HEAP32[r13]=r22;r26=(r22-r23>>2<<2)+r23|0;HEAP32[r11]=r26;r20=r23;r21=r26}if(r3==26){if((_fwrite(r24,1,1,HEAP32[r19>>2])|0)==1){break}else{r25=-1}STACKTOP=r4;return r25}else if(r3==36){STACKTOP=r4;return r25}else if(r3==37){STACKTOP=r4;return r25}else if(r3==38){STACKTOP=r4;return r25}}else{if((_fwrite(r6,4,1,HEAP32[r1+32>>2])|0)==1){break}else{r25=-1}STACKTOP=r4;return r25}}while(0);HEAP32[r11]=0;HEAP32[r12]=0;HEAP32[r13]=0}r25=r9?0:r2;STACKTOP=r4;return r25}function __ZNSt3__110__stdinbufIwED1Ev(r1){var r2;HEAP32[r1>>2]=5248996;r2=HEAP32[r1+4>>2];r1=r2+4|0;if(((tempValue=HEAP32[r1>>2],HEAP32[r1>>2]=tempValue+ -1,tempValue)|0)!=0){return}FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+8>>2]](r2|0);return}function __ZNSt3__110__stdinbufIwED0Ev(r1){var r2,r3,r4;HEAP32[r1>>2]=5248996;r2=HEAP32[r1+4>>2];r3=r2+4|0;if(((tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+ -1,tempValue)|0)!=0){r4=r1;__ZdlPv(r4);return}FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+8>>2]](r2|0);r4=r1;__ZdlPv(r4);return}function __ZNSt3__18ios_base4InitC2Ev(r1){var r2;__ZNSt3__110__stdinbufIcEC2EP7__sFILE(5254404,HEAP32[_stdin>>2]);HEAP32[1313814]=5249268;HEAP32[1313816]=5249288;HEAP32[1313815]=0;HEAP32[1313822]=5254404;HEAP32[1313820]=0;HEAP32[1313821]=0;HEAP32[1313817]=4098;HEAP32[1313819]=0;HEAP32[1313818]=6;_memset(5255296,0,40);r1=HEAP32[__ZNSt3__16locale8__globalEv()>>2];HEAP32[1313823]=r1;r2=r1+4|0;tempValue=HEAP32[r2>>2],HEAP32[r2>>2]=tempValue+1,tempValue;HEAP32[1313834]=0;HEAP32[1313835]=-1;__ZNSt3__111__stdoutbufIcEC2EP7__sFILE(5254300,HEAP32[_stdout>>2]);HEAP32[1313751]=5249180;HEAP32[1313752]=5249200;HEAP32[1313758]=5254300;HEAP32[1313756]=0;HEAP32[1313757]=0;HEAP32[1313753]=4098;HEAP32[1313755]=0;HEAP32[1313754]=6;_memset(5255040,0,40);r2=HEAP32[__ZNSt3__16locale8__globalEv()>>2];HEAP32[1313759]=r2;r1=r2+4|0;tempValue=HEAP32[r1>>2],HEAP32[r1>>2]=tempValue+1,tempValue;HEAP32[1313770]=0;HEAP32[1313771]=-1;__ZNSt3__111__stdoutbufIcEC2EP7__sFILE(5254352,HEAP32[_stderr>>2]);HEAP32[1313793]=5249180;HEAP32[1313794]=5249200;HEAP32[1313800]=5254352;HEAP32[1313798]=0;HEAP32[1313799]=0;HEAP32[1313795]=4098;HEAP32[1313797]=0;HEAP32[1313796]=6;_memset(5255208,0,40);r1=HEAP32[__ZNSt3__16locale8__globalEv()>>2];HEAP32[1313801]=r1;r2=r1+4|0;tempValue=HEAP32[r2>>2],HEAP32[r2>>2]=tempValue+1,tempValue;HEAP32[1313812]=0;HEAP32[1313813]=-1;r2=HEAP32[HEAP32[HEAP32[1313793]-12>>2]+5255196>>2];HEAP32[1313772]=5249180;HEAP32[1313773]=5249200;HEAP32[1313779]=r2;HEAP32[1313777]=(r2|0)==0&1;HEAP32[1313778]=0;HEAP32[1313774]=4098;HEAP32[1313776]=0;HEAP32[1313775]=6;_memset(5255124,0,40);r2=HEAP32[__ZNSt3__16locale8__globalEv()>>2];HEAP32[1313780]=r2;r1=r2+4|0;tempValue=HEAP32[r1>>2],HEAP32[r1>>2]=tempValue+1,tempValue;HEAP32[1313791]=0;HEAP32[1313792]=-1;HEAP32[HEAP32[HEAP32[1313814]-12>>2]+5255328>>2]=5255004;r1=HEAP32[HEAP32[1313793]-12>>2]+5255176|0;HEAP32[r1>>2]=HEAP32[r1>>2]|8192;HEAP32[HEAP32[HEAP32[1313793]-12>>2]+5255244>>2]=5255004;__ZNSt3__110__stdinbufIwEC2EP7__sFILE(5254244,HEAP32[_stdin>>2]);HEAP32[1313729]=5249224;HEAP32[1313731]=5249244;HEAP32[1313730]=0;HEAP32[1313737]=5254244;HEAP32[1313735]=0;HEAP32[1313736]=0;HEAP32[1313732]=4098;HEAP32[1313734]=0;HEAP32[1313733]=6;_memset(5254956,0,40);r1=HEAP32[__ZNSt3__16locale8__globalEv()>>2];HEAP32[1313738]=r1;r2=r1+4|0;tempValue=HEAP32[r2>>2],HEAP32[r2>>2]=tempValue+1,tempValue;HEAP32[1313749]=0;HEAP32[1313750]=-1;__ZNSt3__111__stdoutbufIwEC2EP7__sFILE(5254140,HEAP32[_stdout>>2]);HEAP32[1313662]=5249136;HEAP32[1313663]=5249156;HEAP32[1313669]=5254140;HEAP32[1313667]=0;HEAP32[1313668]=0;HEAP32[1313664]=4098;HEAP32[1313666]=0;HEAP32[1313665]=6;_memset(5254684,0,40);r2=HEAP32[__ZNSt3__16locale8__globalEv()>>2];HEAP32[1313670]=r2;r1=r2+4|0;tempValue=HEAP32[r1>>2],HEAP32[r1>>2]=tempValue+1,tempValue;HEAP32[1313681]=0;HEAP32[1313682]=-1;__ZNSt3__111__stdoutbufIwEC2EP7__sFILE(5254192,HEAP32[_stderr>>2]);HEAP32[1313704]=5249136;HEAP32[1313705]=5249156;HEAP32[1313711]=5254192;HEAP32[1313709]=0;HEAP32[1313710]=0;HEAP32[1313706]=4098;HEAP32[1313708]=0;HEAP32[1313707]=6;_memset(5254852,0,40);r1=HEAP32[__ZNSt3__16locale8__globalEv()>>2];HEAP32[1313712]=r1;r2=r1+4|0;tempValue=HEAP32[r2>>2],HEAP32[r2>>2]=tempValue+1,tempValue;HEAP32[1313723]=0;HEAP32[1313724]=-1;r2=HEAP32[HEAP32[HEAP32[1313704]-12>>2]+5254840>>2];HEAP32[1313683]=5249136;HEAP32[1313684]=5249156;HEAP32[1313690]=r2;HEAP32[1313688]=(r2|0)==0&1;HEAP32[1313689]=0;HEAP32[1313685]=4098;HEAP32[1313687]=0;HEAP32[1313686]=6;_memset(5254768,0,40);r2=HEAP32[__ZNSt3__16locale8__globalEv()>>2];HEAP32[1313691]=r2;r1=r2+4|0;tempValue=HEAP32[r1>>2],HEAP32[r1>>2]=tempValue+1,tempValue;HEAP32[1313702]=0;HEAP32[1313703]=-1;HEAP32[HEAP32[HEAP32[1313729]-12>>2]+5254988>>2]=5254648;r1=HEAP32[HEAP32[1313704]-12>>2]+5254820|0;HEAP32[r1>>2]=HEAP32[r1>>2]|8192;HEAP32[HEAP32[HEAP32[1313704]-12>>2]+5254888>>2]=5254648;return}function __ZNSt3__111__stdoutbufIwEC2EP7__sFILE(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23;r3=STACKTOP;STACKTOP=STACKTOP+12|0;r4=r3,r5=r4>>2;r6=r1|0;HEAP32[r6>>2]=5248996;r7=r1+4|0;r8=HEAP32[__ZNSt3__16locale8__globalEv()>>2];HEAP32[r7>>2]=r8;r9=r8+4|0;tempValue=HEAP32[r9>>2],HEAP32[r9>>2]=tempValue+1,tempValue;r9=(r1+8|0)>>2;HEAP32[r9]=0;HEAP32[r9+1]=0;HEAP32[r9+2]=0;HEAP32[r9+3]=0;HEAP32[r9+4]=0;HEAP32[r9+5]=0;HEAP32[r6>>2]=5249332;HEAP32[r1+32>>2]=r2;r2=r1+36|0;r6=HEAP32[r7>>2],r7=r6>>2;r9=(r6+4|0)>>2;tempValue=HEAP32[r9],HEAP32[r9]=tempValue+1,tempValue;if((HEAP32[1313653]|0)!=-1){HEAP32[r5]=5254612;HEAP32[r5+1]=24;HEAP32[r5+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254612,r4,406)}r4=HEAP32[1313654]-1|0;r5=HEAP32[r7+2];do{if(HEAP32[r7+3]-r5>>2>>>0>r4>>>0){r8=HEAP32[r5+(r4<<2)>>2];if((r8|0)==0){break}if(((tempValue=HEAP32[r9],HEAP32[r9]=tempValue+ -1,tempValue)|0)!=0){r10=r8;HEAP32[r2>>2]=r10;r11=r1+40|0;r12=r11;r13=0;r14=0;r15=r12|0;HEAP32[r15>>2]=r13;r16=r12+4|0;HEAP32[r16>>2]=r14;r17=r1+48|0;r18=r8;r19=HEAP32[r18>>2];r20=r19+28|0;r21=HEAP32[r20>>2];r22=FUNCTION_TABLE[r21](r10);r23=r22&1;HEAP8[r17]=r23;STACKTOP=r3;return}FUNCTION_TABLE[HEAP32[HEAP32[r7]+8>>2]](r6|0);r10=r8;HEAP32[r2>>2]=r10;r11=r1+40|0;r12=r11;r13=0;r14=0;r15=r12|0;HEAP32[r15>>2]=r13;r16=r12+4|0;HEAP32[r16>>2]=r14;r17=r1+48|0;r18=r8;r19=HEAP32[r18>>2];r20=r19+28|0;r21=HEAP32[r20>>2];r22=FUNCTION_TABLE[r21](r10);r23=r22&1;HEAP8[r17]=r23;STACKTOP=r3;return}}while(0);r3=___cxa_allocate_exception(4);HEAP32[r3>>2]=5247488;___cxa_throw(r3,5252700,514)}function __ZNSt3__111__stdoutbufIwE5imbueERKNS_6localeE(r1,r2){var r3,r4,r5,r6,r7,r8;r3=STACKTOP;STACKTOP=STACKTOP+12|0;r4=r3,r5=r4>>2;FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+24>>2]](r1);r6=HEAP32[r2>>2];if((HEAP32[1313653]|0)!=-1){HEAP32[r5]=5254612;HEAP32[r5+1]=24;HEAP32[r5+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254612,r4,406)}r4=HEAP32[1313654]-1|0;r5=HEAP32[r6+8>>2];if(HEAP32[r6+12>>2]-r5>>2>>>0<=r4>>>0){r7=___cxa_allocate_exception(4);r8=r7;HEAP32[r8>>2]=5247488;___cxa_throw(r7,5252700,514)}r6=HEAP32[r5+(r4<<2)>>2];if((r6|0)==0){r7=___cxa_allocate_exception(4);r8=r7;HEAP32[r8>>2]=5247488;___cxa_throw(r7,5252700,514)}else{r7=r6;HEAP32[r1+36>>2]=r7;HEAP8[r1+48|0]=FUNCTION_TABLE[HEAP32[HEAP32[r6>>2]+28>>2]](r7)&1;STACKTOP=r3;return}}function __ZNSt3__110__stdinbufIwEC2EP7__sFILE(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10,r11,r12;r3=STACKTOP;STACKTOP=STACKTOP+12|0;r4=r3,r5=r4>>2;r6=r1|0;HEAP32[r6>>2]=5248996;r7=r1+4|0;r8=HEAP32[__ZNSt3__16locale8__globalEv()>>2];HEAP32[r7>>2]=r8;r9=r8+4|0;tempValue=HEAP32[r9>>2],HEAP32[r9>>2]=tempValue+1,tempValue;r9=(r1+8|0)>>2;HEAP32[r9]=0;HEAP32[r9+1]=0;HEAP32[r9+2]=0;HEAP32[r9+3]=0;HEAP32[r9+4]=0;HEAP32[r9+5]=0;HEAP32[r6>>2]=5249708;HEAP32[r1+32>>2]=r2;r2=r1+40|0;HEAP32[r2>>2]=0;HEAP32[r2+4>>2]=0;r2=HEAP32[r7>>2],r7=r2>>2;r6=(r2+4|0)>>2;tempValue=HEAP32[r6],HEAP32[r6]=tempValue+1,tempValue;if((HEAP32[1313653]|0)!=-1){HEAP32[r5]=5254612;HEAP32[r5+1]=24;HEAP32[r5+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254612,r4,406)}r4=HEAP32[1313654]-1|0;r5=HEAP32[r7+2];do{if(HEAP32[r7+3]-r5>>2>>>0>r4>>>0){r9=HEAP32[r5+(r4<<2)>>2];if((r9|0)==0){break}r8=r9;r10=r1+36|0;HEAP32[r10>>2]=r8;r11=r1+48|0;HEAP32[r11>>2]=FUNCTION_TABLE[HEAP32[HEAP32[r9>>2]+24>>2]](r8);r8=HEAP32[r10>>2];HEAP8[r1+52|0]=FUNCTION_TABLE[HEAP32[HEAP32[r8>>2]+28>>2]](r8)&1;if((HEAP32[r11>>2]|0)<=8){if(((tempValue=HEAP32[r6],HEAP32[r6]=tempValue+ -1,tempValue)|0)!=0){STACKTOP=r3;return}FUNCTION_TABLE[HEAP32[HEAP32[r7]+8>>2]](r2|0);STACKTOP=r3;return}r11=___cxa_allocate_exception(8);HEAP32[r11>>2]=5247512;r8=r11+4|0;if((r8|0)!=0){r10=__Znaj(50),r9=r10>>2;HEAP32[r9+1]=37;HEAP32[r9]=37;r12=r10+12|0;HEAP32[r8>>2]=r12;HEAP32[r9+2]=0;_memcpy(r12,5243144,38)}___cxa_throw(r11,5252712,186)}}while(0);r3=___cxa_allocate_exception(4);HEAP32[r3>>2]=5247488;___cxa_throw(r3,5252700,514)}function __ZNSt3__110__stdinbufIwE9underflowEv(r1){return __ZNSt3__110__stdinbufIwE9__getcharEb(r1,0)}function __ZNSt3__110__stdinbufIwE5uflowEv(r1){return __ZNSt3__110__stdinbufIwE9__getcharEb(r1,1)}function __ZNSt3__110__stdinbufIwE9pbackfailEj(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10,r11,r12;r3=0;r4=STACKTOP;STACKTOP=STACKTOP+20|0;r5=r4;r6=r4+8,r7=r6>>2;r8=r4+12;if((r2|0)==-1){r9=-1;STACKTOP=r4;return r9}HEAP32[r8>>2]=r2;r10=HEAP32[r1+36>>2];r11=r5|0;r12=FUNCTION_TABLE[HEAP32[HEAP32[r10>>2]+12>>2]](r10,r1+40|0,r8,r8+4|0,r4+16,r11,r5+8|0,r6);if((r12|0)==2|(r12|0)==1){r9=-1;STACKTOP=r4;return r9}else if((r12|0)==3){HEAP8[r11]=r2&255;HEAP32[r7]=r5+1|0}r5=r1+32|0;while(1){r1=HEAP32[r7];if(r1>>>0<=r11>>>0){r9=r2;r3=128;break}r12=r1-1|0;HEAP32[r7]=r12;if((_ungetc(HEAP8[r12]<<24>>24,HEAP32[r5>>2])|0)==-1){r9=-1;r3=129;break}}if(r3==128){STACKTOP=r4;return r9}else if(r3==129){STACKTOP=r4;return r9}}function __ZNSt3__110__stdinbufIwE9__getcharEb(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24;r3=0;r4=STACKTOP;STACKTOP=STACKTOP+20|0;r5=r4;r6=r4+8,r7=r6>>2;r8=r4+12;r9=r4+16;r10=HEAP32[r1+48>>2];r11=(r10|0)>1?r10:1;L125:do{if((r11|0)>0){r10=r1+32|0;r12=0;while(1){r13=_fgetc(HEAP32[r10>>2]);if((r13|0)==-1){r14=-1;break}HEAP8[r5+r12|0]=r13&255;r13=r12+1|0;if((r13|0)<(r11|0)){r12=r13}else{break L125}}STACKTOP=r4;return r14}}while(0);L132:do{if((HEAP8[r1+52|0]&1)<<24>>24==0){r12=r1+40|0;r10=r12>>2;r13=r1+36|0;r15=r5|0;r16=r6+4|0;r17=r1+32|0;r18=r11;while(1){r19=HEAP32[r10];r20=HEAP32[r10+1];r21=HEAP32[r13>>2];r22=r5+r18|0;r23=FUNCTION_TABLE[HEAP32[HEAP32[r21>>2]+16>>2]](r21,r12,r15,r22,r8,r6,r16,r9);if((r23|0)==3){r3=142;break}else if((r23|0)==2){r14=-1;r3=153;break}else if((r23|0)!=1){r24=r18;break L132}HEAP32[r10]=r19;HEAP32[r10+1]=r20;if((r18|0)==8){r14=-1;r3=154;break}r20=_fgetc(HEAP32[r17>>2]);if((r20|0)==-1){r14=-1;r3=155;break}HEAP8[r22]=r20&255;r18=r18+1|0}if(r3==142){HEAP32[r7]=HEAP8[r15]<<24>>24;r24=r18;break}else if(r3==153){STACKTOP=r4;return r14}else if(r3==154){STACKTOP=r4;return r14}else if(r3==155){STACKTOP=r4;return r14}}else{HEAP32[r7]=HEAP8[r5|0]<<24>>24;r24=r11}}while(0);L146:do{if(!r2){r11=r1+32|0;r3=r24;while(1){if((r3|0)<=0){break L146}r9=r3-1|0;if((_ungetc(HEAP8[r5+r9|0]<<24>>24,HEAP32[r11>>2])|0)==-1){r14=-1;break}else{r3=r9}}STACKTOP=r4;return r14}}while(0);r14=HEAP32[r7];STACKTOP=r4;return r14}function __ZNSt3__111__stdoutbufIcED1Ev(r1){var r2;HEAP32[r1>>2]=5249064;r2=HEAP32[r1+4>>2];r1=r2+4|0;if(((tempValue=HEAP32[r1>>2],HEAP32[r1>>2]=tempValue+ -1,tempValue)|0)!=0){return}FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+8>>2]](r2|0);return}function __ZNSt3__111__stdoutbufIcED0Ev(r1){var r2,r3,r4;HEAP32[r1>>2]=5249064;r2=HEAP32[r1+4>>2];r3=r2+4|0;if(((tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+ -1,tempValue)|0)!=0){r4=r1;__ZdlPv(r4);return}FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+8>>2]](r2|0);r4=r1;__ZdlPv(r4);return}function __ZNSt3__111__stdoutbufIcE4syncEv(r1){var r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12;r2=0;r3=STACKTOP;STACKTOP=STACKTOP+12|0;r4=r3;r5=r3+8;r6=r1+36|0;r7=r1+40|0;r8=r4|0;r9=r4+8|0;r10=r4;r4=r1+32|0;while(1){r1=HEAP32[r6>>2];r11=FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+20>>2]](r1,r7,r8,r9,r5);r1=HEAP32[r5>>2]-r10|0;if((_fwrite(r8,1,r1,HEAP32[r4>>2])|0)!=(r1|0)){r12=-1;r2=173;break}if((r11|0)==2){r12=-1;r2=171;break}else if((r11|0)!=1){r2=169;break}}if(r2==171){STACKTOP=r3;return r12}else if(r2==173){STACKTOP=r3;return r12}else if(r2==169){r12=((_fflush(HEAP32[r4>>2])|0)!=0)<<31>>31;STACKTOP=r3;return r12}}function __ZNSt3__111__stdoutbufIcE8overflowEi(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26;r3=0;r4=STACKTOP;STACKTOP=STACKTOP+20|0;r5=r4;r6=r4+8;r7=r4+12;r8=r4+16;r9=(r2|0)==-1;if(!r9){r10=r6+1|0;r11=(r1+24|0)>>2;r12=(r1+20|0)>>2;HEAP32[r12]=r6;r13=(r1+28|0)>>2;HEAP32[r13]=r10;HEAP8[r6]=r2&255;HEAP32[r11]=r10;L176:do{if((HEAP8[r1+48|0]&1)<<24>>24==0){r14=r5|0;HEAP32[r7>>2]=r14;r15=r1+36|0;r16=r1+40|0;r17=r5+8|0;r18=r5;r19=r1+32|0;r20=r6;r21=r10;while(1){r22=HEAP32[r15>>2];r23=FUNCTION_TABLE[HEAP32[HEAP32[r22>>2]+12>>2]](r22,r16,r20,r21,r8,r14,r17,r7);r24=HEAP32[r12];if((HEAP32[r8>>2]|0)==(r24|0)){r25=-1;r3=188;break}if((r23|0)==3){r3=180;break}if(r23>>>0>=2){r25=-1;r3=191;break}r22=HEAP32[r7>>2]-r18|0;if((_fwrite(r14,1,r22,HEAP32[r19>>2])|0)!=(r22|0)){r25=-1;r3=190;break}if((r23|0)!=1){break L176}r23=HEAP32[r8>>2];r22=HEAP32[r11];HEAP32[r12]=r23;HEAP32[r13]=r22;r26=r23+(r22-r23)|0;HEAP32[r11]=r26;r20=r23;r21=r26}if(r3==188){STACKTOP=r4;return r25}else if(r3==190){STACKTOP=r4;return r25}else if(r3==180){if((_fwrite(r24,1,1,HEAP32[r19>>2])|0)==1){break}else{r25=-1}STACKTOP=r4;return r25}else if(r3==191){STACKTOP=r4;return r25}}else{if((_fwrite(r6,1,1,HEAP32[r1+32>>2])|0)==1){break}else{r25=-1}STACKTOP=r4;return r25}}while(0);HEAP32[r11]=0;HEAP32[r12]=0;HEAP32[r13]=0}r25=r9?0:r2;STACKTOP=r4;return r25}function __ZNSt3__110__stdinbufIcED1Ev(r1){var r2;HEAP32[r1>>2]=5249064;r2=HEAP32[r1+4>>2];r1=r2+4|0;if(((tempValue=HEAP32[r1>>2],HEAP32[r1>>2]=tempValue+ -1,tempValue)|0)!=0){return}FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+8>>2]](r2|0);return}function __ZNSt3__110__stdinbufIcED0Ev(r1){var r2,r3,r4;HEAP32[r1>>2]=5249064;r2=HEAP32[r1+4>>2];r3=r2+4|0;if(((tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+ -1,tempValue)|0)!=0){r4=r1;__ZdlPv(r4);return}FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+8>>2]](r2|0);r4=r1;__ZdlPv(r4);return}function __ZNSt3__110__stdinbufIwE5imbueERKNS_6localeE(r1,r2){var r3,r4,r5,r6,r7,r8;r3=STACKTOP;STACKTOP=STACKTOP+12|0;r4=r3,r5=r4>>2;r6=HEAP32[r2>>2];if((HEAP32[1313653]|0)!=-1){HEAP32[r5]=5254612;HEAP32[r5+1]=24;HEAP32[r5+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254612,r4,406)}r4=HEAP32[1313654]-1|0;r5=HEAP32[r6+8>>2];if(HEAP32[r6+12>>2]-r5>>2>>>0<=r4>>>0){r7=___cxa_allocate_exception(4);r8=r7;HEAP32[r8>>2]=5247488;___cxa_throw(r7,5252700,514)}r6=HEAP32[r5+(r4<<2)>>2];if((r6|0)==0){r7=___cxa_allocate_exception(4);r8=r7;HEAP32[r8>>2]=5247488;___cxa_throw(r7,5252700,514)}r7=r6;r8=r1+36|0;HEAP32[r8>>2]=r7;r4=r1+48|0;HEAP32[r4>>2]=FUNCTION_TABLE[HEAP32[HEAP32[r6>>2]+24>>2]](r7);r7=HEAP32[r8>>2];HEAP8[r1+52|0]=FUNCTION_TABLE[HEAP32[HEAP32[r7>>2]+28>>2]](r7)&1;if((HEAP32[r4>>2]|0)>8){__ZNSt3__121__throw_runtime_errorEPKc(5243144)}else{STACKTOP=r3;return}}function __ZNSt3__111__stdoutbufIcEC2EP7__sFILE(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23;r3=STACKTOP;STACKTOP=STACKTOP+12|0;r4=r3,r5=r4>>2;r6=r1|0;HEAP32[r6>>2]=5249064;r7=r1+4|0;r8=HEAP32[__ZNSt3__16locale8__globalEv()>>2];HEAP32[r7>>2]=r8;r9=r8+4|0;tempValue=HEAP32[r9>>2],HEAP32[r9>>2]=tempValue+1,tempValue;r9=(r1+8|0)>>2;HEAP32[r9]=0;HEAP32[r9+1]=0;HEAP32[r9+2]=0;HEAP32[r9+3]=0;HEAP32[r9+4]=0;HEAP32[r9+5]=0;HEAP32[r6>>2]=5249400;HEAP32[r1+32>>2]=r2;r2=r1+36|0;r6=HEAP32[r7>>2],r7=r6>>2;r9=(r6+4|0)>>2;tempValue=HEAP32[r9],HEAP32[r9]=tempValue+1,tempValue;if((HEAP32[1313655]|0)!=-1){HEAP32[r5]=5254620;HEAP32[r5+1]=24;HEAP32[r5+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254620,r4,406)}r4=HEAP32[1313656]-1|0;r5=HEAP32[r7+2];do{if(HEAP32[r7+3]-r5>>2>>>0>r4>>>0){r8=HEAP32[r5+(r4<<2)>>2];if((r8|0)==0){break}if(((tempValue=HEAP32[r9],HEAP32[r9]=tempValue+ -1,tempValue)|0)!=0){r10=r8;HEAP32[r2>>2]=r10;r11=r1+40|0;r12=r11;r13=0;r14=0;r15=r12|0;HEAP32[r15>>2]=r13;r16=r12+4|0;HEAP32[r16>>2]=r14;r17=r1+48|0;r18=r8;r19=HEAP32[r18>>2];r20=r19+28|0;r21=HEAP32[r20>>2];r22=FUNCTION_TABLE[r21](r10);r23=r22&1;HEAP8[r17]=r23;STACKTOP=r3;return}FUNCTION_TABLE[HEAP32[HEAP32[r7]+8>>2]](r6|0);r10=r8;HEAP32[r2>>2]=r10;r11=r1+40|0;r12=r11;r13=0;r14=0;r15=r12|0;HEAP32[r15>>2]=r13;r16=r12+4|0;HEAP32[r16>>2]=r14;r17=r1+48|0;r18=r8;r19=HEAP32[r18>>2];r20=r19+28|0;r21=HEAP32[r20>>2];r22=FUNCTION_TABLE[r21](r10);r23=r22&1;HEAP8[r17]=r23;STACKTOP=r3;return}}while(0);r3=___cxa_allocate_exception(4);HEAP32[r3>>2]=5247488;___cxa_throw(r3,5252700,514)}function __ZNSt3__111__stdoutbufIcE5imbueERKNS_6localeE(r1,r2){var r3,r4,r5,r6,r7,r8;r3=STACKTOP;STACKTOP=STACKTOP+12|0;r4=r3,r5=r4>>2;FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+24>>2]](r1);r6=HEAP32[r2>>2];if((HEAP32[1313655]|0)!=-1){HEAP32[r5]=5254620;HEAP32[r5+1]=24;HEAP32[r5+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254620,r4,406)}r4=HEAP32[1313656]-1|0;r5=HEAP32[r6+8>>2];if(HEAP32[r6+12>>2]-r5>>2>>>0<=r4>>>0){r7=___cxa_allocate_exception(4);r8=r7;HEAP32[r8>>2]=5247488;___cxa_throw(r7,5252700,514)}r6=HEAP32[r5+(r4<<2)>>2];if((r6|0)==0){r7=___cxa_allocate_exception(4);r8=r7;HEAP32[r8>>2]=5247488;___cxa_throw(r7,5252700,514)}else{r7=r6;HEAP32[r1+36>>2]=r7;HEAP8[r1+48|0]=FUNCTION_TABLE[HEAP32[HEAP32[r6>>2]+28>>2]](r7)&1;STACKTOP=r3;return}}function __ZNSt3__110__stdinbufIcEC2EP7__sFILE(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10,r11,r12;r3=STACKTOP;STACKTOP=STACKTOP+12|0;r4=r3,r5=r4>>2;r6=r1|0;HEAP32[r6>>2]=5249064;r7=r1+4|0;r8=HEAP32[__ZNSt3__16locale8__globalEv()>>2];HEAP32[r7>>2]=r8;r9=r8+4|0;tempValue=HEAP32[r9>>2],HEAP32[r9>>2]=tempValue+1,tempValue;r9=(r1+8|0)>>2;HEAP32[r9]=0;HEAP32[r9+1]=0;HEAP32[r9+2]=0;HEAP32[r9+3]=0;HEAP32[r9+4]=0;HEAP32[r9+5]=0;HEAP32[r6>>2]=5249776;HEAP32[r1+32>>2]=r2;r2=r1+40|0;HEAP32[r2>>2]=0;HEAP32[r2+4>>2]=0;r2=HEAP32[r7>>2],r7=r2>>2;r6=(r2+4|0)>>2;tempValue=HEAP32[r6],HEAP32[r6]=tempValue+1,tempValue;if((HEAP32[1313655]|0)!=-1){HEAP32[r5]=5254620;HEAP32[r5+1]=24;HEAP32[r5+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254620,r4,406)}r4=HEAP32[1313656]-1|0;r5=HEAP32[r7+2];do{if(HEAP32[r7+3]-r5>>2>>>0>r4>>>0){r9=HEAP32[r5+(r4<<2)>>2];if((r9|0)==0){break}r8=r9;r10=r1+36|0;HEAP32[r10>>2]=r8;r11=r1+48|0;HEAP32[r11>>2]=FUNCTION_TABLE[HEAP32[HEAP32[r9>>2]+24>>2]](r8);r8=HEAP32[r10>>2];HEAP8[r1+52|0]=FUNCTION_TABLE[HEAP32[HEAP32[r8>>2]+28>>2]](r8)&1;if((HEAP32[r11>>2]|0)<=8){if(((tempValue=HEAP32[r6],HEAP32[r6]=tempValue+ -1,tempValue)|0)!=0){STACKTOP=r3;return}FUNCTION_TABLE[HEAP32[HEAP32[r7]+8>>2]](r2|0);STACKTOP=r3;return}r11=___cxa_allocate_exception(8);HEAP32[r11>>2]=5247512;r8=r11+4|0;if((r8|0)!=0){r10=__Znaj(50),r9=r10>>2;HEAP32[r9+1]=37;HEAP32[r9]=37;r12=r10+12|0;HEAP32[r8>>2]=r12;HEAP32[r9+2]=0;_memcpy(r12,5243144,38)}___cxa_throw(r11,5252712,186)}}while(0);r3=___cxa_allocate_exception(4);HEAP32[r3>>2]=5247488;___cxa_throw(r3,5252700,514)}function __ZNKSt11logic_error4whatEv(r1){return HEAP32[r1+4>>2]}function __ZNKSt13runtime_error4whatEv(r1){return HEAP32[r1+4>>2]}function __ZNKSt3__114error_category23default_error_conditionEi(r1,r2,r3){HEAP32[r1>>2]=r3;HEAP32[r1+4>>2]=r2;return}function __ZNKSt3__114error_category10equivalentERKNS_10error_codeEi(r1,r2,r3){var r4;if((HEAP32[r2+4>>2]|0)!=(r1|0)){r4=0;return r4}r4=(HEAP32[r2>>2]|0)==(r3|0);return r4}function __ZNSt3__110__stdinbufIcE9underflowEv(r1){return __ZNSt3__110__stdinbufIcE9__getcharEb(r1,0)}function __ZNSt3__110__stdinbufIcE5uflowEv(r1){return __ZNSt3__110__stdinbufIcE9__getcharEb(r1,1)}function __ZNSt3__110__stdinbufIcE9pbackfailEi(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13;r3=0;r4=STACKTOP;STACKTOP=STACKTOP+20|0;r5=r4;r6=r4+8,r7=r6>>2;r8=r4+12;if((r2|0)==-1){r9=-1;STACKTOP=r4;return r9}r10=r2&255;HEAP8[r8]=r10;r11=HEAP32[r1+36>>2];r12=r5|0;r13=FUNCTION_TABLE[HEAP32[HEAP32[r11>>2]+12>>2]](r11,r1+40|0,r8,r8+1|0,r4+16,r12,r5+8|0,r6);if((r13|0)==3){HEAP8[r12]=r10;HEAP32[r7]=r5+1|0}else if((r13|0)==2|(r13|0)==1){r9=-1;STACKTOP=r4;return r9}r13=r1+32|0;while(1){r1=HEAP32[r7];if(r1>>>0<=r12>>>0){r9=r2;r3=284;break}r5=r1-1|0;HEAP32[r7]=r5;if((_ungetc(HEAP8[r5]<<24>>24,HEAP32[r13>>2])|0)==-1){r9=-1;r3=283;break}}if(r3==283){STACKTOP=r4;return r9}else if(r3==284){STACKTOP=r4;return r9}}function __ZNSt3__110__stdinbufIcE9__getcharEb(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23;r3=0;r4=STACKTOP;STACKTOP=STACKTOP+20|0;r5=r4;r6=r4+8;r7=r4+12;r8=r4+16;r9=HEAP32[r1+48>>2];r10=(r9|0)>1?r9:1;L290:do{if((r10|0)>0){r9=r1+32|0;r11=0;while(1){r12=_fgetc(HEAP32[r9>>2]);if((r12|0)==-1){r13=-1;break}HEAP8[r5+r11|0]=r12&255;r12=r11+1|0;if((r12|0)<(r10|0)){r11=r12}else{break L290}}STACKTOP=r4;return r13}}while(0);L297:do{if((HEAP8[r1+52|0]&1)<<24>>24==0){r11=r1+40|0;r9=r11>>2;r12=r1+36|0;r14=r5|0;r15=r6+1|0;r16=r1+32|0;r17=r10;while(1){r18=HEAP32[r9];r19=HEAP32[r9+1];r20=HEAP32[r12>>2];r21=r5+r17|0;r22=FUNCTION_TABLE[HEAP32[HEAP32[r20>>2]+16>>2]](r20,r11,r14,r21,r7,r6,r15,r8);if((r22|0)==3){r3=297;break}else if((r22|0)==2){r13=-1;r3=309;break}else if((r22|0)!=1){r23=r17;break L297}HEAP32[r9]=r18;HEAP32[r9+1]=r19;if((r17|0)==8){r13=-1;r3=307;break}r19=_fgetc(HEAP32[r16>>2]);if((r19|0)==-1){r13=-1;r3=308;break}HEAP8[r21]=r19&255;r17=r17+1|0}if(r3==297){HEAP8[r6]=HEAP8[r14];r23=r17;break}else if(r3==307){STACKTOP=r4;return r13}else if(r3==308){STACKTOP=r4;return r13}else if(r3==309){STACKTOP=r4;return r13}}else{HEAP8[r6]=HEAP8[r5|0];r23=r10}}while(0);L311:do{if(!r2){r10=r1+32|0;r3=r23;while(1){if((r3|0)<=0){break L311}r8=r3-1|0;if((_ungetc(HEAP8[r5+r8|0]<<24>>24,HEAP32[r10>>2])|0)==-1){r13=-1;break}else{r3=r8}}STACKTOP=r4;return r13}}while(0);r13=HEAPU8[r6];STACKTOP=r4;return r13}function __GLOBAL__I_a(){__ZNSt3__18ios_base4InitC2Ev(0);_atexit(362,5255344,___dso_handle);return}function __ZNSt11logic_errorD0Ev(r1){var r2,r3,r4;HEAP32[r1>>2]=5247560;r2=r1+4|0;r3=HEAP32[r2>>2]-4|0;do{if(((tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+ -1,tempValue)-1|0)<0){r4=HEAP32[r2>>2]-12|0;if((r4|0)==0){break}__ZdaPv(r4)}}while(0);__ZdlPv(r1);return}function __ZNSt11logic_errorD2Ev(r1){var r2,r3,r4;HEAP32[r1>>2]=5247560;r2=r1+4|0;r3=HEAP32[r2>>2]-4|0;if(((tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+ -1,tempValue)-1|0)>=0){r4=r1|0;return}r3=HEAP32[r2>>2]-12|0;if((r3|0)==0){r4=r1|0;return}__ZdaPv(r3);r4=r1|0;return}function __ZNSt13runtime_errorD0Ev(r1){var r2,r3,r4;HEAP32[r1>>2]=5247512;r2=r1+4|0;r3=HEAP32[r2>>2]-4|0;do{if(((tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+ -1,tempValue)-1|0)<0){r4=HEAP32[r2>>2]-12|0;if((r4|0)==0){break}__ZdaPv(r4)}}while(0);__ZdlPv(r1);return}function __ZNSt13runtime_errorD2Ev(r1){var r2,r3,r4;HEAP32[r1>>2]=5247512;r2=r1+4|0;r3=HEAP32[r2>>2]-4|0;if(((tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+ -1,tempValue)-1|0)>=0){r4=r1|0;return}r3=HEAP32[r2>>2]-12|0;if((r3|0)==0){r4=r1|0;return}__ZdaPv(r3);r4=r1|0;return}function __ZNSt12length_errorD0Ev(r1){var r2,r3,r4;HEAP32[r1>>2]=5247560;r2=r1+4|0;r3=HEAP32[r2>>2]-4|0;do{if(((tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+ -1,tempValue)-1|0)<0){r4=HEAP32[r2>>2]-12|0;if((r4|0)==0){break}__ZdaPv(r4)}}while(0);__ZdlPv(r1);return}function __ZNKSt3__114error_category10equivalentEiRKNS_15error_conditionE(r1,r2,r3){var r4,r5,r6;r4=STACKTOP;STACKTOP=STACKTOP+8|0;r5=r4;FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+12>>2]](r5,r1,r2);if((HEAP32[r5+4>>2]|0)!=(HEAP32[r3+4>>2]|0)){r6=0;STACKTOP=r4;return r6}r6=(HEAP32[r5>>2]|0)==(HEAP32[r3>>2]|0);STACKTOP=r4;return r6}function __ZNSt3__112system_errorD0Ev(r1){var r2,r3,r4;HEAP32[r1>>2]=5247512;r2=r1+4|0;r3=HEAP32[r2>>2]-4|0;do{if(((tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+ -1,tempValue)-1|0)<0){r4=HEAP32[r2>>2]-12|0;if((r4|0)==0){break}__ZdaPv(r4)}}while(0);__ZdlPv(r1);return}function __ZNSt3__112system_errorD2Ev(r1){var r2,r3,r4;HEAP32[r1>>2]=5247512;r2=r1+4|0;r3=HEAP32[r2>>2]-4|0;if(((tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+ -1,tempValue)-1|0)>=0){r4=r1|0;return}r3=HEAP32[r2>>2]-12|0;if((r3|0)==0){r4=r1|0;return}__ZdaPv(r3);r4=r1|0;return}function __ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEED1Ev(r1){if((HEAP8[r1]&1)<<24>>24==0){return}__ZdlPv(HEAP32[r1+8>>2]);return}function __ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEED1Ev(r1){if((HEAP8[r1]&1)<<24>>24==0){return}__ZdlPv(HEAP32[r1+8>>2]);return}function __ZNSt3__110__stdinbufIcE5imbueERKNS_6localeE(r1,r2){var r3,r4,r5,r6,r7,r8;r3=STACKTOP;STACKTOP=STACKTOP+12|0;r4=r3,r5=r4>>2;r6=HEAP32[r2>>2];if((HEAP32[1313655]|0)!=-1){HEAP32[r5]=5254620;HEAP32[r5+1]=24;HEAP32[r5+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254620,r4,406)}r4=HEAP32[1313656]-1|0;r5=HEAP32[r6+8>>2];if(HEAP32[r6+12>>2]-r5>>2>>>0<=r4>>>0){r7=___cxa_allocate_exception(4);r8=r7;HEAP32[r8>>2]=5247488;___cxa_throw(r7,5252700,514)}r6=HEAP32[r5+(r4<<2)>>2];if((r6|0)==0){r7=___cxa_allocate_exception(4);r8=r7;HEAP32[r8>>2]=5247488;___cxa_throw(r7,5252700,514)}r7=r6;r8=r1+36|0;HEAP32[r8>>2]=r7;r4=r1+48|0;HEAP32[r4>>2]=FUNCTION_TABLE[HEAP32[HEAP32[r6>>2]+24>>2]](r7);r7=HEAP32[r8>>2];HEAP8[r1+52|0]=FUNCTION_TABLE[HEAP32[HEAP32[r7>>2]+28>>2]](r7)&1;if((HEAP32[r4>>2]|0)>8){__ZNSt3__121__throw_runtime_errorEPKc(5243144)}else{STACKTOP=r3;return}}function __ZNSt3__112system_error6__initERKNS_10error_codeENS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEE(r1,r2,r3){var r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18;r4=STACKTOP;STACKTOP=STACKTOP+12|0;r5=r4,r6=r5>>2;r7=r2|0;r8=HEAP32[r7>>2];r9=r3,r10=r9>>2;do{if((r8|0)!=0){r11=HEAPU8[r9];if((r11&1|0)==0){r12=r11>>>1}else{r12=HEAP32[r3+4>>2]}if((r12|0)==0){r13=r8}else{__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendEPKcj(r3,5244356,2);r13=HEAP32[r7>>2]}r11=HEAP32[r2+4>>2];FUNCTION_TABLE[HEAP32[HEAP32[r11>>2]+24>>2]](r5,r11,r13);r11=r5;r14=HEAP8[r11];if((r14&1)<<24>>24==0){r15=r5+1|0}else{r15=HEAP32[r6+2]}r16=r14&255;if((r16&1|0)==0){r17=r16>>>1}else{r17=HEAP32[r6+1]}__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendEPKcj(r3,r15,r17);if((HEAP8[r11]&1)<<24>>24==0){break}__ZdlPv(HEAP32[r6+2])}}while(0);if((HEAP8[r9]&1)<<24>>24==0){r9=r1>>2;HEAP32[r9]=HEAP32[r10];HEAP32[r9+1]=HEAP32[r10+1];HEAP32[r9+2]=HEAP32[r10+2];STACKTOP=r4;return}r10=HEAP32[r3+8>>2];r9=HEAP32[r3+4>>2];if((r9|0)==-1){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if(r9>>>0<11){HEAP8[r1]=r9<<1&255;r18=r1+1|0}else{r3=r9+16&-16;r6=__Znwj(r3);HEAP32[r1+8>>2]=r6;HEAP32[r1>>2]=r3|1;HEAP32[r1+4>>2]=r9;r18=r6}_memcpy(r18,r10,r9);HEAP8[r18+r9|0]=0;STACKTOP=r4;return}function __ZNSt3__112system_errorC2ENS_10error_codeEPKc(r1,r2,r3){var r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22;r4=STACKTOP;STACKTOP=STACKTOP+24|0;r5=r2;r2=STACKTOP;STACKTOP=STACKTOP+8|0;HEAP32[r2>>2]=HEAP32[r5>>2];HEAP32[r2+4>>2]=HEAP32[r5+4>>2];r5=r4;r6=r4+12,r7=r6>>2;r8=_strlen(r3);if((r8|0)==-1){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if(r8>>>0<11){HEAP8[r6]=r8<<1&255;r9=r6+1|0}else{r10=r8+16&-16;r11=__Znwj(r10);HEAP32[r7+2]=r11;HEAP32[r7]=r10|1;HEAP32[r7+1]=r8;r9=r11}_memcpy(r9,r3,r8);HEAP8[r9+r8|0]=0;__ZNSt3__112system_error6__initERKNS_10error_codeENS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEE(r5,r2,r6);r8=(r1|0)>>2;HEAP32[r8]=5247512;r9=r1+4|0;r3=r5;if((r9|0)!=0){if((HEAP8[r3]&1)<<24>>24==0){r12=r5+1|0}else{r12=HEAP32[r5+8>>2]}r11=_strlen(r12);r10=__Znaj(r11+13|0),r13=r10>>2;HEAP32[r13+1]=r11;HEAP32[r13]=r11;r11=r10+12|0;HEAP32[r9>>2]=r11;HEAP32[r13+2]=0;_strcpy(r11,r12)}if((HEAP8[r3]&1)<<24>>24!=0){__ZdlPv(HEAP32[r5+8>>2])}if((HEAP8[r6]&1)<<24>>24==0){HEAP32[r8]=5249308;r14=r1+8|0;r15=r2;r16=r14;r17=r15|0;r18=HEAP32[r17>>2];r19=r15+4|0;r20=HEAP32[r19>>2];r21=r16|0;HEAP32[r21>>2]=r18;r22=r16+4|0;HEAP32[r22>>2]=r20;STACKTOP=r4;return}__ZdlPv(HEAP32[r7+2]);HEAP32[r8]=5249308;r14=r1+8|0;r15=r2;r16=r14;r17=r15|0;r18=HEAP32[r17>>2];r19=r15+4|0;r20=HEAP32[r19>>2];r21=r16|0;HEAP32[r21>>2]=r18;r22=r16+4|0;HEAP32[r22>>2]=r20;STACKTOP=r4;return}function __ZNSt3__111__call_onceERVmPvPFvS2_E(r1,r2,r3){var r4;r4=r1>>2;L450:do{if((HEAP32[r4]|0)==1){while(1){_pthread_cond_wait(5254464,5254460);if((HEAP32[r4]|0)!=1){break L450}}}}while(0);if((HEAP32[r4]|0)!=0){return}HEAP32[r4]=1;FUNCTION_TABLE[r3](r2);HEAP32[r4]=-1;_pthread_cond_broadcast(5254464);return}function __ZNKSt3__119__iostream_category4nameEv(r1){return 5244212}function __ZNSt3__119__iostream_categoryD1Ev(r1){return}function __ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10;if((r1|0)==(r2|0)){return r1}r3=HEAP8[r2];if((r3&1)<<24>>24==0){r4=r2+1|0}else{r4=HEAP32[r2+8>>2]}r5=r3&255;if((r5&1|0)==0){r6=r5>>>1}else{r6=HEAP32[r2+4>>2]}r2=r1;r5=r1;r3=HEAP8[r5];if((r3&1)<<24>>24==0){r7=10;r8=r3}else{r3=HEAP32[r1>>2];r7=(r3&-2)-1|0;r8=r3&255}if(r7>>>0<r6>>>0){r3=r8&255;if((r3&1|0)==0){r9=r3>>>1}else{r9=HEAP32[r1+4>>2]}__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE21__grow_by_and_replaceEjjjjjjPKc(r1,r7,r6-r7|0,r9,0,r9,r6,r4);return r1}if((r8&1)<<24>>24==0){r10=r2+1|0}else{r10=HEAP32[r1+8>>2]}_memmove(r10,r4,r6,1,0);HEAP8[r10+r6|0]=0;if((HEAP8[r5]&1)<<24>>24==0){HEAP8[r5]=r6<<1&255;return r1}else{HEAP32[r1+4>>2]=r6;return r1}}function __ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10;r3=_strlen(r2);r4=r1;r5=r1;r6=HEAP8[r5];if((r6&1)<<24>>24==0){r7=10;r8=r6}else{r6=HEAP32[r1>>2];r7=(r6&-2)-1|0;r8=r6&255}if(r7>>>0<r3>>>0){r6=r8&255;if((r6&1|0)==0){r9=r6>>>1}else{r9=HEAP32[r1+4>>2]}__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE21__grow_by_and_replaceEjjjjjjPKc(r1,r7,r3-r7|0,r9,0,r9,r3,r2);return r1}if((r8&1)<<24>>24==0){r10=r4+1|0}else{r10=HEAP32[r1+8>>2]}_memmove(r10,r2,r3,1,0);HEAP8[r10+r3|0]=0;if((HEAP8[r5]&1)<<24>>24==0){HEAP8[r5]=r3<<1&255;return r1}else{HEAP32[r1+4>>2]=r3;return r1}}function __ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendEPKcj(r1,r2,r3){var r4,r5,r6,r7,r8,r9;r4=r1;r5=HEAP8[r4];if((r5&1)<<24>>24==0){r6=10;r7=r5}else{r5=HEAP32[r1>>2];r6=(r5&-2)-1|0;r7=r5&255}r5=r7&255;if((r5&1|0)==0){r8=r5>>>1}else{r8=HEAP32[r1+4>>2]}if((r6-r8|0)>>>0<r3>>>0){__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE21__grow_by_and_replaceEjjjjjjPKc(r1,r6,r3-r6+r8|0,r8,r8,0,r3,r2);return r1}if((r3|0)==0){return r1}if((r7&1)<<24>>24==0){r9=r1+1|0}else{r9=HEAP32[r1+8>>2]}_memcpy(r9+r8|0,r2,r3);r2=r8+r3|0;if((HEAP8[r4]&1)<<24>>24==0){HEAP8[r4]=r2<<1&255}else{HEAP32[r1+4>>2]=r2}HEAP8[r9+r2|0]=0;return r1}function __ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(r1,r2,r3){var r4,r5,r6,r7,r8,r9;r4=r1;r5=HEAP8[r4];if((r5&1)<<24>>24==0){r6=1;r7=r5}else{r5=HEAP32[r1>>2];r6=(r5&-2)-1|0;r7=r5&255}if(r6>>>0<r3>>>0){r5=r7&255;if((r5&1|0)==0){r8=r5>>>1}else{r8=HEAP32[r1+4>>2]}__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE21__grow_by_and_replaceEjjjjjjPKw(r1,r6,r3-r6|0,r8,0,r8,r3,r2);return r1}if((r7&1)<<24>>24==0){r9=r1+4|0}else{r9=HEAP32[r1+8>>2]}_wmemmove(r9,r2,r3);HEAP32[r9+(r3<<2)>>2]=0;if((HEAP8[r4]&1)<<24>>24==0){HEAP8[r4]=r3<<1&255;return r1}else{HEAP32[r1+4>>2]=r3;return r1}}function __ZNSt3__18ios_base7failureD0Ev(r1){var r2,r3,r4;HEAP32[r1>>2]=5247512;r2=r1+4|0;r3=HEAP32[r2>>2]-4|0;do{if(((tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+ -1,tempValue)-1|0)<0){r4=HEAP32[r2>>2]-12|0;if((r4|0)==0){break}__ZdaPv(r4)}}while(0);__ZdlPv(r1);return}function __ZNSt3__18ios_base7failureD2Ev(r1){var r2,r3,r4;HEAP32[r1>>2]=5247512;r2=r1+4|0;r3=HEAP32[r2>>2]-4|0;if(((tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+ -1,tempValue)-1|0)>=0){r4=r1|0;return}r3=HEAP32[r2>>2]-12|0;if((r3|0)==0){r4=r1|0;return}__ZdaPv(r3);r4=r1|0;return}function __ZNSt3__18ios_baseD0Ev(r1){__ZNSt3__18ios_baseD2Ev(r1);__ZdlPv(r1);return}function __ZNSt3__18ios_baseD2Ev(r1){var r2,r3,r4,r5,r6,r7;r2=r1>>2;HEAP32[r2]=5248104;r3=HEAP32[r2+10];L579:do{if((r3|0)!=0){r4=r1+32|0;r5=r1+36|0;r6=r3;while(1){r7=r6-1|0;FUNCTION_TABLE[HEAP32[HEAP32[r4>>2]+(r7<<2)>>2]](0,r1,HEAP32[HEAP32[r5>>2]+(r7<<2)>>2]);if((r7|0)==0){break L579}else{r6=r7}}}}while(0);r1=HEAP32[r2+7];r3=r1+4|0;if(((tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1)}_free(HEAP32[r2+8]);_free(HEAP32[r2+9]);_free(HEAP32[r2+12]);_free(HEAP32[r2+15]);return}function __ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(r1){var r2,r3,r4,r5;r1=___cxa_allocate_exception(8);HEAP32[r1>>2]=5247560;r2=r1+4|0;if((r2|0)!=0){r3=__Znaj(25),r4=r3>>2;HEAP32[r4+1]=12;HEAP32[r4]=12;r5=r3+12|0;HEAP32[r2>>2]=r5;HEAP32[r4+2]=0;_memcpy(r5,5243828,13)}HEAP32[r1>>2]=5247536;___cxa_throw(r1,5252724,248)}function __ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE21__grow_by_and_replaceEjjjjjjPKc(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19;if((-3-r2|0)>>>0<r3>>>0){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if((HEAP8[r1]&1)<<24>>24==0){r9=r1+1|0}else{r9=HEAP32[r1+8>>2]}do{if(r2>>>0<2147483631){r10=r3+r2|0;r11=r2<<1;r12=r10>>>0<r11>>>0?r11:r10;if(r12>>>0<11){r13=11;break}r13=r12+16&-16}else{r13=-2}}while(0);r3=__Znwj(r13);if((r5|0)!=0){_memcpy(r3,r9,r5)}if((r7|0)!=0){_memcpy(r3+r5|0,r8,r7)}r8=r4-r6|0;if((r8|0)!=(r5|0)){_memcpy(r3+r7+r5|0,r9+r6+r5|0,r8-r5|0)}if((r2|0)==10){r14=r1+8|0;HEAP32[r14>>2]=r3;r15=r13|1;r16=r1|0;HEAP32[r16>>2]=r15;r17=r8+r7|0;r18=r1+4|0;HEAP32[r18>>2]=r17;r19=r3+r17|0;HEAP8[r19]=0;return}__ZdlPv(r9);r14=r1+8|0;HEAP32[r14>>2]=r3;r15=r13|1;r16=r1|0;HEAP32[r16>>2]=r15;r17=r8+r7|0;r18=r1+4|0;HEAP32[r18>>2]=r17;r19=r3+r17|0;HEAP8[r19]=0;return}function __ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE9__grow_byEjjjjjj(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15;if((-3-r2|0)>>>0<r3>>>0){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if((HEAP8[r1]&1)<<24>>24==0){r8=r1+1|0}else{r8=HEAP32[r1+8>>2]}do{if(r2>>>0<2147483631){r9=r3+r2|0;r10=r2<<1;r11=r9>>>0<r10>>>0?r10:r9;if(r11>>>0<11){r12=11;break}r12=r11+16&-16}else{r12=-2}}while(0);r3=__Znwj(r12);if((r5|0)!=0){_memcpy(r3,r8,r5)}r11=r4-r6|0;if((r11|0)!=(r5|0)){_memcpy(r3+r7+r5|0,r8+r6+r5|0,r11-r5|0)}if((r2|0)==10){r13=r1+8|0;HEAP32[r13>>2]=r3;r14=r12|1;r15=r1|0;HEAP32[r15>>2]=r14;return}__ZdlPv(r8);r13=r1+8|0;HEAP32[r13>>2]=r3;r14=r12|1;r15=r1|0;HEAP32[r15>>2]=r14;return}function __ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE21__grow_by_and_replaceEjjjjjjPKw(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19;if((1073741821-r2|0)>>>0<r3>>>0){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if((HEAP8[r1]&1)<<24>>24==0){r9=r1+4|0}else{r9=HEAP32[r1+8>>2]}do{if(r2>>>0<536870895){r10=r3+r2|0;r11=r2<<1;r12=r10>>>0<r11>>>0?r11:r10;if(r12>>>0<2){r13=2;break}r13=r12+4&-4}else{r13=1073741822}}while(0);r3=__Znwj(r13<<2);if((r5|0)!=0){_wmemcpy(r3,r9,r5)}if((r7|0)!=0){_wmemcpy((r5<<2)+r3|0,r8,r7)}r8=r4-r6|0;if((r8|0)!=(r5|0)){_wmemcpy((r7+r5<<2)+r3|0,(r6+r5<<2)+r9|0,r8-r5|0)}if((r2|0)==1){r14=r1+8|0;HEAP32[r14>>2]=r3;r15=r13|1;r16=r1|0;HEAP32[r16>>2]=r15;r17=r8+r7|0;r18=r1+4|0;HEAP32[r18>>2]=r17;r19=(r17<<2)+r3|0;HEAP32[r19>>2]=0;return}__ZdlPv(r9);r14=r1+8|0;HEAP32[r14>>2]=r3;r15=r13|1;r16=r1|0;HEAP32[r16>>2]=r15;r17=r8+r7|0;r18=r1+4|0;HEAP32[r18>>2]=r17;r19=(r17<<2)+r3|0;HEAP32[r19>>2]=0;return}function __ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE9__grow_byEjjjjjj(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15;if((1073741821-r2|0)>>>0<r3>>>0){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if((HEAP8[r1]&1)<<24>>24==0){r8=r1+4|0}else{r8=HEAP32[r1+8>>2]}do{if(r2>>>0<536870895){r9=r3+r2|0;r10=r2<<1;r11=r9>>>0<r10>>>0?r10:r9;if(r11>>>0<2){r12=2;break}r12=r11+4&-4}else{r12=1073741822}}while(0);r3=__Znwj(r12<<2);if((r5|0)!=0){_wmemcpy(r3,r8,r5)}r11=r4-r6|0;if((r11|0)!=(r5|0)){_wmemcpy((r7+r5<<2)+r3|0,(r6+r5<<2)+r8|0,r11-r5|0)}if((r2|0)==1){r13=r1+8|0;HEAP32[r13>>2]=r3;r14=r12|1;r15=r1|0;HEAP32[r15>>2]=r14;return}__ZdlPv(r8);r13=r1+8|0;HEAP32[r13>>2]=r3;r14=r12|1;r15=r1|0;HEAP32[r15>>2]=r14;return}function __ZNKSt3__119__iostream_category7messageEi(r1,r2,r3){var r4,r5,r6;r2=r1>>2;if((r3|0)==1){r4=__Znwj(48);HEAP32[r2+2]=r4;HEAP32[r2]=49;HEAP32[r2+1]=35;_memcpy(r4,5244428,35);HEAP8[r4+35|0]=0;return}r4=_strerror(r3);r3=_strlen(r4);if((r3|0)==-1){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if(r3>>>0<11){HEAP8[r1]=r3<<1&255;r5=r1+1|0}else{r1=r3+16&-16;r6=__Znwj(r1);HEAP32[r2+2]=r6;HEAP32[r2]=r1|1;HEAP32[r2+1]=r3;r5=r6}_memcpy(r5,r4,r3);HEAP8[r5+r3|0]=0;return}function __ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE5imbueERKNS_6localeE(r1,r2){return}function __ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE6setbufEPci(r1,r2,r3){return r1}function __ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE4syncEv(r1){return 0}function __ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE9showmanycEv(r1){return 0}function __ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE9underflowEv(r1){return-1}function __ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE9pbackfailEi(r1,r2){return-1}function __ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE8overflowEi(r1,r2){return-1}function __ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE5imbueERKNS_6localeE(r1,r2){return}function __ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE6setbufEPwi(r1,r2,r3){return r1}function __ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE4syncEv(r1){return 0}function __ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE9showmanycEv(r1){return 0}function __ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE9underflowEv(r1){return-1}function __ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE9pbackfailEj(r1,r2){return-1}function __ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE8overflowEj(r1,r2){return-1}function __ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE7seekoffExNS_8ios_base7seekdirEj(r1,r2,r3,r4,r5,r6){r6=r1;HEAP32[r6>>2]=0;HEAP32[r6+4>>2]=0;r6=r1+8|0;HEAP32[r6>>2]=-1;HEAP32[r6+4>>2]=-1;return}function __ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE7seekposENS_4fposI10_mbstate_tEEj(r1,r2,r3,r4){r4=STACKTOP;r2=r3>>2;r3=STACKTOP;STACKTOP=STACKTOP+16|0;HEAP32[r3>>2]=HEAP32[r2];HEAP32[r3+4>>2]=HEAP32[r2+1];HEAP32[r3+8>>2]=HEAP32[r2+2];HEAP32[r3+12>>2]=HEAP32[r2+3];r2=r1;HEAP32[r2>>2]=0;HEAP32[r2+4>>2]=0;r2=r1+8|0;HEAP32[r2>>2]=-1;HEAP32[r2+4>>2]=-1;STACKTOP=r4;return}function __ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE7seekoffExNS_8ios_base7seekdirEj(r1,r2,r3,r4,r5,r6){r6=r1;HEAP32[r6>>2]=0;HEAP32[r6+4>>2]=0;r6=r1+8|0;HEAP32[r6>>2]=-1;HEAP32[r6+4>>2]=-1;return}function __ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE7seekposENS_4fposI10_mbstate_tEEj(r1,r2,r3,r4){r4=STACKTOP;r2=r3>>2;r3=STACKTOP;STACKTOP=STACKTOP+16|0;HEAP32[r3>>2]=HEAP32[r2];HEAP32[r3+4>>2]=HEAP32[r2+1];HEAP32[r3+8>>2]=HEAP32[r2+2];HEAP32[r3+12>>2]=HEAP32[r2+3];r2=r1;HEAP32[r2>>2]=0;HEAP32[r2+4>>2]=0;r2=r1+8|0;HEAP32[r2>>2]=-1;HEAP32[r2+4>>2]=-1;STACKTOP=r4;return}function __ZNSt3__115basic_streambufIcNS_11char_traitsIcEEED1Ev(r1){var r2;HEAP32[r1>>2]=5249064;r2=HEAP32[r1+4>>2];r1=r2+4|0;if(((tempValue=HEAP32[r1>>2],HEAP32[r1>>2]=tempValue+ -1,tempValue)|0)!=0){return}FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+8>>2]](r2|0);return}function __ZNSt3__115basic_streambufIcNS_11char_traitsIcEEED0Ev(r1){var r2,r3,r4;HEAP32[r1>>2]=5249064;r2=HEAP32[r1+4>>2];r3=r2+4|0;if(((tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+ -1,tempValue)|0)!=0){r4=r1;__ZdlPv(r4);return}FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+8>>2]](r2|0);r4=r1;__ZdlPv(r4);return}function __ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE6xsgetnEPci(r1,r2,r3){var r4,r5,r6,r7,r8,r9,r10,r11;r4=0;r5=r1;if((r3|0)<=0){r6=0;return r6}r7=r1+12|0;r8=r1+16|0;r9=r2;r2=0;while(1){r10=HEAP32[r7>>2];if(r10>>>0<HEAP32[r8>>2]>>>0){HEAP32[r7>>2]=r10+1|0;r11=HEAP8[r10]}else{r10=FUNCTION_TABLE[HEAP32[HEAP32[r5>>2]+40>>2]](r1);if((r10|0)==-1){r6=r2;r4=674;break}r11=r10&255}HEAP8[r9]=r11;r10=r2+1|0;if((r10|0)<(r3|0)){r9=r9+1|0;r2=r10}else{r6=r10;r4=672;break}}if(r4==672){return r6}else if(r4==674){return r6}}function __ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE5uflowEv(r1){var r2,r3;if((FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+36>>2]](r1)|0)==-1){r2=-1;return r2}r3=r1+12|0;r1=HEAP32[r3>>2];HEAP32[r3>>2]=r1+1|0;r2=HEAPU8[r1];return r2}function __ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE6xsputnEPKci(r1,r2,r3){var r4,r5,r6,r7,r8,r9,r10,r11;r4=0;r5=r1;if((r3|0)<=0){r6=0;return r6}r7=r1+24|0;r8=r1+28|0;r9=0;r10=r2;while(1){r2=HEAP32[r7>>2];if(r2>>>0<HEAP32[r8>>2]>>>0){r11=HEAP8[r10];HEAP32[r7>>2]=r2+1|0;HEAP8[r2]=r11}else{if((FUNCTION_TABLE[HEAP32[HEAP32[r5>>2]+52>>2]](r1,HEAPU8[r10])|0)==-1){r6=r9;r4=688;break}}r11=r9+1|0;if((r11|0)<(r3|0)){r9=r11;r10=r10+1|0}else{r6=r11;r4=687;break}}if(r4==688){return r6}else if(r4==687){return r6}}function __ZNSt3__115basic_streambufIwNS_11char_traitsIwEEED1Ev(r1){var r2;HEAP32[r1>>2]=5248996;r2=HEAP32[r1+4>>2];r1=r2+4|0;if(((tempValue=HEAP32[r1>>2],HEAP32[r1>>2]=tempValue+ -1,tempValue)|0)!=0){return}FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+8>>2]](r2|0);return}function __ZNSt3__115basic_streambufIwNS_11char_traitsIwEEED0Ev(r1){var r2,r3,r4;HEAP32[r1>>2]=5248996;r2=HEAP32[r1+4>>2];r3=r2+4|0;if(((tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+ -1,tempValue)|0)!=0){r4=r1;__ZdlPv(r4);return}FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+8>>2]](r2|0);r4=r1;__ZdlPv(r4);return}function __ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE6xsgetnEPwi(r1,r2,r3){var r4,r5,r6,r7,r8,r9,r10,r11;r4=0;r5=r1;if((r3|0)<=0){r6=0;return r6}r7=r1+12|0;r8=r1+16|0;r9=r2;r2=0;while(1){r10=HEAP32[r7>>2];if(r10>>>0<HEAP32[r8>>2]>>>0){HEAP32[r7>>2]=r10+4|0;r11=HEAP32[r10>>2]}else{r10=FUNCTION_TABLE[HEAP32[HEAP32[r5>>2]+40>>2]](r1);if((r10|0)==-1){r6=r2;r4=709;break}else{r11=r10}}HEAP32[r9>>2]=r11;r10=r2+1|0;if((r10|0)<(r3|0)){r9=r9+4|0;r2=r10}else{r6=r10;r4=708;break}}if(r4==708){return r6}else if(r4==709){return r6}}function __ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE5uflowEv(r1){var r2,r3;if((FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+36>>2]](r1)|0)==-1){r2=-1;return r2}r3=r1+12|0;r1=HEAP32[r3>>2];HEAP32[r3>>2]=r1+4|0;r2=HEAP32[r1>>2];return r2}function __ZNSt3__115basic_streambufIwNS_11char_traitsIwEEE6xsputnEPKwi(r1,r2,r3){var r4,r5,r6,r7,r8,r9,r10,r11;r4=0;r5=r1;if((r3|0)<=0){r6=0;return r6}r7=r1+24|0;r8=r1+28|0;r9=0;r10=r2;while(1){r2=HEAP32[r7>>2];if(r2>>>0<HEAP32[r8>>2]>>>0){r11=HEAP32[r10>>2];HEAP32[r7>>2]=r2+4|0;HEAP32[r2>>2]=r11}else{if((FUNCTION_TABLE[HEAP32[HEAP32[r5>>2]+52>>2]](r1,HEAP32[r10>>2])|0)==-1){r6=r9;r4=722;break}}r11=r9+1|0;if((r11|0)<(r3|0)){r9=r11;r10=r10+4|0}else{r6=r11;r4=724;break}}if(r4==722){return r6}else if(r4==724){return r6}}function __ZNSt3__113basic_istreamIcNS_11char_traitsIcEEED1Ev(r1){__ZNSt3__18ios_baseD2Ev(r1+8|0);return}function __ZNSt3__113basic_istreamIcNS_11char_traitsIcEEED0Ev(r1){__ZNSt3__18ios_baseD2Ev(r1+8|0);__ZdlPv(r1);return}function __ZTv0_n12_NSt3__113basic_istreamIcNS_11char_traitsIcEEED1Ev(r1){__ZNSt3__18ios_baseD2Ev(r1+HEAP32[HEAP32[r1>>2]-12>>2]+8|0);return}function __ZTv0_n12_NSt3__113basic_istreamIcNS_11char_traitsIcEEED0Ev(r1){var r2,r3;r2=r1;r3=HEAP32[HEAP32[r1>>2]-12>>2];__ZNSt3__18ios_baseD2Ev(r3+(r2+8)|0);__ZdlPv(r2+r3|0);return}function __ZNSt3__113basic_istreamIwNS_11char_traitsIwEEED1Ev(r1){__ZNSt3__18ios_baseD2Ev(r1+8|0);return}function __ZNSt3__113basic_istreamIwNS_11char_traitsIwEEED0Ev(r1){__ZNSt3__18ios_baseD2Ev(r1+8|0);__ZdlPv(r1);return}function __ZTv0_n12_NSt3__113basic_istreamIwNS_11char_traitsIwEEED1Ev(r1){__ZNSt3__18ios_baseD2Ev(r1+HEAP32[HEAP32[r1>>2]-12>>2]+8|0);return}function __ZTv0_n12_NSt3__113basic_istreamIwNS_11char_traitsIwEEED0Ev(r1){var r2,r3;r2=r1;r3=HEAP32[HEAP32[r1>>2]-12>>2];__ZNSt3__18ios_baseD2Ev(r3+(r2+8)|0);__ZdlPv(r2+r3|0);return}function __ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEED1Ev(r1){__ZNSt3__18ios_baseD2Ev(r1+4|0);return}function __ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEED0Ev(r1){__ZNSt3__18ios_baseD2Ev(r1+4|0);__ZdlPv(r1);return}function __ZTv0_n12_NSt3__113basic_ostreamIcNS_11char_traitsIcEEED1Ev(r1){__ZNSt3__18ios_baseD2Ev(r1+HEAP32[HEAP32[r1>>2]-12>>2]+4|0);return}function __ZTv0_n12_NSt3__113basic_ostreamIcNS_11char_traitsIcEEED0Ev(r1){var r2,r3;r2=r1;r3=HEAP32[HEAP32[r1>>2]-12>>2];__ZNSt3__18ios_baseD2Ev(r3+(r2+4)|0);__ZdlPv(r2+r3|0);return}function __ZNSt3__113basic_ostreamIwNS_11char_traitsIwEEED1Ev(r1){__ZNSt3__18ios_baseD2Ev(r1+4|0);return}function __ZNSt3__113basic_ostreamIwNS_11char_traitsIwEEED0Ev(r1){__ZNSt3__18ios_baseD2Ev(r1+4|0);__ZdlPv(r1);return}function __ZTv0_n12_NSt3__113basic_ostreamIwNS_11char_traitsIwEEED1Ev(r1){__ZNSt3__18ios_baseD2Ev(r1+HEAP32[HEAP32[r1>>2]-12>>2]+4|0);return}function __ZTv0_n12_NSt3__113basic_ostreamIwNS_11char_traitsIwEEED0Ev(r1){var r2,r3;r2=r1;r3=HEAP32[HEAP32[r1>>2]-12>>2];__ZNSt3__18ios_baseD2Ev(r3+(r2+4)|0);__ZdlPv(r2+r3|0);return}function __ZNSt3__119__iostream_categoryD0Ev(r1){__ZdlPv(r1);return}function __ZNSt3__18ios_base5clearEj(r1,r2){var r3,r4,r5;r3=STACKTOP;STACKTOP=STACKTOP+8|0;r4=r3;r5=(HEAP32[r1+24>>2]|0)==0;if(r5){HEAP32[r1+16>>2]=r2|1}else{HEAP32[r1+16>>2]=r2}if(((r5&1|r2)&HEAP32[r1+20>>2]|0)==0){STACKTOP=r3;return}r3=___cxa_allocate_exception(16);do{if(HEAP8[5255492]<<24>>24==0){if((___cxa_guard_acquire(5255492)|0)==0){break}HEAP32[1311653]=5248852}}while(0);r1=r4;HEAP32[r1>>2]=_bitshift64Shl(5246612,0,32)&0|1;HEAP32[r1+4>>2]=tempRet0&-1|0;__ZNSt3__112system_errorC2ENS_10error_codeEPKc(r3,r4,5243844);HEAP32[r3>>2]=5248124;___cxa_throw(r3,5253244,62)}function __ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEE5flushEv(r1){var r2,r3,r4,r5,r6,r7,r8,r9;r2=STACKTOP;STACKTOP=STACKTOP+8|0;r3=r2;r4=r1>>2;r5=HEAP32[HEAP32[r4]-12>>2]>>2;r6=r1,r7=r6>>2;if((HEAP32[r5+(r7+6)]|0)==0){STACKTOP=r2;return r1}r8=r3|0;HEAP8[r8]=0;HEAP32[r3+4>>2]=r1;do{if((HEAP32[r5+(r7+4)]|0)==0){r9=HEAP32[r5+(r7+18)];if((r9|0)!=0){__ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEE5flushEv(r9)}HEAP8[r8]=1;r9=HEAP32[(HEAP32[HEAP32[r4]-12>>2]+24>>2)+r7];if((FUNCTION_TABLE[HEAP32[HEAP32[r9>>2]+24>>2]](r9)|0)!=-1){break}r9=HEAP32[HEAP32[r4]-12>>2];__ZNSt3__18ios_base5clearEj(r6+r9|0,HEAP32[(r9+16>>2)+r7]|1)}}while(0);__ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEE6sentryD2Ev(r3);STACKTOP=r2;return r1}function __ZNSt3__113basic_ostreamIwNS_11char_traitsIwEEE5flushEv(r1){var r2,r3,r4,r5,r6,r7,r8,r9;r2=STACKTOP;STACKTOP=STACKTOP+8|0;r3=r2;r4=r1>>2;r5=HEAP32[HEAP32[r4]-12>>2]>>2;r6=r1,r7=r6>>2;if((HEAP32[r5+(r7+6)]|0)==0){STACKTOP=r2;return r1}r8=r3|0;HEAP8[r8]=0;HEAP32[r3+4>>2]=r1;do{if((HEAP32[r5+(r7+4)]|0)==0){r9=HEAP32[r5+(r7+18)];if((r9|0)!=0){__ZNSt3__113basic_ostreamIwNS_11char_traitsIwEEE5flushEv(r9)}HEAP8[r8]=1;r9=HEAP32[(HEAP32[HEAP32[r4]-12>>2]+24>>2)+r7];if((FUNCTION_TABLE[HEAP32[HEAP32[r9>>2]+24>>2]](r9)|0)!=-1){break}r9=HEAP32[HEAP32[r4]-12>>2];__ZNSt3__18ios_base5clearEj(r6+r9|0,HEAP32[(r9+16>>2)+r7]|1)}}while(0);__ZNSt3__113basic_ostreamIwNS_11char_traitsIwEEE6sentryD2Ev(r3);STACKTOP=r2;return r1}function __ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEE6sentryD2Ev(r1){var r2,r3,r4;r2=(r1+4|0)>>2;r1=HEAP32[r2];r3=HEAP32[HEAP32[r1>>2]-12>>2]>>2;r4=r1>>2;if((HEAP32[r3+(r4+6)]|0)==0){return}if((HEAP32[r3+(r4+4)]|0)!=0){return}if((HEAP32[r3+(r4+1)]&8192|0)==0){return}if(__ZSt18uncaught_exceptionv()){return}r4=HEAP32[r2];r3=HEAP32[r4+HEAP32[HEAP32[r4>>2]-12>>2]+24>>2];if((FUNCTION_TABLE[HEAP32[HEAP32[r3>>2]+24>>2]](r3)|0)!=-1){return}r3=HEAP32[r2];r2=HEAP32[HEAP32[r3>>2]-12>>2];r4=r3;__ZNSt3__18ios_base5clearEj(r4+r2|0,HEAP32[r2+(r4+16)>>2]|1);return}function __ZNSt3__113basic_ostreamIwNS_11char_traitsIwEEE6sentryD2Ev(r1){var r2,r3,r4;r2=(r1+4|0)>>2;r1=HEAP32[r2];r3=HEAP32[HEAP32[r1>>2]-12>>2]>>2;r4=r1>>2;if((HEAP32[r3+(r4+6)]|0)==0){return}if((HEAP32[r3+(r4+4)]|0)!=0){return}if((HEAP32[r3+(r4+1)]&8192|0)==0){return}if(__ZSt18uncaught_exceptionv()){return}r4=HEAP32[r2];r3=HEAP32[r4+HEAP32[HEAP32[r4>>2]-12>>2]+24>>2];if((FUNCTION_TABLE[HEAP32[HEAP32[r3>>2]+24>>2]](r3)|0)!=-1){return}r3=HEAP32[r2];r2=HEAP32[HEAP32[r3>>2]-12>>2];r4=r3;__ZNSt3__18ios_base5clearEj(r4+r2|0,HEAP32[r2+(r4+16)>>2]|1);return}function __ZNSt3__16locale5__impC2Ej(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48,r49,r50,r51,r52,r53,r54,r55,r56,r57,r58,r59,r60,r61,r62,r63,r64,r65,r66,r67,r68,r69,r70,r71,r72,r73,r74,r75,r76,r77,r78,r79,r80,r81,r82,r83,r84,r85,r86,r87,r88,r89,r90,r91,r92,r93,r94,r95,r96,r97;r3=STACKTOP;STACKTOP=STACKTOP+336|0;r4=r3,r5=r4>>2;r6=r3+12,r7=r6>>2;r8=r3+24,r9=r8>>2;r10=r3+36,r11=r10>>2;r12=r3+48,r13=r12>>2;r14=r3+60,r15=r14>>2;r16=r3+72,r17=r16>>2;r18=r3+84,r19=r18>>2;r20=r3+96,r21=r20>>2;r22=r3+108,r23=r22>>2;r24=r3+120,r25=r24>>2;r26=r3+132,r27=r26>>2;r28=r3+144,r29=r28>>2;r30=r3+156,r31=r30>>2;r32=r3+168,r33=r32>>2;r34=r3+180,r35=r34>>2;r36=r3+192,r37=r36>>2;r38=r3+204,r39=r38>>2;r40=r3+216,r41=r40>>2;r42=r3+228,r43=r42>>2;r44=r3+240,r45=r44>>2;r46=r3+252,r47=r46>>2;r48=r3+264,r49=r48>>2;r50=r3+276,r51=r50>>2;r52=r3+288,r53=r52>>2;r54=r3+300,r55=r54>>2;r56=r3+312,r57=r56>>2;r58=r3+324,r59=r58>>2;HEAP32[r1+4>>2]=r2-1|0;HEAP32[r1>>2]=5248700;r2=r1+8|0;r60=(r2|0)>>2;r61=(r1+12|0)>>2;HEAP8[r1+132|0]=1;r62=r1+20|0;r63=r62;HEAP32[r61]=r63;HEAP32[r60]=r63;HEAP32[r1+16>>2]=r62+112|0;r62=28;r64=r63;while(1){if((r64|0)==0){r65=0}else{HEAP32[r64>>2]=0;r65=HEAP32[r61]}r66=r65+4|0;HEAP32[r61]=r66;r63=r62-1|0;if((r63|0)==0){break}else{r62=r63;r64=r66}}r64=r1+136|0;r1=r64;HEAP8[r64]=2;HEAP8[r1+1|0]=67;HEAP8[r1+2|0]=0;r1=HEAP32[r60];if((r1|0)!=(r66|0)){HEAP32[r61]=(-((r65+ -r1|0)>>>2)<<2)+r65|0}HEAP32[1311702]=0;HEAP32[1311701]=5248432;if((HEAP32[1313651]|0)!=-1){HEAP32[r59]=5254604;HEAP32[r59+1]=24;HEAP32[r59+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254604,r58,406)}r58=HEAP32[1313652];r59=r58-1|0;tempValue=HEAP32[1311702],HEAP32[1311702]=tempValue+1,tempValue;r65=HEAP32[r61];r1=HEAP32[r60];r66=r65-r1>>2;do{if(r66>>>0>r59>>>0){r67=r1}else{if(r66>>>0<r58>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r58-r66|0);r67=HEAP32[r60];break}if(r66>>>0<=r58>>>0){r67=r1;break}r64=(r58<<2)+r1|0;if((r64|0)==(r65|0)){r67=r1;break}HEAP32[r61]=(((r65-4+ -r64|0)>>>2^-1)<<2)+r65|0;r67=r1}}while(0);r1=HEAP32[r67+(r59<<2)>>2];do{if((r1|0)!=0){r67=r1+4|0;if(((tempValue=HEAP32[r67>>2],HEAP32[r67>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r59<<2)>>2]=5246804;HEAP32[1311700]=0;HEAP32[1311699]=5248396;if((HEAP32[1313649]|0)!=-1){HEAP32[r57]=5254596;HEAP32[r57+1]=24;HEAP32[r57+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254596,r56,406)}r56=HEAP32[1313650];r57=r56-1|0;tempValue=HEAP32[1311700],HEAP32[1311700]=tempValue+1,tempValue;r59=HEAP32[r61];r1=HEAP32[r60];r67=r59-r1>>2;do{if(r67>>>0>r57>>>0){r68=r1}else{if(r67>>>0<r56>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r56-r67|0);r68=HEAP32[r60];break}if(r67>>>0<=r56>>>0){r68=r1;break}r65=(r56<<2)+r1|0;if((r65|0)==(r59|0)){r68=r1;break}HEAP32[r61]=(((r59-4+ -r65|0)>>>2^-1)<<2)+r59|0;r68=r1}}while(0);r1=HEAP32[r68+(r57<<2)>>2];do{if((r1|0)!=0){r68=r1+4|0;if(((tempValue=HEAP32[r68>>2],HEAP32[r68>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r57<<2)>>2]=5246796;HEAP32[1311752]=0;HEAP32[1311751]=5248796;HEAP32[1311753]=0;HEAP8[5247016]=0;HEAP32[1311753]=HEAP32[___ctype_b_loc()>>2];if((HEAP32[1313727]|0)!=-1){HEAP32[r55]=5254908;HEAP32[r55+1]=24;HEAP32[r55+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r54,406)}r54=HEAP32[1313728];r55=r54-1|0;tempValue=HEAP32[1311752],HEAP32[1311752]=tempValue+1,tempValue;r57=HEAP32[r61];r1=HEAP32[r60];r68=r57-r1>>2;do{if(r68>>>0>r55>>>0){r69=r1}else{if(r68>>>0<r54>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r54-r68|0);r69=HEAP32[r60];break}if(r68>>>0<=r54>>>0){r69=r1;break}r59=(r54<<2)+r1|0;if((r59|0)==(r57|0)){r69=r1;break}HEAP32[r61]=(((r57-4+ -r59|0)>>>2^-1)<<2)+r57|0;r69=r1}}while(0);r1=HEAP32[r69+(r55<<2)>>2];do{if((r1|0)!=0){r69=r1+4|0;if(((tempValue=HEAP32[r69>>2],HEAP32[r69>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r55<<2)>>2]=5247004;HEAP32[1311750]=0;HEAP32[1311749]=5248724;if((HEAP32[1313725]|0)!=-1){HEAP32[r53]=5254900;HEAP32[r53+1]=24;HEAP32[r53+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r52,406)}r52=HEAP32[1313726];r53=r52-1|0;tempValue=HEAP32[1311750],HEAP32[1311750]=tempValue+1,tempValue;r55=HEAP32[r61];r1=HEAP32[r60];r69=r55-r1>>2;do{if(r69>>>0>r53>>>0){r70=r1}else{if(r69>>>0<r52>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r52-r69|0);r70=HEAP32[r60];break}if(r69>>>0<=r52>>>0){r70=r1;break}r57=(r52<<2)+r1|0;if((r57|0)==(r55|0)){r70=r1;break}HEAP32[r61]=(((r55-4+ -r57|0)>>>2^-1)<<2)+r55|0;r70=r1}}while(0);r1=HEAP32[r70+(r53<<2)>>2];do{if((r1|0)!=0){r70=r1+4|0;if(((tempValue=HEAP32[r70>>2],HEAP32[r70>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r53<<2)>>2]=5246996;HEAP32[1311707]=0;HEAP32[1311706]=5248520;if((HEAP32[1313655]|0)!=-1){HEAP32[r51]=5254620;HEAP32[r51+1]=24;HEAP32[r51+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254620,r50,406)}r50=HEAP32[1313656];r51=r50-1|0;tempValue=HEAP32[1311707],HEAP32[1311707]=tempValue+1,tempValue;r53=HEAP32[r61];r1=HEAP32[r60];r70=r53-r1>>2;do{if(r70>>>0>r51>>>0){r71=r1}else{if(r70>>>0<r50>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r50-r70|0);r71=HEAP32[r60];break}if(r70>>>0<=r50>>>0){r71=r1;break}r55=(r50<<2)+r1|0;if((r55|0)==(r53|0)){r71=r1;break}HEAP32[r61]=(((r53-4+ -r55|0)>>>2^-1)<<2)+r53|0;r71=r1}}while(0);r1=HEAP32[r71+(r51<<2)>>2];do{if((r1|0)!=0){r71=r1+4|0;if(((tempValue=HEAP32[r71>>2],HEAP32[r71>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r51<<2)>>2]=5246824;HEAP32[1311704]=0;HEAP32[1311703]=5248468;HEAP32[1311705]=0;if((HEAP32[1313653]|0)!=-1){HEAP32[r49]=5254612;HEAP32[r49+1]=24;HEAP32[r49+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254612,r48,406)}r48=HEAP32[1313654];r49=r48-1|0;tempValue=HEAP32[1311704],HEAP32[1311704]=tempValue+1,tempValue;r51=HEAP32[r61];r1=HEAP32[r60];r71=r51-r1>>2;do{if(r71>>>0>r49>>>0){r72=r1}else{if(r71>>>0<r48>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r48-r71|0);r72=HEAP32[r60];break}if(r71>>>0<=r48>>>0){r72=r1;break}r53=(r48<<2)+r1|0;if((r53|0)==(r51|0)){r72=r1;break}HEAP32[r61]=(((r51-4+ -r53|0)>>>2^-1)<<2)+r51|0;r72=r1}}while(0);r1=HEAP32[r72+(r49<<2)>>2];do{if((r1|0)!=0){r72=r1+4|0;if(((tempValue=HEAP32[r72>>2],HEAP32[r72>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r49<<2)>>2]=5246812;HEAP32[1311709]=0;HEAP32[1311708]=5248572;if((HEAP32[1313657]|0)!=-1){HEAP32[r47]=5254628;HEAP32[r47+1]=24;HEAP32[r47+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254628,r46,406)}r46=HEAP32[1313658];r47=r46-1|0;tempValue=HEAP32[1311709],HEAP32[1311709]=tempValue+1,tempValue;r49=HEAP32[r61];r1=HEAP32[r60];r72=r49-r1>>2;do{if(r72>>>0>r47>>>0){r73=r1}else{if(r72>>>0<r46>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r46-r72|0);r73=HEAP32[r60];break}if(r72>>>0<=r46>>>0){r73=r1;break}r51=(r46<<2)+r1|0;if((r51|0)==(r49|0)){r73=r1;break}HEAP32[r61]=(((r49-4+ -r51|0)>>>2^-1)<<2)+r49|0;r73=r1}}while(0);r1=HEAP32[r73+(r47<<2)>>2];do{if((r1|0)!=0){r73=r1+4|0;if(((tempValue=HEAP32[r73>>2],HEAP32[r73>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r47<<2)>>2]=5246832;HEAP32[1311711]=0;HEAP32[1311710]=5248624;if((HEAP32[1313659]|0)!=-1){HEAP32[r45]=5254636;HEAP32[r45+1]=24;HEAP32[r45+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254636,r44,406)}r44=HEAP32[1313660];r45=r44-1|0;tempValue=HEAP32[1311711],HEAP32[1311711]=tempValue+1,tempValue;r47=HEAP32[r61];r1=HEAP32[r60];r73=r47-r1>>2;do{if(r73>>>0>r45>>>0){r74=r1}else{if(r73>>>0<r44>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r44-r73|0);r74=HEAP32[r60];break}if(r73>>>0<=r44>>>0){r74=r1;break}r49=(r44<<2)+r1|0;if((r49|0)==(r47|0)){r74=r1;break}HEAP32[r61]=(((r47-4+ -r49|0)>>>2^-1)<<2)+r47|0;r74=r1}}while(0);r1=HEAP32[r74+(r45<<2)>>2];do{if((r1|0)!=0){r74=r1+4|0;if(((tempValue=HEAP32[r74>>2],HEAP32[r74>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r45<<2)>>2]=5246840;HEAP32[1311682]=0;HEAP32[1311681]=5247988;HEAP8[5246732]=46;HEAP8[5246733]=44;HEAP32[1311684]=0;HEAP32[1311685]=0;HEAP32[1311686]=0;if((HEAP32[1313635]|0)!=-1){HEAP32[r43]=5254540;HEAP32[r43+1]=24;HEAP32[r43+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254540,r42,406)}r42=HEAP32[1313636];r43=r42-1|0;tempValue=HEAP32[1311682],HEAP32[1311682]=tempValue+1,tempValue;r45=HEAP32[r61];r1=HEAP32[r60];r74=r45-r1>>2;do{if(r74>>>0>r43>>>0){r75=r1}else{if(r74>>>0<r42>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r42-r74|0);r75=HEAP32[r60];break}if(r74>>>0<=r42>>>0){r75=r1;break}r47=(r42<<2)+r1|0;if((r47|0)==(r45|0)){r75=r1;break}HEAP32[r61]=(((r45-4+ -r47|0)>>>2^-1)<<2)+r45|0;r75=r1}}while(0);r1=HEAP32[r75+(r43<<2)>>2];do{if((r1|0)!=0){r75=r1+4|0;if(((tempValue=HEAP32[r75>>2],HEAP32[r75>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r43<<2)>>2]=5246724;HEAP32[1311675]=0;HEAP32[1311674]=5247944;HEAP32[1311676]=46;HEAP32[1311677]=44;HEAP32[1311678]=0;HEAP32[1311679]=0;HEAP32[1311680]=0;if((HEAP32[1313633]|0)!=-1){HEAP32[r41]=5254532;HEAP32[r41+1]=24;HEAP32[r41+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254532,r40,406)}r40=HEAP32[1313634];r41=r40-1|0;tempValue=HEAP32[1311675],HEAP32[1311675]=tempValue+1,tempValue;r43=HEAP32[r61];r1=HEAP32[r60];r75=r43-r1>>2;do{if(r75>>>0>r41>>>0){r76=r1}else{if(r75>>>0<r40>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r40-r75|0);r76=HEAP32[r60];break}if(r75>>>0<=r40>>>0){r76=r1;break}r45=(r40<<2)+r1|0;if((r45|0)==(r43|0)){r76=r1;break}HEAP32[r61]=(((r43-4+ -r45|0)>>>2^-1)<<2)+r43|0;r76=r1}}while(0);r1=HEAP32[r76+(r41<<2)>>2];do{if((r1|0)!=0){r76=r1+4|0;if(((tempValue=HEAP32[r76>>2],HEAP32[r76>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r41<<2)>>2]=5246696;HEAP32[1311698]=0;HEAP32[1311697]=5248328;if((HEAP32[1313647]|0)!=-1){HEAP32[r39]=5254588;HEAP32[r39+1]=24;HEAP32[r39+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254588,r38,406)}r38=HEAP32[1313648];r39=r38-1|0;tempValue=HEAP32[1311698],HEAP32[1311698]=tempValue+1,tempValue;r41=HEAP32[r61];r1=HEAP32[r60];r76=r41-r1>>2;do{if(r76>>>0>r39>>>0){r77=r1}else{if(r76>>>0<r38>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r38-r76|0);r77=HEAP32[r60];break}if(r76>>>0<=r38>>>0){r77=r1;break}r43=(r38<<2)+r1|0;if((r43|0)==(r41|0)){r77=r1;break}HEAP32[r61]=(((r41-4+ -r43|0)>>>2^-1)<<2)+r41|0;r77=r1}}while(0);r1=HEAP32[r77+(r39<<2)>>2];do{if((r1|0)!=0){r77=r1+4|0;if(((tempValue=HEAP32[r77>>2],HEAP32[r77>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r39<<2)>>2]=5246788;HEAP32[1311696]=0;HEAP32[1311695]=5248260;if((HEAP32[1313645]|0)!=-1){HEAP32[r37]=5254580;HEAP32[r37+1]=24;HEAP32[r37+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254580,r36,406)}r36=HEAP32[1313646];r37=r36-1|0;tempValue=HEAP32[1311696],HEAP32[1311696]=tempValue+1,tempValue;r39=HEAP32[r61];r1=HEAP32[r60];r77=r39-r1>>2;do{if(r77>>>0>r37>>>0){r78=r1}else{if(r77>>>0<r36>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r36-r77|0);r78=HEAP32[r60];break}if(r77>>>0<=r36>>>0){r78=r1;break}r41=(r36<<2)+r1|0;if((r41|0)==(r39|0)){r78=r1;break}HEAP32[r61]=(((r39-4+ -r41|0)>>>2^-1)<<2)+r39|0;r78=r1}}while(0);r1=HEAP32[r78+(r37<<2)>>2];do{if((r1|0)!=0){r78=r1+4|0;if(((tempValue=HEAP32[r78>>2],HEAP32[r78>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r37<<2)>>2]=5246780;HEAP32[1311694]=0;HEAP32[1311693]=5248204;if((HEAP32[1313643]|0)!=-1){HEAP32[r35]=5254572;HEAP32[r35+1]=24;HEAP32[r35+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254572,r34,406)}r34=HEAP32[1313644];r35=r34-1|0;tempValue=HEAP32[1311694],HEAP32[1311694]=tempValue+1,tempValue;r37=HEAP32[r61];r1=HEAP32[r60];r78=r37-r1>>2;do{if(r78>>>0>r35>>>0){r79=r1}else{if(r78>>>0<r34>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r34-r78|0);r79=HEAP32[r60];break}if(r78>>>0<=r34>>>0){r79=r1;break}r39=(r34<<2)+r1|0;if((r39|0)==(r37|0)){r79=r1;break}HEAP32[r61]=(((r37-4+ -r39|0)>>>2^-1)<<2)+r37|0;r79=r1}}while(0);r1=HEAP32[r79+(r35<<2)>>2];do{if((r1|0)!=0){r79=r1+4|0;if(((tempValue=HEAP32[r79>>2],HEAP32[r79>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r35<<2)>>2]=5246772;HEAP32[1311692]=0;HEAP32[1311691]=5248148;if((HEAP32[1313641]|0)!=-1){HEAP32[r33]=5254564;HEAP32[r33+1]=24;HEAP32[r33+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254564,r32,406)}r32=HEAP32[1313642];r33=r32-1|0;tempValue=HEAP32[1311692],HEAP32[1311692]=tempValue+1,tempValue;r35=HEAP32[r61];r1=HEAP32[r60];r79=r35-r1>>2;do{if(r79>>>0>r33>>>0){r80=r1}else{if(r79>>>0<r32>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r32-r79|0);r80=HEAP32[r60];break}if(r79>>>0<=r32>>>0){r80=r1;break}r37=(r32<<2)+r1|0;if((r37|0)==(r35|0)){r80=r1;break}HEAP32[r61]=(((r35-4+ -r37|0)>>>2^-1)<<2)+r35|0;r80=r1}}while(0);r1=HEAP32[r80+(r33<<2)>>2];do{if((r1|0)!=0){r80=r1+4|0;if(((tempValue=HEAP32[r80>>2],HEAP32[r80>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r33<<2)>>2]=5246764;HEAP32[1311762]=0;HEAP32[1311761]=5249648;if((HEAP32[1313852]|0)!=-1){HEAP32[r31]=5255408;HEAP32[r31+1]=24;HEAP32[r31+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5255408,r30,406)}r30=HEAP32[1313853];r31=r30-1|0;tempValue=HEAP32[1311762],HEAP32[1311762]=tempValue+1,tempValue;r33=HEAP32[r61];r1=HEAP32[r60];r80=r33-r1>>2;do{if(r80>>>0>r31>>>0){r81=r1}else{if(r80>>>0<r30>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r30-r80|0);r81=HEAP32[r60];break}if(r80>>>0<=r30>>>0){r81=r1;break}r35=(r30<<2)+r1|0;if((r35|0)==(r33|0)){r81=r1;break}HEAP32[r61]=(((r33-4+ -r35|0)>>>2^-1)<<2)+r33|0;r81=r1}}while(0);r1=HEAP32[r81+(r31<<2)>>2];do{if((r1|0)!=0){r81=r1+4|0;if(((tempValue=HEAP32[r81>>2],HEAP32[r81>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r31<<2)>>2]=5247044;HEAP32[1311760]=0;HEAP32[1311759]=5249588;if((HEAP32[1313850]|0)!=-1){HEAP32[r29]=5255400;HEAP32[r29+1]=24;HEAP32[r29+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5255400,r28,406)}r28=HEAP32[1313851];r29=r28-1|0;tempValue=HEAP32[1311760],HEAP32[1311760]=tempValue+1,tempValue;r31=HEAP32[r61];r1=HEAP32[r60];r81=r31-r1>>2;do{if(r81>>>0>r29>>>0){r82=r1}else{if(r81>>>0<r28>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r28-r81|0);r82=HEAP32[r60];break}if(r81>>>0<=r28>>>0){r82=r1;break}r33=(r28<<2)+r1|0;if((r33|0)==(r31|0)){r82=r1;break}HEAP32[r61]=(((r31-4+ -r33|0)>>>2^-1)<<2)+r31|0;r82=r1}}while(0);r1=HEAP32[r82+(r29<<2)>>2];do{if((r1|0)!=0){r82=r1+4|0;if(((tempValue=HEAP32[r82>>2],HEAP32[r82>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r29<<2)>>2]=5247036;HEAP32[1311758]=0;HEAP32[1311757]=5249528;if((HEAP32[1313848]|0)!=-1){HEAP32[r27]=5255392;HEAP32[r27+1]=24;HEAP32[r27+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5255392,r26,406)}r26=HEAP32[1313849];r27=r26-1|0;tempValue=HEAP32[1311758],HEAP32[1311758]=tempValue+1,tempValue;r29=HEAP32[r61];r1=HEAP32[r60];r82=r29-r1>>2;do{if(r82>>>0>r27>>>0){r83=r1}else{if(r82>>>0<r26>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r26-r82|0);r83=HEAP32[r60];break}if(r82>>>0<=r26>>>0){r83=r1;break}r31=(r26<<2)+r1|0;if((r31|0)==(r29|0)){r83=r1;break}HEAP32[r61]=(((r29-4+ -r31|0)>>>2^-1)<<2)+r29|0;r83=r1}}while(0);r1=HEAP32[r83+(r27<<2)>>2];do{if((r1|0)!=0){r83=r1+4|0;if(((tempValue=HEAP32[r83>>2],HEAP32[r83>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r27<<2)>>2]=5247028;HEAP32[1311756]=0;HEAP32[1311755]=5249468;if((HEAP32[1313846]|0)!=-1){HEAP32[r25]=5255384;HEAP32[r25+1]=24;HEAP32[r25+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5255384,r24,406)}r24=HEAP32[1313847];r25=r24-1|0;tempValue=HEAP32[1311756],HEAP32[1311756]=tempValue+1,tempValue;r27=HEAP32[r61];r1=HEAP32[r60];r83=r27-r1>>2;do{if(r83>>>0>r25>>>0){r84=r1}else{if(r83>>>0<r24>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r24-r83|0);r84=HEAP32[r60];break}if(r83>>>0<=r24>>>0){r84=r1;break}r29=(r24<<2)+r1|0;if((r29|0)==(r27|0)){r84=r1;break}HEAP32[r61]=(((r27-4+ -r29|0)>>>2^-1)<<2)+r27|0;r84=r1}}while(0);r1=HEAP32[r84+(r25<<2)>>2];do{if((r1|0)!=0){r84=r1+4|0;if(((tempValue=HEAP32[r84>>2],HEAP32[r84>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r25<<2)>>2]=5247020;HEAP32[1311661]=0;HEAP32[1311660]=5247680;if((HEAP32[1313623]|0)!=-1){HEAP32[r23]=5254492;HEAP32[r23+1]=24;HEAP32[r23+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254492,r22,406)}r22=HEAP32[1313624];r23=r22-1|0;tempValue=HEAP32[1311661],HEAP32[1311661]=tempValue+1,tempValue;r25=HEAP32[r61];r1=HEAP32[r60];r84=r25-r1>>2;do{if(r84>>>0>r23>>>0){r85=r1}else{if(r84>>>0<r22>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r22-r84|0);r85=HEAP32[r60];break}if(r84>>>0<=r22>>>0){r85=r1;break}r27=(r22<<2)+r1|0;if((r27|0)==(r25|0)){r85=r1;break}HEAP32[r61]=(((r25-4+ -r27|0)>>>2^-1)<<2)+r25|0;r85=r1}}while(0);r1=HEAP32[r85+(r23<<2)>>2];do{if((r1|0)!=0){r85=r1+4|0;if(((tempValue=HEAP32[r85>>2],HEAP32[r85>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r23<<2)>>2]=5246640;HEAP32[1311659]=0;HEAP32[1311658]=5247648;if((HEAP32[1313621]|0)!=-1){HEAP32[r21]=5254484;HEAP32[r21+1]=24;HEAP32[r21+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254484,r20,406)}r20=HEAP32[1313622];r21=r20-1|0;tempValue=HEAP32[1311659],HEAP32[1311659]=tempValue+1,tempValue;r23=HEAP32[r61];r1=HEAP32[r60];r85=r23-r1>>2;do{if(r85>>>0>r21>>>0){r86=r1}else{if(r85>>>0<r20>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r20-r85|0);r86=HEAP32[r60];break}if(r85>>>0<=r20>>>0){r86=r1;break}r25=(r20<<2)+r1|0;if((r25|0)==(r23|0)){r86=r1;break}HEAP32[r61]=(((r23-4+ -r25|0)>>>2^-1)<<2)+r23|0;r86=r1}}while(0);r1=HEAP32[r86+(r21<<2)>>2];do{if((r1|0)!=0){r86=r1+4|0;if(((tempValue=HEAP32[r86>>2],HEAP32[r86>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r21<<2)>>2]=5246632;HEAP32[1311657]=0;HEAP32[1311656]=5247616;if((HEAP32[1313619]|0)!=-1){HEAP32[r19]=5254476;HEAP32[r19+1]=24;HEAP32[r19+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254476,r18,406)}r18=HEAP32[1313620];r19=r18-1|0;tempValue=HEAP32[1311657],HEAP32[1311657]=tempValue+1,tempValue;r21=HEAP32[r61];r1=HEAP32[r60];r86=r21-r1>>2;do{if(r86>>>0>r19>>>0){r87=r1}else{if(r86>>>0<r18>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r18-r86|0);r87=HEAP32[r60];break}if(r86>>>0<=r18>>>0){r87=r1;break}r23=(r18<<2)+r1|0;if((r23|0)==(r21|0)){r87=r1;break}HEAP32[r61]=(((r21-4+ -r23|0)>>>2^-1)<<2)+r21|0;r87=r1}}while(0);r1=HEAP32[r87+(r19<<2)>>2];do{if((r1|0)!=0){r87=r1+4|0;if(((tempValue=HEAP32[r87>>2],HEAP32[r87>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r19<<2)>>2]=5246624;HEAP32[1311655]=0;HEAP32[1311654]=5247584;if((HEAP32[1313617]|0)!=-1){HEAP32[r17]=5254468;HEAP32[r17+1]=24;HEAP32[r17+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254468,r16,406)}r16=HEAP32[1313618];r17=r16-1|0;tempValue=HEAP32[1311655],HEAP32[1311655]=tempValue+1,tempValue;r19=HEAP32[r61];r1=HEAP32[r60];r87=r19-r1>>2;do{if(r87>>>0>r17>>>0){r88=r1}else{if(r87>>>0<r16>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r16-r87|0);r88=HEAP32[r60];break}if(r87>>>0<=r16>>>0){r88=r1;break}r21=(r16<<2)+r1|0;if((r21|0)==(r19|0)){r88=r1;break}HEAP32[r61]=(((r19-4+ -r21|0)>>>2^-1)<<2)+r19|0;r88=r1}}while(0);r1=HEAP32[r88+(r17<<2)>>2];do{if((r1|0)!=0){r88=r1+4|0;if(((tempValue=HEAP32[r88>>2],HEAP32[r88>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r17<<2)>>2]=5246616;HEAP32[1311672]=0;HEAP32[1311671]=5247856;HEAP32[1311673]=5247904;if((HEAP32[1313631]|0)!=-1){HEAP32[r15]=5254524;HEAP32[r15+1]=24;HEAP32[r15+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254524,r14,406)}r14=HEAP32[1313632];r15=r14-1|0;tempValue=HEAP32[1311672],HEAP32[1311672]=tempValue+1,tempValue;r17=HEAP32[r61];r1=HEAP32[r60];r88=r17-r1>>2;do{if(r88>>>0>r15>>>0){r89=r1}else{if(r88>>>0<r14>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r14-r88|0);r89=HEAP32[r60];break}if(r88>>>0<=r14>>>0){r89=r1;break}r19=(r14<<2)+r1|0;if((r19|0)==(r17|0)){r89=r1;break}HEAP32[r61]=(((r17-4+ -r19|0)>>>2^-1)<<2)+r17|0;r89=r1}}while(0);r1=HEAP32[r89+(r15<<2)>>2];do{if((r1|0)!=0){r89=r1+4|0;if(((tempValue=HEAP32[r89>>2],HEAP32[r89>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r15<<2)>>2]=5246684;HEAP32[1311669]=0;HEAP32[1311668]=5247768;HEAP32[1311670]=5247816;if((HEAP32[1313629]|0)!=-1){HEAP32[r13]=5254516;HEAP32[r13+1]=24;HEAP32[r13+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254516,r12,406)}r12=HEAP32[1313630];r13=r12-1|0;tempValue=HEAP32[1311669],HEAP32[1311669]=tempValue+1,tempValue;r15=HEAP32[r61];r1=HEAP32[r60];r89=r15-r1>>2;do{if(r89>>>0>r13>>>0){r90=r1}else{if(r89>>>0<r12>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r12-r89|0);r90=HEAP32[r60];break}if(r89>>>0<=r12>>>0){r90=r1;break}r17=(r12<<2)+r1|0;if((r17|0)==(r15|0)){r90=r1;break}HEAP32[r61]=(((r15-4+ -r17|0)>>>2^-1)<<2)+r15|0;r90=r1}}while(0);r1=HEAP32[r90+(r13<<2)>>2];do{if((r1|0)!=0){r90=r1+4|0;if(((tempValue=HEAP32[r90>>2],HEAP32[r90>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r13<<2)>>2]=5246672;HEAP32[1311666]=0;HEAP32[1311665]=5248676;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);HEAP32[1311667]=HEAP32[1311652];HEAP32[1311665]=5247740;if((HEAP32[1313627]|0)!=-1){HEAP32[r11]=5254508;HEAP32[r11+1]=24;HEAP32[r11+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254508,r10,406)}r10=HEAP32[1313628];r11=r10-1|0;tempValue=HEAP32[1311666],HEAP32[1311666]=tempValue+1,tempValue;r13=HEAP32[r61];r1=HEAP32[r60];r90=r13-r1>>2;do{if(r90>>>0>r11>>>0){r91=r1}else{if(r90>>>0<r10>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r10-r90|0);r91=HEAP32[r60];break}if(r90>>>0<=r10>>>0){r91=r1;break}r15=(r10<<2)+r1|0;if((r15|0)==(r13|0)){r91=r1;break}HEAP32[r61]=(((r13-4+ -r15|0)>>>2^-1)<<2)+r13|0;r91=r1}}while(0);r1=HEAP32[r91+(r11<<2)>>2];do{if((r1|0)!=0){r91=r1+4|0;if(((tempValue=HEAP32[r91>>2],HEAP32[r91>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r11<<2)>>2]=5246660;HEAP32[1311663]=0;HEAP32[1311662]=5248676;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);HEAP32[1311664]=HEAP32[1311652];HEAP32[1311662]=5247712;if((HEAP32[1313625]|0)!=-1){HEAP32[r9]=5254500;HEAP32[r9+1]=24;HEAP32[r9+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254500,r8,406)}r8=HEAP32[1313626];r9=r8-1|0;tempValue=HEAP32[1311663],HEAP32[1311663]=tempValue+1,tempValue;r11=HEAP32[r61];r1=HEAP32[r60];r91=r11-r1>>2;do{if(r91>>>0>r9>>>0){r92=r1}else{if(r91>>>0<r8>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r8-r91|0);r92=HEAP32[r60];break}if(r91>>>0<=r8>>>0){r92=r1;break}r13=(r8<<2)+r1|0;if((r13|0)==(r11|0)){r92=r1;break}HEAP32[r61]=(((r11-4+ -r13|0)>>>2^-1)<<2)+r11|0;r92=r1}}while(0);r1=HEAP32[r92+(r9<<2)>>2];do{if((r1|0)!=0){r92=r1+4|0;if(((tempValue=HEAP32[r92>>2],HEAP32[r92>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r9<<2)>>2]=5246648;HEAP32[1311690]=0;HEAP32[1311689]=5248068;if((HEAP32[1313639]|0)!=-1){HEAP32[r7]=5254556;HEAP32[r7+1]=24;HEAP32[r7+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254556,r6,406)}r6=HEAP32[1313640];r7=r6-1|0;tempValue=HEAP32[1311690],HEAP32[1311690]=tempValue+1,tempValue;r9=HEAP32[r61];r1=HEAP32[r60];r92=r9-r1>>2;do{if(r92>>>0>r7>>>0){r93=r1}else{if(r92>>>0<r6>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r6-r92|0);r93=HEAP32[r60];break}if(r92>>>0<=r6>>>0){r93=r1;break}r11=(r6<<2)+r1|0;if((r11|0)==(r9|0)){r93=r1;break}HEAP32[r61]=(((r9-4+ -r11|0)>>>2^-1)<<2)+r9|0;r93=r1}}while(0);r1=HEAP32[r93+(r7<<2)>>2];do{if((r1|0)!=0){r93=r1+4|0;if(((tempValue=HEAP32[r93>>2],HEAP32[r93>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0)}}while(0);HEAP32[HEAP32[r60]+(r7<<2)>>2]=5246756;HEAP32[1311688]=0;HEAP32[1311687]=5248032;if((HEAP32[1313637]|0)!=-1){HEAP32[r5]=5254548;HEAP32[r5+1]=24;HEAP32[r5+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254548,r4,406)}r4=HEAP32[1313638];r5=r4-1|0;tempValue=HEAP32[1311688],HEAP32[1311688]=tempValue+1,tempValue;r7=HEAP32[r61];r1=HEAP32[r60];r93=r7-r1>>2;do{if(r93>>>0>r5>>>0){r94=r1}else{if(r93>>>0<r4>>>0){__ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r2,r4-r93|0);r94=HEAP32[r60];break}if(r93>>>0<=r4>>>0){r94=r1;break}r9=(r4<<2)+r1|0;if((r9|0)==(r7|0)){r94=r1;break}HEAP32[r61]=(((r7-4+ -r9|0)>>>2^-1)<<2)+r7|0;r94=r1}}while(0);r1=HEAP32[r94+(r5<<2)>>2];if((r1|0)==0){r95=HEAP32[r60];r96=(r5<<2)+r95|0,r97=r96>>2;HEAP32[r97]=5246748;STACKTOP=r3;return}r94=r1+4|0;if(((tempValue=HEAP32[r94>>2],HEAP32[r94>>2]=tempValue+ -1,tempValue)|0)!=0){r95=HEAP32[r60];r96=(r5<<2)+r95|0,r97=r96>>2;HEAP32[r97]=5246748;STACKTOP=r3;return}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+8>>2]](r1|0);r95=HEAP32[r60];r96=(r5<<2)+r95|0,r97=r96>>2;HEAP32[r97]=5246748;STACKTOP=r3;return}function __ZNSt3__16locale5facetD2Ev(r1){return}function __ZNKSt3__15ctypeIcE8do_widenEc(r1,r2){return r2}function __ZNKSt3__17codecvtIcc10_mbstate_tE6do_outERS1_PKcS5_RS5_PcS7_RS7_(r1,r2,r3,r4,r5,r6,r7,r8){HEAP32[r5>>2]=r3;HEAP32[r8>>2]=r6;return 3}function __ZNKSt3__17codecvtIcc10_mbstate_tE5do_inERS1_PKcS5_RS5_PcS7_RS7_(r1,r2,r3,r4,r5,r6,r7,r8){HEAP32[r5>>2]=r3;HEAP32[r8>>2]=r6;return 3}function __ZNKSt3__17codecvtIcc10_mbstate_tE10do_unshiftERS1_PcS4_RS4_(r1,r2,r3,r4,r5){HEAP32[r5>>2]=r3;return 3}function __ZNKSt3__17codecvtIcc10_mbstate_tE11do_encodingEv(r1){return 1}function __ZNKSt3__17codecvtIcc10_mbstate_tE16do_always_noconvEv(r1){return 1}function __ZNKSt3__17codecvtIcc10_mbstate_tE13do_max_lengthEv(r1){return 1}function __ZNKSt3__15ctypeIwE8do_widenEc(r1,r2){return r2<<24>>24}function __ZNKSt3__15ctypeIwE9do_narrowEwc(r1,r2,r3){return r2>>>0<128?r2&255:r3}function __ZNKSt3__15ctypeIcE9do_narrowEcc(r1,r2,r3){return r2<<24>>24>-1?r2:r3}function __ZNKSt3__17codecvtIcc10_mbstate_tE9do_lengthERS1_PKcS5_j(r1,r2,r3,r4,r5){r2=r4-r3|0;return r2>>>0<r5>>>0?r2:r5}function __ZNSt3__16locale2id6__initEv(r1){HEAP32[r1+4>>2]=(tempValue=HEAP32[1313661],HEAP32[1313661]=tempValue+1,tempValue)+1|0;return}function __ZNKSt3__15ctypeIwE8do_widenEPKcS3_Pw(r1,r2,r3,r4){var r5,r6,r7;if((r2|0)==(r3|0)){r5=r2;return r5}else{r6=r2;r7=r4}while(1){HEAP32[r7>>2]=HEAP8[r6]<<24>>24;r4=r6+1|0;if((r4|0)==(r3|0)){r5=r3;break}else{r6=r4;r7=r7+4|0}}return r5}function __ZNKSt3__15ctypeIwE9do_narrowEPKwS3_cPc(r1,r2,r3,r4,r5){var r6,r7,r8;if((r2|0)==(r3|0)){r6=r2;return r6}r1=((r3-4+ -r2|0)>>>2)+1|0;r7=r2;r8=r5;while(1){r5=HEAP32[r7>>2];HEAP8[r8]=r5>>>0<128?r5&255:r4;r5=r7+4|0;if((r5|0)==(r3|0)){break}else{r7=r5;r8=r8+1|0}}r6=(r1<<2)+r2|0;return r6}function __ZNKSt3__15ctypeIcE8do_widenEPKcS3_Pc(r1,r2,r3,r4){var r5,r6,r7;if((r2|0)==(r3|0)){r5=r2;return r5}else{r6=r2;r7=r4}while(1){HEAP8[r7]=HEAP8[r6];r4=r6+1|0;if((r4|0)==(r3|0)){r5=r3;break}else{r6=r4;r7=r7+1|0}}return r5}function __ZNKSt3__15ctypeIcE9do_narrowEPKcS3_cPc(r1,r2,r3,r4,r5){var r6,r7,r8;if((r2|0)==(r3|0)){r6=r2;return r6}else{r7=r2;r8=r5}while(1){r5=HEAP8[r7];HEAP8[r8]=r5<<24>>24>-1?r5:r4;r5=r7+1|0;if((r5|0)==(r3|0)){r6=r3;break}else{r7=r5;r8=r8+1|0}}return r6}function __ZNSt3__16locale5__impD0Ev(r1){__ZNSt3__16locale5__impD2Ev(r1);__ZdlPv(r1);return}function __ZNSt3__16locale5__impD2Ev(r1){var r2,r3,r4,r5,r6,r7,r8,r9,r10,r11;HEAP32[r1>>2]=5248700;r2=(r1+12|0)>>2;r3=HEAP32[r2];r4=(r1+8|0)>>2;r5=HEAP32[r4];L1414:do{if((r3|0)==(r5|0)){r6=r3}else{r7=0;r8=r5;while(1){r9=HEAP32[r8+(r7<<2)>>2];do{if((r9|0)!=0){r10=r9+4|0;if(((tempValue=HEAP32[r10>>2],HEAP32[r10>>2]=tempValue+ -1,tempValue)|0)!=0){break}FUNCTION_TABLE[HEAP32[HEAP32[r9>>2]+8>>2]](r9|0)}}while(0);r9=r7+1|0;r10=HEAP32[r4];if(r9>>>0<HEAP32[r2]-r10>>2>>>0){r7=r9;r8=r10}else{r6=r10;break L1414}}}}while(0);if((HEAP8[r1+136|0]&1)<<24>>24==0){r11=r6}else{__ZdlPv(HEAP32[r1+144>>2]);r11=HEAP32[r4]}if((r11|0)==0){return}r4=HEAP32[r2];if((r11|0)!=(r4|0)){HEAP32[r2]=(((r4-4+ -r11|0)>>>2^-1)<<2)+r4|0}if((r11|0)==(r1+20|0)){HEAP8[r1+132|0]=0;return}else{__ZdlPv(r11);return}}function __ZNSt3__16locale8__globalEv(){var r1,r2,r3;if(HEAP8[5255468]<<24>>24!=0){r1=HEAP32[1311648];return r1}if((___cxa_guard_acquire(5255468)|0)==0){r1=HEAP32[1311648];return r1}do{if(HEAP8[5255476]<<24>>24==0){if((___cxa_guard_acquire(5255476)|0)==0){break}__ZNSt3__16locale5__impC2Ej(5246848,1);HEAP32[1311650]=5246848;HEAP32[1311649]=5246600}}while(0);r2=HEAP32[HEAP32[1311649]>>2];HEAP32[1311651]=r2;r3=r2+4|0;tempValue=HEAP32[r3>>2],HEAP32[r3>>2]=tempValue+1,tempValue;HEAP32[1311648]=5246604;r1=HEAP32[1311648];return r1}function __ZNSt3__16locale5facetD0Ev(r1){__ZdlPv(r1);return}function __ZNSt3__15ctypeIwED0Ev(r1){__ZdlPv(r1);return}function __ZNSt3__15ctypeIcED0Ev(r1){var r2;HEAP32[r1>>2]=5248796;r2=HEAP32[r1+8>>2];do{if((r2|0)!=0){if((HEAP8[r1+12|0]&1)<<24>>24==0){break}__ZdaPv(r2)}}while(0);__ZdlPv(r1);return}function __ZNSt3__15ctypeIcED2Ev(r1){var r2;HEAP32[r1>>2]=5248796;r2=HEAP32[r1+8>>2];if((r2|0)==0){return}if((HEAP8[r1+12|0]&1)<<24>>24==0){return}__ZdaPv(r2);return}function __ZNSt3__17codecvtIcc10_mbstate_tED0Ev(r1){__ZdlPv(r1);return}function __ZNSt3__17codecvtIwc10_mbstate_tED0Ev(r1){var r2;HEAP32[r1>>2]=5248468;r2=HEAP32[r1+8>>2];if((r2|0)!=0){_freelocale(r2)}__ZdlPv(r1);return}function __ZNSt3__17codecvtIwc10_mbstate_tED2Ev(r1){var r2;HEAP32[r1>>2]=5248468;r2=HEAP32[r1+8>>2];if((r2|0)==0){return}_freelocale(r2);return}function __ZNSt3__16locale5facet16__on_zero_sharedEv(r1){if((r1|0)==0){return}FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+4>>2]](r1);return}function __ZNKSt3__15ctypeIwE5do_isEtw(r1,r2,r3){var r4;if(r3>>>0>=128){r4=0;return r4}r4=(HEAP16[HEAP32[___ctype_b_loc()>>2]+(r3<<1)>>1]&r2)<<16>>16!=0;return r4}function __ZNKSt3__15ctypeIwE5do_isEPKwS3_Pt(r1,r2,r3,r4){var r5,r6,r7,r8;if((r2|0)==(r3|0)){r5=r2;return r5}else{r6=r2;r7=r4}while(1){r4=HEAP32[r6>>2];if(r4>>>0<128){r8=HEAP16[HEAP32[___ctype_b_loc()>>2]+(r4<<1)>>1]}else{r8=0}HEAP16[r7>>1]=r8;r4=r6+4|0;if((r4|0)==(r3|0)){r5=r3;break}else{r6=r4;r7=r7+2|0}}return r5}function __ZNKSt3__15ctypeIwE10do_scan_isEtPKwS3_(r1,r2,r3,r4){var r5,r6;r1=0;if((r3|0)==(r4|0)){r5=r3;return r5}else{r6=r3}while(1){r3=HEAP32[r6>>2];if(r3>>>0<128){if((HEAP16[HEAP32[___ctype_b_loc()>>2]+(r3<<1)>>1]&r2)<<16>>16!=0){r5=r6;r1=1384;break}}r3=r6+4|0;if((r3|0)==(r4|0)){r5=r4;r1=1383;break}else{r6=r3}}if(r1==1384){return r5}else if(r1==1383){return r5}}function __ZNKSt3__15ctypeIwE11do_scan_notEtPKwS3_(r1,r2,r3,r4){var r5;r1=r3;while(1){if((r1|0)==(r4|0)){r5=r4;break}r3=HEAP32[r1>>2];if(r3>>>0>=128){r5=r1;break}if((HEAP16[HEAP32[___ctype_b_loc()>>2]+(r3<<1)>>1]&r2)<<16>>16==0){r5=r1;break}else{r1=r1+4|0}}return r5}function __ZNKSt3__15ctypeIwE10do_toupperEw(r1,r2){var r3;if(r2>>>0>=128){r3=r2;return r3}r3=HEAP32[HEAP32[___ctype_toupper_loc()>>2]+(r2<<2)>>2];return r3}function __ZNKSt3__15ctypeIwE10do_toupperEPwPKw(r1,r2,r3){var r4,r5,r6;if((r2|0)==(r3|0)){r4=r2;return r4}else{r5=r2}while(1){r2=HEAP32[r5>>2];if(r2>>>0<128){r6=HEAP32[HEAP32[___ctype_toupper_loc()>>2]+(r2<<2)>>2]}else{r6=r2}HEAP32[r5>>2]=r6;r2=r5+4|0;if((r2|0)==(r3|0)){r4=r3;break}else{r5=r2}}return r4}function __ZNKSt3__15ctypeIwE10do_tolowerEw(r1,r2){var r3;if(r2>>>0>=128){r3=r2;return r3}r3=HEAP32[HEAP32[___ctype_tolower_loc()>>2]+(r2<<2)>>2];return r3}function __ZNKSt3__15ctypeIwE10do_tolowerEPwPKw(r1,r2,r3){var r4,r5,r6;if((r2|0)==(r3|0)){r4=r2;return r4}else{r5=r2}while(1){r2=HEAP32[r5>>2];if(r2>>>0<128){r6=HEAP32[HEAP32[___ctype_tolower_loc()>>2]+(r2<<2)>>2]}else{r6=r2}HEAP32[r5>>2]=r6;r2=r5+4|0;if((r2|0)==(r3|0)){r4=r3;break}else{r5=r2}}return r4}function __ZNKSt3__15ctypeIcE10do_toupperEc(r1,r2){var r3;if(r2<<24>>24<=-1){r3=r2;return r3}r3=HEAP32[HEAP32[___ctype_toupper_loc()>>2]+(r2<<24>>24<<2)>>2]&255;return r3}function __ZNKSt3__15ctypeIcE10do_toupperEPcPKc(r1,r2,r3){var r4,r5,r6;if((r2|0)==(r3|0)){r4=r2;return r4}else{r5=r2}while(1){r2=HEAP8[r5];if(r2<<24>>24>-1){r6=HEAP32[HEAP32[___ctype_toupper_loc()>>2]+(r2<<24>>24<<2)>>2]&255}else{r6=r2}HEAP8[r5]=r6;r2=r5+1|0;if((r2|0)==(r3|0)){r4=r3;break}else{r5=r2}}return r4}function __ZNKSt3__15ctypeIcE10do_tolowerEc(r1,r2){var r3;if(r2<<24>>24<=-1){r3=r2;return r3}r3=HEAP32[HEAP32[___ctype_tolower_loc()>>2]+(r2<<24>>24<<2)>>2]&255;return r3}function __ZNKSt3__15ctypeIcE10do_tolowerEPcPKc(r1,r2,r3){var r4,r5,r6;if((r2|0)==(r3|0)){r4=r2;return r4}else{r5=r2}while(1){r2=HEAP8[r5];if(r2<<24>>24>-1){r6=HEAP32[HEAP32[___ctype_tolower_loc()>>2]+(r2<<24>>24<<2)>>2]&255}else{r6=r2}HEAP8[r5]=r6;r2=r5+1|0;if((r2|0)==(r3|0)){r4=r3;break}else{r5=r2}}return r4}function __ZNKSt3__17codecvtIwc10_mbstate_tE16do_always_noconvEv(r1){return 0}function __ZNSt3__17codecvtIDsc10_mbstate_tED0Ev(r1){__ZdlPv(r1);return}function __ZNKSt3__17codecvtIDsc10_mbstate_tE6do_outERS1_PKDsS5_RS5_PcS7_RS7_(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10;r2=STACKTOP;STACKTOP=STACKTOP+8|0;r1=r2;r9=r2+4;HEAP32[r1>>2]=r3;HEAP32[r9>>2]=r6;r10=__ZNSt3__1L13utf16_to_utf8EPKtS1_RS1_PhS3_RS3_mNS_12codecvt_modeE(r3,r4,r1,r6,r7,r9,1114111,0);HEAP32[r5>>2]=(HEAP32[r1>>2]-r3>>1<<1)+r3|0;HEAP32[r8>>2]=r6+(HEAP32[r9>>2]-r6)|0;STACKTOP=r2;return r10}function __ZNKSt3__17codecvtIwc10_mbstate_tE6do_outERS1_PKwS5_RS5_PcS7_RS7_(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35;r9=r8>>2;r8=r5>>2;r10=0;r11=STACKTOP;STACKTOP=STACKTOP+12|0;r12=r11;r13=r11+8;r14=r3;while(1){if((r14|0)==(r4|0)){r15=r4;break}if((HEAP32[r14>>2]|0)==0){r15=r14;break}else{r14=r14+4|0}}HEAP32[r9]=r6;HEAP32[r8]=r3;L1586:do{if((r3|0)==(r4|0)|(r6|0)==(r7|0)){r16=r3}else{r14=r2;r17=r12;r18=r7;r19=(r1+8|0)>>2;r20=r13|0;r21=r6;r22=r3;r23=r15;while(1){r24=HEAP32[r14+4>>2];HEAP32[r17>>2]=HEAP32[r14>>2];HEAP32[r17+4>>2]=r24;r24=_uselocale(HEAP32[r19]);r25=_wcsnrtombs(r21,r5,r23-r22>>2,r18-r21|0,r2);if((r24|0)!=0){_uselocale(r24)}if((r25|0)==-1){r10=1473;break}else if((r25|0)==0){r26=1;r10=1509;break}r24=HEAP32[r9]+r25|0;HEAP32[r9]=r24;if((r24|0)==(r7|0)){r10=1506;break}if((r23|0)==(r4|0)){r27=r4;r28=r24;r29=HEAP32[r8]}else{r24=_uselocale(HEAP32[r19]);r25=_wcrtomb(r20,0,r2);if((r24|0)!=0){_uselocale(r24)}if((r25|0)==-1){r26=2;r10=1510;break}r24=HEAP32[r9];if(r25>>>0>(r18-r24|0)>>>0){r26=1;r10=1511;break}L1605:do{if((r25|0)!=0){r30=r25;r31=r20;r32=r24;while(1){r33=HEAP8[r31];HEAP32[r9]=r32+1|0;HEAP8[r32]=r33;r33=r30-1|0;if((r33|0)==0){break L1605}r30=r33;r31=r31+1|0;r32=HEAP32[r9]}}}while(0);r24=HEAP32[r8]+4|0;HEAP32[r8]=r24;r25=r24;while(1){if((r25|0)==(r4|0)){r34=r4;break}if((HEAP32[r25>>2]|0)==0){r34=r25;break}else{r25=r25+4|0}}r27=r34;r28=HEAP32[r9];r29=r24}if((r29|0)==(r4|0)|(r28|0)==(r7|0)){r16=r29;break L1586}else{r21=r28;r22=r29;r23=r27}}if(r10==1473){HEAP32[r9]=r21;L1617:do{if((r22|0)==(HEAP32[r8]|0)){r35=r22}else{r23=r22;r20=r21;while(1){r18=HEAP32[r23>>2];r17=_uselocale(HEAP32[r19]);r14=_wcrtomb(r20,r18,r12);if((r17|0)!=0){_uselocale(r17)}if((r14|0)==-1){r35=r23;break L1617}r17=HEAP32[r9]+r14|0;HEAP32[r9]=r17;r14=r23+4|0;if((r14|0)==(HEAP32[r8]|0)){r35=r14;break L1617}else{r23=r14;r20=r17}}}}while(0);HEAP32[r8]=r35;r26=2;STACKTOP=r11;return r26}else if(r10==1506){r16=HEAP32[r8];break}else if(r10==1509){STACKTOP=r11;return r26}else if(r10==1510){STACKTOP=r11;return r26}else if(r10==1511){STACKTOP=r11;return r26}}}while(0);r26=(r16|0)!=(r4|0)&1;STACKTOP=r11;return r26}function __ZNKSt3__17codecvtIwc10_mbstate_tE5do_inERS1_PKcS5_RS5_PwS7_RS7_(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33;r9=r8>>2;r8=r5>>2;r10=0;r11=STACKTOP;STACKTOP=STACKTOP+8|0;r12=r11;r13=r3;while(1){if((r13|0)==(r4|0)){r14=r4;break}if(HEAP8[r13]<<24>>24==0){r14=r13;break}else{r13=r13+1|0}}HEAP32[r9]=r6;HEAP32[r8]=r3;L1638:do{if((r3|0)==(r4|0)|(r6|0)==(r7|0)){r15=r3}else{r13=r2;r16=r12;r17=r7;r18=(r1+8|0)>>2;r19=r6;r20=r3;r21=r14;while(1){r22=HEAP32[r13+4>>2];HEAP32[r16>>2]=HEAP32[r13>>2];HEAP32[r16+4>>2]=r22;r23=r21;r22=_uselocale(HEAP32[r18]);r24=_mbsnrtowcs(r19,r5,r23-r20|0,r17-r19>>2,r2);if((r22|0)!=0){_uselocale(r22)}if((r24|0)==-1){r10=1528;break}else if((r24|0)==0){r25=2;r10=1568;break}r22=(r24<<2)+HEAP32[r9]|0;HEAP32[r9]=r22;if((r22|0)==(r7|0)){r10=1560;break}r24=HEAP32[r8];if((r21|0)==(r4|0)){r26=r4;r27=r22;r28=r24}else{r29=_uselocale(HEAP32[r18]);r30=_mbrtowc(r22,r24,1,r2);if((r29|0)!=0){_uselocale(r29)}if((r30|0)!=0){r25=2;r10=1565;break}HEAP32[r9]=HEAP32[r9]+4|0;r30=HEAP32[r8]+1|0;HEAP32[r8]=r30;r29=r30;while(1){if((r29|0)==(r4|0)){r31=r4;break}if(HEAP8[r29]<<24>>24==0){r31=r29;break}else{r29=r29+1|0}}r26=r31;r27=HEAP32[r9];r28=r30}if((r28|0)==(r4|0)|(r27|0)==(r7|0)){r15=r28;break L1638}else{r19=r27;r20=r28;r21=r26}}if(r10==1528){HEAP32[r9]=r19;L1662:do{if((r20|0)==(HEAP32[r8]|0)){r32=r20}else{r21=r19;r17=r20;while(1){r16=_uselocale(HEAP32[r18]);r13=_mbrtowc(r21,r17,r23-r17|0,r12);if((r16|0)!=0){_uselocale(r16)}if((r13|0)==-2){r10=1540;break}else if((r13|0)==0){r33=r17+1|0}else if((r13|0)==-1){r10=1539;break}else{r33=r17+r13|0}r13=HEAP32[r9]+4|0;HEAP32[r9]=r13;if((r33|0)==(HEAP32[r8]|0)){r32=r33;break L1662}else{r21=r13;r17=r33}}if(r10==1540){HEAP32[r8]=r17;r25=1;STACKTOP=r11;return r25}else if(r10==1539){HEAP32[r8]=r17;r25=2;STACKTOP=r11;return r25}}}while(0);HEAP32[r8]=r32;r25=(r32|0)!=(r4|0)&1;STACKTOP=r11;return r25}else if(r10==1565){STACKTOP=r11;return r25}else if(r10==1568){STACKTOP=r11;return r25}else if(r10==1560){r15=HEAP32[r8];break}}}while(0);r25=(r15|0)!=(r4|0)&1;STACKTOP=r11;return r25}function __ZNKSt3__17codecvtIwc10_mbstate_tE10do_unshiftERS1_PcS4_RS4_(r1,r2,r3,r4,r5){var r6,r7,r8,r9,r10,r11;r6=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r5>>2]=r3;r3=r6|0;r7=_uselocale(HEAP32[r1+8>>2]);r1=_wcrtomb(r3,0,r2);if((r7|0)!=0){_uselocale(r7)}L1690:do{if((r1|0)==-1|(r1|0)==0){r8=2}else{r7=r1-1|0;r2=HEAP32[r5>>2];if(r7>>>0>(r4-r2|0)>>>0){r8=1;break}if((r7|0)==0){r8=0;break}else{r9=r7;r10=r3;r11=r2}while(1){r2=HEAP8[r10];HEAP32[r5>>2]=r11+1|0;HEAP8[r11]=r2;r2=r9-1|0;if((r2|0)==0){r8=0;break L1690}r9=r2;r10=r10+1|0;r11=HEAP32[r5>>2]}}}while(0);STACKTOP=r6;return r8}function __ZNKSt3__17codecvtIwc10_mbstate_tE11do_encodingEv(r1){var r2,r3,r4,r5,r6;r2=r1+8|0;r1=_uselocale(HEAP32[r2>>2]);r3=_mbtowc(0,0,1);if((r1|0)!=0){_uselocale(r1)}if((r3|0)!=0){r4=-1;return r4}r3=HEAP32[r2>>2];if((r3|0)==0){r4=1;return r4}r4=_uselocale(r3);r3=___locale_mb_cur_max();if((r4|0)==0){r5=(r3|0)==1;r6=r5&1;return r6}_uselocale(r4);r5=(r3|0)==1;r6=r5&1;return r6}function __ZNKSt3__17codecvtIwc10_mbstate_tE9do_lengthERS1_PKcS5_j(r1,r2,r3,r4,r5){var r6,r7,r8,r9,r10,r11,r12,r13,r14;r6=0;if((r5|0)==0|(r3|0)==(r4|0)){r7=0;return r7}r8=r4;r9=r1+8|0;r1=r3;r3=0;r10=0;while(1){r11=_uselocale(HEAP32[r9>>2]);r12=_mbrlen(r1,r8-r1|0,r2);if((r11|0)!=0){_uselocale(r11)}if((r12|0)==0){r13=1;r14=r1+1|0}else if((r12|0)==-1|(r12|0)==-2){r7=r3;r6=1624;break}else{r13=r12;r14=r1+r12|0}r12=r13+r3|0;r11=r10+1|0;if(r11>>>0>=r5>>>0|(r14|0)==(r4|0)){r7=r12;r6=1625;break}else{r1=r14;r3=r12;r10=r11}}if(r6==1625){return r7}else if(r6==1624){return r7}}function __ZNKSt3__17codecvtIwc10_mbstate_tE13do_max_lengthEv(r1){var r2,r3,r4;r2=HEAP32[r1+8>>2];do{if((r2|0)==0){r3=1}else{r1=_uselocale(r2);r4=___locale_mb_cur_max();if((r1|0)==0){r3=r4;break}_uselocale(r1);r3=r4}}while(0);return r3}function __ZNKSt3__17codecvtIDsc10_mbstate_tE10do_unshiftERS1_PcS4_RS4_(r1,r2,r3,r4,r5){HEAP32[r5>>2]=r3;return 3}function __ZNKSt3__17codecvtIDsc10_mbstate_tE11do_encodingEv(r1){return 0}function __ZNKSt3__17codecvtIDsc10_mbstate_tE16do_always_noconvEv(r1){return 0}function __ZNKSt3__17codecvtIDsc10_mbstate_tE13do_max_lengthEv(r1){return 4}function __ZNSt3__1L13utf16_to_utf8EPKtS1_RS1_PhS3_RS3_mNS_12codecvt_modeE(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10,r11,r12,r13,r14;r9=r6>>2;r6=r3>>2;r3=0;HEAP32[r6]=r1;HEAP32[r9]=r4;do{if((r8&2|0)!=0){if((r5-r4|0)<3){r10=1;return r10}else{HEAP32[r9]=r4+1|0;HEAP8[r4]=-17;r1=HEAP32[r9];HEAP32[r9]=r1+1|0;HEAP8[r1]=-69;r1=HEAP32[r9];HEAP32[r9]=r1+1|0;HEAP8[r1]=-65;break}}}while(0);r4=r2;r8=HEAP32[r6];if(r8>>>0>=r2>>>0){r10=0;return r10}r1=r5;r5=r8;L1753:while(1){r8=HEAP16[r5>>1];r11=r8&65535;if(r11>>>0>r7>>>0){r10=2;r3=1669;break}do{if((r8&65535)<128){r12=HEAP32[r9];if((r1-r12|0)<1){r10=1;r3=1672;break L1753}HEAP32[r9]=r12+1|0;HEAP8[r12]=r8&255}else{if((r8&65535)<2048){r12=HEAP32[r9];if((r1-r12|0)<2){r10=1;r3=1670;break L1753}HEAP32[r9]=r12+1|0;HEAP8[r12]=(r11>>>6|192)&255;r12=HEAP32[r9];HEAP32[r9]=r12+1|0;HEAP8[r12]=(r11&63|128)&255;break}if((r8&65535)<55296){r12=HEAP32[r9];if((r1-r12|0)<3){r10=1;r3=1676;break L1753}HEAP32[r9]=r12+1|0;HEAP8[r12]=(r11>>>12|224)&255;r12=HEAP32[r9];HEAP32[r9]=r12+1|0;HEAP8[r12]=(r11>>>6&63|128)&255;r12=HEAP32[r9];HEAP32[r9]=r12+1|0;HEAP8[r12]=(r11&63|128)&255;break}if((r8&65535)>=56320){if((r8&65535)<57344){r10=2;r3=1680;break L1753}r12=HEAP32[r9];if((r1-r12|0)<3){r10=1;r3=1681;break L1753}HEAP32[r9]=r12+1|0;HEAP8[r12]=(r11>>>12|224)&255;r12=HEAP32[r9];HEAP32[r9]=r12+1|0;HEAP8[r12]=(r11>>>6&63|128)&255;r12=HEAP32[r9];HEAP32[r9]=r12+1|0;HEAP8[r12]=(r11&63|128)&255;break}if((r4-r5|0)<4){r10=1;r3=1674;break L1753}r12=r5+2|0;r13=HEAPU16[r12>>1];if((r13&64512|0)!=56320){r10=2;r3=1678;break L1753}if((r1-HEAP32[r9]|0)<4){r10=1;r3=1675;break L1753}r14=r11&960;if(((r14<<10)+65536|r11<<10&64512|r13&1023)>>>0>r7>>>0){r10=2;r3=1679;break L1753}HEAP32[r6]=r12;r12=(r14>>>6)+1|0;r14=HEAP32[r9];HEAP32[r9]=r14+1|0;HEAP8[r14]=(r12>>>2|240)&255;r14=HEAP32[r9];HEAP32[r9]=r14+1|0;HEAP8[r14]=(r11>>>2&15|r12<<4&48|128)&255;r12=HEAP32[r9];HEAP32[r9]=r12+1|0;HEAP8[r12]=(r11<<4&48|r13>>>6&15|128)&255;r12=HEAP32[r9];HEAP32[r9]=r12+1|0;HEAP8[r12]=(r13&63|128)&255}}while(0);r11=HEAP32[r6]+2|0;HEAP32[r6]=r11;if(r11>>>0<r2>>>0){r5=r11}else{r10=0;r3=1671;break}}if(r3==1669){return r10}else if(r3==1670){return r10}else if(r3==1671){return r10}else if(r3==1678){return r10}else if(r3==1679){return r10}else if(r3==1680){return r10}else if(r3==1681){return r10}else if(r3==1674){return r10}else if(r3==1675){return r10}else if(r3==1676){return r10}else if(r3==1672){return r10}}function __ZNSt3__1L13utf8_to_utf16EPKhS1_RS1_PtS3_RS3_mNS_12codecvt_modeE(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22;r9=r6>>2;r6=r3>>2;r3=0;HEAP32[r6]=r1;HEAP32[r9]=r4;r4=HEAP32[r6];do{if((r8&4|0)==0){r10=r4}else{if((r2-r4|0)<=2){r10=r4;break}if(HEAP8[r4]<<24>>24!=-17){r10=r4;break}if(HEAP8[r4+1|0]<<24>>24!=-69){r10=r4;break}if(HEAP8[r4+2|0]<<24>>24!=-65){r10=r4;break}r1=r4+3|0;HEAP32[r6]=r1;r10=r1}}while(0);L1798:do{if(r10>>>0<r2>>>0){r4=r2;r8=r5;r1=HEAP32[r9],r11=r1>>1;r12=r10;L1800:while(1){if(r1>>>0>=r5>>>0){r13=r12;break L1798}r14=HEAP8[r12];r15=r14&255;if(r15>>>0>r7>>>0){r16=2;r3=1732;break}do{if(r14<<24>>24>-1){HEAP16[r11]=r14&255;HEAP32[r6]=HEAP32[r6]+1|0}else{if((r14&255)<194){r16=2;r3=1726;break L1800}if((r14&255)<224){if((r4-r12|0)<2){r16=1;r3=1727;break L1800}r17=HEAPU8[r12+1|0];if((r17&192|0)!=128){r16=2;r3=1728;break L1800}r18=r17&63|r15<<6&1984;if(r18>>>0>r7>>>0){r16=2;r3=1729;break L1800}HEAP16[r11]=r18&65535;HEAP32[r6]=HEAP32[r6]+2|0;break}if((r14&255)<240){if((r4-r12|0)<3){r16=1;r3=1730;break L1800}r18=HEAP8[r12+1|0];r17=HEAP8[r12+2|0];if((r15|0)==237){if((r18&-32)<<24>>24!=-128){r16=2;r3=1723;break L1800}}else if((r15|0)==224){if((r18&-32)<<24>>24!=-96){r16=2;r3=1731;break L1800}}else{if((r18&-64)<<24>>24!=-128){r16=2;r3=1738;break L1800}}r19=r17&255;if((r19&192|0)!=128){r16=2;r3=1741;break L1800}r17=(r18&255)<<6&4032|r15<<12|r19&63;if((r17&65535)>>>0>r7>>>0){r16=2;r3=1742;break L1800}HEAP16[r11]=r17&65535;HEAP32[r6]=HEAP32[r6]+3|0;break}if((r14&255)>=245){r16=2;r3=1736;break L1800}if((r4-r12|0)<4){r16=1;r3=1743;break L1800}r17=HEAP8[r12+1|0];r19=HEAP8[r12+2|0];r18=HEAP8[r12+3|0];if((r15|0)==240){if((r17+112&255)>=48){r16=2;r3=1724;break L1800}}else if((r15|0)==244){if((r17&-16)<<24>>24!=-128){r16=2;r3=1733;break L1800}}else{if((r17&-64)<<24>>24!=-128){r16=2;r3=1735;break L1800}}r20=r19&255;if((r20&192|0)!=128){r16=2;r3=1737;break L1800}r19=r18&255;if((r19&192|0)!=128){r16=2;r3=1734;break L1800}if((r8-r1|0)<4){r16=1;r3=1740;break L1800}r18=r15&7;r21=r17&255;r17=r20<<6;r22=r19&63;if((r21<<12&258048|r18<<18|r17&4032|r22)>>>0>r7>>>0){r16=2;r3=1739;break L1800}HEAP16[r11]=(r21<<2&60|r20>>>4&3|((r21>>>4&3|r18<<2)<<6)+16320|55296)&65535;r18=HEAP32[r9]+2|0;HEAP32[r9]=r18;HEAP16[r18>>1]=(r22|r17&960|56320)&65535;HEAP32[r6]=HEAP32[r6]+4|0}}while(0);r15=HEAP32[r9]+2|0;HEAP32[r9]=r15;r14=HEAP32[r6];if(r14>>>0<r2>>>0){r1=r15,r11=r1>>1;r12=r14}else{r13=r14;break L1798}}if(r3==1743){return r16}else if(r3==1726){return r16}else if(r3==1727){return r16}else if(r3==1728){return r16}else if(r3==1729){return r16}else if(r3==1730){return r16}else if(r3==1731){return r16}else if(r3==1732){return r16}else if(r3==1733){return r16}else if(r3==1734){return r16}else if(r3==1735){return r16}else if(r3==1736){return r16}else if(r3==1737){return r16}else if(r3==1738){return r16}else if(r3==1739){return r16}else if(r3==1740){return r16}else if(r3==1741){return r16}else if(r3==1742){return r16}else if(r3==1723){return r16}else if(r3==1724){return r16}}else{r13=r10}}while(0);r16=r13>>>0<r2>>>0&1;return r16}function __ZNSt3__1L20utf8_to_utf16_lengthEPKhS1_jmNS_12codecvt_modeE(r1,r2,r3,r4,r5){var r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21;r6=0;do{if((r5&4|0)==0){r7=r1}else{if((r2-r1|0)<=2){r7=r1;break}if(HEAP8[r1]<<24>>24!=-17){r7=r1;break}if(HEAP8[r1+1|0]<<24>>24!=-69){r7=r1;break}r7=HEAP8[r1+2|0]<<24>>24==-65?r1+3|0:r1}}while(0);L1867:do{if(r7>>>0<r2>>>0&(r3|0)!=0){r5=r2;r8=0;r9=r7;L1869:while(1){r10=HEAP8[r9];r11=r10&255;if(r11>>>0>r4>>>0){r12=r9;break L1867}do{if(r10<<24>>24>-1){r13=r9+1|0;r14=r8}else{if((r10&255)<194){r12=r9;break L1867}if((r10&255)<224){if((r5-r9|0)<2){r12=r9;break L1867}r15=HEAPU8[r9+1|0];if((r15&192|0)!=128){r12=r9;break L1867}if((r15&63|r11<<6&1984)>>>0>r4>>>0){r12=r9;break L1867}r13=r9+2|0;r14=r8;break}if((r10&255)<240){r16=r9;if((r5-r16|0)<3){r12=r9;break L1867}r15=HEAP8[r9+1|0];r17=HEAP8[r9+2|0];if((r11|0)==224){if((r15&-32)<<24>>24!=-96){r6=1764;break L1869}}else if((r11|0)==237){if((r15&-32)<<24>>24!=-128){r6=1766;break L1869}}else{if((r15&-64)<<24>>24!=-128){r6=1768;break L1869}}r18=r17&255;if((r18&192|0)!=128){r12=r9;break L1867}if(((r15&255)<<6&4032|r11<<12&61440|r18&63)>>>0>r4>>>0){r12=r9;break L1867}r13=r9+3|0;r14=r8;break}if((r10&255)>=245){r12=r9;break L1867}r19=r9;if((r5-r19|0)<4){r12=r9;break L1867}if((r3-r8|0)>>>0<2){r12=r9;break L1867}r18=HEAP8[r9+1|0];r15=HEAP8[r9+2|0];r17=HEAP8[r9+3|0];if((r11|0)==244){if((r18&-16)<<24>>24!=-128){r6=1779;break L1869}}else if((r11|0)==240){if((r18+112&255)>=48){r6=1777;break L1869}}else{if((r18&-64)<<24>>24!=-128){r6=1781;break L1869}}r20=r15&255;if((r20&192|0)!=128){r12=r9;break L1867}r15=r17&255;if((r15&192|0)!=128){r12=r9;break L1867}if(((r18&255)<<12&258048|r11<<18&1835008|r20<<6&4032|r15&63)>>>0>r4>>>0){r12=r9;break L1867}r13=r9+4|0;r14=r8+1|0}}while(0);r11=r14+1|0;if(r13>>>0<r2>>>0&r11>>>0<r3>>>0){r8=r11;r9=r13}else{r12=r13;break L1867}}if(r6==1779){r21=r19-r1|0;return r21}else if(r6==1764){r21=r16-r1|0;return r21}else if(r6==1766){r21=r16-r1|0;return r21}else if(r6==1777){r21=r19-r1|0;return r21}else if(r6==1781){r21=r19-r1|0;return r21}else if(r6==1768){r21=r16-r1|0;return r21}}else{r12=r7}}while(0);r21=r12-r1|0;return r21}function __ZNKSt3__17codecvtIDsc10_mbstate_tE5do_inERS1_PKcS5_RS5_PDsS7_RS7_(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10;r2=STACKTOP;STACKTOP=STACKTOP+8|0;r1=r2;r9=r2+4;HEAP32[r1>>2]=r3;HEAP32[r9>>2]=r6;r10=__ZNSt3__1L13utf8_to_utf16EPKhS1_RS1_PtS3_RS3_mNS_12codecvt_modeE(r3,r4,r1,r6,r7,r9,1114111,0);HEAP32[r5>>2]=r3+(HEAP32[r1>>2]-r3)|0;HEAP32[r8>>2]=(HEAP32[r9>>2]-r6>>1<<1)+r6|0;STACKTOP=r2;return r10}function __ZNKSt3__17codecvtIDsc10_mbstate_tE9do_lengthERS1_PKcS5_j(r1,r2,r3,r4,r5){return __ZNSt3__1L20utf8_to_utf16_lengthEPKhS1_jmNS_12codecvt_modeE(r3,r4,r5,1114111,0)}function __ZNSt3__17codecvtIDic10_mbstate_tED0Ev(r1){__ZdlPv(r1);return}function __ZNKSt3__17codecvtIDic10_mbstate_tE6do_outERS1_PKDiS5_RS5_PcS7_RS7_(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10;r2=STACKTOP;STACKTOP=STACKTOP+8|0;r1=r2;r9=r2+4;HEAP32[r1>>2]=r3;HEAP32[r9>>2]=r6;r10=__ZNSt3__1L12ucs4_to_utf8EPKjS1_RS1_PhS3_RS3_mNS_12codecvt_modeE(r3,r4,r1,r6,r7,r9,1114111,0);HEAP32[r5>>2]=(HEAP32[r1>>2]-r3>>2<<2)+r3|0;HEAP32[r8>>2]=r6+(HEAP32[r9>>2]-r6)|0;STACKTOP=r2;return r10}function __ZNKSt3__17codecvtIDic10_mbstate_tE10do_unshiftERS1_PcS4_RS4_(r1,r2,r3,r4,r5){HEAP32[r5>>2]=r3;return 3}function __ZNKSt3__17codecvtIDic10_mbstate_tE11do_encodingEv(r1){return 0}function __ZNKSt3__17codecvtIDic10_mbstate_tE16do_always_noconvEv(r1){return 0}function __ZNKSt3__17codecvtIDic10_mbstate_tE13do_max_lengthEv(r1){return 4}function __ZNSt3__1L12ucs4_to_utf8EPKjS1_RS1_PhS3_RS3_mNS_12codecvt_modeE(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10,r11,r12;r9=r6>>2;r6=0;HEAP32[r3>>2]=r1;HEAP32[r9]=r4;do{if((r8&2|0)!=0){if((r5-r4|0)<3){r10=1;return r10}else{HEAP32[r9]=r4+1|0;HEAP8[r4]=-17;r1=HEAP32[r9];HEAP32[r9]=r1+1|0;HEAP8[r1]=-69;r1=HEAP32[r9];HEAP32[r9]=r1+1|0;HEAP8[r1]=-65;break}}}while(0);r4=HEAP32[r3>>2];if(r4>>>0>=r2>>>0){r10=0;return r10}r8=r5;r5=r4;L1938:while(1){r4=HEAP32[r5>>2];if((r4&-2048|0)==55296|r4>>>0>r7>>>0){r10=2;r6=1823;break}do{if(r4>>>0<128){r1=HEAP32[r9];if((r8-r1|0)<1){r10=1;r6=1825;break L1938}HEAP32[r9]=r1+1|0;HEAP8[r1]=r4&255}else{if(r4>>>0<2048){r1=HEAP32[r9];if((r8-r1|0)<2){r10=1;r6=1824;break L1938}HEAP32[r9]=r1+1|0;HEAP8[r1]=(r4>>>6|192)&255;r1=HEAP32[r9];HEAP32[r9]=r1+1|0;HEAP8[r1]=(r4&63|128)&255;break}r1=HEAP32[r9];r11=r8-r1|0;if(r4>>>0<65536){if((r11|0)<3){r10=1;r6=1829;break L1938}HEAP32[r9]=r1+1|0;HEAP8[r1]=(r4>>>12|224)&255;r12=HEAP32[r9];HEAP32[r9]=r12+1|0;HEAP8[r12]=(r4>>>6&63|128)&255;r12=HEAP32[r9];HEAP32[r9]=r12+1|0;HEAP8[r12]=(r4&63|128)&255;break}else{if((r11|0)<4){r10=1;r6=1830;break L1938}HEAP32[r9]=r1+1|0;HEAP8[r1]=(r4>>>18|240)&255;r1=HEAP32[r9];HEAP32[r9]=r1+1|0;HEAP8[r1]=(r4>>>12&63|128)&255;r1=HEAP32[r9];HEAP32[r9]=r1+1|0;HEAP8[r1]=(r4>>>6&63|128)&255;r1=HEAP32[r9];HEAP32[r9]=r1+1|0;HEAP8[r1]=(r4&63|128)&255;break}}}while(0);r4=HEAP32[r3>>2]+4|0;HEAP32[r3>>2]=r4;if(r4>>>0<r2>>>0){r5=r4}else{r10=0;r6=1828;break}}if(r6==1829){return r10}else if(r6==1828){return r10}else if(r6==1824){return r10}else if(r6==1825){return r10}else if(r6==1830){return r10}else if(r6==1823){return r10}}function __ZNSt3__1L12utf8_to_ucs4EPKhS1_RS1_PjS3_RS3_mNS_12codecvt_modeE(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19;r9=r3>>2;r3=0;HEAP32[r9]=r1;HEAP32[r6>>2]=r4;r4=HEAP32[r9];do{if((r8&4|0)==0){r10=r4}else{if((r2-r4|0)<=2){r10=r4;break}if(HEAP8[r4]<<24>>24!=-17){r10=r4;break}if(HEAP8[r4+1|0]<<24>>24!=-69){r10=r4;break}if(HEAP8[r4+2|0]<<24>>24!=-65){r10=r4;break}r1=r4+3|0;HEAP32[r9]=r1;r10=r1}}while(0);L1970:do{if(r10>>>0<r2>>>0){r4=r2;r8=HEAP32[r6>>2],r1=r8>>2;r11=r10;L1972:while(1){if(r8>>>0>=r5>>>0){r12=r11;break L1970}r13=HEAP8[r11];r14=r13&255;do{if(r13<<24>>24>-1){if(r14>>>0>r7>>>0){r15=2;r3=1871;break L1972}HEAP32[r1]=r14;HEAP32[r9]=HEAP32[r9]+1|0}else{if((r13&255)<194){r15=2;r3=1873;break L1972}if((r13&255)<224){if((r4-r11|0)<2){r15=1;r3=1881;break L1972}r16=HEAPU8[r11+1|0];if((r16&192|0)!=128){r15=2;r3=1887;break L1972}r17=r16&63|r14<<6&1984;if(r17>>>0>r7>>>0){r15=2;r3=1888;break L1972}HEAP32[r1]=r17;HEAP32[r9]=HEAP32[r9]+2|0;break}if((r13&255)<240){if((r4-r11|0)<3){r15=1;r3=1882;break L1972}r17=HEAP8[r11+1|0];r16=HEAP8[r11+2|0];if((r14|0)==237){if((r17&-32)<<24>>24!=-128){r15=2;r3=1878;break L1972}}else if((r14|0)==224){if((r17&-32)<<24>>24!=-96){r15=2;r3=1886;break L1972}}else{if((r17&-64)<<24>>24!=-128){r15=2;r3=1879;break L1972}}r18=r16&255;if((r18&192|0)!=128){r15=2;r3=1885;break L1972}r16=(r17&255)<<6&4032|r14<<12&61440|r18&63;if(r16>>>0>r7>>>0){r15=2;r3=1872;break L1972}HEAP32[r1]=r16;HEAP32[r9]=HEAP32[r9]+3|0;break}if((r13&255)>=245){r15=2;r3=1883;break L1972}if((r4-r11|0)<4){r15=1;r3=1880;break L1972}r16=HEAP8[r11+1|0];r18=HEAP8[r11+2|0];r17=HEAP8[r11+3|0];if((r14|0)==240){if((r16+112&255)>=48){r15=2;r3=1875;break L1972}}else if((r14|0)==244){if((r16&-16)<<24>>24!=-128){r15=2;r3=1876;break L1972}}else{if((r16&-64)<<24>>24!=-128){r15=2;r3=1884;break L1972}}r19=r18&255;if((r19&192|0)!=128){r15=2;r3=1889;break L1972}r18=r17&255;if((r18&192|0)!=128){r15=2;r3=1890;break L1972}r17=(r16&255)<<12&258048|r14<<18&1835008|r19<<6&4032|r18&63;if(r17>>>0>r7>>>0){r15=2;r3=1877;break L1972}HEAP32[r1]=r17;HEAP32[r9]=HEAP32[r9]+4|0}}while(0);r14=HEAP32[r6>>2]+4|0;HEAP32[r6>>2]=r14;r13=HEAP32[r9];if(r13>>>0<r2>>>0){r8=r14,r1=r8>>2;r11=r13}else{r12=r13;break L1970}}if(r3==1871){return r15}else if(r3==1872){return r15}else if(r3==1873){return r15}else if(r3==1875){return r15}else if(r3==1876){return r15}else if(r3==1882){return r15}else if(r3==1883){return r15}else if(r3==1884){return r15}else if(r3==1885){return r15}else if(r3==1886){return r15}else if(r3==1887){return r15}else if(r3==1877){return r15}else if(r3==1878){return r15}else if(r3==1879){return r15}else if(r3==1880){return r15}else if(r3==1881){return r15}else if(r3==1888){return r15}else if(r3==1889){return r15}else if(r3==1890){return r15}}else{r12=r10}}while(0);r15=r12>>>0<r2>>>0&1;return r15}function __ZNSt3__1L19utf8_to_ucs4_lengthEPKhS1_jmNS_12codecvt_modeE(r1,r2,r3,r4,r5){var r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20;r6=0;do{if((r5&4|0)==0){r7=r1}else{if((r2-r1|0)<=2){r7=r1;break}if(HEAP8[r1]<<24>>24!=-17){r7=r1;break}if(HEAP8[r1+1|0]<<24>>24!=-69){r7=r1;break}r7=HEAP8[r1+2|0]<<24>>24==-65?r1+3|0:r1}}while(0);L2037:do{if(r7>>>0<r2>>>0&(r3|0)!=0){r5=r2;r8=1;r9=r7;L2039:while(1){r10=HEAP8[r9];r11=r10&255;do{if(r10<<24>>24>-1){if(r11>>>0>r4>>>0){r12=r9;break L2037}r13=r9+1|0}else{if((r10&255)<194){r12=r9;break L2037}if((r10&255)<224){if((r5-r9|0)<2){r12=r9;break L2037}r14=HEAPU8[r9+1|0];if((r14&192|0)!=128){r12=r9;break L2037}if((r14&63|r11<<6&1984)>>>0>r4>>>0){r12=r9;break L2037}r13=r9+2|0;break}if((r10&255)<240){r15=r9;if((r5-r15|0)<3){r12=r9;break L2037}r14=HEAP8[r9+1|0];r16=HEAP8[r9+2|0];if((r11|0)==224){if((r14&-32)<<24>>24!=-96){r6=1911;break L2039}}else if((r11|0)==237){if((r14&-32)<<24>>24!=-128){r6=1913;break L2039}}else{if((r14&-64)<<24>>24!=-128){r6=1915;break L2039}}r17=r16&255;if((r17&192|0)!=128){r12=r9;break L2037}if(((r14&255)<<6&4032|r11<<12&61440|r17&63)>>>0>r4>>>0){r12=r9;break L2037}r13=r9+3|0;break}if((r10&255)>=245){r12=r9;break L2037}r18=r9;if((r5-r18|0)<4){r12=r9;break L2037}r17=HEAP8[r9+1|0];r14=HEAP8[r9+2|0];r16=HEAP8[r9+3|0];if((r11|0)==240){if((r17+112&255)>=48){r6=1923;break L2039}}else if((r11|0)==244){if((r17&-16)<<24>>24!=-128){r6=1925;break L2039}}else{if((r17&-64)<<24>>24!=-128){r6=1927;break L2039}}r19=r14&255;if((r19&192|0)!=128){r12=r9;break L2037}r14=r16&255;if((r14&192|0)!=128){r12=r9;break L2037}if(((r17&255)<<12&258048|r11<<18&1835008|r19<<6&4032|r14&63)>>>0>r4>>>0){r12=r9;break L2037}r13=r9+4|0}}while(0);if(!(r13>>>0<r2>>>0&r8>>>0<r3>>>0)){r12=r13;break L2037}r8=r8+1|0;r9=r13}if(r6==1927){r20=r18-r1|0;return r20}else if(r6==1911){r20=r15-r1|0;return r20}else if(r6==1913){r20=r15-r1|0;return r20}else if(r6==1915){r20=r15-r1|0;return r20}else if(r6==1923){r20=r18-r1|0;return r20}else if(r6==1925){r20=r18-r1|0;return r20}}else{r12=r7}}while(0);r20=r12-r1|0;return r20}function __ZNKSt3__18numpunctIcE16do_decimal_pointEv(r1){return HEAP8[r1+8|0]}function __ZNKSt3__18numpunctIwE16do_decimal_pointEv(r1){return HEAP32[r1+8>>2]}function __ZNKSt3__18numpunctIcE16do_thousands_sepEv(r1){return HEAP8[r1+9|0]}function __ZNKSt3__18numpunctIwE16do_thousands_sepEv(r1){return HEAP32[r1+12>>2]}function __ZNKSt3__18numpunctIcE11do_truenameEv(r1,r2){r2=r1;HEAP8[r1]=8;r1=r2+1|0;tempBigInt=1702195828;HEAP8[r1]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r1+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r1+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r1+3|0]=tempBigInt&255;HEAP8[r2+5|0]=0;return}function __ZNKSt3__17codecvtIDic10_mbstate_tE5do_inERS1_PKcS5_RS5_PDiS7_RS7_(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10;r2=STACKTOP;STACKTOP=STACKTOP+8|0;r1=r2;r9=r2+4;HEAP32[r1>>2]=r3;HEAP32[r9>>2]=r6;r10=__ZNSt3__1L12utf8_to_ucs4EPKhS1_RS1_PjS3_RS3_mNS_12codecvt_modeE(r3,r4,r1,r6,r7,r9,1114111,0);HEAP32[r5>>2]=r3+(HEAP32[r1>>2]-r3)|0;HEAP32[r8>>2]=(HEAP32[r9>>2]-r6>>2<<2)+r6|0;STACKTOP=r2;return r10}function __ZNKSt3__17codecvtIDic10_mbstate_tE9do_lengthERS1_PKcS5_j(r1,r2,r3,r4,r5){return __ZNSt3__1L19utf8_to_ucs4_lengthEPKhS1_jmNS_12codecvt_modeE(r3,r4,r5,1114111,0)}function __ZNSt3__116__narrow_to_utf8ILj32EED0Ev(r1){__ZdlPv(r1);return}function __ZNSt3__117__widen_from_utf8ILj32EED0Ev(r1){__ZdlPv(r1);return}function __ZNSt3__18numpunctIcED0Ev(r1){var r2;HEAP32[r1>>2]=5247988;if((HEAP8[r1+12|0]&1)<<24>>24==0){r2=r1;__ZdlPv(r2);return}__ZdlPv(HEAP32[r1+20>>2]);r2=r1;__ZdlPv(r2);return}function __ZNSt3__18numpunctIcED2Ev(r1){HEAP32[r1>>2]=5247988;if((HEAP8[r1+12|0]&1)<<24>>24==0){return}__ZdlPv(HEAP32[r1+20>>2]);return}function __ZNSt3__18numpunctIwED0Ev(r1){var r2;HEAP32[r1>>2]=5247944;if((HEAP8[r1+16|0]&1)<<24>>24==0){r2=r1;__ZdlPv(r2);return}__ZdlPv(HEAP32[r1+24>>2]);r2=r1;__ZdlPv(r2);return}function __ZNSt3__18numpunctIwED2Ev(r1){HEAP32[r1>>2]=5247944;if((HEAP8[r1+16|0]&1)<<24>>24==0){return}__ZdlPv(HEAP32[r1+24>>2]);return}function __ZNKSt3__18numpunctIcE11do_groupingEv(r1,r2){var r3,r4,r5,r6;r3=r2+12|0,r4=r3>>2;if((HEAP8[r3]&1)<<24>>24==0){r3=r1>>2;HEAP32[r3]=HEAP32[r4];HEAP32[r3+1]=HEAP32[r4+1];HEAP32[r3+2]=HEAP32[r4+2];return}r4=HEAP32[r2+20>>2];r3=HEAP32[r2+16>>2];if((r3|0)==-1){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if(r3>>>0<11){HEAP8[r1]=r3<<1&255;r5=r1+1|0}else{r2=r3+16&-16;r6=__Znwj(r2);HEAP32[r1+8>>2]=r6;HEAP32[r1>>2]=r2|1;HEAP32[r1+4>>2]=r3;r5=r6}_memcpy(r5,r4,r3);HEAP8[r5+r3|0]=0;return}function __ZNKSt3__18numpunctIwE11do_groupingEv(r1,r2){var r3,r4,r5,r6;r3=r2+16|0,r4=r3>>2;if((HEAP8[r3]&1)<<24>>24==0){r3=r1>>2;HEAP32[r3]=HEAP32[r4];HEAP32[r3+1]=HEAP32[r4+1];HEAP32[r3+2]=HEAP32[r4+2];return}r4=HEAP32[r2+24>>2];r3=HEAP32[r2+20>>2];if((r3|0)==-1){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if(r3>>>0<11){HEAP8[r1]=r3<<1&255;r5=r1+1|0}else{r2=r3+16&-16;r6=__Znwj(r2);HEAP32[r1+8>>2]=r6;HEAP32[r1>>2]=r2|1;HEAP32[r1+4>>2]=r3;r5=r6}_memcpy(r5,r4,r3);HEAP8[r5+r3|0]=0;return}function __ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r1,r2,r3,r4){var r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19;r5=r1;r6=r1;r7=HEAP8[r6];r8=r7&255;if((r8&1|0)==0){r9=r8>>>1}else{r9=HEAP32[r1+4>>2]}if((r9|0)==0){return}do{if((r2|0)==(r3|0)){r10=r7}else{r9=r3-4|0;if(r9>>>0>r2>>>0){r11=r2;r12=r9}else{r10=r7;break}while(1){r9=HEAP32[r11>>2];HEAP32[r11>>2]=HEAP32[r12>>2];HEAP32[r12>>2]=r9;r9=r11+4|0;r8=r12-4|0;if(r9>>>0<r8>>>0){r11=r9;r12=r8}else{break}}r10=HEAP8[r6]}}while(0);if((r10&1)<<24>>24==0){r13=r5+1|0}else{r13=HEAP32[r1+8>>2]}r5=r10&255;if((r5&1|0)==0){r14=r5>>>1}else{r14=HEAP32[r1+4>>2]}r1=r3-4|0;r3=HEAP8[r13];r5=r3<<24>>24;r10=r3<<24>>24<1|r3<<24>>24==127;L2167:do{if(r1>>>0>r2>>>0){r3=r13+r14|0;r6=r13;r12=r2;r11=r5;r7=r10;while(1){if(!r7){if((r11|0)!=(HEAP32[r12>>2]|0)){break}}r8=(r3-r6|0)>1?r6+1|0:r6;r9=r12+4|0;r15=HEAP8[r8];r16=r15<<24>>24;r17=r15<<24>>24<1|r15<<24>>24==127;if(r9>>>0<r1>>>0){r6=r8;r12=r9;r11=r16;r7=r17}else{r18=r16;r19=r17;break L2167}}HEAP32[r4>>2]=4;return}else{r18=r5;r19=r10}}while(0);if(r19){return}r19=HEAP32[r1>>2];if(!(r18>>>0<r19>>>0|(r19|0)==0)){return}HEAP32[r4>>2]=4;return}function __ZNKSt3__18numpunctIcE12do_falsenameEv(r1,r2){r2=r1;HEAP8[r1]=10;r1=r2+1|0;HEAP8[r1]=HEAP8[5243784];HEAP8[r1+1|0]=HEAP8[5243785|0];HEAP8[r1+2|0]=HEAP8[5243786|0];HEAP8[r1+3|0]=HEAP8[5243787|0];HEAP8[r1+4|0]=HEAP8[5243788|0];HEAP8[r2+6|0]=0;return}function __ZNKSt3__120__time_get_c_storageIcE7__weeksEv(r1){var r2;if(HEAP8[5255564]<<24>>24!=0){r2=HEAP32[1311850];return r2}if((___cxa_guard_acquire(5255564)|0)==0){r2=HEAP32[1311850];return r2}do{if(HEAP8[5255452]<<24>>24==0){if((___cxa_guard_acquire(5255452)|0)==0){break}_memset(5246136,0,168);_atexit(298,0,___dso_handle)}}while(0);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246136,5243952);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246148,5243944);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246160,5243936);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246172,5243924);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246184,5243912);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246196,5243904);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246208,5243892);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246220,5243888);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246232,5243884);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246244,5243880);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246256,5243876);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246268,5243872);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246280,5243868);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246292,5243864);HEAP32[1311850]=5246136;r2=HEAP32[1311850];return r2}function __ZNKSt3__120__time_get_c_storageIwE7__weeksEv(r1){var r2;if(HEAP8[5255508]<<24>>24!=0){r2=HEAP32[1311835];return r2}if((___cxa_guard_acquire(5255508)|0)==0){r2=HEAP32[1311835];return r2}do{if(HEAP8[5255428]<<24>>24==0){if((___cxa_guard_acquire(5255428)|0)==0){break}_memset(5245392,0,168);_atexit(492,0,___dso_handle)}}while(0);__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245392,5244284,_wcslen(5244284));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245404,5244256,_wcslen(5244256));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245416,5244224,_wcslen(5244224));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245428,5244172,_wcslen(5244172));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245440,5244136,_wcslen(5244136));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245452,5244108,_wcslen(5244108));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245464,5244072,_wcslen(5244072));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245476,5244056,_wcslen(5244056));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245488,5244040,_wcslen(5244040));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245500,5244024,_wcslen(5244024));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245512,5244008,_wcslen(5244008));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245524,5243992,_wcslen(5243992));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245536,5243976,_wcslen(5243976));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245548,5243960,_wcslen(5243960));HEAP32[1311835]=5245392;r2=HEAP32[1311835];return r2}function __ZNKSt3__120__time_get_c_storageIcE8__monthsEv(r1){var r2;if(HEAP8[5255556]<<24>>24!=0){r2=HEAP32[1311849];return r2}if((___cxa_guard_acquire(5255556)|0)==0){r2=HEAP32[1311849];return r2}do{if(HEAP8[5255444]<<24>>24==0){if((___cxa_guard_acquire(5255444)|0)==0){break}_memset(5245848,0,288);_atexit(382,0,___dso_handle)}}while(0);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5245848,5244496);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5245860,5244484);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5245872,5244476);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5245884,5244468);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5245896,5244464);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5245908,5244420);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5245920,5244412);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5245932,5244404);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5245944,5244392);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5245956,5244384);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5245968,5244372);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5245980,5244360);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5245992,5244352);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246004,5244348);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246016,5244344);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246028,5244340);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246040,5244464);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246052,5244336);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246064,5244332);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246076,5244328);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246088,5244324);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246100,5244320);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246112,5244316);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246124,5244312);HEAP32[1311849]=5245848;r2=HEAP32[1311849];return r2}function __ZNKSt3__120__time_get_c_storageIwE8__monthsEv(r1){var r2;if(HEAP8[5255500]<<24>>24!=0){r2=HEAP32[1311834];return r2}if((___cxa_guard_acquire(5255500)|0)==0){r2=HEAP32[1311834];return r2}do{if(HEAP8[5255420]<<24>>24==0){if((___cxa_guard_acquire(5255420)|0)==0){break}_memset(5245104,0,288);_atexit(418,0,___dso_handle)}}while(0);__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245104,5243376,_wcslen(5243376));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245116,5243340,_wcslen(5243340));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245128,5243316,_wcslen(5243316));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245140,5243292,_wcslen(5243292));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245152,5244616,_wcslen(5244616));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245164,5243272,_wcslen(5243272));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245176,5243252,_wcslen(5243252));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245188,5243224,_wcslen(5243224));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245200,5243184,_wcslen(5243184));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245212,5243112,_wcslen(5243112));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245224,5243076,_wcslen(5243076));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245236,5243040,_wcslen(5243040));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245248,5243024,_wcslen(5243024));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245260,5243008,_wcslen(5243008));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245272,5242992,_wcslen(5242992));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245284,5242976,_wcslen(5242976));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245296,5244616,_wcslen(5244616));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245308,5244600,_wcslen(5244600));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245320,5244584,_wcslen(5244584));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245332,5244568,_wcslen(5244568));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245344,5244552,_wcslen(5244552));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245356,5244536,_wcslen(5244536));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245368,5244520,_wcslen(5244520));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245380,5244504,_wcslen(5244504));HEAP32[1311834]=5245104;r2=HEAP32[1311834];return r2}function __ZNKSt3__120__time_get_c_storageIcE7__am_pmEv(r1){var r2;if(HEAP8[5255572]<<24>>24!=0){r2=HEAP32[1311851];return r2}if((___cxa_guard_acquire(5255572)|0)==0){r2=HEAP32[1311851];return r2}do{if(HEAP8[5255460]<<24>>24==0){if((___cxa_guard_acquire(5255460)|0)==0){break}_memset(5246304,0,288);_atexit(420,0,___dso_handle)}}while(0);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246304,5243412);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6assignEPKc(5246316,5243408);HEAP32[1311851]=5246304;r2=HEAP32[1311851];return r2}function __ZNKSt3__120__time_get_c_storageIwE7__am_pmEv(r1){var r2;if(HEAP8[5255516]<<24>>24!=0){r2=HEAP32[1311836];return r2}if((___cxa_guard_acquire(5255516)|0)==0){r2=HEAP32[1311836];return r2}do{if(HEAP8[5255436]<<24>>24==0){if((___cxa_guard_acquire(5255436)|0)==0){break}_memset(5245560,0,288);_atexit(686,0,___dso_handle)}}while(0);__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245560,5243428,_wcslen(5243428));__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(5245572,5243416,_wcslen(5243416));HEAP32[1311836]=5245560;r2=HEAP32[1311836];return r2}function __ZNKSt3__120__time_get_c_storageIcE3__xEv(r1){var r2;if(HEAP8[5255580]<<24>>24!=0){return 5247408}if((___cxa_guard_acquire(5255580)|0)==0){return 5247408}HEAP8[5247408]=16;r1=5247409;r2=r1|0;tempBigInt=623865125;HEAP8[r2]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+3|0]=tempBigInt&255;r2=r1+4|0;tempBigInt=2032480100;HEAP8[r2]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+3|0]=tempBigInt&255;HEAP8[5247417]=0;_atexit(718,5247408,___dso_handle);return 5247408}function __ZNKSt3__18numpunctIwE11do_truenameEv(r1,r2){var r3,r4,r5,r6,r7;r2=_wcslen(5243792);if(r2>>>0>1073741822){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if(r2>>>0<2){HEAP8[r1]=r2<<1&255;r3=r1+4|0;r4=_wmemcpy(r3,5243792,r2);r5=(r2<<2)+r3|0;HEAP32[r5>>2]=0;return}else{r6=r2+4&-4;r7=__Znwj(r6<<2);HEAP32[r1+8>>2]=r7;HEAP32[r1>>2]=r6|1;HEAP32[r1+4>>2]=r2;r3=r7;r4=_wmemcpy(r3,5243792,r2);r5=(r2<<2)+r3|0;HEAP32[r5>>2]=0;return}}function __ZNKSt3__18numpunctIwE12do_falsenameEv(r1,r2){var r3,r4,r5,r6,r7;r2=_wcslen(5243760);if(r2>>>0>1073741822){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if(r2>>>0<2){HEAP8[r1]=r2<<1&255;r3=r1+4|0;r4=_wmemcpy(r3,5243760,r2);r5=(r2<<2)+r3|0;HEAP32[r5>>2]=0;return}else{r6=r2+4&-4;r7=__Znwj(r6<<2);HEAP32[r1+8>>2]=r7;HEAP32[r1>>2]=r6|1;HEAP32[r1+4>>2]=r2;r3=r7;r4=_wmemcpy(r3,5243760,r2);r5=(r2<<2)+r3|0;HEAP32[r5>>2]=0;return}}function __ZNKSt3__110moneypunctIcLb0EE16do_decimal_pointEv(r1){return 127}function __ZNKSt3__110moneypunctIcLb0EE16do_thousands_sepEv(r1){return 127}function __ZNKSt3__110moneypunctIcLb0EE14do_frac_digitsEv(r1){return 0}function __ZNKSt3__110moneypunctIcLb1EE16do_decimal_pointEv(r1){return 127}function __ZNKSt3__110moneypunctIcLb1EE16do_thousands_sepEv(r1){return 127}function __ZNKSt3__110moneypunctIcLb1EE14do_frac_digitsEv(r1){return 0}function __ZNKSt3__110moneypunctIwLb0EE16do_decimal_pointEv(r1){return 2147483647}function __ZNKSt3__110moneypunctIwLb0EE16do_thousands_sepEv(r1){return 2147483647}function __ZNKSt3__110moneypunctIwLb0EE14do_frac_digitsEv(r1){return 0}function __ZNKSt3__110moneypunctIwLb1EE16do_decimal_pointEv(r1){return 2147483647}function __ZNKSt3__110moneypunctIwLb1EE16do_thousands_sepEv(r1){return 2147483647}function __ZNKSt3__110moneypunctIwLb1EE14do_frac_digitsEv(r1){return 0}function __ZNSt3__112__do_nothingEPv(r1){return}function __ZNKSt3__17collateIcE7do_hashEPKcS3_(r1,r2,r3){var r4,r5,r6,r7;if((r2|0)==(r3|0)){r4=0;return r4}else{r5=r2;r6=0}while(1){r2=(HEAP8[r5]<<24>>24)+(r6<<4)|0;r1=r2&-268435456;r7=(r1>>>24|r1)^r2;r2=r5+1|0;if((r2|0)==(r3|0)){r4=r7;break}else{r5=r2;r6=r7}}return r4}function __ZNKSt3__17collateIwE7do_hashEPKwS3_(r1,r2,r3){var r4,r5,r6,r7;if((r2|0)==(r3|0)){r4=0;return r4}else{r5=r2;r6=0}while(1){r2=(r6<<4)+HEAP32[r5>>2]|0;r1=r2&-268435456;r7=(r1>>>24|r1)^r2;r2=r5+4|0;if((r2|0)==(r3|0)){r4=r7;break}else{r5=r2;r6=r7}}return r4}function __ZNKSt3__120__time_get_c_storageIcE3__XEv(r1){var r2;if(HEAP8[5255604]<<24>>24!=0){return 5247444}if((___cxa_guard_acquire(5255604)|0)==0){return 5247444}HEAP8[5247444]=16;r1=5247445;r2=r1|0;tempBigInt=624576549;HEAP8[r2]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+3|0]=tempBigInt&255;r2=r1+4|0;tempBigInt=1394948685;HEAP8[r2]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+3|0]=tempBigInt&255;HEAP8[5247453]=0;_atexit(718,5247444,___dso_handle);return 5247444}function __ZNKSt3__120__time_get_c_storageIcE3__cEv(r1){if(HEAP8[5255596]<<24>>24!=0){return 5247432}if((___cxa_guard_acquire(5255596)|0)==0){return 5247432}r1=__Znwj(32);HEAP32[1311860]=r1;HEAP32[1311858]=33;HEAP32[1311859]=20;_memcpy(r1,5243664,20);HEAP8[r1+20|0]=0;_atexit(718,5247432,___dso_handle);return 5247432}function __ZNKSt3__120__time_get_c_storageIcE3__rEv(r1){if(HEAP8[5255588]<<24>>24!=0){return 5247420}if((___cxa_guard_acquire(5255588)|0)==0){return 5247420}r1=__Znwj(16);HEAP32[1311857]=r1;HEAP32[1311855]=17;HEAP32[1311856]=11;_memcpy(r1,5243552,11);HEAP8[r1+11|0]=0;_atexit(718,5247420,___dso_handle);return 5247420}function __ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE11do_get_timeES4_S4_RNS_8ios_baseERjP2tm(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10;r8=STACKTOP;STACKTOP=STACKTOP+8|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+4;HEAP32[r9>>2]=HEAP32[r3>>2];HEAP32[r10>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKcSC_(r1,r2,r9,r10,r5,r6,r7,5247312,5247320);STACKTOP=r8;return}function __ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE11do_get_dateES4_S4_RNS_8ios_baseERjP2tm(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15;r8=STACKTOP;STACKTOP=STACKTOP+8|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+4;r11=r2+8|0;r12=FUNCTION_TABLE[HEAP32[HEAP32[r11>>2]+20>>2]](r11);HEAP32[r9>>2]=HEAP32[r3>>2];HEAP32[r10>>2]=HEAP32[r4>>2];r4=r12;r3=HEAP8[r12];if((r3&1)<<24>>24==0){r13=r4+1|0;r14=r4+1|0}else{r4=HEAP32[r12+8>>2];r13=r4;r14=r4}r4=r3&255;if((r4&1|0)==0){r15=r4>>>1}else{r15=HEAP32[r12+4>>2]}__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKcSC_(r1,r2,r9,r10,r5,r6,r7,r14,r13+r15|0);STACKTOP=r8;return}function __ZNKSt3__120__time_get_c_storageIwE3__xEv(r1){var r2,r3,r4;if(HEAP8[5255524]<<24>>24!=0){return 5247348}if((___cxa_guard_acquire(5255524)|0)==0){return 5247348}r1=_wcslen(5243724);if(r1>>>0>1073741822){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if(r1>>>0<2){HEAP8[5247348]=r1<<1&255;r2=5247352}else{r3=r1+4&-4;r4=__Znwj(r3<<2);HEAP32[1311839]=r4;HEAP32[1311837]=r3|1;HEAP32[1311838]=r1;r2=r4}_wmemcpy(r2,5243724,r1);HEAP32[r2+(r1<<2)>>2]=0;_atexit(486,5247348,___dso_handle);return 5247348}function __ZNKSt3__120__time_get_c_storageIwE3__XEv(r1){var r2,r3,r4;if(HEAP8[5255548]<<24>>24!=0){return 5247384}if((___cxa_guard_acquire(5255548)|0)==0){return 5247384}r1=_wcslen(5243688);if(r1>>>0>1073741822){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if(r1>>>0<2){HEAP8[5247384]=r1<<1&255;r2=5247388}else{r3=r1+4&-4;r4=__Znwj(r3<<2);HEAP32[1311848]=r4;HEAP32[1311846]=r3|1;HEAP32[1311847]=r1;r2=r4}_wmemcpy(r2,5243688,r1);HEAP32[r2+(r1<<2)>>2]=0;_atexit(486,5247384,___dso_handle);return 5247384}function __ZNKSt3__120__time_get_c_storageIwE3__cEv(r1){var r2,r3,r4;if(HEAP8[5255540]<<24>>24!=0){return 5247372}if((___cxa_guard_acquire(5255540)|0)==0){return 5247372}r1=_wcslen(5243564);if(r1>>>0>1073741822){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if(r1>>>0<2){HEAP8[5247372]=r1<<1&255;r2=5247376}else{r3=r1+4&-4;r4=__Znwj(r3<<2);HEAP32[1311845]=r4;HEAP32[1311843]=r3|1;HEAP32[1311844]=r1;r2=r4}_wmemcpy(r2,5243564,r1);HEAP32[r2+(r1<<2)>>2]=0;_atexit(486,5247372,___dso_handle);return 5247372}function __ZNKSt3__120__time_get_c_storageIwE3__rEv(r1){var r2,r3,r4;if(HEAP8[5255532]<<24>>24!=0){return 5247360}if((___cxa_guard_acquire(5255532)|0)==0){return 5247360}r1=_wcslen(5243504);if(r1>>>0>1073741822){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if(r1>>>0<2){HEAP8[5247360]=r1<<1&255;r2=5247364}else{r3=r1+4&-4;r4=__Znwj(r3<<2);HEAP32[1311842]=r4;HEAP32[1311840]=r3|1;HEAP32[1311841]=r1;r2=r4}_wmemcpy(r2,5243504,r1);HEAP32[r2+(r1<<2)>>2]=0;_atexit(486,5247360,___dso_handle);return 5247360}function __ZNSt3__121__throw_runtime_errorEPKc(r1){var r2,r3,r4,r5,r6;r2=___cxa_allocate_exception(8);HEAP32[r2>>2]=5247512;r3=r2+4|0;if((r3|0)==0){___cxa_throw(r2,5252712,186)}r4=_strlen(r1);r5=__Znaj(r4+13|0),r6=r5>>2;HEAP32[r6+1]=r4;HEAP32[r6]=r4;r4=r5+12|0;HEAP32[r3>>2]=r4;HEAP32[r6+2]=0;_strcpy(r4,r1);___cxa_throw(r2,5252712,186)}function __ZNKSt3__110__time_put8__do_putEPwRS1_PK2tmcc(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11,r12,r13,r14;r7=STACKTOP;STACKTOP=STACKTOP+116|0;r8=r7;r9=r7+104;r10=r7+112;r11=r7+4|0;r12=r8|0;HEAP8[r12]=37;r13=r8+1|0;HEAP8[r13]=r5;r14=r8+2|0;HEAP8[r14]=r6;HEAP8[r8+3|0]=0;if(r6<<24>>24!=0){HEAP8[r13]=r6;HEAP8[r14]=r5}r5=r1|0;_strftime(r11,100,r12,r4,HEAP32[r5>>2]);r4=r9;HEAP32[r4>>2]=0;HEAP32[r4+4>>2]=0;HEAP32[r10>>2]=r11;r11=HEAP32[r3>>2]-r2>>2;r4=_uselocale(HEAP32[r5>>2]);r5=_mbsrtowcs(r2,r10,r11,r9);if((r4|0)!=0){_uselocale(r4)}if((r5|0)==-1){__ZNSt3__121__throw_runtime_errorEPKc(5243480)}else{HEAP32[r3>>2]=(r5<<2)+r2|0;STACKTOP=r7;return}}function __ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE14do_get_weekdayES4_S4_RNS_8ios_baseERjP2tm(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20;r8=STACKTOP;STACKTOP=STACKTOP+16|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+4,r11=r10>>2;r12=HEAP32[r5+28>>2],r5=r12>>2;r13=(r12+4|0)>>2;tempValue=HEAP32[r13],HEAP32[r13]=tempValue+1,tempValue;if((HEAP32[1313727]|0)!=-1){HEAP32[r11]=5254908;HEAP32[r11+1]=24;HEAP32[r11+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r10,406)}r10=HEAP32[1313728]-1|0;r11=HEAP32[r5+2];do{if(HEAP32[r5+3]-r11>>2>>>0>r10>>>0){r14=HEAP32[r11+(r10<<2)>>2];if((r14|0)==0){break}if(((tempValue=HEAP32[r13],HEAP32[r13]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r5]+8>>2]](r12)}r15=HEAP32[r4>>2];r16=r2+8|0;r17=FUNCTION_TABLE[HEAP32[HEAP32[r16>>2]>>2]](r16);HEAP32[r9>>2]=r15;r15=__ZNSt3__114__scan_keywordINS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEPKNS_12basic_stringIcS3_NS_9allocatorIcEEEENS_5ctypeIcEEEET0_RT_SE_SD_SD_RKT1_Rjb(r3,r9,r17,r17+168|0,r14,r6,0)-r17|0;if((r15|0)>=168){r18=r3|0;r19=HEAP32[r18>>2];r20=r1|0;HEAP32[r20>>2]=r19;STACKTOP=r8;return}HEAP32[r7+24>>2]=((r15|0)/12&-1|0)%7;r18=r3|0;r19=HEAP32[r18>>2];r20=r1|0;HEAP32[r20>>2]=r19;STACKTOP=r8;return}}while(0);r8=___cxa_allocate_exception(4);HEAP32[r8>>2]=5247488;___cxa_throw(r8,5252700,514)}function __ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE16do_get_monthnameES4_S4_RNS_8ios_baseERjP2tm(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20;r8=STACKTOP;STACKTOP=STACKTOP+16|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+4,r11=r10>>2;r12=HEAP32[r5+28>>2],r5=r12>>2;r13=(r12+4|0)>>2;tempValue=HEAP32[r13],HEAP32[r13]=tempValue+1,tempValue;if((HEAP32[1313727]|0)!=-1){HEAP32[r11]=5254908;HEAP32[r11+1]=24;HEAP32[r11+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r10,406)}r10=HEAP32[1313728]-1|0;r11=HEAP32[r5+2];do{if(HEAP32[r5+3]-r11>>2>>>0>r10>>>0){r14=HEAP32[r11+(r10<<2)>>2];if((r14|0)==0){break}if(((tempValue=HEAP32[r13],HEAP32[r13]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r5]+8>>2]](r12)}r15=HEAP32[r4>>2];r16=r2+8|0;r17=FUNCTION_TABLE[HEAP32[HEAP32[r16>>2]+4>>2]](r16);HEAP32[r9>>2]=r15;r15=__ZNSt3__114__scan_keywordINS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEPKNS_12basic_stringIcS3_NS_9allocatorIcEEEENS_5ctypeIcEEEET0_RT_SE_SD_SD_RKT1_Rjb(r3,r9,r17,r17+288|0,r14,r6,0)-r17|0;if((r15|0)>=288){r18=r3|0;r19=HEAP32[r18>>2];r20=r1|0;HEAP32[r20>>2]=r19;STACKTOP=r8;return}HEAP32[r7+16>>2]=((r15|0)/12&-1|0)%12;r18=r3|0;r19=HEAP32[r18>>2];r20=r1|0;HEAP32[r20>>2]=r19;STACKTOP=r8;return}}while(0);r8=___cxa_allocate_exception(4);HEAP32[r8>>2]=5247488;___cxa_throw(r8,5252700,514)}function __ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE11do_get_timeES4_S4_RNS_8ios_baseERjP2tm(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10;r8=STACKTOP;STACKTOP=STACKTOP+8|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+4;HEAP32[r9>>2]=HEAP32[r3>>2];HEAP32[r10>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKwSC_(r1,r2,r9,r10,r5,r6,r7,5247236,5247268);STACKTOP=r8;return}function __ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE11do_get_dateES4_S4_RNS_8ios_baseERjP2tm(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15;r8=STACKTOP;STACKTOP=STACKTOP+8|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+4;r11=r2+8|0;r12=FUNCTION_TABLE[HEAP32[HEAP32[r11>>2]+20>>2]](r11);HEAP32[r9>>2]=HEAP32[r3>>2];HEAP32[r10>>2]=HEAP32[r4>>2];r4=HEAP8[r12];if((r4&1)<<24>>24==0){r13=r12+4|0;r14=r12+4|0}else{r3=HEAP32[r12+8>>2];r13=r3;r14=r3}r3=r4&255;if((r3&1|0)==0){r15=r3>>>1}else{r15=HEAP32[r12+4>>2]}__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKwSC_(r1,r2,r9,r10,r5,r6,r7,r14,(r15<<2)+r13|0);STACKTOP=r8;return}function __ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE11do_get_yearES4_S4_RNS_8ios_baseERjP2tm(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18;r2=STACKTOP;STACKTOP=STACKTOP+16|0;r8=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r8>>2];r8=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r8>>2];r8=r2;r9=r2+4,r10=r9>>2;r11=HEAP32[r5+28>>2],r5=r11>>2;r12=(r11+4|0)>>2;tempValue=HEAP32[r12],HEAP32[r12]=tempValue+1,tempValue;if((HEAP32[1313727]|0)!=-1){HEAP32[r10]=5254908;HEAP32[r10+1]=24;HEAP32[r10+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r9,406)}r9=HEAP32[1313728]-1|0;r10=HEAP32[r5+2];do{if(HEAP32[r5+3]-r10>>2>>>0>r9>>>0){r13=HEAP32[r10+(r9<<2)>>2];if((r13|0)==0){break}if(((tempValue=HEAP32[r12],HEAP32[r12]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r5]+8>>2]](r11)}HEAP32[r8>>2]=HEAP32[r4>>2];r14=__ZNSt3__120__get_up_to_n_digitsIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r8,r6,r13,4);if((HEAP32[r6>>2]&4|0)!=0){r15=r3|0;r16=HEAP32[r15>>2];r17=r1|0;HEAP32[r17>>2]=r16;STACKTOP=r2;return}if((r14|0)<69){r18=r14+2e3|0}else{r18=(r14-69|0)>>>0<31?r14+1900|0:r14}HEAP32[r7+20>>2]=r18-1900|0;r15=r3|0;r16=HEAP32[r15>>2];r17=r1|0;HEAP32[r17>>2]=r16;STACKTOP=r2;return}}while(0);r2=___cxa_allocate_exception(4);HEAP32[r2>>2]=5247488;___cxa_throw(r2,5252700,514)}function __ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjP2tmcc(r1,r2,r3,r4,r5,r6,r7,r8,r9){var r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48,r49,r50,r51,r52,r53,r54,r55,r56,r57,r58,r59,r60,r61,r62,r63,r64,r65,r66,r67,r68;r9=r7>>2;r10=r6>>2;r11=STACKTOP;STACKTOP=STACKTOP+164|0;r12=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r12>>2];r12=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r12>>2];r12=r11;r13=r11+4;r14=r11+8;r15=r11+12;r16=r11+16;r17=r11+20;r18=r11+24;r19=r11+28;r20=r11+32;r21=r11+36;r22=r11+40;r23=r11+44;r24=r11+48,r25=r24>>2;r26=r11+60;r27=r11+64;r28=r11+68;r29=r11+72;r30=r11+76;r31=r11+80;r32=r11+84;r33=r11+88;r34=r11+92;r35=r11+96;r36=r11+100;r37=r11+104;r38=r11+108;r39=r11+112;r40=r11+116;r41=r11+120;r42=r11+124;r43=r11+128;r44=r11+132;r45=r11+136;r46=r11+140;r47=r11+144;r48=r11+148;r49=r11+152;r50=r11+156;r51=r11+160;HEAP32[r10]=0;r52=HEAP32[r5+28>>2],r53=r52>>2;r54=(r52+4|0)>>2;tempValue=HEAP32[r54],HEAP32[r54]=tempValue+1,tempValue;if((HEAP32[1313727]|0)!=-1){HEAP32[r25]=5254908;HEAP32[r25+1]=24;HEAP32[r25+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r24,406)}r24=HEAP32[1313728]-1|0;r25=HEAP32[r53+2];do{if(HEAP32[r53+3]-r25>>2>>>0>r24>>>0){r55=HEAP32[r25+(r24<<2)>>2];if((r55|0)==0){break}r56=r55;if(((tempValue=HEAP32[r54],HEAP32[r54]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r53]+8>>2]](r52)}r55=r8<<24>>24;L2629:do{if((r55|0)==97|(r55|0)==65){r57=HEAP32[r4>>2];r58=r2+8|0;r59=FUNCTION_TABLE[HEAP32[HEAP32[r58>>2]>>2]](r58);HEAP32[r23>>2]=r57;r57=__ZNSt3__114__scan_keywordINS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEPKNS_12basic_stringIcS3_NS_9allocatorIcEEEENS_5ctypeIcEEEET0_RT_SE_SD_SD_RKT1_Rjb(r3,r23,r59,r59+168|0,r56,r6,0)-r59|0;if((r57|0)>=168){break}HEAP32[r9+6]=((r57|0)/12&-1|0)%7}else if((r55|0)==77){HEAP32[r16>>2]=HEAP32[r4>>2];r57=__ZNSt3__120__get_up_to_n_digitsIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r16,r6,r56,2);r59=HEAP32[r10];if((r59&4|0)==0&(r57|0)<60){HEAP32[r9+1]=r57;break}else{HEAP32[r10]=r59|4;break}}else if((r55|0)==82){r59=r3|0;HEAP32[r41>>2]=HEAP32[r59>>2];HEAP32[r42>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKcSC_(r40,r2,r41,r42,r5,r6,r7,5247276,5247281);HEAP32[r59>>2]=HEAP32[r40>>2]}else if((r55|0)==83){HEAP32[r15>>2]=HEAP32[r4>>2];r59=__ZNSt3__120__get_up_to_n_digitsIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r15,r6,r56,2);r57=HEAP32[r10];if((r57&4|0)==0&(r59|0)<61){HEAP32[r9]=r59;break}else{HEAP32[r10]=r57|4;break}}else if((r55|0)==84){r57=r3|0;HEAP32[r44>>2]=HEAP32[r57>>2];HEAP32[r45>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKcSC_(r43,r2,r44,r45,r5,r6,r7,5247268,5247276);HEAP32[r57>>2]=HEAP32[r43>>2]}else if((r55|0)==119){HEAP32[r14>>2]=HEAP32[r4>>2];r57=__ZNSt3__120__get_up_to_n_digitsIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r14,r6,r56,1);r59=HEAP32[r10];if((r59&4|0)==0&(r57|0)<7){HEAP32[r9+6]=r57;break}else{HEAP32[r10]=r59|4;break}}else if((r55|0)==37){HEAP32[r51>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE13__get_percentERS4_S4_RjRKNS_5ctypeIcEE(0,r3,r51,r6,r56)}else if((r55|0)==89){HEAP32[r12>>2]=HEAP32[r4>>2];r59=__ZNSt3__120__get_up_to_n_digitsIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r12,r6,r56,4);if((HEAP32[r10]&4|0)!=0){break}HEAP32[r9+5]=r59-1900|0}else if((r55|0)==114){r59=r3|0;HEAP32[r38>>2]=HEAP32[r59>>2];HEAP32[r39>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKcSC_(r37,r2,r38,r39,r5,r6,r7,5247284,5247295);HEAP32[r59>>2]=HEAP32[r37>>2]}else if((r55|0)==120){r59=HEAP32[HEAP32[r2>>2]+20>>2];HEAP32[r46>>2]=HEAP32[r3>>2];HEAP32[r47>>2]=HEAP32[r4>>2];FUNCTION_TABLE[r59](r1,r2,r46,r47,r5,r6,r7);STACKTOP=r11;return}else if((r55|0)==88){r59=r2+8|0;r57=FUNCTION_TABLE[HEAP32[HEAP32[r59>>2]+24>>2]](r59);r59=r3|0;HEAP32[r49>>2]=HEAP32[r59>>2];HEAP32[r50>>2]=HEAP32[r4>>2];r58=r57;r60=HEAP8[r57];if((r60&1)<<24>>24==0){r61=r58+1|0;r62=r58+1|0}else{r58=HEAP32[r57+8>>2];r61=r58;r62=r58}r58=r60&255;if((r58&1|0)==0){r63=r58>>>1}else{r63=HEAP32[r57+4>>2]}__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKcSC_(r48,r2,r49,r50,r5,r6,r7,r62,r61+r63|0);HEAP32[r59>>2]=HEAP32[r48>>2]}else if((r55|0)==72){HEAP32[r20>>2]=HEAP32[r4>>2];r59=__ZNSt3__120__get_up_to_n_digitsIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r20,r6,r56,2);r57=HEAP32[r10];if((r57&4|0)==0&(r59|0)<24){HEAP32[r9+2]=r59;break}else{HEAP32[r10]=r57|4;break}}else if((r55|0)==100|(r55|0)==101){r57=r7+12|0;HEAP32[r21>>2]=HEAP32[r4>>2];r59=__ZNSt3__120__get_up_to_n_digitsIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r21,r6,r56,2);r58=HEAP32[r10];do{if((r58&4|0)==0){if((r59-1|0)>>>0>=31){break}HEAP32[r57>>2]=r59;break L2629}}while(0);HEAP32[r10]=r58|4}else if((r55|0)==98|(r55|0)==66|(r55|0)==104){r59=HEAP32[r4>>2];r57=r2+8|0;r60=FUNCTION_TABLE[HEAP32[HEAP32[r57>>2]+4>>2]](r57);HEAP32[r22>>2]=r59;r59=__ZNSt3__114__scan_keywordINS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEPKNS_12basic_stringIcS3_NS_9allocatorIcEEEENS_5ctypeIcEEEET0_RT_SE_SD_SD_RKT1_Rjb(r3,r22,r60,r60+288|0,r56,r6,0)-r60|0;if((r59|0)>=288){break}HEAP32[r9+4]=((r59|0)/12&-1|0)%12}else if((r55|0)==99){r59=r2+8|0;r60=FUNCTION_TABLE[HEAP32[HEAP32[r59>>2]+12>>2]](r59);r59=r3|0;HEAP32[r27>>2]=HEAP32[r59>>2];HEAP32[r28>>2]=HEAP32[r4>>2];r57=r60;r64=HEAP8[r60];if((r64&1)<<24>>24==0){r65=r57+1|0;r66=r57+1|0}else{r57=HEAP32[r60+8>>2];r65=r57;r66=r57}r57=r64&255;if((r57&1|0)==0){r67=r57>>>1}else{r67=HEAP32[r60+4>>2]}__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKcSC_(r26,r2,r27,r28,r5,r6,r7,r66,r65+r67|0);HEAP32[r59>>2]=HEAP32[r26>>2]}else if((r55|0)==109){HEAP32[r17>>2]=HEAP32[r4>>2];r59=__ZNSt3__120__get_up_to_n_digitsIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r17,r6,r56,2)-1|0;r60=HEAP32[r10];if((r60&4|0)==0&(r59|0)<12){HEAP32[r9+4]=r59;break}else{HEAP32[r10]=r60|4;break}}else if((r55|0)==68){r60=r3|0;HEAP32[r30>>2]=HEAP32[r60>>2];HEAP32[r31>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKcSC_(r29,r2,r30,r31,r5,r6,r7,5247304,5247312);HEAP32[r60>>2]=HEAP32[r29>>2]}else if((r55|0)==106){HEAP32[r18>>2]=HEAP32[r4>>2];r60=__ZNSt3__120__get_up_to_n_digitsIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r18,r6,r56,3);r59=HEAP32[r10];if((r59&4|0)==0&(r60|0)<366){HEAP32[r9+7]=r60;break}else{HEAP32[r10]=r59|4;break}}else if((r55|0)==70){r59=r3|0;HEAP32[r33>>2]=HEAP32[r59>>2];HEAP32[r34>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKcSC_(r32,r2,r33,r34,r5,r6,r7,5247296,5247304);HEAP32[r59>>2]=HEAP32[r32>>2]}else if((r55|0)==121){HEAP32[r13>>2]=HEAP32[r4>>2];r59=__ZNSt3__120__get_up_to_n_digitsIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r13,r6,r56,4);if((HEAP32[r10]&4|0)!=0){break}if((r59|0)<69){r68=r59+2e3|0}else{r68=(r59-69|0)>>>0<31?r59+1900|0:r59}HEAP32[r9+5]=r68-1900|0}else if((r55|0)==73){r59=r7+8|0;HEAP32[r19>>2]=HEAP32[r4>>2];r60=__ZNSt3__120__get_up_to_n_digitsIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r19,r6,r56,2);r57=HEAP32[r10];do{if((r57&4|0)==0){if((r60-1|0)>>>0>=12){break}HEAP32[r59>>2]=r60;break L2629}}while(0);HEAP32[r10]=r57|4}else if((r55|0)==110|(r55|0)==116){HEAP32[r35>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE17__get_white_spaceERS4_S4_RjRKNS_5ctypeIcEE(0,r3,r35,r6,r56)}else if((r55|0)==112){HEAP32[r36>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE11__get_am_pmERiRS4_S4_RjRKNS_5ctypeIcEE(r2,r7+8|0,r3,r36,r6,r56)}else{HEAP32[r10]=HEAP32[r10]|4}}while(0);HEAP32[r1>>2]=HEAP32[r3>>2];STACKTOP=r11;return}}while(0);r11=___cxa_allocate_exception(4);HEAP32[r11>>2]=5247488;___cxa_throw(r11,5252700,514)}function __ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE14do_get_weekdayES4_S4_RNS_8ios_baseERjP2tm(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20;r8=STACKTOP;STACKTOP=STACKTOP+16|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+4,r11=r10>>2;r12=HEAP32[r5+28>>2],r5=r12>>2;r13=(r12+4|0)>>2;tempValue=HEAP32[r13],HEAP32[r13]=tempValue+1,tempValue;if((HEAP32[1313725]|0)!=-1){HEAP32[r11]=5254900;HEAP32[r11+1]=24;HEAP32[r11+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r10,406)}r10=HEAP32[1313726]-1|0;r11=HEAP32[r5+2];do{if(HEAP32[r5+3]-r11>>2>>>0>r10>>>0){r14=HEAP32[r11+(r10<<2)>>2];if((r14|0)==0){break}if(((tempValue=HEAP32[r13],HEAP32[r13]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r5]+8>>2]](r12)}r15=HEAP32[r4>>2];r16=r2+8|0;r17=FUNCTION_TABLE[HEAP32[HEAP32[r16>>2]>>2]](r16);HEAP32[r9>>2]=r15;r15=__ZNSt3__114__scan_keywordINS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEPKNS_12basic_stringIwS3_NS_9allocatorIwEEEENS_5ctypeIwEEEET0_RT_SE_SD_SD_RKT1_Rjb(r3,r9,r17,r17+168|0,r14,r6,0)-r17|0;if((r15|0)>=168){r18=r3|0;r19=HEAP32[r18>>2];r20=r1|0;HEAP32[r20>>2]=r19;STACKTOP=r8;return}HEAP32[r7+24>>2]=((r15|0)/12&-1|0)%7;r18=r3|0;r19=HEAP32[r18>>2];r20=r1|0;HEAP32[r20>>2]=r19;STACKTOP=r8;return}}while(0);r8=___cxa_allocate_exception(4);HEAP32[r8>>2]=5247488;___cxa_throw(r8,5252700,514)}function __ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE16do_get_monthnameES4_S4_RNS_8ios_baseERjP2tm(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20;r8=STACKTOP;STACKTOP=STACKTOP+16|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+4,r11=r10>>2;r12=HEAP32[r5+28>>2],r5=r12>>2;r13=(r12+4|0)>>2;tempValue=HEAP32[r13],HEAP32[r13]=tempValue+1,tempValue;if((HEAP32[1313725]|0)!=-1){HEAP32[r11]=5254900;HEAP32[r11+1]=24;HEAP32[r11+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r10,406)}r10=HEAP32[1313726]-1|0;r11=HEAP32[r5+2];do{if(HEAP32[r5+3]-r11>>2>>>0>r10>>>0){r14=HEAP32[r11+(r10<<2)>>2];if((r14|0)==0){break}if(((tempValue=HEAP32[r13],HEAP32[r13]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r5]+8>>2]](r12)}r15=HEAP32[r4>>2];r16=r2+8|0;r17=FUNCTION_TABLE[HEAP32[HEAP32[r16>>2]+4>>2]](r16);HEAP32[r9>>2]=r15;r15=__ZNSt3__114__scan_keywordINS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEPKNS_12basic_stringIwS3_NS_9allocatorIwEEEENS_5ctypeIwEEEET0_RT_SE_SD_SD_RKT1_Rjb(r3,r9,r17,r17+288|0,r14,r6,0)-r17|0;if((r15|0)>=288){r18=r3|0;r19=HEAP32[r18>>2];r20=r1|0;HEAP32[r20>>2]=r19;STACKTOP=r8;return}HEAP32[r7+16>>2]=((r15|0)/12&-1|0)%12;r18=r3|0;r19=HEAP32[r18>>2];r20=r1|0;HEAP32[r20>>2]=r19;STACKTOP=r8;return}}while(0);r8=___cxa_allocate_exception(4);HEAP32[r8>>2]=5247488;___cxa_throw(r8,5252700,514)}function __ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE11do_get_yearES4_S4_RNS_8ios_baseERjP2tm(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18;r2=STACKTOP;STACKTOP=STACKTOP+16|0;r8=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r8>>2];r8=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r8>>2];r8=r2;r9=r2+4,r10=r9>>2;r11=HEAP32[r5+28>>2],r5=r11>>2;r12=(r11+4|0)>>2;tempValue=HEAP32[r12],HEAP32[r12]=tempValue+1,tempValue;if((HEAP32[1313725]|0)!=-1){HEAP32[r10]=5254900;HEAP32[r10+1]=24;HEAP32[r10+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r9,406)}r9=HEAP32[1313726]-1|0;r10=HEAP32[r5+2];do{if(HEAP32[r5+3]-r10>>2>>>0>r9>>>0){r13=HEAP32[r10+(r9<<2)>>2];if((r13|0)==0){break}if(((tempValue=HEAP32[r12],HEAP32[r12]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r5]+8>>2]](r11)}HEAP32[r8>>2]=HEAP32[r4>>2];r14=__ZNSt3__120__get_up_to_n_digitsIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r8,r6,r13,4);if((HEAP32[r6>>2]&4|0)!=0){r15=r3|0;r16=HEAP32[r15>>2];r17=r1|0;HEAP32[r17>>2]=r16;STACKTOP=r2;return}if((r14|0)<69){r18=r14+2e3|0}else{r18=(r14-69|0)>>>0<31?r14+1900|0:r14}HEAP32[r7+20>>2]=r18-1900|0;r15=r3|0;r16=HEAP32[r15>>2];r17=r1|0;HEAP32[r17>>2]=r16;STACKTOP=r2;return}}while(0);r2=___cxa_allocate_exception(4);HEAP32[r2>>2]=5247488;___cxa_throw(r2,5252700,514)}function __ZNKSt3__18time_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEcPK2tmcc(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19;r5=STACKTOP;STACKTOP=STACKTOP+104|0;r4=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r4>>2];r4=r5;r9=r5+4;r10=r9|0;r11=r4|0;HEAP8[r11]=37;r12=r4+1|0;HEAP8[r12]=r7;r13=r4+2|0;HEAP8[r13]=r8;HEAP8[r4+3|0]=0;if(r8<<24>>24!=0){HEAP8[r12]=r8;HEAP8[r13]=r7}r7=_strftime(r10,100,r11,r6,HEAP32[r2+8>>2]);r2=r9+r7|0;r9=HEAP32[r3>>2];if((r7|0)==0){r14=r9;r15=r1|0;HEAP32[r15>>2]=r14;STACKTOP=r5;return}else{r16=r9;r17=r10}while(1){r10=HEAP8[r17];if((r16|0)==0){r18=0}else{r9=r16+24|0;r7=HEAP32[r9>>2];if((r7|0)==(HEAP32[r16+28>>2]|0)){r19=FUNCTION_TABLE[HEAP32[HEAP32[r16>>2]+52>>2]](r16,r10&255)}else{HEAP32[r9>>2]=r7+1|0;HEAP8[r7]=r10;r19=r10&255}r18=(r19|0)==-1?0:r16}r10=r17+1|0;if((r10|0)==(r2|0)){r14=r18;break}else{r16=r18;r17=r10}}r15=r1|0;HEAP32[r15>>2]=r14;STACKTOP=r5;return}function __ZNKSt3__18time_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwPK2tmcc(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10,r11,r12,r13,r14,r15,r16;r5=STACKTOP;STACKTOP=STACKTOP+404|0;r4=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r4>>2];r4=r5;r9=r5+400;r10=r4|0;HEAP32[r9>>2]=r4+400|0;__ZNKSt3__110__time_put8__do_putEPwRS1_PK2tmcc(r2+8|0,r10,r9,r6,r7,r8);r8=HEAP32[r9>>2];r9=HEAP32[r3>>2];if((r10|0)==(r8|0)){r11=r9;r12=r1|0;HEAP32[r12>>2]=r11;STACKTOP=r5;return}else{r13=r9;r14=r10}while(1){r10=HEAP32[r14>>2];if((r13|0)==0){r15=0}else{r9=r13+24|0;r3=HEAP32[r9>>2];if((r3|0)==(HEAP32[r13+28>>2]|0)){r16=FUNCTION_TABLE[HEAP32[HEAP32[r13>>2]+52>>2]](r13,r10)}else{HEAP32[r9>>2]=r3+4|0;HEAP32[r3>>2]=r10;r16=r10}r15=(r16|0)==-1?0:r13}r10=r14+4|0;if((r10|0)==(r8|0)){r11=r15;break}else{r13=r15;r14=r10}}r12=r1|0;HEAP32[r12>>2]=r11;STACKTOP=r5;return}function __ZNKSt3__18messagesIcE7do_openERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEERKNS_6localeE(r1,r2,r3){var r4;if((HEAP8[r2]&1)<<24>>24==0){r4=r2+1|0}else{r4=HEAP32[r2+8>>2]}r2=__Z7catopenPKci(r4,200);return r2>>>(((r2|0)!=-1&1)>>>0)}function __ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjP2tmcc(r1,r2,r3,r4,r5,r6,r7,r8,r9){var r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48,r49,r50,r51,r52,r53,r54,r55,r56,r57,r58,r59,r60,r61,r62,r63,r64,r65,r66,r67,r68;r9=r7>>2;r10=r6>>2;r11=STACKTOP;STACKTOP=STACKTOP+164|0;r12=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r12>>2];r12=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r12>>2];r12=r11;r13=r11+4;r14=r11+8;r15=r11+12;r16=r11+16;r17=r11+20;r18=r11+24;r19=r11+28;r20=r11+32;r21=r11+36;r22=r11+40;r23=r11+44;r24=r11+48,r25=r24>>2;r26=r11+60;r27=r11+64;r28=r11+68;r29=r11+72;r30=r11+76;r31=r11+80;r32=r11+84;r33=r11+88;r34=r11+92;r35=r11+96;r36=r11+100;r37=r11+104;r38=r11+108;r39=r11+112;r40=r11+116;r41=r11+120;r42=r11+124;r43=r11+128;r44=r11+132;r45=r11+136;r46=r11+140;r47=r11+144;r48=r11+148;r49=r11+152;r50=r11+156;r51=r11+160;HEAP32[r10]=0;r52=HEAP32[r5+28>>2],r53=r52>>2;r54=(r52+4|0)>>2;tempValue=HEAP32[r54],HEAP32[r54]=tempValue+1,tempValue;if((HEAP32[1313725]|0)!=-1){HEAP32[r25]=5254900;HEAP32[r25+1]=24;HEAP32[r25+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r24,406)}r24=HEAP32[1313726]-1|0;r25=HEAP32[r53+2];do{if(HEAP32[r53+3]-r25>>2>>>0>r24>>>0){r55=HEAP32[r25+(r24<<2)>>2];if((r55|0)==0){break}r56=r55;if(((tempValue=HEAP32[r54],HEAP32[r54]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r53]+8>>2]](r52)}r55=r8<<24>>24;L2805:do{if((r55|0)==97|(r55|0)==65){r57=HEAP32[r4>>2];r58=r2+8|0;r59=FUNCTION_TABLE[HEAP32[HEAP32[r58>>2]>>2]](r58);HEAP32[r23>>2]=r57;r57=__ZNSt3__114__scan_keywordINS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEPKNS_12basic_stringIwS3_NS_9allocatorIwEEEENS_5ctypeIwEEEET0_RT_SE_SD_SD_RKT1_Rjb(r3,r23,r59,r59+168|0,r56,r6,0)-r59|0;if((r57|0)>=168){break}HEAP32[r9+6]=((r57|0)/12&-1|0)%7}else if((r55|0)==98|(r55|0)==66|(r55|0)==104){r57=HEAP32[r4>>2];r59=r2+8|0;r58=FUNCTION_TABLE[HEAP32[HEAP32[r59>>2]+4>>2]](r59);HEAP32[r22>>2]=r57;r57=__ZNSt3__114__scan_keywordINS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEPKNS_12basic_stringIwS3_NS_9allocatorIwEEEENS_5ctypeIwEEEET0_RT_SE_SD_SD_RKT1_Rjb(r3,r22,r58,r58+288|0,r56,r6,0)-r58|0;if((r57|0)>=288){break}HEAP32[r9+4]=((r57|0)/12&-1|0)%12}else if((r55|0)==99){r57=r2+8|0;r58=FUNCTION_TABLE[HEAP32[HEAP32[r57>>2]+12>>2]](r57);r57=r3|0;HEAP32[r27>>2]=HEAP32[r57>>2];HEAP32[r28>>2]=HEAP32[r4>>2];r59=HEAP8[r58];if((r59&1)<<24>>24==0){r60=r58+4|0;r61=r58+4|0}else{r62=HEAP32[r58+8>>2];r60=r62;r61=r62}r62=r59&255;if((r62&1|0)==0){r63=r62>>>1}else{r63=HEAP32[r58+4>>2]}__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKwSC_(r26,r2,r27,r28,r5,r6,r7,r61,(r63<<2)+r60|0);HEAP32[r57>>2]=HEAP32[r26>>2]}else if((r55|0)==100|(r55|0)==101){r57=r7+12|0;HEAP32[r21>>2]=HEAP32[r4>>2];r58=__ZNSt3__120__get_up_to_n_digitsIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r21,r6,r56,2);r62=HEAP32[r10];do{if((r62&4|0)==0){if((r58-1|0)>>>0>=31){break}HEAP32[r57>>2]=r58;break L2805}}while(0);HEAP32[r10]=r62|4}else if((r55|0)==68){r58=r3|0;HEAP32[r30>>2]=HEAP32[r58>>2];HEAP32[r31>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKwSC_(r29,r2,r30,r31,r5,r6,r7,5247204,5247236);HEAP32[r58>>2]=HEAP32[r29>>2]}else if((r55|0)==70){r58=r3|0;HEAP32[r33>>2]=HEAP32[r58>>2];HEAP32[r34>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKwSC_(r32,r2,r33,r34,r5,r6,r7,5247172,5247204);HEAP32[r58>>2]=HEAP32[r32>>2]}else if((r55|0)==72){HEAP32[r20>>2]=HEAP32[r4>>2];r58=__ZNSt3__120__get_up_to_n_digitsIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r20,r6,r56,2);r57=HEAP32[r10];if((r57&4|0)==0&(r58|0)<24){HEAP32[r9+2]=r58;break}else{HEAP32[r10]=r57|4;break}}else if((r55|0)==73){r57=r7+8|0;HEAP32[r19>>2]=HEAP32[r4>>2];r58=__ZNSt3__120__get_up_to_n_digitsIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r19,r6,r56,2);r59=HEAP32[r10];do{if((r59&4|0)==0){if((r58-1|0)>>>0>=12){break}HEAP32[r57>>2]=r58;break L2805}}while(0);HEAP32[r10]=r59|4}else if((r55|0)==106){HEAP32[r18>>2]=HEAP32[r4>>2];r58=__ZNSt3__120__get_up_to_n_digitsIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r18,r6,r56,3);r57=HEAP32[r10];if((r57&4|0)==0&(r58|0)<366){HEAP32[r9+7]=r58;break}else{HEAP32[r10]=r57|4;break}}else if((r55|0)==109){HEAP32[r17>>2]=HEAP32[r4>>2];r57=__ZNSt3__120__get_up_to_n_digitsIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r17,r6,r56,2)-1|0;r58=HEAP32[r10];if((r58&4|0)==0&(r57|0)<12){HEAP32[r9+4]=r57;break}else{HEAP32[r10]=r58|4;break}}else if((r55|0)==77){HEAP32[r16>>2]=HEAP32[r4>>2];r58=__ZNSt3__120__get_up_to_n_digitsIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r16,r6,r56,2);r57=HEAP32[r10];if((r57&4|0)==0&(r58|0)<60){HEAP32[r9+1]=r58;break}else{HEAP32[r10]=r57|4;break}}else if((r55|0)==110|(r55|0)==116){HEAP32[r35>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE17__get_white_spaceERS4_S4_RjRKNS_5ctypeIwEE(0,r3,r35,r6,r56)}else if((r55|0)==112){HEAP32[r36>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE11__get_am_pmERiRS4_S4_RjRKNS_5ctypeIwEE(r2,r7+8|0,r3,r36,r6,r56)}else if((r55|0)==114){r57=r3|0;HEAP32[r38>>2]=HEAP32[r57>>2];HEAP32[r39>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKwSC_(r37,r2,r38,r39,r5,r6,r7,5247128,5247172);HEAP32[r57>>2]=HEAP32[r37>>2]}else if((r55|0)==82){r57=r3|0;HEAP32[r41>>2]=HEAP32[r57>>2];HEAP32[r42>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKwSC_(r40,r2,r41,r42,r5,r6,r7,5247108,5247128);HEAP32[r57>>2]=HEAP32[r40>>2]}else if((r55|0)==83){HEAP32[r15>>2]=HEAP32[r4>>2];r57=__ZNSt3__120__get_up_to_n_digitsIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r15,r6,r56,2);r58=HEAP32[r10];if((r58&4|0)==0&(r57|0)<61){HEAP32[r9]=r57;break}else{HEAP32[r10]=r58|4;break}}else if((r55|0)==84){r58=r3|0;HEAP32[r44>>2]=HEAP32[r58>>2];HEAP32[r45>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKwSC_(r43,r2,r44,r45,r5,r6,r7,5247076,5247108);HEAP32[r58>>2]=HEAP32[r43>>2]}else if((r55|0)==119){HEAP32[r14>>2]=HEAP32[r4>>2];r58=__ZNSt3__120__get_up_to_n_digitsIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r14,r6,r56,1);r57=HEAP32[r10];if((r57&4|0)==0&(r58|0)<7){HEAP32[r9+6]=r58;break}else{HEAP32[r10]=r57|4;break}}else if((r55|0)==120){r57=HEAP32[HEAP32[r2>>2]+20>>2];HEAP32[r46>>2]=HEAP32[r3>>2];HEAP32[r47>>2]=HEAP32[r4>>2];FUNCTION_TABLE[r57](r1,r2,r46,r47,r5,r6,r7);STACKTOP=r11;return}else if((r55|0)==88){r57=r2+8|0;r58=FUNCTION_TABLE[HEAP32[HEAP32[r57>>2]+24>>2]](r57);r57=r3|0;HEAP32[r49>>2]=HEAP32[r57>>2];HEAP32[r50>>2]=HEAP32[r4>>2];r62=HEAP8[r58];if((r62&1)<<24>>24==0){r64=r58+4|0;r65=r58+4|0}else{r66=HEAP32[r58+8>>2];r64=r66;r65=r66}r66=r62&255;if((r66&1|0)==0){r67=r66>>>1}else{r67=HEAP32[r58+4>>2]}__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKwSC_(r48,r2,r49,r50,r5,r6,r7,r65,(r67<<2)+r64|0);HEAP32[r57>>2]=HEAP32[r48>>2]}else if((r55|0)==121){HEAP32[r13>>2]=HEAP32[r4>>2];r57=__ZNSt3__120__get_up_to_n_digitsIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r13,r6,r56,4);if((HEAP32[r10]&4|0)!=0){break}if((r57|0)<69){r68=r57+2e3|0}else{r68=(r57-69|0)>>>0<31?r57+1900|0:r57}HEAP32[r9+5]=r68-1900|0}else if((r55|0)==89){HEAP32[r12>>2]=HEAP32[r4>>2];r57=__ZNSt3__120__get_up_to_n_digitsIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r3,r12,r6,r56,4);if((HEAP32[r10]&4|0)!=0){break}HEAP32[r9+5]=r57-1900|0}else if((r55|0)==37){HEAP32[r51>>2]=HEAP32[r4>>2];__ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE13__get_percentERS4_S4_RjRKNS_5ctypeIwEE(0,r3,r51,r6,r56)}else{HEAP32[r10]=HEAP32[r10]|4}}while(0);HEAP32[r1>>2]=HEAP32[r3>>2];STACKTOP=r11;return}}while(0);r11=___cxa_allocate_exception(4);HEAP32[r11>>2]=5247488;___cxa_throw(r11,5252700,514)}function __ZNSt3__17collateIcED1Ev(r1){return}function __ZNKSt3__17collateIcE10do_compareEPKcS3_S3_S3_(r1,r2,r3,r4,r5){var r6,r7,r8,r9,r10,r11;r1=0;L2887:do{if((r4|0)==(r5|0)){r6=r2}else{r7=r2;r8=r4;while(1){if((r7|0)==(r3|0)){r9=-1;r1=2678;break}r10=HEAP8[r7];r11=HEAP8[r8];if(r10<<24>>24<r11<<24>>24){r9=-1;r1=2677;break}if(r11<<24>>24<r10<<24>>24){r9=1;r1=2676;break}r10=r7+1|0;r11=r8+1|0;if((r11|0)==(r5|0)){r6=r10;break L2887}else{r7=r10;r8=r11}}if(r1==2676){return r9}else if(r1==2677){return r9}else if(r1==2678){return r9}}}while(0);r9=(r6|0)!=(r3|0)&1;return r9}function __ZNKSt3__18messagesIcE8do_closeEi(r1,r2){__Z8catcloseP8_nl_catd((r2|0)==-1?-1:r2<<1);return}function __ZNKSt3__18messagesIwE7do_openERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEERKNS_6localeE(r1,r2,r3){var r4;if((HEAP8[r2]&1)<<24>>24==0){r4=r2+1|0}else{r4=HEAP32[r2+8>>2]}r2=__Z7catopenPKci(r4,200);return r2>>>(((r2|0)!=-1&1)>>>0)}function __ZNKSt3__18messagesIwE8do_closeEi(r1,r2){__Z8catcloseP8_nl_catd((r2|0)==-1?-1:r2<<1);return}function __ZNSt3__17collateIcED0Ev(r1){__ZdlPv(r1);return}function __ZNKSt3__18messagesIcE6do_getEiiiRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEE(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38;r2=0;r7=STACKTOP;STACKTOP=STACKTOP+12|0;r8=r7;r9=r8,r10=r9>>2;HEAP32[r10]=0;HEAP32[r10+1]=0;HEAP32[r10+2]=0;r10=r1,r11=r10>>2;r12=r6;r13=HEAP8[r6];if((r13&1)<<24>>24==0){r14=r12+1|0;r15=r12+1|0}else{r12=HEAP32[r6+8>>2];r14=r12;r15=r12}r12=r13&255;if((r12&1|0)==0){r16=r12>>>1}else{r16=HEAP32[r6+4>>2]}r6=r14+r16|0;do{if(r15>>>0<r6>>>0){r16=r8+1|0;r14=(r8+8|0)>>2;r12=r8|0;r13=r8+4|0;r17=r15;r18=0;while(1){r19=HEAP8[r17];if((r18&1)<<24>>24==0){r20=10;r21=r18}else{r22=HEAP32[r12>>2];r20=(r22&-2)-1|0;r21=r22&255}r22=r21&255;r23=(r22&1|0)==0?r22>>>1:HEAP32[r13>>2];if((r23|0)==(r20|0)){if((r20|0)==-3){r2=2699;break}r22=(r21&1)<<24>>24==0?r16:HEAP32[r14];do{if(r20>>>0<2147483631){r24=r20+1|0;r25=r20<<1;r26=r24>>>0<r25>>>0?r25:r24;if(r26>>>0<11){r27=11;break}r27=r26+16&-16}else{r27=-2}}while(0);r26=__Znwj(r27);_memcpy(r26,r22,r20);if((r20|0)!=10){__ZdlPv(r22)}HEAP32[r14]=r26;r26=r27|1;HEAP32[r12>>2]=r26;r28=r26&255}else{r28=r21}r26=(r28&1)<<24>>24==0?r16:HEAP32[r14];HEAP8[r26+r23|0]=r19;r24=r23+1|0;HEAP8[r26+r24|0]=0;r26=HEAP8[r9];if((r26&1)<<24>>24==0){r25=r24<<1&255;HEAP8[r9]=r25;r29=r25}else{HEAP32[r13>>2]=r24;r29=r26}r26=r17+1|0;if(r26>>>0<r6>>>0){r17=r26;r18=r29}else{r2=2712;break}}if(r2==2699){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}else if(r2==2712){r18=(r3|0)==-1?-1:r3<<1;if((r29&1)<<24>>24==0){r30=r18;r2=2717;break}r31=HEAP32[r8+8>>2];r32=r18;break}}else{r30=(r3|0)==-1?-1:r3<<1;r2=2717;break}}while(0);if(r2==2717){r31=r8+1|0;r32=r30}r30=__Z7catgetsP8_nl_catdiiPKc(r32,r4,r5,r31);HEAP32[r11]=0;HEAP32[r11+1]=0;HEAP32[r11+2]=0;r11=_strlen(r30);r31=r30+r11|0;L2949:do{if((r11|0)>0){r5=r1+1|0;r4=r1+4|0;r32=r1+8|0;r2=r1|0;r3=r30;r29=0;while(1){r6=HEAP8[r3];if((r29&1)<<24>>24==0){r33=10;r34=r29}else{r28=HEAP32[r2>>2];r33=(r28&-2)-1|0;r34=r28&255}r28=r34&255;if((r28&1|0)==0){r35=r28>>>1}else{r35=HEAP32[r4>>2]}if((r35|0)==(r33|0)){__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE9__grow_byEjjjjjj(r1,r33,1,r33,r33,0,0);r36=HEAP8[r10]}else{r36=r34}if((r36&1)<<24>>24==0){r37=r5}else{r37=HEAP32[r32>>2]}HEAP8[r37+r35|0]=r6;r6=r35+1|0;HEAP8[r37+r6|0]=0;r28=HEAP8[r10];if((r28&1)<<24>>24==0){r21=r6<<1&255;HEAP8[r10]=r21;r38=r21}else{HEAP32[r4>>2]=r6;r38=r28}r28=r3+1|0;if(r28>>>0<r31>>>0){r3=r28;r29=r38}else{break L2949}}}}while(0);if((HEAP8[r9]&1)<<24>>24==0){STACKTOP=r7;return}__ZdlPv(HEAP32[r8+8>>2]);STACKTOP=r7;return}function __ZNKSt3__18messagesIwE6do_getEiiiRKNS_12basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEEE(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48,r49,r50,r51,r52,r53,r54,r55,r56,r57;r2=0;r7=STACKTOP;STACKTOP=STACKTOP+204|0;r8=r7;r9=r7+8;r10=r7+40;r11=r7+44,r12=r11>>2;r13=r7+48;r14=r7+56;r15=r7+184;r16=r7+188,r17=r16>>2;r18=r7+192;r19=r18,r20=r19>>2;r21=STACKTOP;STACKTOP=STACKTOP+8|0;r22=STACKTOP;STACKTOP=STACKTOP+8|0;HEAP32[r20]=0;HEAP32[r20+1]=0;HEAP32[r20+2]=0;r20=r1,r23=r20>>2;r24=r21|0;HEAP32[r21+4>>2]=0;HEAP32[r21>>2]=5248944;r25=HEAP8[r6];if((r25&1)<<24>>24==0){r26=r6+4|0;r27=r6+4|0}else{r28=HEAP32[r6+8>>2];r26=r28;r27=r28}r28=r25&255;if((r28&1|0)==0){r29=r28>>>1}else{r29=HEAP32[r6+4>>2]}r6=(r29<<2)+r26|0;do{if(r27>>>0<r6>>>0){r26=r21;r29=r9|0;r28=r9+32|0;r25=r18+1|0;r30=(r18+8|0)>>2;r31=r18|0;r32=r18+4|0;r33=r27;r34=5248944;L2987:while(1){HEAP32[r12]=r33;r35=(FUNCTION_TABLE[HEAP32[r34+12>>2]](r24,r8,r33,r6,r11,r29,r28,r10)|0)==2;if(r35|(HEAP32[r12]|0)==(r33|0)){__ZNSt3__121__throw_runtime_errorEPKc(5243480)}L2993:do{if(r29>>>0<HEAP32[r10>>2]>>>0){r36=r29;r37=HEAP8[r19];while(1){r38=HEAP8[r36];if((r37&1)<<24>>24==0){r39=10;r40=r37}else{r41=HEAP32[r31>>2];r39=(r41&-2)-1|0;r40=r41&255}r41=r40&255;r42=(r41&1|0)==0?r41>>>1:HEAP32[r32>>2];if((r42|0)==(r39|0)){if((r39|0)==-3){r2=2763;break L2987}r41=(r40&1)<<24>>24==0?r25:HEAP32[r30];do{if(r39>>>0<2147483631){r43=r39+1|0;r44=r39<<1;r45=r43>>>0<r44>>>0?r44:r43;if(r45>>>0<11){r46=11;break}r46=r45+16&-16}else{r46=-2}}while(0);r45=__Znwj(r46);_memcpy(r45,r41,r39);if((r39|0)!=10){__ZdlPv(r41)}HEAP32[r30]=r45;r45=r46|1;HEAP32[r31>>2]=r45;r47=r45&255}else{r47=r40}r45=(r47&1)<<24>>24==0?r25:HEAP32[r30];HEAP8[r45+r42|0]=r38;r43=r42+1|0;HEAP8[r45+r43|0]=0;r45=HEAP8[r19];if((r45&1)<<24>>24==0){r44=r43<<1&255;HEAP8[r19]=r44;r48=r44}else{HEAP32[r32>>2]=r43;r48=r45}r45=r36+1|0;if(r45>>>0<HEAP32[r10>>2]>>>0){r36=r45;r37=r48}else{break L2993}}}}while(0);r37=HEAP32[r12];if(r37>>>0>=r6>>>0|r35){r2=2778;break}r33=r37;r34=HEAP32[r26>>2]}if(r2==2778){r26=(r3|0)==-1?-1:r3<<1;if((HEAP8[r19]&1)<<24>>24==0){r49=r26;r2=2785;break}r50=HEAP32[r18+8>>2];r51=r26;break}else if(r2==2763){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}}else{r49=(r3|0)==-1?-1:r3<<1;r2=2785;break}}while(0);if(r2==2785){r50=r18+1|0;r51=r49}r49=__Z7catgetsP8_nl_catdiiPKc(r51,r4,r5,r50);HEAP32[r23]=0;HEAP32[r23+1]=0;HEAP32[r23+2]=0;r23=r22|0;HEAP32[r22+4>>2]=0;HEAP32[r22>>2]=5248892;r50=_strlen(r49);r5=r49+r50|0;L3028:do{if((r50|0)>=1){r4=r22;r51=r5;r2=r14|0;r3=r14+128|0;r6=r1+4|0;r12=r1+8|0;r48=r1|0;r10=r49;r47=5248892;while(1){HEAP32[r17]=r10;r40=(FUNCTION_TABLE[HEAP32[r47+16>>2]](r23,r13,r10,(r51-r10|0)>32?r10+32|0:r5,r16,r2,r3,r15)|0)==2;if(r40|(HEAP32[r17]|0)==(r10|0)){__ZNSt3__121__throw_runtime_errorEPKc(5243480)}L3036:do{if(r2>>>0<HEAP32[r15>>2]>>>0){r46=r2;r39=HEAP8[r20];while(1){r11=HEAP32[r46>>2];if((r39&1)<<24>>24==0){r52=1;r53=r39}else{r8=HEAP32[r48>>2];r52=(r8&-2)-1|0;r53=r8&255}r8=r53&255;if((r8&1|0)==0){r54=r8>>>1}else{r54=HEAP32[r6>>2]}if((r54|0)==(r52|0)){__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE9__grow_byEjjjjjj(r1,r52,1,r52,r52,0,0);r55=HEAP8[r20]}else{r55=r53}if((r55&1)<<24>>24==0){r56=r6}else{r56=HEAP32[r12>>2]}HEAP32[r56+(r54<<2)>>2]=r11;r11=r54+1|0;HEAP32[r56+(r11<<2)>>2]=0;r8=HEAP8[r20];if((r8&1)<<24>>24==0){r24=r11<<1&255;HEAP8[r20]=r24;r57=r24}else{HEAP32[r6>>2]=r11;r57=r8}r8=r46+4|0;if(r8>>>0<HEAP32[r15>>2]>>>0){r46=r8;r39=r57}else{break L3036}}}}while(0);r35=HEAP32[r17];if(r35>>>0>=r5>>>0|r40){break L3028}r10=r35;r47=HEAP32[r4>>2]}}}while(0);if((HEAP8[r19]&1)<<24>>24==0){STACKTOP=r7;return}__ZdlPv(HEAP32[r18+8>>2]);STACKTOP=r7;return}function __ZNSt3__17collateIwED1Ev(r1){return}function __ZNSt3__110moneypunctIcLb0EED1Ev(r1){return}function __ZNSt3__110moneypunctIcLb1EED1Ev(r1){return}function __ZNSt3__110moneypunctIwLb0EED1Ev(r1){return}function __ZNSt3__110moneypunctIwLb1EED1Ev(r1){return}function __ZNSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEED1Ev(r1){return}function __ZNKSt3__110moneypunctIcLb0EE13do_pos_formatEv(r1,r2){r2=r1;tempBigInt=67109634;HEAP8[r2]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+3|0]=tempBigInt&255;return}function __ZNKSt3__110moneypunctIcLb0EE13do_neg_formatEv(r1,r2){r2=r1;tempBigInt=67109634;HEAP8[r2]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+3|0]=tempBigInt&255;return}function __ZNKSt3__110moneypunctIcLb1EE13do_pos_formatEv(r1,r2){r2=r1;tempBigInt=67109634;HEAP8[r2]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+3|0]=tempBigInt&255;return}function __ZNKSt3__110moneypunctIcLb1EE13do_neg_formatEv(r1,r2){r2=r1;tempBigInt=67109634;HEAP8[r2]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+3|0]=tempBigInt&255;return}function __ZNKSt3__110moneypunctIwLb0EE13do_pos_formatEv(r1,r2){r2=r1;tempBigInt=67109634;HEAP8[r2]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+3|0]=tempBigInt&255;return}function __ZNKSt3__110moneypunctIwLb0EE13do_neg_formatEv(r1,r2){r2=r1;tempBigInt=67109634;HEAP8[r2]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+3|0]=tempBigInt&255;return}function __ZNKSt3__110moneypunctIwLb1EE13do_pos_formatEv(r1,r2){r2=r1;tempBigInt=67109634;HEAP8[r2]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+3|0]=tempBigInt&255;return}function __ZNKSt3__110moneypunctIwLb1EE13do_neg_formatEv(r1,r2){r2=r1;tempBigInt=67109634;HEAP8[r2]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r2+3|0]=tempBigInt&255;return}function __ZNKSt3__17collateIwE10do_compareEPKwS3_S3_S3_(r1,r2,r3,r4,r5){var r6,r7,r8,r9,r10,r11;r1=0;L3080:do{if((r4|0)==(r5|0)){r6=r2}else{r7=r2;r8=r4;while(1){if((r7|0)==(r3|0)){r9=-1;r1=2844;break}r10=HEAP32[r7>>2];r11=HEAP32[r8>>2];if((r10|0)<(r11|0)){r9=-1;r1=2846;break}if((r11|0)<(r10|0)){r9=1;r1=2843;break}r10=r7+4|0;r11=r8+4|0;if((r11|0)==(r5|0)){r6=r10;break L3080}else{r7=r10;r8=r11}}if(r1==2843){return r9}else if(r1==2844){return r9}else if(r1==2846){return r9}}}while(0);r9=(r6|0)!=(r3|0)&1;return r9}function __ZNKSt3__110moneypunctIcLb0EE16do_negative_signEv(r1,r2){r2=r1;HEAP8[r1]=2;HEAP8[r2+1|0]=45;HEAP8[r2+2|0]=0;return}function __ZNKSt3__110moneypunctIcLb1EE16do_negative_signEv(r1,r2){r2=r1;HEAP8[r1]=2;HEAP8[r2+1|0]=45;HEAP8[r2+2|0]=0;return}function __ZNSt3__17collateIwED0Ev(r1){__ZdlPv(r1);return}function __ZNSt3__110moneypunctIcLb0EED0Ev(r1){__ZdlPv(r1);return}function __ZNKSt3__110moneypunctIcLb0EE11do_groupingEv(r1,r2){r2=r1>>2;HEAP32[r2]=0;HEAP32[r2+1]=0;HEAP32[r2+2]=0;return}function __ZNKSt3__110moneypunctIcLb0EE14do_curr_symbolEv(r1,r2){r2=r1>>2;HEAP32[r2]=0;HEAP32[r2+1]=0;HEAP32[r2+2]=0;return}function __ZNKSt3__110moneypunctIcLb0EE16do_positive_signEv(r1,r2){r2=r1>>2;HEAP32[r2]=0;HEAP32[r2+1]=0;HEAP32[r2+2]=0;return}function __ZNSt3__110moneypunctIcLb1EED0Ev(r1){__ZdlPv(r1);return}function __ZNKSt3__110moneypunctIcLb1EE11do_groupingEv(r1,r2){r2=r1>>2;HEAP32[r2]=0;HEAP32[r2+1]=0;HEAP32[r2+2]=0;return}function __ZNKSt3__110moneypunctIcLb1EE14do_curr_symbolEv(r1,r2){r2=r1>>2;HEAP32[r2]=0;HEAP32[r2+1]=0;HEAP32[r2+2]=0;return}function __ZNKSt3__110moneypunctIcLb1EE16do_positive_signEv(r1,r2){r2=r1>>2;HEAP32[r2]=0;HEAP32[r2+1]=0;HEAP32[r2+2]=0;return}function __ZNSt3__110moneypunctIwLb0EED0Ev(r1){__ZdlPv(r1);return}function __ZNKSt3__110moneypunctIwLb0EE11do_groupingEv(r1,r2){r2=r1>>2;HEAP32[r2]=0;HEAP32[r2+1]=0;HEAP32[r2+2]=0;return}function __ZNKSt3__110moneypunctIwLb0EE14do_curr_symbolEv(r1,r2){r2=r1>>2;HEAP32[r2]=0;HEAP32[r2+1]=0;HEAP32[r2+2]=0;return}function __ZNKSt3__110moneypunctIwLb0EE16do_positive_signEv(r1,r2){r2=r1>>2;HEAP32[r2]=0;HEAP32[r2+1]=0;HEAP32[r2+2]=0;return}function __ZNKSt3__110moneypunctIwLb0EE16do_negative_signEv(r1,r2){HEAP8[r1]=2;r2=r1+4|0;_wmemset(r2,45,1);HEAP32[r2+4>>2]=0;return}function __ZNSt3__110moneypunctIwLb1EED0Ev(r1){__ZdlPv(r1);return}function __ZNKSt3__110moneypunctIwLb1EE11do_groupingEv(r1,r2){r2=r1>>2;HEAP32[r2]=0;HEAP32[r2+1]=0;HEAP32[r2+2]=0;return}function __ZNKSt3__110moneypunctIwLb1EE14do_curr_symbolEv(r1,r2){r2=r1>>2;HEAP32[r2]=0;HEAP32[r2+1]=0;HEAP32[r2+2]=0;return}function __ZNKSt3__110moneypunctIwLb1EE16do_positive_signEv(r1,r2){r2=r1>>2;HEAP32[r2]=0;HEAP32[r2+1]=0;HEAP32[r2+2]=0;return}function __ZNKSt3__110moneypunctIwLb1EE16do_negative_signEv(r1,r2){HEAP8[r1]=2;r2=r1+4|0;_wmemset(r2,45,1);HEAP32[r2+4>>2]=0;return}function __ZNSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEED0Ev(r1){__ZdlPv(r1);return}function __ZNKSt3__17collateIcE12do_transformEPKcS3_(r1,r2,r3,r4){var r5,r6,r7,r8,r9;r2=r3;r5=r4-r2|0;if((r5|0)==-1){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if(r5>>>0<11){HEAP8[r1]=r5<<1&255;r6=r1+1|0}else{r7=r5+16&-16;r8=__Znwj(r7);HEAP32[r1+8>>2]=r8;HEAP32[r1>>2]=r7|1;HEAP32[r1+4>>2]=r5;r6=r8}if((r3|0)==(r4|0)){r9=r6;HEAP8[r9]=0;return}r8=r4+ -r2|0;r2=r6;r5=r3;while(1){HEAP8[r2]=HEAP8[r5];r3=r5+1|0;if((r3|0)==(r4|0)){break}else{r2=r2+1|0;r5=r3}}r9=r6+r8|0;HEAP8[r9]=0;return}function __ZNKSt3__17collateIwE12do_transformEPKwS3_(r1,r2,r3,r4){var r5,r6,r7,r8,r9;r2=r3;r5=r4-r2|0;r6=r5>>2;if(r6>>>0>1073741822){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if(r6>>>0<2){HEAP8[r1]=r5>>>1&255;r7=r1+4|0}else{r5=r6+4&-4;r8=__Znwj(r5<<2);HEAP32[r1+8>>2]=r8;HEAP32[r1>>2]=r5|1;HEAP32[r1+4>>2]=r6;r7=r8}if((r3|0)==(r4|0)){r9=r7;HEAP32[r9>>2]=0;return}r8=(r4-4+ -r2|0)>>>2;r2=r7;r6=r3;while(1){HEAP32[r2>>2]=HEAP32[r6>>2];r3=r6+4|0;if((r3|0)==(r4|0)){break}else{r2=r2+4|0;r6=r3}}r9=(r8+1<<2)+r7|0;HEAP32[r9>>2]=0;return}function __ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRb(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26;r8=STACKTOP;STACKTOP=STACKTOP+68|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8,r10=r9>>2;r11=r8+12,r12=r11>>2;r13=r8+24;r14=r8+28;r15=r8+32;r16=r8+36;r17=r8+40;r18=r8+64;if((HEAP32[r5+4>>2]&1|0)==0){HEAP32[r13>>2]=-1;r19=HEAP32[HEAP32[r2>>2]+16>>2];r20=r3|0;HEAP32[r15>>2]=HEAP32[r20>>2];HEAP32[r16>>2]=HEAP32[r4>>2];FUNCTION_TABLE[r19](r14,r2,r15,r16,r5,r6,r13);r16=HEAP32[r14>>2];HEAP32[r20>>2]=r16;r20=HEAP32[r13>>2];if((r20|0)==1){HEAP8[r7]=1}else if((r20|0)==0){HEAP8[r7]=0}else{HEAP8[r7]=1;HEAP32[r6>>2]=4}HEAP32[r1>>2]=r16;STACKTOP=r8;return}r16=r5+28|0;r5=HEAP32[r16>>2],r20=r5>>2;r13=(r5+4|0)>>2;tempValue=HEAP32[r13],HEAP32[r13]=tempValue+1,tempValue;if((HEAP32[1313727]|0)!=-1){HEAP32[r12]=5254908;HEAP32[r12+1]=24;HEAP32[r12+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r11,406)}r11=HEAP32[1313728]-1|0;r12=HEAP32[r20+2];do{if(HEAP32[r20+3]-r12>>2>>>0>r11>>>0){r14=HEAP32[r12+(r11<<2)>>2];if((r14|0)==0){break}r15=r14;if(((tempValue=HEAP32[r13],HEAP32[r13]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r20]+8>>2]](r5)}r14=HEAP32[r16>>2],r2=r14>>2;r19=(r14+4|0)>>2;tempValue=HEAP32[r19],HEAP32[r19]=tempValue+1,tempValue;if((HEAP32[1313635]|0)!=-1){HEAP32[r10]=5254540;HEAP32[r10+1]=24;HEAP32[r10+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254540,r9,406)}r21=HEAP32[1313636]-1|0;r22=HEAP32[r2+2];do{if(HEAP32[r2+3]-r22>>2>>>0>r21>>>0){r23=HEAP32[r22+(r21<<2)>>2];if((r23|0)==0){break}r24=r23;if(((tempValue=HEAP32[r19],HEAP32[r19]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r2]+8>>2]](r14)}r25=r17|0;r26=r23;FUNCTION_TABLE[HEAP32[HEAP32[r26>>2]+24>>2]](r25,r24);FUNCTION_TABLE[HEAP32[HEAP32[r26>>2]+28>>2]](r17+12|0,r24);HEAP32[r18>>2]=HEAP32[r4>>2];HEAP8[r7]=(__ZNSt3__114__scan_keywordINS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEPKNS_12basic_stringIcS3_NS_9allocatorIcEEEENS_5ctypeIcEEEET0_RT_SE_SD_SD_RKT1_Rjb(r3,r18,r25,r17+24|0,r15,r6,1)|0)==(r25|0)&1;HEAP32[r1>>2]=HEAP32[r3>>2];if((HEAP8[r17+12|0]&1)<<24>>24!=0){__ZdlPv(HEAP32[r17+20>>2])}if((HEAP8[r17]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r17+8>>2]);STACKTOP=r8;return}}while(0);r15=___cxa_allocate_exception(4);HEAP32[r15>>2]=5247488;___cxa_throw(r15,5252700,514)}}while(0);r8=___cxa_allocate_exception(4);HEAP32[r8>>2]=5247488;___cxa_throw(r8,5252700,514)}function __ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRl(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+256|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+28;r11=r8+32;r12=r8+44;r13=r8+84;r14=r8+88;r15=r8+248,r16=r15>>2;r17=r8+252;r18=HEAP32[r5+4>>2]&74;if((r18|0)==8){r19=16}else if((r18|0)==0){r19=0}else if((r18|0)==64){r19=8}else{r19=10}r18=r9|0;__ZNSt3__19__num_getIcE17__stage2_int_prepERNS_8ios_baseEPcRc(r11,r5,r18,r10);r5=r12|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r12=r14|0;HEAP32[r16]=r12;HEAP32[r17>>2]=0;r9=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP8[r10];r10=HEAP32[r9],r20=r10>>2;L6:while(1){do{if((r10|0)==0){r21=0,r22=r21>>2}else{if((HEAP32[r20+3]|0)!=(HEAP32[r20+4]|0)){r21=r10,r22=r21>>2;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r20]+36>>2]](r10)|0)!=-1){r21=r10,r22=r21>>2;break}HEAP32[r9]=0;r21=0,r22=r21>>2}}while(0);r23=(r21|0)==0;r24=HEAP32[r3],r25=r24>>2;do{if((r24|0)==0){r2=17}else{if((HEAP32[r25+3]|0)!=(HEAP32[r25+4]|0)){if(r23){r26=r24;r27=0;break}else{r28=r24,r29=r28>>2;r30=0;break L6}}if((FUNCTION_TABLE[HEAP32[HEAP32[r25]+36>>2]](r24)|0)==-1){HEAP32[r3]=0;r2=17;break}else{r31=(r24|0)==0;if(r23^r31){r26=r24;r27=r31;break}else{r28=r24,r29=r28>>2;r30=r31;break L6}}}}while(0);if(r2==17){r2=0;if(r23){r28=0,r29=r28>>2;r30=1;break}else{r26=0;r27=1}}r24=(r21+12|0)>>2;r25=HEAP32[r24];r31=r21+16|0;if((r25|0)==(HEAP32[r31>>2]|0)){r32=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r32=HEAPU8[r25]}if((__ZNSt3__19__num_getIcE17__stage2_int_loopEciPcRS2_RjcRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSD_S2_(r32&255,r19,r5,r13,r17,r4,r11,r12,r15,r18)|0)!=0){r28=r26,r29=r28>>2;r30=r27;break}r25=HEAP32[r24];if((r25|0)==(HEAP32[r31>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r22]+40>>2]](r21);r10=r21,r20=r10>>2;continue}else{HEAP32[r24]=r25+1|0;r10=r21,r20=r10>>2;continue}}r10=r11;r20=HEAPU8[r10];if((r20&1|0)==0){r33=r20>>>1}else{r33=HEAP32[r11+4>>2]}do{if((r33|0)!=0){r20=HEAP32[r16];if((r20-r14|0)>=160){break}r27=HEAP32[r17>>2];HEAP32[r16]=r20+4|0;HEAP32[r20>>2]=r27}}while(0);HEAP32[r7>>2]=__ZNSt3__125__num_get_signed_integralIlEET_PKcS3_Rji(r5,HEAP32[r13>>2],r6,r19);__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r11,r12,HEAP32[r16],r6);do{if(r23){r34=0}else{if((HEAP32[r22+3]|0)!=(HEAP32[r22+4]|0)){r34=r21;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)|0)!=-1){r34=r21;break}HEAP32[r9]=0;r34=0}}while(0);r9=(r34|0)==0;L50:do{if(r30){r2=47}else{do{if((HEAP32[r29+3]|0)==(HEAP32[r29+4]|0)){if((FUNCTION_TABLE[HEAP32[HEAP32[r29]+36>>2]](r28)|0)!=-1){break}HEAP32[r3]=0;r2=47;break L50}}while(0);if(r9^(r28|0)==0){break}else{r2=49;break}}}while(0);do{if(r2==47){if(r9){r2=49;break}else{break}}}while(0);if(r2==49){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r34;if((HEAP8[r10]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r11+8>>2]);STACKTOP=r8;return}function __ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRx(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+256|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+28;r11=r8+32;r12=r8+44;r13=r8+84;r14=r8+88;r15=r8+248,r16=r15>>2;r17=r8+252;r18=HEAP32[r5+4>>2]&74;if((r18|0)==8){r19=16}else if((r18|0)==0){r19=0}else if((r18|0)==64){r19=8}else{r19=10}r18=r9|0;__ZNSt3__19__num_getIcE17__stage2_int_prepERNS_8ios_baseEPcRc(r11,r5,r18,r10);r5=r12|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r12=r14|0;HEAP32[r16]=r12;HEAP32[r17>>2]=0;r9=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP8[r10];r10=HEAP32[r9],r20=r10>>2;L72:while(1){do{if((r10|0)==0){r21=0,r22=r21>>2}else{if((HEAP32[r20+3]|0)!=(HEAP32[r20+4]|0)){r21=r10,r22=r21>>2;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r20]+36>>2]](r10)|0)!=-1){r21=r10,r22=r21>>2;break}HEAP32[r9]=0;r21=0,r22=r21>>2}}while(0);r23=(r21|0)==0;r24=HEAP32[r3],r25=r24>>2;do{if((r24|0)==0){r2=72}else{if((HEAP32[r25+3]|0)!=(HEAP32[r25+4]|0)){if(r23){r26=r24;r27=0;break}else{r28=r24,r29=r28>>2;r30=0;break L72}}if((FUNCTION_TABLE[HEAP32[HEAP32[r25]+36>>2]](r24)|0)==-1){HEAP32[r3]=0;r2=72;break}else{r31=(r24|0)==0;if(r23^r31){r26=r24;r27=r31;break}else{r28=r24,r29=r28>>2;r30=r31;break L72}}}}while(0);if(r2==72){r2=0;if(r23){r28=0,r29=r28>>2;r30=1;break}else{r26=0;r27=1}}r24=(r21+12|0)>>2;r25=HEAP32[r24];r31=r21+16|0;if((r25|0)==(HEAP32[r31>>2]|0)){r32=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r32=HEAPU8[r25]}if((__ZNSt3__19__num_getIcE17__stage2_int_loopEciPcRS2_RjcRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSD_S2_(r32&255,r19,r5,r13,r17,r4,r11,r12,r15,r18)|0)!=0){r28=r26,r29=r28>>2;r30=r27;break}r25=HEAP32[r24];if((r25|0)==(HEAP32[r31>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r22]+40>>2]](r21);r10=r21,r20=r10>>2;continue}else{HEAP32[r24]=r25+1|0;r10=r21,r20=r10>>2;continue}}r10=r11;r20=HEAPU8[r10];if((r20&1|0)==0){r33=r20>>>1}else{r33=HEAP32[r11+4>>2]}do{if((r33|0)!=0){r20=HEAP32[r16];if((r20-r14|0)>=160){break}r27=HEAP32[r17>>2];HEAP32[r16]=r20+4|0;HEAP32[r20>>2]=r27}}while(0);HEAP32[r7>>2]=__ZNSt3__125__num_get_signed_integralIxEET_PKcS3_Rji(r5,HEAP32[r13>>2],r6,r19);HEAP32[r7+4>>2]=tempRet0;__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r11,r12,HEAP32[r16],r6);do{if(r23){r34=0}else{if((HEAP32[r22+3]|0)!=(HEAP32[r22+4]|0)){r34=r21;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)|0)!=-1){r34=r21;break}HEAP32[r9]=0;r34=0}}while(0);r9=(r34|0)==0;L116:do{if(r30){r2=102}else{do{if((HEAP32[r29+3]|0)==(HEAP32[r29+4]|0)){if((FUNCTION_TABLE[HEAP32[HEAP32[r29]+36>>2]](r28)|0)!=-1){break}HEAP32[r3]=0;r2=102;break L116}}while(0);if(r9^(r28|0)==0){break}else{r2=104;break}}}while(0);do{if(r2==102){if(r9){r2=104;break}else{break}}}while(0);if(r2==104){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r34;if((HEAP8[r10]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r11+8>>2]);STACKTOP=r8;return}function __ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRt(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+256|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+28;r11=r8+32;r12=r8+44;r13=r8+84;r14=r8+88;r15=r8+248,r16=r15>>2;r17=r8+252;r18=HEAP32[r5+4>>2]&74;if((r18|0)==0){r19=0}else if((r18|0)==64){r19=8}else if((r18|0)==8){r19=16}else{r19=10}r18=r9|0;__ZNSt3__19__num_getIcE17__stage2_int_prepERNS_8ios_baseEPcRc(r11,r5,r18,r10);r5=r12|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r12=r14|0;HEAP32[r16]=r12;HEAP32[r17>>2]=0;r9=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP8[r10];r10=HEAP32[r9],r20=r10>>2;L138:while(1){do{if((r10|0)==0){r21=0,r22=r21>>2}else{if((HEAP32[r20+3]|0)!=(HEAP32[r20+4]|0)){r21=r10,r22=r21>>2;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r20]+36>>2]](r10)|0)!=-1){r21=r10,r22=r21>>2;break}HEAP32[r9]=0;r21=0,r22=r21>>2}}while(0);r23=(r21|0)==0;r24=HEAP32[r3],r25=r24>>2;do{if((r24|0)==0){r2=127}else{if((HEAP32[r25+3]|0)!=(HEAP32[r25+4]|0)){if(r23){r26=r24;r27=0;break}else{r28=r24,r29=r28>>2;r30=0;break L138}}if((FUNCTION_TABLE[HEAP32[HEAP32[r25]+36>>2]](r24)|0)==-1){HEAP32[r3]=0;r2=127;break}else{r31=(r24|0)==0;if(r23^r31){r26=r24;r27=r31;break}else{r28=r24,r29=r28>>2;r30=r31;break L138}}}}while(0);if(r2==127){r2=0;if(r23){r28=0,r29=r28>>2;r30=1;break}else{r26=0;r27=1}}r24=(r21+12|0)>>2;r25=HEAP32[r24];r31=r21+16|0;if((r25|0)==(HEAP32[r31>>2]|0)){r32=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r32=HEAPU8[r25]}if((__ZNSt3__19__num_getIcE17__stage2_int_loopEciPcRS2_RjcRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSD_S2_(r32&255,r19,r5,r13,r17,r4,r11,r12,r15,r18)|0)!=0){r28=r26,r29=r28>>2;r30=r27;break}r25=HEAP32[r24];if((r25|0)==(HEAP32[r31>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r22]+40>>2]](r21);r10=r21,r20=r10>>2;continue}else{HEAP32[r24]=r25+1|0;r10=r21,r20=r10>>2;continue}}r10=r11;r20=HEAPU8[r10];if((r20&1|0)==0){r33=r20>>>1}else{r33=HEAP32[r11+4>>2]}do{if((r33|0)!=0){r20=HEAP32[r16];if((r20-r14|0)>=160){break}r27=HEAP32[r17>>2];HEAP32[r16]=r20+4|0;HEAP32[r20>>2]=r27}}while(0);HEAP16[r7>>1]=__ZNSt3__127__num_get_unsigned_integralItEET_PKcS3_Rji(r5,HEAP32[r13>>2],r6,r19);__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r11,r12,HEAP32[r16],r6);do{if(r23){r34=0}else{if((HEAP32[r22+3]|0)!=(HEAP32[r22+4]|0)){r34=r21;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)|0)!=-1){r34=r21;break}HEAP32[r9]=0;r34=0}}while(0);r9=(r34|0)==0;L182:do{if(r30){r2=157}else{do{if((HEAP32[r29+3]|0)==(HEAP32[r29+4]|0)){if((FUNCTION_TABLE[HEAP32[HEAP32[r29]+36>>2]](r28)|0)!=-1){break}HEAP32[r3]=0;r2=157;break L182}}while(0);if(r9^(r28|0)==0){break}else{r2=159;break}}}while(0);do{if(r2==157){if(r9){r2=159;break}else{break}}}while(0);if(r2==159){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r34;if((HEAP8[r10]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r11+8>>2]);STACKTOP=r8;return}function __ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjS8_(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+256|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+28;r11=r8+32;r12=r8+44;r13=r8+84;r14=r8+88;r15=r8+248,r16=r15>>2;r17=r8+252;r18=HEAP32[r5+4>>2]&74;if((r18|0)==8){r19=16}else if((r18|0)==0){r19=0}else if((r18|0)==64){r19=8}else{r19=10}r18=r9|0;__ZNSt3__19__num_getIcE17__stage2_int_prepERNS_8ios_baseEPcRc(r11,r5,r18,r10);r5=r12|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r12=r14|0;HEAP32[r16]=r12;HEAP32[r17>>2]=0;r9=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP8[r10];r10=HEAP32[r9],r20=r10>>2;L204:while(1){do{if((r10|0)==0){r21=0,r22=r21>>2}else{if((HEAP32[r20+3]|0)!=(HEAP32[r20+4]|0)){r21=r10,r22=r21>>2;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r20]+36>>2]](r10)|0)!=-1){r21=r10,r22=r21>>2;break}HEAP32[r9]=0;r21=0,r22=r21>>2}}while(0);r23=(r21|0)==0;r24=HEAP32[r3],r25=r24>>2;do{if((r24|0)==0){r2=182}else{if((HEAP32[r25+3]|0)!=(HEAP32[r25+4]|0)){if(r23){r26=r24;r27=0;break}else{r28=r24,r29=r28>>2;r30=0;break L204}}if((FUNCTION_TABLE[HEAP32[HEAP32[r25]+36>>2]](r24)|0)==-1){HEAP32[r3]=0;r2=182;break}else{r31=(r24|0)==0;if(r23^r31){r26=r24;r27=r31;break}else{r28=r24,r29=r28>>2;r30=r31;break L204}}}}while(0);if(r2==182){r2=0;if(r23){r28=0,r29=r28>>2;r30=1;break}else{r26=0;r27=1}}r24=(r21+12|0)>>2;r25=HEAP32[r24];r31=r21+16|0;if((r25|0)==(HEAP32[r31>>2]|0)){r32=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r32=HEAPU8[r25]}if((__ZNSt3__19__num_getIcE17__stage2_int_loopEciPcRS2_RjcRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSD_S2_(r32&255,r19,r5,r13,r17,r4,r11,r12,r15,r18)|0)!=0){r28=r26,r29=r28>>2;r30=r27;break}r25=HEAP32[r24];if((r25|0)==(HEAP32[r31>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r22]+40>>2]](r21);r10=r21,r20=r10>>2;continue}else{HEAP32[r24]=r25+1|0;r10=r21,r20=r10>>2;continue}}r10=r11;r20=HEAPU8[r10];if((r20&1|0)==0){r33=r20>>>1}else{r33=HEAP32[r11+4>>2]}do{if((r33|0)!=0){r20=HEAP32[r16];if((r20-r14|0)>=160){break}r27=HEAP32[r17>>2];HEAP32[r16]=r20+4|0;HEAP32[r20>>2]=r27}}while(0);HEAP32[r7>>2]=__ZNSt3__127__num_get_unsigned_integralIjEET_PKcS3_Rji(r5,HEAP32[r13>>2],r6,r19);__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r11,r12,HEAP32[r16],r6);do{if(r23){r34=0}else{if((HEAP32[r22+3]|0)!=(HEAP32[r22+4]|0)){r34=r21;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)|0)!=-1){r34=r21;break}HEAP32[r9]=0;r34=0}}while(0);r9=(r34|0)==0;L248:do{if(r30){r2=212}else{do{if((HEAP32[r29+3]|0)==(HEAP32[r29+4]|0)){if((FUNCTION_TABLE[HEAP32[HEAP32[r29]+36>>2]](r28)|0)!=-1){break}HEAP32[r3]=0;r2=212;break L248}}while(0);if(r9^(r28|0)==0){break}else{r2=214;break}}}while(0);do{if(r2==212){if(r9){r2=214;break}else{break}}}while(0);if(r2==214){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r34;if((HEAP8[r10]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r11+8>>2]);STACKTOP=r8;return}function __ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRm(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+256|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+28;r11=r8+32;r12=r8+44;r13=r8+84;r14=r8+88;r15=r8+248,r16=r15>>2;r17=r8+252;r18=HEAP32[r5+4>>2]&74;if((r18|0)==0){r19=0}else if((r18|0)==64){r19=8}else if((r18|0)==8){r19=16}else{r19=10}r18=r9|0;__ZNSt3__19__num_getIcE17__stage2_int_prepERNS_8ios_baseEPcRc(r11,r5,r18,r10);r5=r12|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r12=r14|0;HEAP32[r16]=r12;HEAP32[r17>>2]=0;r9=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP8[r10];r10=HEAP32[r9],r20=r10>>2;L270:while(1){do{if((r10|0)==0){r21=0,r22=r21>>2}else{if((HEAP32[r20+3]|0)!=(HEAP32[r20+4]|0)){r21=r10,r22=r21>>2;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r20]+36>>2]](r10)|0)!=-1){r21=r10,r22=r21>>2;break}HEAP32[r9]=0;r21=0,r22=r21>>2}}while(0);r23=(r21|0)==0;r24=HEAP32[r3],r25=r24>>2;do{if((r24|0)==0){r2=237}else{if((HEAP32[r25+3]|0)!=(HEAP32[r25+4]|0)){if(r23){r26=r24;r27=0;break}else{r28=r24,r29=r28>>2;r30=0;break L270}}if((FUNCTION_TABLE[HEAP32[HEAP32[r25]+36>>2]](r24)|0)==-1){HEAP32[r3]=0;r2=237;break}else{r31=(r24|0)==0;if(r23^r31){r26=r24;r27=r31;break}else{r28=r24,r29=r28>>2;r30=r31;break L270}}}}while(0);if(r2==237){r2=0;if(r23){r28=0,r29=r28>>2;r30=1;break}else{r26=0;r27=1}}r24=(r21+12|0)>>2;r25=HEAP32[r24];r31=r21+16|0;if((r25|0)==(HEAP32[r31>>2]|0)){r32=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r32=HEAPU8[r25]}if((__ZNSt3__19__num_getIcE17__stage2_int_loopEciPcRS2_RjcRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSD_S2_(r32&255,r19,r5,r13,r17,r4,r11,r12,r15,r18)|0)!=0){r28=r26,r29=r28>>2;r30=r27;break}r25=HEAP32[r24];if((r25|0)==(HEAP32[r31>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r22]+40>>2]](r21);r10=r21,r20=r10>>2;continue}else{HEAP32[r24]=r25+1|0;r10=r21,r20=r10>>2;continue}}r10=r11;r20=HEAPU8[r10];if((r20&1|0)==0){r33=r20>>>1}else{r33=HEAP32[r11+4>>2]}do{if((r33|0)!=0){r20=HEAP32[r16];if((r20-r14|0)>=160){break}r27=HEAP32[r17>>2];HEAP32[r16]=r20+4|0;HEAP32[r20>>2]=r27}}while(0);HEAP32[r7>>2]=__ZNSt3__127__num_get_unsigned_integralImEET_PKcS3_Rji(r5,HEAP32[r13>>2],r6,r19);__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r11,r12,HEAP32[r16],r6);do{if(r23){r34=0}else{if((HEAP32[r22+3]|0)!=(HEAP32[r22+4]|0)){r34=r21;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)|0)!=-1){r34=r21;break}HEAP32[r9]=0;r34=0}}while(0);r9=(r34|0)==0;L314:do{if(r30){r2=267}else{do{if((HEAP32[r29+3]|0)==(HEAP32[r29+4]|0)){if((FUNCTION_TABLE[HEAP32[HEAP32[r29]+36>>2]](r28)|0)!=-1){break}HEAP32[r3]=0;r2=267;break L314}}while(0);if(r9^(r28|0)==0){break}else{r2=269;break}}}while(0);do{if(r2==267){if(r9){r2=269;break}else{break}}}while(0);if(r2==269){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r34;if((HEAP8[r10]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r11+8>>2]);STACKTOP=r8;return}function __ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRy(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+256|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+28;r11=r8+32;r12=r8+44;r13=r8+84;r14=r8+88;r15=r8+248,r16=r15>>2;r17=r8+252;r18=HEAP32[r5+4>>2]&74;if((r18|0)==0){r19=0}else if((r18|0)==8){r19=16}else if((r18|0)==64){r19=8}else{r19=10}r18=r9|0;__ZNSt3__19__num_getIcE17__stage2_int_prepERNS_8ios_baseEPcRc(r11,r5,r18,r10);r5=r12|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r12=r14|0;HEAP32[r16]=r12;HEAP32[r17>>2]=0;r9=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP8[r10];r10=HEAP32[r9],r20=r10>>2;L336:while(1){do{if((r10|0)==0){r21=0,r22=r21>>2}else{if((HEAP32[r20+3]|0)!=(HEAP32[r20+4]|0)){r21=r10,r22=r21>>2;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r20]+36>>2]](r10)|0)!=-1){r21=r10,r22=r21>>2;break}HEAP32[r9]=0;r21=0,r22=r21>>2}}while(0);r23=(r21|0)==0;r24=HEAP32[r3],r25=r24>>2;do{if((r24|0)==0){r2=292}else{if((HEAP32[r25+3]|0)!=(HEAP32[r25+4]|0)){if(r23){r26=r24;r27=0;break}else{r28=r24,r29=r28>>2;r30=0;break L336}}if((FUNCTION_TABLE[HEAP32[HEAP32[r25]+36>>2]](r24)|0)==-1){HEAP32[r3]=0;r2=292;break}else{r31=(r24|0)==0;if(r23^r31){r26=r24;r27=r31;break}else{r28=r24,r29=r28>>2;r30=r31;break L336}}}}while(0);if(r2==292){r2=0;if(r23){r28=0,r29=r28>>2;r30=1;break}else{r26=0;r27=1}}r24=(r21+12|0)>>2;r25=HEAP32[r24];r31=r21+16|0;if((r25|0)==(HEAP32[r31>>2]|0)){r32=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r32=HEAPU8[r25]}if((__ZNSt3__19__num_getIcE17__stage2_int_loopEciPcRS2_RjcRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSD_S2_(r32&255,r19,r5,r13,r17,r4,r11,r12,r15,r18)|0)!=0){r28=r26,r29=r28>>2;r30=r27;break}r25=HEAP32[r24];if((r25|0)==(HEAP32[r31>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r22]+40>>2]](r21);r10=r21,r20=r10>>2;continue}else{HEAP32[r24]=r25+1|0;r10=r21,r20=r10>>2;continue}}r10=r11;r20=HEAPU8[r10];if((r20&1|0)==0){r33=r20>>>1}else{r33=HEAP32[r11+4>>2]}do{if((r33|0)!=0){r20=HEAP32[r16];if((r20-r14|0)>=160){break}r27=HEAP32[r17>>2];HEAP32[r16]=r20+4|0;HEAP32[r20>>2]=r27}}while(0);HEAP32[r7>>2]=__ZNSt3__127__num_get_unsigned_integralIyEET_PKcS3_Rji(r5,HEAP32[r13>>2],r6,r19);HEAP32[r7+4>>2]=tempRet0;__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r11,r12,HEAP32[r16],r6);do{if(r23){r34=0}else{if((HEAP32[r22+3]|0)!=(HEAP32[r22+4]|0)){r34=r21;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)|0)!=-1){r34=r21;break}HEAP32[r9]=0;r34=0}}while(0);r9=(r34|0)==0;L380:do{if(r30){r2=322}else{do{if((HEAP32[r29+3]|0)==(HEAP32[r29+4]|0)){if((FUNCTION_TABLE[HEAP32[HEAP32[r29]+36>>2]](r28)|0)!=-1){break}HEAP32[r3]=0;r2=322;break L380}}while(0);if(r9^(r28|0)==0){break}else{r2=324;break}}}while(0);do{if(r2==322){if(r9){r2=324;break}else{break}}}while(0);if(r2==324){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r34;if((HEAP8[r10]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r11+8>>2]);STACKTOP=r8;return}function __ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRf(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+276|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+36;r11=r8+40;r12=r8+44;r13=r8+96;r14=r8+100;r15=r8+260,r16=r15>>2;r17=r8+264;r18=r8+268;r19=r8+272;r20=r8+4|0;__ZNSt3__19__num_getIcE19__stage2_float_prepERNS_8ios_baseEPcRcS5_(r12,r5,r20,r10,r11);r5=r8+56|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r21=r14|0;HEAP32[r16]=r21;HEAP32[r17>>2]=0;HEAP8[r18]=1;HEAP8[r19]=69;r22=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP8[r10];r10=HEAP8[r11];r11=HEAP32[r22],r23=r11>>2;L397:while(1){do{if((r11|0)==0){r24=0}else{if((HEAP32[r23+3]|0)!=(HEAP32[r23+4]|0)){r24=r11;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r23]+36>>2]](r11)|0)!=-1){r24=r11;break}HEAP32[r22]=0;r24=0}}while(0);r25=(r24|0)==0;r26=HEAP32[r3],r27=r26>>2;do{if((r26|0)==0){r2=343}else{if((HEAP32[r27+3]|0)!=(HEAP32[r27+4]|0)){if(r25){break}else{break L397}}if((FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r26)|0)==-1){HEAP32[r3]=0;r2=343;break}else{if(r25^(r26|0)==0){break}else{break L397}}}}while(0);if(r2==343){r2=0;if(r25){break}}r26=(r24+12|0)>>2;r27=HEAP32[r26];r28=r24+16|0;if((r27|0)==(HEAP32[r28>>2]|0)){r29=FUNCTION_TABLE[HEAP32[HEAP32[r24>>2]+36>>2]](r24)}else{r29=HEAPU8[r27]}if((__ZNSt3__19__num_getIcE19__stage2_float_loopEcRbRcPcRS4_ccRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSE_RjS4_(r29&255,r18,r19,r5,r13,r4,r10,r12,r21,r15,r17,r20)|0)!=0){break}r27=HEAP32[r26];if((r27|0)==(HEAP32[r28>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r24>>2]+40>>2]](r24);r11=r24,r23=r11>>2;continue}else{HEAP32[r26]=r27+1|0;r11=r24,r23=r11>>2;continue}}r11=r12;r23=HEAPU8[r11];if((r23&1|0)==0){r30=r23>>>1}else{r30=HEAP32[r12+4>>2]}do{if((r30|0)!=0){if((HEAP8[r18]&1)<<24>>24==0){break}r23=HEAP32[r16];if((r23-r14|0)>=160){break}r24=HEAP32[r17>>2];HEAP32[r16]=r23+4|0;HEAP32[r23>>2]=r24}}while(0);r17=HEAP32[r13>>2];do{if((r5|0)==(r17|0)){HEAP32[r6>>2]=4;r31=0}else{do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r25=_strtod(r5,r9);if((HEAP32[r9>>2]|0)==(r17|0)){r31=r25;break}else{HEAP32[r6>>2]=4;r31=0;break}}}while(0);HEAPF32[r7>>2]=r31;__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r12,r21,HEAP32[r16],r6);r16=HEAP32[r22],r21=r16>>2;do{if((r16|0)==0){r32=0}else{if((HEAP32[r21+3]|0)!=(HEAP32[r21+4]|0)){r32=r16;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r21]+36>>2]](r16)|0)!=-1){r32=r16;break}HEAP32[r22]=0;r32=0}}while(0);r22=(r32|0)==0;r16=HEAP32[r3],r21=r16>>2;do{if((r16|0)==0){r2=384}else{if((HEAP32[r21+3]|0)!=(HEAP32[r21+4]|0)){if(r22){break}else{r2=386;break}}if((FUNCTION_TABLE[HEAP32[HEAP32[r21]+36>>2]](r16)|0)==-1){HEAP32[r3]=0;r2=384;break}else{if(r22^(r16|0)==0){break}else{r2=386;break}}}}while(0);do{if(r2==384){if(r22){r2=386;break}else{break}}}while(0);if(r2==386){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r32;if((HEAP8[r11]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r12+8>>2]);STACKTOP=r8;return}function __ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRd(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+276|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+36;r11=r8+40;r12=r8+44;r13=r8+96;r14=r8+100;r15=r8+260,r16=r15>>2;r17=r8+264;r18=r8+268;r19=r8+272;r20=r8+4|0;__ZNSt3__19__num_getIcE19__stage2_float_prepERNS_8ios_baseEPcRcS5_(r12,r5,r20,r10,r11);r5=r8+56|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r21=r14|0;HEAP32[r16]=r21;HEAP32[r17>>2]=0;HEAP8[r18]=1;HEAP8[r19]=69;r22=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP8[r10];r10=HEAP8[r11];r11=HEAP32[r22],r23=r11>>2;L472:while(1){do{if((r11|0)==0){r24=0}else{if((HEAP32[r23+3]|0)!=(HEAP32[r23+4]|0)){r24=r11;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r23]+36>>2]](r11)|0)!=-1){r24=r11;break}HEAP32[r22]=0;r24=0}}while(0);r25=(r24|0)==0;r26=HEAP32[r3],r27=r26>>2;do{if((r26|0)==0){r2=405}else{if((HEAP32[r27+3]|0)!=(HEAP32[r27+4]|0)){if(r25){break}else{break L472}}if((FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r26)|0)==-1){HEAP32[r3]=0;r2=405;break}else{if(r25^(r26|0)==0){break}else{break L472}}}}while(0);if(r2==405){r2=0;if(r25){break}}r26=(r24+12|0)>>2;r27=HEAP32[r26];r28=r24+16|0;if((r27|0)==(HEAP32[r28>>2]|0)){r29=FUNCTION_TABLE[HEAP32[HEAP32[r24>>2]+36>>2]](r24)}else{r29=HEAPU8[r27]}if((__ZNSt3__19__num_getIcE19__stage2_float_loopEcRbRcPcRS4_ccRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSE_RjS4_(r29&255,r18,r19,r5,r13,r4,r10,r12,r21,r15,r17,r20)|0)!=0){break}r27=HEAP32[r26];if((r27|0)==(HEAP32[r28>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r24>>2]+40>>2]](r24);r11=r24,r23=r11>>2;continue}else{HEAP32[r26]=r27+1|0;r11=r24,r23=r11>>2;continue}}r11=r12;r23=HEAPU8[r11];if((r23&1|0)==0){r30=r23>>>1}else{r30=HEAP32[r12+4>>2]}do{if((r30|0)!=0){if((HEAP8[r18]&1)<<24>>24==0){break}r23=HEAP32[r16];if((r23-r14|0)>=160){break}r24=HEAP32[r17>>2];HEAP32[r16]=r23+4|0;HEAP32[r23>>2]=r24}}while(0);r17=HEAP32[r13>>2];do{if((r5|0)==(r17|0)){HEAP32[r6>>2]=4;r31=0}else{do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r25=_strtod(r5,r9);if((HEAP32[r9>>2]|0)==(r17|0)){r31=r25;break}HEAP32[r6>>2]=4;r31=0}}while(0);HEAPF64[tempDoublePtr>>3]=r31,HEAP32[r7>>2]=HEAP32[tempDoublePtr>>2],HEAP32[r7+4>>2]=HEAP32[tempDoublePtr+4>>2];__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r12,r21,HEAP32[r16],r6);r16=HEAP32[r22],r21=r16>>2;do{if((r16|0)==0){r32=0}else{if((HEAP32[r21+3]|0)!=(HEAP32[r21+4]|0)){r32=r16;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r21]+36>>2]](r16)|0)!=-1){r32=r16;break}HEAP32[r22]=0;r32=0}}while(0);r22=(r32|0)==0;r16=HEAP32[r3],r21=r16>>2;do{if((r16|0)==0){r2=445}else{if((HEAP32[r21+3]|0)!=(HEAP32[r21+4]|0)){if(r22){break}else{r2=447;break}}if((FUNCTION_TABLE[HEAP32[HEAP32[r21]+36>>2]](r16)|0)==-1){HEAP32[r3]=0;r2=445;break}else{if(r22^(r16|0)==0){break}else{r2=447;break}}}}while(0);do{if(r2==445){if(r22){r2=447;break}else{break}}}while(0);if(r2==447){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r32;if((HEAP8[r11]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r12+8>>2]);STACKTOP=r8;return}function __ZNSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEED1Ev(r1){return}function __ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRe(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+276|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+36;r11=r8+40;r12=r8+44;r13=r8+96;r14=r8+100;r15=r8+260,r16=r15>>2;r17=r8+264;r18=r8+268;r19=r8+272;r20=r8+4|0;__ZNSt3__19__num_getIcE19__stage2_float_prepERNS_8ios_baseEPcRcS5_(r12,r5,r20,r10,r11);r5=r8+56|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r21=r14|0;HEAP32[r16]=r21;HEAP32[r17>>2]=0;HEAP8[r18]=1;HEAP8[r19]=69;r22=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP8[r10];r10=HEAP8[r11];r11=HEAP32[r22],r23=r11>>2;L546:while(1){do{if((r11|0)==0){r24=0}else{if((HEAP32[r23+3]|0)!=(HEAP32[r23+4]|0)){r24=r11;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r23]+36>>2]](r11)|0)!=-1){r24=r11;break}HEAP32[r22]=0;r24=0}}while(0);r25=(r24|0)==0;r26=HEAP32[r3],r27=r26>>2;do{if((r26|0)==0){r2=467}else{if((HEAP32[r27+3]|0)!=(HEAP32[r27+4]|0)){if(r25){break}else{break L546}}if((FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r26)|0)==-1){HEAP32[r3]=0;r2=467;break}else{if(r25^(r26|0)==0){break}else{break L546}}}}while(0);if(r2==467){r2=0;if(r25){break}}r26=(r24+12|0)>>2;r27=HEAP32[r26];r28=r24+16|0;if((r27|0)==(HEAP32[r28>>2]|0)){r29=FUNCTION_TABLE[HEAP32[HEAP32[r24>>2]+36>>2]](r24)}else{r29=HEAPU8[r27]}if((__ZNSt3__19__num_getIcE19__stage2_float_loopEcRbRcPcRS4_ccRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSE_RjS4_(r29&255,r18,r19,r5,r13,r4,r10,r12,r21,r15,r17,r20)|0)!=0){break}r27=HEAP32[r26];if((r27|0)==(HEAP32[r28>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r24>>2]+40>>2]](r24);r11=r24,r23=r11>>2;continue}else{HEAP32[r26]=r27+1|0;r11=r24,r23=r11>>2;continue}}r11=r12;r23=HEAPU8[r11];if((r23&1|0)==0){r30=r23>>>1}else{r30=HEAP32[r12+4>>2]}do{if((r30|0)!=0){if((HEAP8[r18]&1)<<24>>24==0){break}r23=HEAP32[r16];if((r23-r14|0)>=160){break}r24=HEAP32[r17>>2];HEAP32[r16]=r23+4|0;HEAP32[r23>>2]=r24}}while(0);r17=HEAP32[r13>>2];do{if((r5|0)==(r17|0)){HEAP32[r6>>2]=4;r31=1.1125369292536007e-308}else{do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r25=_strtod(r5,r9);if((HEAP32[r9>>2]|0)==(r17|0)){r31=r25;break}else{HEAP32[r6>>2]=4;r31=1.1125369292536007e-308;break}}}while(0);HEAPF64[tempDoublePtr>>3]=r31,HEAP32[r7>>2]=HEAP32[tempDoublePtr>>2],HEAP32[r7+4>>2]=HEAP32[tempDoublePtr+4>>2];__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r12,r21,HEAP32[r16],r6);r16=HEAP32[r22],r21=r16>>2;do{if((r16|0)==0){r32=0}else{if((HEAP32[r21+3]|0)!=(HEAP32[r21+4]|0)){r32=r16;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r21]+36>>2]](r16)|0)!=-1){r32=r16;break}HEAP32[r22]=0;r32=0}}while(0);r22=(r32|0)==0;r16=HEAP32[r3],r21=r16>>2;do{if((r16|0)==0){r2=508}else{if((HEAP32[r21+3]|0)!=(HEAP32[r21+4]|0)){if(r22){break}else{r2=510;break}}if((FUNCTION_TABLE[HEAP32[HEAP32[r21]+36>>2]](r16)|0)==-1){HEAP32[r3]=0;r2=508;break}else{if(r22^(r16|0)==0){break}else{r2=510;break}}}}while(0);do{if(r2==508){if(r22){r2=510;break}else{break}}}while(0);if(r2==510){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r32;if((HEAP8[r11]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r12+8>>2]);STACKTOP=r8;return}function __ZNSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEED0Ev(r1){__ZdlPv(r1);return}function __ZNKSt3__17num_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_RNS_8ios_baseERjRPv(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+52|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8,r10=r9>>2;r11=r8+12;r12=r8+40;r13=r12,r14=r13>>2;r15=STACKTOP;STACKTOP=STACKTOP+40|0;r16=STACKTOP;STACKTOP=STACKTOP+4|0;r17=STACKTOP;STACKTOP=STACKTOP+160|0;r18=STACKTOP;STACKTOP=STACKTOP+4|0;r19=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r14]=0;HEAP32[r14+1]=0;HEAP32[r14+2]=0;r14=HEAP32[r5+28>>2],r5=r14>>2;r20=(r14+4|0)>>2;tempValue=HEAP32[r20],HEAP32[r20]=tempValue+1,tempValue;if((HEAP32[1313727]|0)!=-1){HEAP32[r10]=5254908;HEAP32[r10+1]=24;HEAP32[r10+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r9,406)}r9=HEAP32[1313728]-1|0;r10=HEAP32[r5+2];do{if(HEAP32[r5+3]-r10>>2>>>0>r9>>>0){r21=HEAP32[r10+(r9<<2)>>2];if((r21|0)==0){break}r22=r11|0;FUNCTION_TABLE[HEAP32[HEAP32[r21>>2]+32>>2]](r21,5255348,5255374,r22);if(((tempValue=HEAP32[r20],HEAP32[r20]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r5]+8>>2]](r14)}r21=r15|0;_memset(r21,0,40);HEAP32[r16>>2]=r21;r23=r17|0;HEAP32[r18>>2]=r23;HEAP32[r19>>2]=0;r24=(r3|0)>>2;r25=(r4|0)>>2;r26=HEAP32[r24],r27=r26>>2;L632:while(1){do{if((r26|0)==0){r28=0}else{if((HEAP32[r27+3]|0)!=(HEAP32[r27+4]|0)){r28=r26;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r26)|0)!=-1){r28=r26;break}HEAP32[r24]=0;r28=0}}while(0);r29=(r28|0)==0;r30=HEAP32[r25],r31=r30>>2;do{if((r30|0)==0){r2=539}else{if((HEAP32[r31+3]|0)!=(HEAP32[r31+4]|0)){if(r29){break}else{break L632}}if((FUNCTION_TABLE[HEAP32[HEAP32[r31]+36>>2]](r30)|0)==-1){HEAP32[r25]=0;r2=539;break}else{if(r29^(r30|0)==0){break}else{break L632}}}}while(0);if(r2==539){r2=0;if(r29){break}}r30=(r28+12|0)>>2;r31=HEAP32[r30];r32=r28+16|0;if((r31|0)==(HEAP32[r32>>2]|0)){r33=FUNCTION_TABLE[HEAP32[HEAP32[r28>>2]+36>>2]](r28)}else{r33=HEAPU8[r31]}if((__ZNSt3__19__num_getIcE17__stage2_int_loopEciPcRS2_RjcRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSD_S2_(r33&255,16,r21,r16,r19,0,r12,r23,r18,r22)|0)!=0){break}r31=HEAP32[r30];if((r31|0)==(HEAP32[r32>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r28>>2]+40>>2]](r28);r26=r28,r27=r26>>2;continue}else{HEAP32[r30]=r31+1|0;r26=r28,r27=r26>>2;continue}}HEAP8[r15+39|0]=0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);if((__ZNSt3__110__sscanf_lEPKcPvS1_z(r21,HEAP32[1311652],5243476,(tempInt=STACKTOP,STACKTOP=STACKTOP+4|0,HEAP32[tempInt>>2]=r7,tempInt))|0)!=1){HEAP32[r6>>2]=4}r26=HEAP32[r24],r27=r26>>2;do{if((r26|0)==0){r34=0}else{if((HEAP32[r27+3]|0)!=(HEAP32[r27+4]|0)){r34=r26;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r26)|0)!=-1){r34=r26;break}HEAP32[r24]=0;r34=0}}while(0);r24=(r34|0)==0;r26=HEAP32[r25],r27=r26>>2;do{if((r26|0)==0){r2=572}else{if((HEAP32[r27+3]|0)!=(HEAP32[r27+4]|0)){if(r24){break}else{r2=574;break}}if((FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r26)|0)==-1){HEAP32[r25]=0;r2=572;break}else{if(r24^(r26|0)==0){break}else{r2=574;break}}}}while(0);do{if(r2==572){if(r24){r2=574;break}else{break}}}while(0);if(r2==574){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r34;if((HEAP8[r13]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r12+8>>2]);STACKTOP=r8;return}}while(0);r8=___cxa_allocate_exception(4);HEAP32[r8>>2]=5247488;___cxa_throw(r8,5252700,514)}function __ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRl(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+332|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+104;r11=r8+108;r12=r8+120;r13=r8+160;r14=r8+164;r15=r8+324,r16=r15>>2;r17=r8+328;r18=HEAP32[r5+4>>2]&74;if((r18|0)==64){r19=8}else if((r18|0)==0){r19=0}else if((r18|0)==8){r19=16}else{r19=10}r18=r9|0;__ZNSt3__19__num_getIwE17__stage2_int_prepERNS_8ios_baseEPwRw(r11,r5,r18,r10);r5=r12|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r12=r14|0;HEAP32[r16]=r12;HEAP32[r17>>2]=0;r9=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP32[r10>>2];r10=HEAP32[r9],r20=r10>>2;L702:while(1){do{if((r10|0)==0){r21=0,r22=r21>>2}else{r23=HEAP32[r20+3];if((r23|0)==(HEAP32[r20+4]|0)){r24=FUNCTION_TABLE[HEAP32[HEAP32[r20]+36>>2]](r10)}else{r24=HEAP32[r23>>2]}if((r24|0)!=-1){r21=r10,r22=r21>>2;break}HEAP32[r9]=0;r21=0,r22=r21>>2}}while(0);r25=(r21|0)==0;r23=HEAP32[r3],r26=r23>>2;do{if((r23|0)==0){r2=600}else{r27=HEAP32[r26+3];if((r27|0)==(HEAP32[r26+4]|0)){r28=FUNCTION_TABLE[HEAP32[HEAP32[r26]+36>>2]](r23)}else{r28=HEAP32[r27>>2]}if((r28|0)==-1){HEAP32[r3]=0;r2=600;break}else{r27=(r23|0)==0;if(r25^r27){r29=r23;r30=r27;break}else{r31=r23,r32=r31>>2;r33=r27;break L702}}}}while(0);if(r2==600){r2=0;if(r25){r31=0,r32=r31>>2;r33=1;break}else{r29=0;r30=1}}r23=(r21+12|0)>>2;r26=HEAP32[r23];r27=r21+16|0;if((r26|0)==(HEAP32[r27>>2]|0)){r34=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r34=HEAP32[r26>>2]}if((__ZNSt3__19__num_getIwE17__stage2_int_loopEwiPcRS2_RjwRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSD_Pw(r34,r19,r5,r13,r17,r4,r11,r12,r15,r18)|0)!=0){r31=r29,r32=r31>>2;r33=r30;break}r26=HEAP32[r23];if((r26|0)==(HEAP32[r27>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r22]+40>>2]](r21);r10=r21,r20=r10>>2;continue}else{HEAP32[r23]=r26+4|0;r10=r21,r20=r10>>2;continue}}r10=r11;r20=HEAPU8[r10];if((r20&1|0)==0){r35=r20>>>1}else{r35=HEAP32[r11+4>>2]}do{if((r35|0)!=0){r20=HEAP32[r16];if((r20-r14|0)>=160){break}r30=HEAP32[r17>>2];HEAP32[r16]=r20+4|0;HEAP32[r20>>2]=r30}}while(0);HEAP32[r7>>2]=__ZNSt3__125__num_get_signed_integralIlEET_PKcS3_Rji(r5,HEAP32[r13>>2],r6,r19);__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r11,r12,HEAP32[r16],r6);do{if(r25){r36=0}else{r16=HEAP32[r22+3];if((r16|0)==(HEAP32[r22+4]|0)){r37=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r37=HEAP32[r16>>2]}if((r37|0)!=-1){r36=r21;break}HEAP32[r9]=0;r36=0}}while(0);r9=(r36|0)==0;do{if(r33){r2=632}else{r21=HEAP32[r32+3];if((r21|0)==(HEAP32[r32+4]|0)){r38=FUNCTION_TABLE[HEAP32[HEAP32[r32]+36>>2]](r31)}else{r38=HEAP32[r21>>2]}if((r38|0)==-1){HEAP32[r3]=0;r2=632;break}else{if(r9^(r31|0)==0){break}else{r2=634;break}}}}while(0);do{if(r2==632){if(r9){r2=634;break}else{break}}}while(0);if(r2==634){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r36;if((HEAP8[r10]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r11+8>>2]);STACKTOP=r8;return}function __ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRb(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26;r8=STACKTOP;STACKTOP=STACKTOP+68|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8,r10=r9>>2;r11=r8+12,r12=r11>>2;r13=r8+24;r14=r8+28;r15=r8+32;r16=r8+36;r17=r8+40;r18=r8+64;if((HEAP32[r5+4>>2]&1|0)==0){HEAP32[r13>>2]=-1;r19=HEAP32[HEAP32[r2>>2]+16>>2];r20=r3|0;HEAP32[r15>>2]=HEAP32[r20>>2];HEAP32[r16>>2]=HEAP32[r4>>2];FUNCTION_TABLE[r19](r14,r2,r15,r16,r5,r6,r13);r16=HEAP32[r14>>2];HEAP32[r20>>2]=r16;r20=HEAP32[r13>>2];if((r20|0)==0){HEAP8[r7]=0}else if((r20|0)==1){HEAP8[r7]=1}else{HEAP8[r7]=1;HEAP32[r6>>2]=4}HEAP32[r1>>2]=r16;STACKTOP=r8;return}r16=r5+28|0;r5=HEAP32[r16>>2],r20=r5>>2;r13=(r5+4|0)>>2;tempValue=HEAP32[r13],HEAP32[r13]=tempValue+1,tempValue;if((HEAP32[1313725]|0)!=-1){HEAP32[r12]=5254900;HEAP32[r12+1]=24;HEAP32[r12+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r11,406)}r11=HEAP32[1313726]-1|0;r12=HEAP32[r20+2];do{if(HEAP32[r20+3]-r12>>2>>>0>r11>>>0){r14=HEAP32[r12+(r11<<2)>>2];if((r14|0)==0){break}r15=r14;if(((tempValue=HEAP32[r13],HEAP32[r13]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r20]+8>>2]](r5)}r14=HEAP32[r16>>2],r2=r14>>2;r19=(r14+4|0)>>2;tempValue=HEAP32[r19],HEAP32[r19]=tempValue+1,tempValue;if((HEAP32[1313633]|0)!=-1){HEAP32[r10]=5254532;HEAP32[r10+1]=24;HEAP32[r10+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254532,r9,406)}r21=HEAP32[1313634]-1|0;r22=HEAP32[r2+2];do{if(HEAP32[r2+3]-r22>>2>>>0>r21>>>0){r23=HEAP32[r22+(r21<<2)>>2];if((r23|0)==0){break}r24=r23;if(((tempValue=HEAP32[r19],HEAP32[r19]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r2]+8>>2]](r14)}r25=r17|0;r26=r23;FUNCTION_TABLE[HEAP32[HEAP32[r26>>2]+24>>2]](r25,r24);FUNCTION_TABLE[HEAP32[HEAP32[r26>>2]+28>>2]](r17+12|0,r24);HEAP32[r18>>2]=HEAP32[r4>>2];HEAP8[r7]=(__ZNSt3__114__scan_keywordINS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEPKNS_12basic_stringIwS3_NS_9allocatorIwEEEENS_5ctypeIwEEEET0_RT_SE_SD_SD_RKT1_Rjb(r3,r18,r25,r17+24|0,r15,r6,1)|0)==(r25|0)&1;HEAP32[r1>>2]=HEAP32[r3>>2];if((HEAP8[r17+12|0]&1)<<24>>24!=0){__ZdlPv(HEAP32[r17+20>>2])}if((HEAP8[r17]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r17+8>>2]);STACKTOP=r8;return}}while(0);r15=___cxa_allocate_exception(4);HEAP32[r15>>2]=5247488;___cxa_throw(r15,5252700,514)}}while(0);r8=___cxa_allocate_exception(4);HEAP32[r8>>2]=5247488;___cxa_throw(r8,5252700,514)}function __ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRx(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+332|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+104;r11=r8+108;r12=r8+120;r13=r8+160;r14=r8+164;r15=r8+324,r16=r15>>2;r17=r8+328;r18=HEAP32[r5+4>>2]&74;if((r18|0)==8){r19=16}else if((r18|0)==64){r19=8}else if((r18|0)==0){r19=0}else{r19=10}r18=r9|0;__ZNSt3__19__num_getIwE17__stage2_int_prepERNS_8ios_baseEPwRw(r11,r5,r18,r10);r5=r12|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r12=r14|0;HEAP32[r16]=r12;HEAP32[r17>>2]=0;r9=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP32[r10>>2];r10=HEAP32[r9],r20=r10>>2;L816:while(1){do{if((r10|0)==0){r21=0,r22=r21>>2}else{r23=HEAP32[r20+3];if((r23|0)==(HEAP32[r20+4]|0)){r24=FUNCTION_TABLE[HEAP32[HEAP32[r20]+36>>2]](r10)}else{r24=HEAP32[r23>>2]}if((r24|0)!=-1){r21=r10,r22=r21>>2;break}HEAP32[r9]=0;r21=0,r22=r21>>2}}while(0);r25=(r21|0)==0;r23=HEAP32[r3],r26=r23>>2;do{if((r23|0)==0){r2=705}else{r27=HEAP32[r26+3];if((r27|0)==(HEAP32[r26+4]|0)){r28=FUNCTION_TABLE[HEAP32[HEAP32[r26]+36>>2]](r23)}else{r28=HEAP32[r27>>2]}if((r28|0)==-1){HEAP32[r3]=0;r2=705;break}else{r27=(r23|0)==0;if(r25^r27){r29=r23;r30=r27;break}else{r31=r23,r32=r31>>2;r33=r27;break L816}}}}while(0);if(r2==705){r2=0;if(r25){r31=0,r32=r31>>2;r33=1;break}else{r29=0;r30=1}}r23=(r21+12|0)>>2;r26=HEAP32[r23];r27=r21+16|0;if((r26|0)==(HEAP32[r27>>2]|0)){r34=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r34=HEAP32[r26>>2]}if((__ZNSt3__19__num_getIwE17__stage2_int_loopEwiPcRS2_RjwRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSD_Pw(r34,r19,r5,r13,r17,r4,r11,r12,r15,r18)|0)!=0){r31=r29,r32=r31>>2;r33=r30;break}r26=HEAP32[r23];if((r26|0)==(HEAP32[r27>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r22]+40>>2]](r21);r10=r21,r20=r10>>2;continue}else{HEAP32[r23]=r26+4|0;r10=r21,r20=r10>>2;continue}}r10=r11;r20=HEAPU8[r10];if((r20&1|0)==0){r35=r20>>>1}else{r35=HEAP32[r11+4>>2]}do{if((r35|0)!=0){r20=HEAP32[r16];if((r20-r14|0)>=160){break}r30=HEAP32[r17>>2];HEAP32[r16]=r20+4|0;HEAP32[r20>>2]=r30}}while(0);HEAP32[r7>>2]=__ZNSt3__125__num_get_signed_integralIxEET_PKcS3_Rji(r5,HEAP32[r13>>2],r6,r19);HEAP32[r7+4>>2]=tempRet0;__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r11,r12,HEAP32[r16],r6);do{if(r25){r36=0}else{r16=HEAP32[r22+3];if((r16|0)==(HEAP32[r22+4]|0)){r37=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r37=HEAP32[r16>>2]}if((r37|0)!=-1){r36=r21;break}HEAP32[r9]=0;r36=0}}while(0);r9=(r36|0)==0;do{if(r33){r2=737}else{r21=HEAP32[r32+3];if((r21|0)==(HEAP32[r32+4]|0)){r38=FUNCTION_TABLE[HEAP32[HEAP32[r32]+36>>2]](r31)}else{r38=HEAP32[r21>>2]}if((r38|0)==-1){HEAP32[r3]=0;r2=737;break}else{if(r9^(r31|0)==0){break}else{r2=739;break}}}}while(0);do{if(r2==737){if(r9){r2=739;break}else{break}}}while(0);if(r2==739){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r36;if((HEAP8[r10]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r11+8>>2]);STACKTOP=r8;return}function __ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRt(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+332|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+104;r11=r8+108;r12=r8+120;r13=r8+160;r14=r8+164;r15=r8+324,r16=r15>>2;r17=r8+328;r18=HEAP32[r5+4>>2]&74;if((r18|0)==8){r19=16}else if((r18|0)==0){r19=0}else if((r18|0)==64){r19=8}else{r19=10}r18=r9|0;__ZNSt3__19__num_getIwE17__stage2_int_prepERNS_8ios_baseEPwRw(r11,r5,r18,r10);r5=r12|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r12=r14|0;HEAP32[r16]=r12;HEAP32[r17>>2]=0;r9=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP32[r10>>2];r10=HEAP32[r9],r20=r10>>2;L888:while(1){do{if((r10|0)==0){r21=0,r22=r21>>2}else{r23=HEAP32[r20+3];if((r23|0)==(HEAP32[r20+4]|0)){r24=FUNCTION_TABLE[HEAP32[HEAP32[r20]+36>>2]](r10)}else{r24=HEAP32[r23>>2]}if((r24|0)!=-1){r21=r10,r22=r21>>2;break}HEAP32[r9]=0;r21=0,r22=r21>>2}}while(0);r25=(r21|0)==0;r23=HEAP32[r3],r26=r23>>2;do{if((r23|0)==0){r2=763}else{r27=HEAP32[r26+3];if((r27|0)==(HEAP32[r26+4]|0)){r28=FUNCTION_TABLE[HEAP32[HEAP32[r26]+36>>2]](r23)}else{r28=HEAP32[r27>>2]}if((r28|0)==-1){HEAP32[r3]=0;r2=763;break}else{r27=(r23|0)==0;if(r25^r27){r29=r23;r30=r27;break}else{r31=r23,r32=r31>>2;r33=r27;break L888}}}}while(0);if(r2==763){r2=0;if(r25){r31=0,r32=r31>>2;r33=1;break}else{r29=0;r30=1}}r23=(r21+12|0)>>2;r26=HEAP32[r23];r27=r21+16|0;if((r26|0)==(HEAP32[r27>>2]|0)){r34=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r34=HEAP32[r26>>2]}if((__ZNSt3__19__num_getIwE17__stage2_int_loopEwiPcRS2_RjwRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSD_Pw(r34,r19,r5,r13,r17,r4,r11,r12,r15,r18)|0)!=0){r31=r29,r32=r31>>2;r33=r30;break}r26=HEAP32[r23];if((r26|0)==(HEAP32[r27>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r22]+40>>2]](r21);r10=r21,r20=r10>>2;continue}else{HEAP32[r23]=r26+4|0;r10=r21,r20=r10>>2;continue}}r10=r11;r20=HEAPU8[r10];if((r20&1|0)==0){r35=r20>>>1}else{r35=HEAP32[r11+4>>2]}do{if((r35|0)!=0){r20=HEAP32[r16];if((r20-r14|0)>=160){break}r30=HEAP32[r17>>2];HEAP32[r16]=r20+4|0;HEAP32[r20>>2]=r30}}while(0);HEAP16[r7>>1]=__ZNSt3__127__num_get_unsigned_integralItEET_PKcS3_Rji(r5,HEAP32[r13>>2],r6,r19);__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r11,r12,HEAP32[r16],r6);do{if(r25){r36=0}else{r16=HEAP32[r22+3];if((r16|0)==(HEAP32[r22+4]|0)){r37=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r37=HEAP32[r16>>2]}if((r37|0)!=-1){r36=r21;break}HEAP32[r9]=0;r36=0}}while(0);r9=(r36|0)==0;do{if(r33){r2=795}else{r21=HEAP32[r32+3];if((r21|0)==(HEAP32[r32+4]|0)){r38=FUNCTION_TABLE[HEAP32[HEAP32[r32]+36>>2]](r31)}else{r38=HEAP32[r21>>2]}if((r38|0)==-1){HEAP32[r3]=0;r2=795;break}else{if(r9^(r31|0)==0){break}else{r2=797;break}}}}while(0);do{if(r2==795){if(r9){r2=797;break}else{break}}}while(0);if(r2==797){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r36;if((HEAP8[r10]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r11+8>>2]);STACKTOP=r8;return}function __ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjS8_(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+332|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+104;r11=r8+108;r12=r8+120;r13=r8+160;r14=r8+164;r15=r8+324,r16=r15>>2;r17=r8+328;r18=HEAP32[r5+4>>2]&74;if((r18|0)==0){r19=0}else if((r18|0)==64){r19=8}else if((r18|0)==8){r19=16}else{r19=10}r18=r9|0;__ZNSt3__19__num_getIwE17__stage2_int_prepERNS_8ios_baseEPwRw(r11,r5,r18,r10);r5=r12|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r12=r14|0;HEAP32[r16]=r12;HEAP32[r17>>2]=0;r9=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP32[r10>>2];r10=HEAP32[r9],r20=r10>>2;L960:while(1){do{if((r10|0)==0){r21=0,r22=r21>>2}else{r23=HEAP32[r20+3];if((r23|0)==(HEAP32[r20+4]|0)){r24=FUNCTION_TABLE[HEAP32[HEAP32[r20]+36>>2]](r10)}else{r24=HEAP32[r23>>2]}if((r24|0)!=-1){r21=r10,r22=r21>>2;break}HEAP32[r9]=0;r21=0,r22=r21>>2}}while(0);r25=(r21|0)==0;r23=HEAP32[r3],r26=r23>>2;do{if((r23|0)==0){r2=821}else{r27=HEAP32[r26+3];if((r27|0)==(HEAP32[r26+4]|0)){r28=FUNCTION_TABLE[HEAP32[HEAP32[r26]+36>>2]](r23)}else{r28=HEAP32[r27>>2]}if((r28|0)==-1){HEAP32[r3]=0;r2=821;break}else{r27=(r23|0)==0;if(r25^r27){r29=r23;r30=r27;break}else{r31=r23,r32=r31>>2;r33=r27;break L960}}}}while(0);if(r2==821){r2=0;if(r25){r31=0,r32=r31>>2;r33=1;break}else{r29=0;r30=1}}r23=(r21+12|0)>>2;r26=HEAP32[r23];r27=r21+16|0;if((r26|0)==(HEAP32[r27>>2]|0)){r34=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r34=HEAP32[r26>>2]}if((__ZNSt3__19__num_getIwE17__stage2_int_loopEwiPcRS2_RjwRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSD_Pw(r34,r19,r5,r13,r17,r4,r11,r12,r15,r18)|0)!=0){r31=r29,r32=r31>>2;r33=r30;break}r26=HEAP32[r23];if((r26|0)==(HEAP32[r27>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r22]+40>>2]](r21);r10=r21,r20=r10>>2;continue}else{HEAP32[r23]=r26+4|0;r10=r21,r20=r10>>2;continue}}r10=r11;r20=HEAPU8[r10];if((r20&1|0)==0){r35=r20>>>1}else{r35=HEAP32[r11+4>>2]}do{if((r35|0)!=0){r20=HEAP32[r16];if((r20-r14|0)>=160){break}r30=HEAP32[r17>>2];HEAP32[r16]=r20+4|0;HEAP32[r20>>2]=r30}}while(0);HEAP32[r7>>2]=__ZNSt3__127__num_get_unsigned_integralIjEET_PKcS3_Rji(r5,HEAP32[r13>>2],r6,r19);__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r11,r12,HEAP32[r16],r6);do{if(r25){r36=0}else{r16=HEAP32[r22+3];if((r16|0)==(HEAP32[r22+4]|0)){r37=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r37=HEAP32[r16>>2]}if((r37|0)!=-1){r36=r21;break}HEAP32[r9]=0;r36=0}}while(0);r9=(r36|0)==0;do{if(r33){r2=853}else{r21=HEAP32[r32+3];if((r21|0)==(HEAP32[r32+4]|0)){r38=FUNCTION_TABLE[HEAP32[HEAP32[r32]+36>>2]](r31)}else{r38=HEAP32[r21>>2]}if((r38|0)==-1){HEAP32[r3]=0;r2=853;break}else{if(r9^(r31|0)==0){break}else{r2=855;break}}}}while(0);do{if(r2==853){if(r9){r2=855;break}else{break}}}while(0);if(r2==855){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r36;if((HEAP8[r10]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r11+8>>2]);STACKTOP=r8;return}function __ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRm(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+332|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+104;r11=r8+108;r12=r8+120;r13=r8+160;r14=r8+164;r15=r8+324,r16=r15>>2;r17=r8+328;r18=HEAP32[r5+4>>2]&74;if((r18|0)==0){r19=0}else if((r18|0)==64){r19=8}else if((r18|0)==8){r19=16}else{r19=10}r18=r9|0;__ZNSt3__19__num_getIwE17__stage2_int_prepERNS_8ios_baseEPwRw(r11,r5,r18,r10);r5=r12|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r12=r14|0;HEAP32[r16]=r12;HEAP32[r17>>2]=0;r9=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP32[r10>>2];r10=HEAP32[r9],r20=r10>>2;L1032:while(1){do{if((r10|0)==0){r21=0,r22=r21>>2}else{r23=HEAP32[r20+3];if((r23|0)==(HEAP32[r20+4]|0)){r24=FUNCTION_TABLE[HEAP32[HEAP32[r20]+36>>2]](r10)}else{r24=HEAP32[r23>>2]}if((r24|0)!=-1){r21=r10,r22=r21>>2;break}HEAP32[r9]=0;r21=0,r22=r21>>2}}while(0);r25=(r21|0)==0;r23=HEAP32[r3],r26=r23>>2;do{if((r23|0)==0){r2=879}else{r27=HEAP32[r26+3];if((r27|0)==(HEAP32[r26+4]|0)){r28=FUNCTION_TABLE[HEAP32[HEAP32[r26]+36>>2]](r23)}else{r28=HEAP32[r27>>2]}if((r28|0)==-1){HEAP32[r3]=0;r2=879;break}else{r27=(r23|0)==0;if(r25^r27){r29=r23;r30=r27;break}else{r31=r23,r32=r31>>2;r33=r27;break L1032}}}}while(0);if(r2==879){r2=0;if(r25){r31=0,r32=r31>>2;r33=1;break}else{r29=0;r30=1}}r23=(r21+12|0)>>2;r26=HEAP32[r23];r27=r21+16|0;if((r26|0)==(HEAP32[r27>>2]|0)){r34=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r34=HEAP32[r26>>2]}if((__ZNSt3__19__num_getIwE17__stage2_int_loopEwiPcRS2_RjwRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSD_Pw(r34,r19,r5,r13,r17,r4,r11,r12,r15,r18)|0)!=0){r31=r29,r32=r31>>2;r33=r30;break}r26=HEAP32[r23];if((r26|0)==(HEAP32[r27>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r22]+40>>2]](r21);r10=r21,r20=r10>>2;continue}else{HEAP32[r23]=r26+4|0;r10=r21,r20=r10>>2;continue}}r10=r11;r20=HEAPU8[r10];if((r20&1|0)==0){r35=r20>>>1}else{r35=HEAP32[r11+4>>2]}do{if((r35|0)!=0){r20=HEAP32[r16];if((r20-r14|0)>=160){break}r30=HEAP32[r17>>2];HEAP32[r16]=r20+4|0;HEAP32[r20>>2]=r30}}while(0);HEAP32[r7>>2]=__ZNSt3__127__num_get_unsigned_integralImEET_PKcS3_Rji(r5,HEAP32[r13>>2],r6,r19);__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r11,r12,HEAP32[r16],r6);do{if(r25){r36=0}else{r16=HEAP32[r22+3];if((r16|0)==(HEAP32[r22+4]|0)){r37=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r37=HEAP32[r16>>2]}if((r37|0)!=-1){r36=r21;break}HEAP32[r9]=0;r36=0}}while(0);r9=(r36|0)==0;do{if(r33){r2=911}else{r21=HEAP32[r32+3];if((r21|0)==(HEAP32[r32+4]|0)){r38=FUNCTION_TABLE[HEAP32[HEAP32[r32]+36>>2]](r31)}else{r38=HEAP32[r21>>2]}if((r38|0)==-1){HEAP32[r3]=0;r2=911;break}else{if(r9^(r31|0)==0){break}else{r2=913;break}}}}while(0);do{if(r2==911){if(r9){r2=913;break}else{break}}}while(0);if(r2==913){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r36;if((HEAP8[r10]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r11+8>>2]);STACKTOP=r8;return}function __ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRy(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+332|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+104;r11=r8+108;r12=r8+120;r13=r8+160;r14=r8+164;r15=r8+324,r16=r15>>2;r17=r8+328;r18=HEAP32[r5+4>>2]&74;if((r18|0)==0){r19=0}else if((r18|0)==8){r19=16}else if((r18|0)==64){r19=8}else{r19=10}r18=r9|0;__ZNSt3__19__num_getIwE17__stage2_int_prepERNS_8ios_baseEPwRw(r11,r5,r18,r10);r5=r12|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r12=r14|0;HEAP32[r16]=r12;HEAP32[r17>>2]=0;r9=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP32[r10>>2];r10=HEAP32[r9],r20=r10>>2;L1104:while(1){do{if((r10|0)==0){r21=0,r22=r21>>2}else{r23=HEAP32[r20+3];if((r23|0)==(HEAP32[r20+4]|0)){r24=FUNCTION_TABLE[HEAP32[HEAP32[r20]+36>>2]](r10)}else{r24=HEAP32[r23>>2]}if((r24|0)!=-1){r21=r10,r22=r21>>2;break}HEAP32[r9]=0;r21=0,r22=r21>>2}}while(0);r25=(r21|0)==0;r23=HEAP32[r3],r26=r23>>2;do{if((r23|0)==0){r2=937}else{r27=HEAP32[r26+3];if((r27|0)==(HEAP32[r26+4]|0)){r28=FUNCTION_TABLE[HEAP32[HEAP32[r26]+36>>2]](r23)}else{r28=HEAP32[r27>>2]}if((r28|0)==-1){HEAP32[r3]=0;r2=937;break}else{r27=(r23|0)==0;if(r25^r27){r29=r23;r30=r27;break}else{r31=r23,r32=r31>>2;r33=r27;break L1104}}}}while(0);if(r2==937){r2=0;if(r25){r31=0,r32=r31>>2;r33=1;break}else{r29=0;r30=1}}r23=(r21+12|0)>>2;r26=HEAP32[r23];r27=r21+16|0;if((r26|0)==(HEAP32[r27>>2]|0)){r34=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r34=HEAP32[r26>>2]}if((__ZNSt3__19__num_getIwE17__stage2_int_loopEwiPcRS2_RjwRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSD_Pw(r34,r19,r5,r13,r17,r4,r11,r12,r15,r18)|0)!=0){r31=r29,r32=r31>>2;r33=r30;break}r26=HEAP32[r23];if((r26|0)==(HEAP32[r27>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r22]+40>>2]](r21);r10=r21,r20=r10>>2;continue}else{HEAP32[r23]=r26+4|0;r10=r21,r20=r10>>2;continue}}r10=r11;r20=HEAPU8[r10];if((r20&1|0)==0){r35=r20>>>1}else{r35=HEAP32[r11+4>>2]}do{if((r35|0)!=0){r20=HEAP32[r16];if((r20-r14|0)>=160){break}r30=HEAP32[r17>>2];HEAP32[r16]=r20+4|0;HEAP32[r20>>2]=r30}}while(0);HEAP32[r7>>2]=__ZNSt3__127__num_get_unsigned_integralIyEET_PKcS3_Rji(r5,HEAP32[r13>>2],r6,r19);HEAP32[r7+4>>2]=tempRet0;__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r11,r12,HEAP32[r16],r6);do{if(r25){r36=0}else{r16=HEAP32[r22+3];if((r16|0)==(HEAP32[r22+4]|0)){r37=FUNCTION_TABLE[HEAP32[HEAP32[r22]+36>>2]](r21)}else{r37=HEAP32[r16>>2]}if((r37|0)!=-1){r36=r21;break}HEAP32[r9]=0;r36=0}}while(0);r9=(r36|0)==0;do{if(r33){r2=969}else{r21=HEAP32[r32+3];if((r21|0)==(HEAP32[r32+4]|0)){r38=FUNCTION_TABLE[HEAP32[HEAP32[r32]+36>>2]](r31)}else{r38=HEAP32[r21>>2]}if((r38|0)==-1){HEAP32[r3]=0;r2=969;break}else{if(r9^(r31|0)==0){break}else{r2=971;break}}}}while(0);do{if(r2==969){if(r9){r2=971;break}else{break}}}while(0);if(r2==971){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r36;if((HEAP8[r10]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r11+8>>2]);STACKTOP=r8;return}function __ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRf(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+372|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+132;r11=r8+136;r12=r8+140;r13=r8+192;r14=r8+196;r15=r8+356,r16=r15>>2;r17=r8+360;r18=r8+364;r19=r8+368;r20=r8+4|0;__ZNSt3__19__num_getIwE19__stage2_float_prepERNS_8ios_baseEPwRwS5_(r12,r5,r20,r10,r11);r5=r8+152|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r21=r14|0;HEAP32[r16]=r21;HEAP32[r17>>2]=0;HEAP8[r18]=1;HEAP8[r19]=69;r22=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP32[r10>>2];r10=HEAP32[r11>>2];r11=HEAP32[r22],r23=r11>>2;L1171:while(1){do{if((r11|0)==0){r24=0}else{r25=HEAP32[r23+3];if((r25|0)==(HEAP32[r23+4]|0)){r26=FUNCTION_TABLE[HEAP32[HEAP32[r23]+36>>2]](r11)}else{r26=HEAP32[r25>>2]}if((r26|0)!=-1){r24=r11;break}HEAP32[r22]=0;r24=0}}while(0);r25=(r24|0)==0;r27=HEAP32[r3],r28=r27>>2;do{if((r27|0)==0){r2=991}else{r29=HEAP32[r28+3];if((r29|0)==(HEAP32[r28+4]|0)){r30=FUNCTION_TABLE[HEAP32[HEAP32[r28]+36>>2]](r27)}else{r30=HEAP32[r29>>2]}if((r30|0)==-1){HEAP32[r3]=0;r2=991;break}else{if(r25^(r27|0)==0){break}else{break L1171}}}}while(0);if(r2==991){r2=0;if(r25){break}}r27=(r24+12|0)>>2;r28=HEAP32[r27];r29=r24+16|0;if((r28|0)==(HEAP32[r29>>2]|0)){r31=FUNCTION_TABLE[HEAP32[HEAP32[r24>>2]+36>>2]](r24)}else{r31=HEAP32[r28>>2]}if((__ZNSt3__19__num_getIwE19__stage2_float_loopEwRbRcPcRS4_wwRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSE_RjPw(r31,r18,r19,r5,r13,r4,r10,r12,r21,r15,r17,r20)|0)!=0){break}r28=HEAP32[r27];if((r28|0)==(HEAP32[r29>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r24>>2]+40>>2]](r24);r11=r24,r23=r11>>2;continue}else{HEAP32[r27]=r28+4|0;r11=r24,r23=r11>>2;continue}}r11=r12;r23=HEAPU8[r11];if((r23&1|0)==0){r32=r23>>>1}else{r32=HEAP32[r12+4>>2]}do{if((r32|0)!=0){if((HEAP8[r18]&1)<<24>>24==0){break}r23=HEAP32[r16];if((r23-r14|0)>=160){break}r24=HEAP32[r17>>2];HEAP32[r16]=r23+4|0;HEAP32[r23>>2]=r24}}while(0);r17=HEAP32[r13>>2];do{if((r5|0)==(r17|0)){HEAP32[r6>>2]=4;r33=0}else{do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r25=_strtod(r5,r9);if((HEAP32[r9>>2]|0)==(r17|0)){r33=r25;break}else{HEAP32[r6>>2]=4;r33=0;break}}}while(0);HEAPF32[r7>>2]=r33;__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r12,r21,HEAP32[r16],r6);r16=HEAP32[r22],r21=r16>>2;do{if((r16|0)==0){r34=0}else{r33=HEAP32[r21+3];if((r33|0)==(HEAP32[r21+4]|0)){r35=FUNCTION_TABLE[HEAP32[HEAP32[r21]+36>>2]](r16)}else{r35=HEAP32[r33>>2]}if((r35|0)!=-1){r34=r16;break}HEAP32[r22]=0;r34=0}}while(0);r22=(r34|0)==0;r16=HEAP32[r3],r35=r16>>2;do{if((r16|0)==0){r2=1033}else{r21=HEAP32[r35+3];if((r21|0)==(HEAP32[r35+4]|0)){r36=FUNCTION_TABLE[HEAP32[HEAP32[r35]+36>>2]](r16)}else{r36=HEAP32[r21>>2]}if((r36|0)==-1){HEAP32[r3]=0;r2=1033;break}else{if(r22^(r16|0)==0){break}else{r2=1035;break}}}}while(0);do{if(r2==1033){if(r22){r2=1035;break}else{break}}}while(0);if(r2==1035){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r34;if((HEAP8[r11]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r12+8>>2]);STACKTOP=r8;return}function __ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRd(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+372|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+132;r11=r8+136;r12=r8+140;r13=r8+192;r14=r8+196;r15=r8+356,r16=r15>>2;r17=r8+360;r18=r8+364;r19=r8+368;r20=r8+4|0;__ZNSt3__19__num_getIwE19__stage2_float_prepERNS_8ios_baseEPwRwS5_(r12,r5,r20,r10,r11);r5=r8+152|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r21=r14|0;HEAP32[r16]=r21;HEAP32[r17>>2]=0;HEAP8[r18]=1;HEAP8[r19]=69;r22=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP32[r10>>2];r10=HEAP32[r11>>2];r11=HEAP32[r22],r23=r11>>2;L1250:while(1){do{if((r11|0)==0){r24=0}else{r25=HEAP32[r23+3];if((r25|0)==(HEAP32[r23+4]|0)){r26=FUNCTION_TABLE[HEAP32[HEAP32[r23]+36>>2]](r11)}else{r26=HEAP32[r25>>2]}if((r26|0)!=-1){r24=r11;break}HEAP32[r22]=0;r24=0}}while(0);r25=(r24|0)==0;r27=HEAP32[r3],r28=r27>>2;do{if((r27|0)==0){r2=1055}else{r29=HEAP32[r28+3];if((r29|0)==(HEAP32[r28+4]|0)){r30=FUNCTION_TABLE[HEAP32[HEAP32[r28]+36>>2]](r27)}else{r30=HEAP32[r29>>2]}if((r30|0)==-1){HEAP32[r3]=0;r2=1055;break}else{if(r25^(r27|0)==0){break}else{break L1250}}}}while(0);if(r2==1055){r2=0;if(r25){break}}r27=(r24+12|0)>>2;r28=HEAP32[r27];r29=r24+16|0;if((r28|0)==(HEAP32[r29>>2]|0)){r31=FUNCTION_TABLE[HEAP32[HEAP32[r24>>2]+36>>2]](r24)}else{r31=HEAP32[r28>>2]}if((__ZNSt3__19__num_getIwE19__stage2_float_loopEwRbRcPcRS4_wwRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSE_RjPw(r31,r18,r19,r5,r13,r4,r10,r12,r21,r15,r17,r20)|0)!=0){break}r28=HEAP32[r27];if((r28|0)==(HEAP32[r29>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r24>>2]+40>>2]](r24);r11=r24,r23=r11>>2;continue}else{HEAP32[r27]=r28+4|0;r11=r24,r23=r11>>2;continue}}r11=r12;r23=HEAPU8[r11];if((r23&1|0)==0){r32=r23>>>1}else{r32=HEAP32[r12+4>>2]}do{if((r32|0)!=0){if((HEAP8[r18]&1)<<24>>24==0){break}r23=HEAP32[r16];if((r23-r14|0)>=160){break}r24=HEAP32[r17>>2];HEAP32[r16]=r23+4|0;HEAP32[r23>>2]=r24}}while(0);r17=HEAP32[r13>>2];do{if((r5|0)==(r17|0)){HEAP32[r6>>2]=4;r33=0}else{do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r25=_strtod(r5,r9);if((HEAP32[r9>>2]|0)==(r17|0)){r33=r25;break}HEAP32[r6>>2]=4;r33=0}}while(0);HEAPF64[tempDoublePtr>>3]=r33,HEAP32[r7>>2]=HEAP32[tempDoublePtr>>2],HEAP32[r7+4>>2]=HEAP32[tempDoublePtr+4>>2];__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r12,r21,HEAP32[r16],r6);r16=HEAP32[r22],r21=r16>>2;do{if((r16|0)==0){r34=0}else{r7=HEAP32[r21+3];if((r7|0)==(HEAP32[r21+4]|0)){r35=FUNCTION_TABLE[HEAP32[HEAP32[r21]+36>>2]](r16)}else{r35=HEAP32[r7>>2]}if((r35|0)!=-1){r34=r16;break}HEAP32[r22]=0;r34=0}}while(0);r22=(r34|0)==0;r16=HEAP32[r3],r35=r16>>2;do{if((r16|0)==0){r2=1096}else{r21=HEAP32[r35+3];if((r21|0)==(HEAP32[r35+4]|0)){r36=FUNCTION_TABLE[HEAP32[HEAP32[r35]+36>>2]](r16)}else{r36=HEAP32[r21>>2]}if((r36|0)==-1){HEAP32[r3]=0;r2=1096;break}else{if(r22^(r16|0)==0){break}else{r2=1098;break}}}}while(0);do{if(r2==1096){if(r22){r2=1098;break}else{break}}}while(0);if(r2==1098){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r34;if((HEAP8[r11]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r12+8>>2]);STACKTOP=r8;return}function __ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRe(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+372|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8;r10=r8+132;r11=r8+136;r12=r8+140;r13=r8+192;r14=r8+196;r15=r8+356,r16=r15>>2;r17=r8+360;r18=r8+364;r19=r8+368;r20=r8+4|0;__ZNSt3__19__num_getIwE19__stage2_float_prepERNS_8ios_baseEPwRwS5_(r12,r5,r20,r10,r11);r5=r8+152|0;_memset(r5,0,40);HEAP32[r13>>2]=r5;r21=r14|0;HEAP32[r16]=r21;HEAP32[r17>>2]=0;HEAP8[r18]=1;HEAP8[r19]=69;r22=(r3|0)>>2;r3=(r4|0)>>2;r4=HEAP32[r10>>2];r10=HEAP32[r11>>2];r11=HEAP32[r22],r23=r11>>2;L1327:while(1){do{if((r11|0)==0){r24=0}else{r25=HEAP32[r23+3];if((r25|0)==(HEAP32[r23+4]|0)){r26=FUNCTION_TABLE[HEAP32[HEAP32[r23]+36>>2]](r11)}else{r26=HEAP32[r25>>2]}if((r26|0)!=-1){r24=r11;break}HEAP32[r22]=0;r24=0}}while(0);r25=(r24|0)==0;r27=HEAP32[r3],r28=r27>>2;do{if((r27|0)==0){r2=1118}else{r29=HEAP32[r28+3];if((r29|0)==(HEAP32[r28+4]|0)){r30=FUNCTION_TABLE[HEAP32[HEAP32[r28]+36>>2]](r27)}else{r30=HEAP32[r29>>2]}if((r30|0)==-1){HEAP32[r3]=0;r2=1118;break}else{if(r25^(r27|0)==0){break}else{break L1327}}}}while(0);if(r2==1118){r2=0;if(r25){break}}r27=(r24+12|0)>>2;r28=HEAP32[r27];r29=r24+16|0;if((r28|0)==(HEAP32[r29>>2]|0)){r31=FUNCTION_TABLE[HEAP32[HEAP32[r24>>2]+36>>2]](r24)}else{r31=HEAP32[r28>>2]}if((__ZNSt3__19__num_getIwE19__stage2_float_loopEwRbRcPcRS4_wwRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSE_RjPw(r31,r18,r19,r5,r13,r4,r10,r12,r21,r15,r17,r20)|0)!=0){break}r28=HEAP32[r27];if((r28|0)==(HEAP32[r29>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r24>>2]+40>>2]](r24);r11=r24,r23=r11>>2;continue}else{HEAP32[r27]=r28+4|0;r11=r24,r23=r11>>2;continue}}r11=r12;r23=HEAPU8[r11];if((r23&1|0)==0){r32=r23>>>1}else{r32=HEAP32[r12+4>>2]}do{if((r32|0)!=0){if((HEAP8[r18]&1)<<24>>24==0){break}r23=HEAP32[r16];if((r23-r14|0)>=160){break}r24=HEAP32[r17>>2];HEAP32[r16]=r23+4|0;HEAP32[r23>>2]=r24}}while(0);r17=HEAP32[r13>>2];do{if((r5|0)==(r17|0)){HEAP32[r6>>2]=4;r33=1.1125369292536007e-308}else{do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r25=_strtod(r5,r9);if((HEAP32[r9>>2]|0)==(r17|0)){r33=r25;break}else{HEAP32[r6>>2]=4;r33=1.1125369292536007e-308;break}}}while(0);HEAPF64[tempDoublePtr>>3]=r33,HEAP32[r7>>2]=HEAP32[tempDoublePtr>>2],HEAP32[r7+4>>2]=HEAP32[tempDoublePtr+4>>2];__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r12,r21,HEAP32[r16],r6);r16=HEAP32[r22],r21=r16>>2;do{if((r16|0)==0){r34=0}else{r7=HEAP32[r21+3];if((r7|0)==(HEAP32[r21+4]|0)){r35=FUNCTION_TABLE[HEAP32[HEAP32[r21]+36>>2]](r16)}else{r35=HEAP32[r7>>2]}if((r35|0)!=-1){r34=r16;break}HEAP32[r22]=0;r34=0}}while(0);r22=(r34|0)==0;r16=HEAP32[r3],r35=r16>>2;do{if((r16|0)==0){r2=1160}else{r21=HEAP32[r35+3];if((r21|0)==(HEAP32[r35+4]|0)){r36=FUNCTION_TABLE[HEAP32[HEAP32[r35]+36>>2]](r16)}else{r36=HEAP32[r21>>2]}if((r36|0)==-1){HEAP32[r3]=0;r2=1160;break}else{if(r22^(r16|0)==0){break}else{r2=1162;break}}}}while(0);do{if(r2==1160){if(r22){r2=1162;break}else{break}}}while(0);if(r2==1162){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r34;if((HEAP8[r11]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r12+8>>2]);STACKTOP=r8;return}function __ZNSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEED1Ev(r1){return}function __ZNSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEED0Ev(r1){__ZdlPv(r1);return}function __ZNKSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEcl(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26;r2=0;r7=STACKTOP;STACKTOP=STACKTOP+60|0;r8=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r8>>2];r8=r7;r9=r7+8;r10=r7+20;r11=r7+44;r12=r7+48;r13=r7+52;r14=r7+56;r15=r8|0;HEAP8[r15]=HEAP8[5247320];HEAP8[r15+1|0]=HEAP8[5247321|0];HEAP8[r15+2|0]=HEAP8[5247322|0];HEAP8[r15+3|0]=HEAP8[5247323|0];HEAP8[r15+4|0]=HEAP8[5247324|0];HEAP8[r15+5|0]=HEAP8[5247325|0];r16=r8+1|0;r17=r4+4|0;r18=HEAP32[r17>>2];if((r18&2048|0)==0){r19=r16}else{HEAP8[r16]=43;r19=r8+2|0}if((r18&512|0)==0){r20=r19}else{HEAP8[r19]=35;r20=r19+1|0}HEAP8[r20]=108;r19=r20+1|0;r20=r18&74;do{if((r20|0)==8){if((r18&16384|0)==0){HEAP8[r19]=120;break}else{HEAP8[r19]=88;break}}else if((r20|0)==64){HEAP8[r19]=111}else{HEAP8[r19]=100}}while(0);r19=r9|0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r20=__ZNSt3__111__sprintf_lEPcPvPKcz(r19,HEAP32[1311652],r15,(tempInt=STACKTOP,STACKTOP=STACKTOP+4|0,HEAP32[tempInt>>2]=r6,tempInt));r6=r9+r20|0;r15=HEAP32[r17>>2]&176;do{if((r15|0)==32){r21=r6}else if((r15|0)==16){r17=HEAP8[r19];if(r17<<24>>24==45|r17<<24>>24==43){r21=r9+1|0;break}if(!((r20|0)>1&r17<<24>>24==48)){r2=1192;break}r17=HEAP8[r9+1|0];if(!(r17<<24>>24==120|r17<<24>>24==88)){r2=1192;break}r21=r9+2|0;break}else{r2=1192}}while(0);if(r2==1192){r21=r19}r2=r10|0;r10=r13|0;r9=HEAP32[r4+28>>2];HEAP32[r10>>2]=r9;r20=r9+4|0;tempValue=HEAP32[r20>>2],HEAP32[r20>>2]=tempValue+1,tempValue;__ZNSt3__19__num_putIcE21__widen_and_group_intEPcS2_S2_S2_RS2_S3_RKNS_6localeE(r19,r21,r6,r2,r11,r12,r13);r13=HEAP32[r10>>2];r10=r13+4|0;if(((tempValue=HEAP32[r10>>2],HEAP32[r10>>2]=tempValue+ -1,tempValue)|0)!=0){r22=r3|0;r23=HEAP32[r22>>2];r24=r14|0;HEAP32[r24>>2]=r23;r25=HEAP32[r11>>2];r26=HEAP32[r12>>2];__ZNSt3__116__pad_and_outputIcNS_11char_traitsIcEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r14,r2,r25,r26,r4,r5);STACKTOP=r7;return}FUNCTION_TABLE[HEAP32[HEAP32[r13>>2]+8>>2]](r13|0);r22=r3|0;r23=HEAP32[r22>>2];r24=r14|0;HEAP32[r24>>2]=r23;r25=HEAP32[r11>>2];r26=HEAP32[r12>>2];__ZNSt3__116__pad_and_outputIcNS_11char_traitsIcEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r14,r2,r25,r26,r4,r5);STACKTOP=r7;return}function __ZNKSt3__17num_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_RNS_8ios_baseERjRPv(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+128|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r9>>2];r9=r8,r10=r9>>2;r11=r8+12;r12=r8+116;r13=r12,r14=r13>>2;r15=STACKTOP;STACKTOP=STACKTOP+40|0;r16=STACKTOP;STACKTOP=STACKTOP+4|0;r17=STACKTOP;STACKTOP=STACKTOP+160|0;r18=STACKTOP;STACKTOP=STACKTOP+4|0;r19=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r14]=0;HEAP32[r14+1]=0;HEAP32[r14+2]=0;r14=HEAP32[r5+28>>2],r5=r14>>2;r20=(r14+4|0)>>2;tempValue=HEAP32[r20],HEAP32[r20]=tempValue+1,tempValue;if((HEAP32[1313725]|0)!=-1){HEAP32[r10]=5254900;HEAP32[r10+1]=24;HEAP32[r10+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r9,406)}r9=HEAP32[1313726]-1|0;r10=HEAP32[r5+2];do{if(HEAP32[r5+3]-r10>>2>>>0>r9>>>0){r21=HEAP32[r10+(r9<<2)>>2];if((r21|0)==0){break}r22=r11|0;FUNCTION_TABLE[HEAP32[HEAP32[r21>>2]+48>>2]](r21,5255348,5255374,r22);if(((tempValue=HEAP32[r20],HEAP32[r20]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r5]+8>>2]](r14)}r21=r15|0;_memset(r21,0,40);HEAP32[r16>>2]=r21;r23=r17|0;HEAP32[r18>>2]=r23;HEAP32[r19>>2]=0;r24=(r3|0)>>2;r25=(r4|0)>>2;r26=HEAP32[r24],r27=r26>>2;L1453:while(1){do{if((r26|0)==0){r28=0}else{r29=HEAP32[r27+3];if((r29|0)==(HEAP32[r27+4]|0)){r30=FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r26)}else{r30=HEAP32[r29>>2]}if((r30|0)!=-1){r28=r26;break}HEAP32[r24]=0;r28=0}}while(0);r29=(r28|0)==0;r31=HEAP32[r25],r32=r31>>2;do{if((r31|0)==0){r2=1224}else{r33=HEAP32[r32+3];if((r33|0)==(HEAP32[r32+4]|0)){r34=FUNCTION_TABLE[HEAP32[HEAP32[r32]+36>>2]](r31)}else{r34=HEAP32[r33>>2]}if((r34|0)==-1){HEAP32[r25]=0;r2=1224;break}else{if(r29^(r31|0)==0){break}else{break L1453}}}}while(0);if(r2==1224){r2=0;if(r29){break}}r31=(r28+12|0)>>2;r32=HEAP32[r31];r33=r28+16|0;if((r32|0)==(HEAP32[r33>>2]|0)){r35=FUNCTION_TABLE[HEAP32[HEAP32[r28>>2]+36>>2]](r28)}else{r35=HEAP32[r32>>2]}if((__ZNSt3__19__num_getIwE17__stage2_int_loopEwiPcRS2_RjwRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSD_Pw(r35,16,r21,r16,r19,0,r12,r23,r18,r22)|0)!=0){break}r32=HEAP32[r31];if((r32|0)==(HEAP32[r33>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r28>>2]+40>>2]](r28);r26=r28,r27=r26>>2;continue}else{HEAP32[r31]=r32+4|0;r26=r28,r27=r26>>2;continue}}HEAP8[r15+39|0]=0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);if((__ZNSt3__110__sscanf_lEPKcPvS1_z(r21,HEAP32[1311652],5243476,(tempInt=STACKTOP,STACKTOP=STACKTOP+4|0,HEAP32[tempInt>>2]=r7,tempInt))|0)!=1){HEAP32[r6>>2]=4}r26=HEAP32[r24],r27=r26>>2;do{if((r26|0)==0){r36=0}else{r22=HEAP32[r27+3];if((r22|0)==(HEAP32[r27+4]|0)){r37=FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r26)}else{r37=HEAP32[r22>>2]}if((r37|0)!=-1){r36=r26;break}HEAP32[r24]=0;r36=0}}while(0);r24=(r36|0)==0;r26=HEAP32[r25],r27=r26>>2;do{if((r26|0)==0){r2=1258}else{r21=HEAP32[r27+3];if((r21|0)==(HEAP32[r27+4]|0)){r38=FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r26)}else{r38=HEAP32[r21>>2]}if((r38|0)==-1){HEAP32[r25]=0;r2=1258;break}else{if(r24^(r26|0)==0){break}else{r2=1260;break}}}}while(0);do{if(r2==1258){if(r24){r2=1260;break}else{break}}}while(0);if(r2==1260){HEAP32[r6>>2]=HEAP32[r6>>2]|2}HEAP32[r1>>2]=r36;if((HEAP8[r13]&1)<<24>>24==0){STACKTOP=r8;return}__ZdlPv(HEAP32[r12+8>>2]);STACKTOP=r8;return}}while(0);r8=___cxa_allocate_exception(4);HEAP32[r8>>2]=5247488;___cxa_throw(r8,5252700,514)}function __ZNKSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEcb(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24;r7=STACKTOP;STACKTOP=STACKTOP+28|0;r8=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r8>>2];r8=r7,r9=r8>>2;r10=r7+12;r11=r7+16;if((HEAP32[r4+4>>2]&1|0)==0){r12=HEAP32[HEAP32[r2>>2]+24>>2];HEAP32[r10>>2]=HEAP32[r3>>2];FUNCTION_TABLE[r12](r1,r2,r10,r4,r5,r6&1);STACKTOP=r7;return}r5=HEAP32[r4+28>>2],r4=r5>>2;r10=(r5+4|0)>>2;tempValue=HEAP32[r10],HEAP32[r10]=tempValue+1,tempValue;if((HEAP32[1313635]|0)!=-1){HEAP32[r9]=5254540;HEAP32[r9+1]=24;HEAP32[r9+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254540,r8,406)}r8=HEAP32[1313636]-1|0;r9=HEAP32[r4+2];do{if(HEAP32[r4+3]-r9>>2>>>0>r8>>>0){r2=HEAP32[r9+(r8<<2)>>2];if((r2|0)==0){break}r12=r2;if(((tempValue=HEAP32[r10],HEAP32[r10]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r4]+8>>2]](r5)}r13=HEAP32[r2>>2];if(r6){FUNCTION_TABLE[HEAP32[r13+24>>2]](r11,r12)}else{FUNCTION_TABLE[HEAP32[r13+28>>2]](r11,r12)}r12=r11;r13=r11;r2=HEAP8[r13];if((r2&1)<<24>>24==0){r14=r12+1|0;r15=r14;r16=r14;r17=r11+8|0}else{r14=r11+8|0;r15=HEAP32[r14>>2];r16=r12+1|0;r17=r14}r14=(r3|0)>>2;r12=r11+4|0;r18=r15;r19=r2;while(1){r20=(r19&1)<<24>>24==0;if(r20){r21=r16}else{r21=HEAP32[r17>>2]}r2=r19&255;if((r18|0)==(r21+((r2&1|0)==0?r2>>>1:HEAP32[r12>>2])|0)){break}r2=HEAP8[r18];r22=HEAP32[r14];do{if((r22|0)!=0){r23=r22+24|0;r24=HEAP32[r23>>2];if((r24|0)!=(HEAP32[r22+28>>2]|0)){HEAP32[r23>>2]=r24+1|0;HEAP8[r24]=r2;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r22>>2]+52>>2]](r22,r2&255)|0)!=-1){break}HEAP32[r14]=0}}while(0);r18=r18+1|0;r19=HEAP8[r13]}HEAP32[r1>>2]=HEAP32[r14];if(r20){STACKTOP=r7;return}__ZdlPv(HEAP32[r17>>2]);STACKTOP=r7;return}}while(0);r7=___cxa_allocate_exception(4);HEAP32[r7>>2]=5247488;___cxa_throw(r7,5252700,514)}function __ZNKSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEcx(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+92|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r8;r10=r8+8;r11=r8+32;r12=r8+76;r13=r8+80;r14=r8+84;r15=r8+88;HEAP32[r9>>2]=37;HEAP32[r9+4>>2]=0;r16=r9;r9=r16+1|0;r17=r4+4|0;r18=HEAP32[r17>>2];if((r18&2048|0)==0){r19=r9}else{HEAP8[r9]=43;r19=r16+2|0}if((r18&512|0)==0){r20=r19}else{HEAP8[r19]=35;r20=r19+1|0}r19=r20+2|0;HEAP8[r20]=108;HEAP8[r20+1|0]=108;r20=r18&74;do{if((r20|0)==8){if((r18&16384|0)==0){HEAP8[r19]=120;break}else{HEAP8[r19]=88;break}}else if((r20|0)==64){HEAP8[r19]=111}else{HEAP8[r19]=100}}while(0);r19=r10|0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r20=__ZNSt3__111__sprintf_lEPcPvPKcz(r19,HEAP32[1311652],r16,(tempInt=STACKTOP,STACKTOP=STACKTOP+8|0,HEAP32[tempInt>>2]=r6,HEAP32[tempInt+4>>2]=r7,tempInt));r7=r10+r20|0;r6=HEAP32[r17>>2]&176;do{if((r6|0)==16){r17=HEAP8[r19];if(r17<<24>>24==45|r17<<24>>24==43){r21=r10+1|0;break}if(!((r20|0)>1&r17<<24>>24==48)){r2=1328;break}r17=HEAP8[r10+1|0];if(!(r17<<24>>24==120|r17<<24>>24==88)){r2=1328;break}r21=r10+2|0;break}else if((r6|0)==32){r21=r7}else{r2=1328}}while(0);if(r2==1328){r21=r19}r2=r11|0;r11=r14|0;r6=HEAP32[r4+28>>2];HEAP32[r11>>2]=r6;r10=r6+4|0;tempValue=HEAP32[r10>>2],HEAP32[r10>>2]=tempValue+1,tempValue;__ZNSt3__19__num_putIcE21__widen_and_group_intEPcS2_S2_S2_RS2_S3_RKNS_6localeE(r19,r21,r7,r2,r12,r13,r14);r14=HEAP32[r11>>2];r11=r14+4|0;if(((tempValue=HEAP32[r11>>2],HEAP32[r11>>2]=tempValue+ -1,tempValue)|0)!=0){r22=r3|0;r23=HEAP32[r22>>2];r24=r15|0;HEAP32[r24>>2]=r23;r25=HEAP32[r12>>2];r26=HEAP32[r13>>2];__ZNSt3__116__pad_and_outputIcNS_11char_traitsIcEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r15,r2,r25,r26,r4,r5);STACKTOP=r8;return}FUNCTION_TABLE[HEAP32[HEAP32[r14>>2]+8>>2]](r14|0);r22=r3|0;r23=HEAP32[r22>>2];r24=r15|0;HEAP32[r24>>2]=r23;r25=HEAP32[r12>>2];r26=HEAP32[r13>>2];__ZNSt3__116__pad_and_outputIcNS_11char_traitsIcEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r15,r2,r25,r26,r4,r5);STACKTOP=r8;return}function __ZNKSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEcm(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26;r2=0;r7=STACKTOP;STACKTOP=STACKTOP+60|0;r8=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r8>>2];r8=r7;r9=r7+8;r10=r7+20;r11=r7+44;r12=r7+48;r13=r7+52;r14=r7+56;r15=r8|0;HEAP8[r15]=HEAP8[5247320];HEAP8[r15+1|0]=HEAP8[5247321|0];HEAP8[r15+2|0]=HEAP8[5247322|0];HEAP8[r15+3|0]=HEAP8[5247323|0];HEAP8[r15+4|0]=HEAP8[5247324|0];HEAP8[r15+5|0]=HEAP8[5247325|0];r16=r8+1|0;r17=r4+4|0;r18=HEAP32[r17>>2];if((r18&2048|0)==0){r19=r16}else{HEAP8[r16]=43;r19=r8+2|0}if((r18&512|0)==0){r20=r19}else{HEAP8[r19]=35;r20=r19+1|0}HEAP8[r20]=108;r19=r20+1|0;r20=r18&74;do{if((r20|0)==64){HEAP8[r19]=111}else if((r20|0)==8){if((r18&16384|0)==0){HEAP8[r19]=120;break}else{HEAP8[r19]=88;break}}else{HEAP8[r19]=117}}while(0);r19=r9|0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r18=__ZNSt3__111__sprintf_lEPcPvPKcz(r19,HEAP32[1311652],r15,(tempInt=STACKTOP,STACKTOP=STACKTOP+4|0,HEAP32[tempInt>>2]=r6,tempInt));r6=r9+r18|0;r15=HEAP32[r17>>2]&176;do{if((r15|0)==32){r21=r6}else if((r15|0)==16){r17=HEAP8[r19];if(r17<<24>>24==45|r17<<24>>24==43){r21=r9+1|0;break}if(!((r18|0)>1&r17<<24>>24==48)){r2=1359;break}r17=HEAP8[r9+1|0];if(!(r17<<24>>24==120|r17<<24>>24==88)){r2=1359;break}r21=r9+2|0;break}else{r2=1359}}while(0);if(r2==1359){r21=r19}r2=r10|0;r10=r13|0;r9=HEAP32[r4+28>>2];HEAP32[r10>>2]=r9;r18=r9+4|0;tempValue=HEAP32[r18>>2],HEAP32[r18>>2]=tempValue+1,tempValue;__ZNSt3__19__num_putIcE21__widen_and_group_intEPcS2_S2_S2_RS2_S3_RKNS_6localeE(r19,r21,r6,r2,r11,r12,r13);r13=HEAP32[r10>>2];r10=r13+4|0;if(((tempValue=HEAP32[r10>>2],HEAP32[r10>>2]=tempValue+ -1,tempValue)|0)!=0){r22=r3|0;r23=HEAP32[r22>>2];r24=r14|0;HEAP32[r24>>2]=r23;r25=HEAP32[r11>>2];r26=HEAP32[r12>>2];__ZNSt3__116__pad_and_outputIcNS_11char_traitsIcEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r14,r2,r25,r26,r4,r5);STACKTOP=r7;return}FUNCTION_TABLE[HEAP32[HEAP32[r13>>2]+8>>2]](r13|0);r22=r3|0;r23=HEAP32[r22>>2];r24=r14|0;HEAP32[r24>>2]=r23;r25=HEAP32[r11>>2];r26=HEAP32[r12>>2];__ZNSt3__116__pad_and_outputIcNS_11char_traitsIcEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r14,r2,r25,r26,r4,r5);STACKTOP=r7;return}function __ZNKSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEcy(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+92|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r8;r10=r8+8;r11=r8+32;r12=r8+76;r13=r8+80;r14=r8+84;r15=r8+88;HEAP32[r9>>2]=37;HEAP32[r9+4>>2]=0;r16=r9;r9=r16+1|0;r17=r4+4|0;r18=HEAP32[r17>>2];if((r18&2048|0)==0){r19=r9}else{HEAP8[r9]=43;r19=r16+2|0}if((r18&512|0)==0){r20=r19}else{HEAP8[r19]=35;r20=r19+1|0}r19=r20+2|0;HEAP8[r20]=108;HEAP8[r20+1|0]=108;r20=r18&74;do{if((r20|0)==64){HEAP8[r19]=111}else if((r20|0)==8){if((r18&16384|0)==0){HEAP8[r19]=120;break}else{HEAP8[r19]=88;break}}else{HEAP8[r19]=117}}while(0);r19=r10|0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r18=__ZNSt3__111__sprintf_lEPcPvPKcz(r19,HEAP32[1311652],r16,(tempInt=STACKTOP,STACKTOP=STACKTOP+8|0,HEAP32[tempInt>>2]=r6,HEAP32[tempInt+4>>2]=r7,tempInt));r7=r10+r18|0;r6=HEAP32[r17>>2]&176;do{if((r6|0)==16){r17=HEAP8[r19];if(r17<<24>>24==45|r17<<24>>24==43){r21=r10+1|0;break}if(!((r18|0)>1&r17<<24>>24==48)){r2=1390;break}r17=HEAP8[r10+1|0];if(!(r17<<24>>24==120|r17<<24>>24==88)){r2=1390;break}r21=r10+2|0;break}else if((r6|0)==32){r21=r7}else{r2=1390}}while(0);if(r2==1390){r21=r19}r2=r11|0;r11=r14|0;r6=HEAP32[r4+28>>2];HEAP32[r11>>2]=r6;r10=r6+4|0;tempValue=HEAP32[r10>>2],HEAP32[r10>>2]=tempValue+1,tempValue;__ZNSt3__19__num_putIcE21__widen_and_group_intEPcS2_S2_S2_RS2_S3_RKNS_6localeE(r19,r21,r7,r2,r12,r13,r14);r14=HEAP32[r11>>2];r11=r14+4|0;if(((tempValue=HEAP32[r11>>2],HEAP32[r11>>2]=tempValue+ -1,tempValue)|0)!=0){r22=r3|0;r23=HEAP32[r22>>2];r24=r15|0;HEAP32[r24>>2]=r23;r25=HEAP32[r12>>2];r26=HEAP32[r13>>2];__ZNSt3__116__pad_and_outputIcNS_11char_traitsIcEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r15,r2,r25,r26,r4,r5);STACKTOP=r8;return}FUNCTION_TABLE[HEAP32[HEAP32[r14>>2]+8>>2]](r14|0);r22=r3|0;r23=HEAP32[r22>>2];r24=r15|0;HEAP32[r24>>2]=r23;r25=HEAP32[r12>>2];r26=HEAP32[r13>>2];__ZNSt3__116__pad_and_outputIcNS_11char_traitsIcEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r15,r2,r25,r26,r4,r5);STACKTOP=r8;return}function __ZNKSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEcd(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32;r2=0;r7=STACKTOP;STACKTOP=STACKTOP+124|0;r8=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r8>>2];r8=r7;r9=r7+8;r10=r7+40,r11=r10>>2;r12=r7+44;r13=r7+104;r14=r7+108;r15=r7+112;r16=r7+116;r17=r7+120;HEAP32[r8>>2]=37;HEAP32[r8+4>>2]=0;r18=r8;r8=r18+1|0;r19=r4+4|0;r20=HEAP32[r19>>2];if((r20&2048|0)==0){r21=r8}else{HEAP8[r8]=43;r21=r18+2|0}if((r20&1024|0)==0){r22=r21}else{HEAP8[r21]=35;r22=r21+1|0}r21=r20&260;r8=r20>>>14;do{if((r21|0)==260){if((r8&1|0)==0){HEAP8[r22]=97;r23=0;break}else{HEAP8[r22]=65;r23=0;break}}else{HEAP8[r22]=46;r20=r22+2|0;HEAP8[r22+1|0]=42;if((r21|0)==4){if((r8&1|0)==0){HEAP8[r20]=102;r23=1;break}else{HEAP8[r20]=70;r23=1;break}}else if((r21|0)==256){if((r8&1|0)==0){HEAP8[r20]=101;r23=1;break}else{HEAP8[r20]=69;r23=1;break}}else{if((r8&1|0)==0){HEAP8[r20]=103;r23=1;break}else{HEAP8[r20]=71;r23=1;break}}}}while(0);r8=r9|0;HEAP32[r11]=r8;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r9=HEAP32[1311652];if(r23){r24=__ZNSt3__112__snprintf_lEPcjPvPKcz(r8,30,r9,r18,(tempInt=STACKTOP,STACKTOP=STACKTOP+12|0,HEAP32[tempInt>>2]=HEAP32[r4+8>>2],HEAPF64[tempDoublePtr>>3]=r6,HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+8>>2]=HEAP32[tempDoublePtr+4>>2],tempInt))}else{r24=__ZNSt3__112__snprintf_lEPcjPvPKcz(r8,30,r9,r18,(tempInt=STACKTOP,STACKTOP=STACKTOP+8|0,HEAPF64[tempDoublePtr>>3]=r6,HEAP32[tempInt>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr+4>>2],tempInt))}do{if((r24|0)>29){r9=HEAP8[5255484]<<24>>24==0;if(r23){do{if(r9){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r25=__ZNSt3__112__asprintf_lEPPcPvPKcz(r10,HEAP32[1311652],r18,(tempInt=STACKTOP,STACKTOP=STACKTOP+12|0,HEAP32[tempInt>>2]=HEAP32[r4+8>>2],HEAPF64[tempDoublePtr>>3]=r6,HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+8>>2]=HEAP32[tempDoublePtr+4>>2],tempInt))}else{do{if(r9){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r25=__ZNSt3__112__asprintf_lEPPcPvPKcz(r10,HEAP32[1311652],r18,(tempInt=STACKTOP,STACKTOP=STACKTOP+12|0,HEAP32[tempInt>>2]=HEAP32[r4+8>>2],HEAPF64[tempDoublePtr>>3]=r6,HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+8>>2]=HEAP32[tempDoublePtr+4>>2],tempInt))}r9=HEAP32[r11];if((r9|0)!=0){r26=r25;r27=r9;r28=r9;break}r9=___cxa_allocate_exception(4);HEAP32[r9>>2]=5247464;___cxa_throw(r9,5252688,66)}else{r26=r24;r27=0;r28=HEAP32[r11]}}while(0);r24=r28+r26|0;r25=HEAP32[r19>>2]&176;do{if((r25|0)==32){r29=r24}else if((r25|0)==16){r19=HEAP8[r28];if(r19<<24>>24==45|r19<<24>>24==43){r29=r28+1|0;break}if(!((r26|0)>1&r19<<24>>24==48)){r2=1452;break}r19=HEAP8[r28+1|0];if(!(r19<<24>>24==120|r19<<24>>24==88)){r2=1452;break}r29=r28+2|0;break}else{r2=1452}}while(0);if(r2==1452){r29=r28}do{if((r28|0)==(r8|0)){r30=r12|0;r31=0;r32=r8}else{r2=_malloc(r26<<1);if((r2|0)!=0){r30=r2;r31=r2;r32=HEAP32[r11];break}r2=___cxa_allocate_exception(4);HEAP32[r2>>2]=5247464;___cxa_throw(r2,5252688,66)}}while(0);r11=r15|0;r26=HEAP32[r4+28>>2];HEAP32[r11>>2]=r26;r8=r26+4|0;tempValue=HEAP32[r8>>2],HEAP32[r8>>2]=tempValue+1,tempValue;__ZNSt3__19__num_putIcE23__widen_and_group_floatEPcS2_S2_S2_RS2_S3_RKNS_6localeE(r32,r29,r24,r30,r13,r14,r15);r15=HEAP32[r11>>2];r11=r15+4|0;if(((tempValue=HEAP32[r11>>2],HEAP32[r11>>2]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r15>>2]+8>>2]](r15|0)}r15=r3|0;HEAP32[r17>>2]=HEAP32[r15>>2];__ZNSt3__116__pad_and_outputIcNS_11char_traitsIcEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r16,r17,r30,HEAP32[r13>>2],HEAP32[r14>>2],r4,r5);r5=HEAP32[r16>>2];HEAP32[r15>>2]=r5;HEAP32[r1>>2]=r5;if((r31|0)!=0){_free(r31)}if((r27|0)==0){STACKTOP=r7;return}_free(r27);STACKTOP=r7;return}function __ZNKSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEce(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32;r2=0;r7=STACKTOP;STACKTOP=STACKTOP+124|0;r8=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r8>>2];r8=r7;r9=r7+8;r10=r7+40,r11=r10>>2;r12=r7+44;r13=r7+104;r14=r7+108;r15=r7+112;r16=r7+116;r17=r7+120;HEAP32[r8>>2]=37;HEAP32[r8+4>>2]=0;r18=r8;r8=r18+1|0;r19=r4+4|0;r20=HEAP32[r19>>2];if((r20&2048|0)==0){r21=r8}else{HEAP8[r8]=43;r21=r18+2|0}if((r20&1024|0)==0){r22=r21}else{HEAP8[r21]=35;r22=r21+1|0}r21=r20&260;r8=r20>>>14;do{if((r21|0)==260){HEAP8[r22]=76;r20=r22+1|0;if((r8&1|0)==0){HEAP8[r20]=97;r23=0;break}else{HEAP8[r20]=65;r23=0;break}}else{HEAP8[r22]=46;HEAP8[r22+1|0]=42;HEAP8[r22+2|0]=76;r20=r22+3|0;if((r21|0)==256){if((r8&1|0)==0){HEAP8[r20]=101;r23=1;break}else{HEAP8[r20]=69;r23=1;break}}else if((r21|0)==4){if((r8&1|0)==0){HEAP8[r20]=102;r23=1;break}else{HEAP8[r20]=70;r23=1;break}}else{if((r8&1|0)==0){HEAP8[r20]=103;r23=1;break}else{HEAP8[r20]=71;r23=1;break}}}}while(0);r8=r9|0;HEAP32[r11]=r8;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r9=HEAP32[1311652];if(r23){r24=__ZNSt3__112__snprintf_lEPcjPvPKcz(r8,30,r9,r18,(tempInt=STACKTOP,STACKTOP=STACKTOP+12|0,HEAP32[tempInt>>2]=HEAP32[r4+8>>2],HEAPF64[tempDoublePtr>>3]=r6,HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+8>>2]=HEAP32[tempDoublePtr+4>>2],tempInt))}else{r24=__ZNSt3__112__snprintf_lEPcjPvPKcz(r8,30,r9,r18,(tempInt=STACKTOP,STACKTOP=STACKTOP+8|0,HEAPF64[tempDoublePtr>>3]=r6,HEAP32[tempInt>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr+4>>2],tempInt))}do{if((r24|0)>29){r9=HEAP8[5255484]<<24>>24==0;if(r23){do{if(r9){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r25=__ZNSt3__112__asprintf_lEPPcPvPKcz(r10,HEAP32[1311652],r18,(tempInt=STACKTOP,STACKTOP=STACKTOP+12|0,HEAP32[tempInt>>2]=HEAP32[r4+8>>2],HEAPF64[tempDoublePtr>>3]=r6,HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+8>>2]=HEAP32[tempDoublePtr+4>>2],tempInt))}else{do{if(r9){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r25=__ZNSt3__112__asprintf_lEPPcPvPKcz(r10,HEAP32[1311652],r18,(tempInt=STACKTOP,STACKTOP=STACKTOP+8|0,HEAPF64[tempDoublePtr>>3]=r6,HEAP32[tempInt>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr+4>>2],tempInt))}r9=HEAP32[r11];if((r9|0)!=0){r26=r25;r27=r9;r28=r9;break}r9=___cxa_allocate_exception(4);HEAP32[r9>>2]=5247464;___cxa_throw(r9,5252688,66)}else{r26=r24;r27=0;r28=HEAP32[r11]}}while(0);r24=r28+r26|0;r25=HEAP32[r19>>2]&176;do{if((r25|0)==32){r29=r24}else if((r25|0)==16){r19=HEAP8[r28];if(r19<<24>>24==45|r19<<24>>24==43){r29=r28+1|0;break}if(!((r26|0)>1&r19<<24>>24==48)){r2=1529;break}r19=HEAP8[r28+1|0];if(!(r19<<24>>24==120|r19<<24>>24==88)){r2=1529;break}r29=r28+2|0;break}else{r2=1529}}while(0);if(r2==1529){r29=r28}do{if((r28|0)==(r8|0)){r30=r12|0;r31=0;r32=r8}else{r2=_malloc(r26<<1);if((r2|0)!=0){r30=r2;r31=r2;r32=HEAP32[r11];break}r2=___cxa_allocate_exception(4);HEAP32[r2>>2]=5247464;___cxa_throw(r2,5252688,66)}}while(0);r11=r15|0;r26=HEAP32[r4+28>>2];HEAP32[r11>>2]=r26;r8=r26+4|0;tempValue=HEAP32[r8>>2],HEAP32[r8>>2]=tempValue+1,tempValue;__ZNSt3__19__num_putIcE23__widen_and_group_floatEPcS2_S2_S2_RS2_S3_RKNS_6localeE(r32,r29,r24,r30,r13,r14,r15);r15=HEAP32[r11>>2];r11=r15+4|0;if(((tempValue=HEAP32[r11>>2],HEAP32[r11>>2]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r15>>2]+8>>2]](r15|0)}r15=r3|0;HEAP32[r17>>2]=HEAP32[r15>>2];__ZNSt3__116__pad_and_outputIcNS_11char_traitsIcEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r16,r17,r30,HEAP32[r13>>2],HEAP32[r14>>2],r4,r5);r5=HEAP32[r16>>2];HEAP32[r15>>2]=r5;HEAP32[r1>>2]=r5;if((r31|0)!=0){_free(r31)}if((r27|0)==0){STACKTOP=r7;return}_free(r27);STACKTOP=r7;return}function __ZNSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEED1Ev(r1){return}function __ZNSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEED0Ev(r1){__ZdlPv(r1);return}function __ZNKSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwl(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26;r2=0;r7=STACKTOP;STACKTOP=STACKTOP+120|0;r8=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r8>>2];r8=r7;r9=r7+8;r10=r7+20;r11=r7+104;r12=r7+108;r13=r7+112;r14=r7+116;r15=r8|0;HEAP8[r15]=HEAP8[5247320];HEAP8[r15+1|0]=HEAP8[5247321|0];HEAP8[r15+2|0]=HEAP8[5247322|0];HEAP8[r15+3|0]=HEAP8[5247323|0];HEAP8[r15+4|0]=HEAP8[5247324|0];HEAP8[r15+5|0]=HEAP8[5247325|0];r16=r8+1|0;r17=r4+4|0;r18=HEAP32[r17>>2];if((r18&2048|0)==0){r19=r16}else{HEAP8[r16]=43;r19=r8+2|0}if((r18&512|0)==0){r20=r19}else{HEAP8[r19]=35;r20=r19+1|0}HEAP8[r20]=108;r19=r20+1|0;r20=r18&74;do{if((r20|0)==8){if((r18&16384|0)==0){HEAP8[r19]=120;break}else{HEAP8[r19]=88;break}}else if((r20|0)==64){HEAP8[r19]=111}else{HEAP8[r19]=100}}while(0);r19=r9|0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r20=__ZNSt3__111__sprintf_lEPcPvPKcz(r19,HEAP32[1311652],r15,(tempInt=STACKTOP,STACKTOP=STACKTOP+4|0,HEAP32[tempInt>>2]=r6,tempInt));r6=r9+r20|0;r15=HEAP32[r17>>2]&176;do{if((r15|0)==32){r21=r6}else if((r15|0)==16){r17=HEAP8[r19];if(r17<<24>>24==45|r17<<24>>24==43){r21=r9+1|0;break}if(!((r20|0)>1&r17<<24>>24==48)){r2=1577;break}r17=HEAP8[r9+1|0];if(!(r17<<24>>24==120|r17<<24>>24==88)){r2=1577;break}r21=r9+2|0;break}else{r2=1577}}while(0);if(r2==1577){r21=r19}r2=r10|0;r10=r13|0;r9=HEAP32[r4+28>>2];HEAP32[r10>>2]=r9;r20=r9+4|0;tempValue=HEAP32[r20>>2],HEAP32[r20>>2]=tempValue+1,tempValue;__ZNSt3__19__num_putIwE21__widen_and_group_intEPcS2_S2_PwRS3_S4_RKNS_6localeE(r19,r21,r6,r2,r11,r12,r13);r13=HEAP32[r10>>2];r10=r13+4|0;if(((tempValue=HEAP32[r10>>2],HEAP32[r10>>2]=tempValue+ -1,tempValue)|0)!=0){r22=r3|0;r23=HEAP32[r22>>2];r24=r14|0;HEAP32[r24>>2]=r23;r25=HEAP32[r11>>2];r26=HEAP32[r12>>2];__ZNSt3__116__pad_and_outputIwNS_11char_traitsIwEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r14,r2,r25,r26,r4,r5);STACKTOP=r7;return}FUNCTION_TABLE[HEAP32[HEAP32[r13>>2]+8>>2]](r13|0);r22=r3|0;r23=HEAP32[r22>>2];r24=r14|0;HEAP32[r24>>2]=r23;r25=HEAP32[r11>>2];r26=HEAP32[r12>>2];__ZNSt3__116__pad_and_outputIwNS_11char_traitsIwEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r14,r2,r25,r26,r4,r5);STACKTOP=r7;return}function __ZNKSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwx(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+212|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r8;r10=r8+8;r11=r8+32;r12=r8+196;r13=r8+200;r14=r8+204;r15=r8+208;HEAP32[r9>>2]=37;HEAP32[r9+4>>2]=0;r16=r9;r9=r16+1|0;r17=r4+4|0;r18=HEAP32[r17>>2];if((r18&2048|0)==0){r19=r9}else{HEAP8[r9]=43;r19=r16+2|0}if((r18&512|0)==0){r20=r19}else{HEAP8[r19]=35;r20=r19+1|0}r19=r20+2|0;HEAP8[r20]=108;HEAP8[r20+1|0]=108;r20=r18&74;do{if((r20|0)==64){HEAP8[r19]=111}else if((r20|0)==8){if((r18&16384|0)==0){HEAP8[r19]=120;break}else{HEAP8[r19]=88;break}}else{HEAP8[r19]=100}}while(0);r19=r10|0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r18=__ZNSt3__111__sprintf_lEPcPvPKcz(r19,HEAP32[1311652],r16,(tempInt=STACKTOP,STACKTOP=STACKTOP+8|0,HEAP32[tempInt>>2]=r6,HEAP32[tempInt+4>>2]=r7,tempInt));r7=r10+r18|0;r6=HEAP32[r17>>2]&176;do{if((r6|0)==32){r21=r7}else if((r6|0)==16){r17=HEAP8[r19];if(r17<<24>>24==45|r17<<24>>24==43){r21=r10+1|0;break}if(!((r18|0)>1&r17<<24>>24==48)){r2=1608;break}r17=HEAP8[r10+1|0];if(!(r17<<24>>24==120|r17<<24>>24==88)){r2=1608;break}r21=r10+2|0;break}else{r2=1608}}while(0);if(r2==1608){r21=r19}r2=r11|0;r11=r14|0;r10=HEAP32[r4+28>>2];HEAP32[r11>>2]=r10;r18=r10+4|0;tempValue=HEAP32[r18>>2],HEAP32[r18>>2]=tempValue+1,tempValue;__ZNSt3__19__num_putIwE21__widen_and_group_intEPcS2_S2_PwRS3_S4_RKNS_6localeE(r19,r21,r7,r2,r12,r13,r14);r14=HEAP32[r11>>2];r11=r14+4|0;if(((tempValue=HEAP32[r11>>2],HEAP32[r11>>2]=tempValue+ -1,tempValue)|0)!=0){r22=r3|0;r23=HEAP32[r22>>2];r24=r15|0;HEAP32[r24>>2]=r23;r25=HEAP32[r12>>2];r26=HEAP32[r13>>2];__ZNSt3__116__pad_and_outputIwNS_11char_traitsIwEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r15,r2,r25,r26,r4,r5);STACKTOP=r8;return}FUNCTION_TABLE[HEAP32[HEAP32[r14>>2]+8>>2]](r14|0);r22=r3|0;r23=HEAP32[r22>>2];r24=r15|0;HEAP32[r24>>2]=r23;r25=HEAP32[r12>>2];r26=HEAP32[r13>>2];__ZNSt3__116__pad_and_outputIwNS_11char_traitsIwEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r15,r2,r25,r26,r4,r5);STACKTOP=r8;return}function __ZNKSt3__17num_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_RNS_8ios_baseEcPKv(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23;r2=0;r7=STACKTOP;STACKTOP=STACKTOP+84|0;r8=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r8>>2];r8=r7,r9=r8>>2;r10=r7+20;r11=r7+40;r12=r7+80;r13=r7+12|0;HEAP8[r13]=HEAP8[5247328];HEAP8[r13+1|0]=HEAP8[5247329|0];HEAP8[r13+2|0]=HEAP8[5247330|0];HEAP8[r13+3|0]=HEAP8[5247331|0];HEAP8[r13+4|0]=HEAP8[5247332|0];HEAP8[r13+5|0]=HEAP8[5247333|0];r14=r10|0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r15=__ZNSt3__111__sprintf_lEPcPvPKcz(r14,HEAP32[1311652],r13,(tempInt=STACKTOP,STACKTOP=STACKTOP+4|0,HEAP32[tempInt>>2]=r6,tempInt));r6=r10+r15|0;r13=HEAP32[r4+4>>2]&176;do{if((r13|0)==16){r16=HEAP8[r14];if(r16<<24>>24==45|r16<<24>>24==43){r17=r10+1|0;break}if(!((r15|0)>1&r16<<24>>24==48)){r2=1629;break}r16=HEAP8[r10+1|0];if(!(r16<<24>>24==120|r16<<24>>24==88)){r2=1629;break}r17=r10+2|0;break}else if((r13|0)==32){r17=r6}else{r2=1629}}while(0);if(r2==1629){r17=r14}r2=HEAP32[r4+28>>2],r13=r2>>2;r16=(r2+4|0)>>2;tempValue=HEAP32[r16],HEAP32[r16]=tempValue+1,tempValue;if((HEAP32[1313727]|0)!=-1){HEAP32[r9]=5254908;HEAP32[r9+1]=24;HEAP32[r9+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r8,406)}r8=HEAP32[1313728]-1|0;r9=HEAP32[r13+2];do{if(HEAP32[r13+3]-r9>>2>>>0>r8>>>0){r18=HEAP32[r9+(r8<<2)>>2];if((r18|0)==0){break}if(((tempValue=HEAP32[r16],HEAP32[r16]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r13]+8>>2]](r2)}r19=r11|0;FUNCTION_TABLE[HEAP32[HEAP32[r18>>2]+32>>2]](r18,r14,r6,r19);r18=r11+r15|0;if((r17|0)==(r6|0)){r20=r18;r21=r3|0;r22=HEAP32[r21>>2];r23=r12|0;HEAP32[r23>>2]=r22;__ZNSt3__116__pad_and_outputIcNS_11char_traitsIcEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r12,r19,r20,r18,r4,r5);STACKTOP=r7;return}r20=r11+(r17-r10)|0;r21=r3|0;r22=HEAP32[r21>>2];r23=r12|0;HEAP32[r23>>2]=r22;__ZNSt3__116__pad_and_outputIcNS_11char_traitsIcEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r12,r19,r20,r18,r4,r5);STACKTOP=r7;return}}while(0);r7=___cxa_allocate_exception(4);HEAP32[r7>>2]=5247488;___cxa_throw(r7,5252700,514)}function __ZNKSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwb(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25;r7=STACKTOP;STACKTOP=STACKTOP+28|0;r8=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r8>>2];r8=r7,r9=r8>>2;r10=r7+12;r11=r7+16;if((HEAP32[r4+4>>2]&1|0)==0){r12=HEAP32[HEAP32[r2>>2]+24>>2];HEAP32[r10>>2]=HEAP32[r3>>2];FUNCTION_TABLE[r12](r1,r2,r10,r4,r5,r6&1);STACKTOP=r7;return}r5=HEAP32[r4+28>>2],r4=r5>>2;r10=(r5+4|0)>>2;tempValue=HEAP32[r10],HEAP32[r10]=tempValue+1,tempValue;if((HEAP32[1313633]|0)!=-1){HEAP32[r9]=5254532;HEAP32[r9+1]=24;HEAP32[r9+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254532,r8,406)}r8=HEAP32[1313634]-1|0;r9=HEAP32[r4+2];do{if(HEAP32[r4+3]-r9>>2>>>0>r8>>>0){r2=HEAP32[r9+(r8<<2)>>2];if((r2|0)==0){break}r12=r2;if(((tempValue=HEAP32[r10],HEAP32[r10]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r4]+8>>2]](r5)}r13=HEAP32[r2>>2];if(r6){FUNCTION_TABLE[HEAP32[r13+24>>2]](r11,r12)}else{FUNCTION_TABLE[HEAP32[r13+28>>2]](r11,r12)}r12=r11;r13=HEAP8[r12];if((r13&1)<<24>>24==0){r2=r11+4|0;r14=r2;r15=r2;r16=r11+8|0}else{r2=r11+8|0;r14=HEAP32[r2>>2];r15=r11+4|0;r16=r2}r2=(r3|0)>>2;r17=r14;r18=r13;while(1){r19=(r18&1)<<24>>24==0;if(r19){r20=r15}else{r20=HEAP32[r16>>2]}r13=r18&255;if((r13&1|0)==0){r21=r13>>>1}else{r21=HEAP32[r15>>2]}if((r17|0)==((r21<<2)+r20|0)){break}r13=HEAP32[r17>>2];r22=HEAP32[r2];do{if((r22|0)!=0){r23=r22+24|0;r24=HEAP32[r23>>2];if((r24|0)==(HEAP32[r22+28>>2]|0)){r25=FUNCTION_TABLE[HEAP32[HEAP32[r22>>2]+52>>2]](r22,r13)}else{HEAP32[r23>>2]=r24+4|0;HEAP32[r24>>2]=r13;r25=r13}if((r25|0)!=-1){break}HEAP32[r2]=0}}while(0);r17=r17+4|0;r18=HEAP8[r12]}HEAP32[r1>>2]=HEAP32[r2];if(r19){STACKTOP=r7;return}__ZdlPv(HEAP32[r16>>2]);STACKTOP=r7;return}}while(0);r7=___cxa_allocate_exception(4);HEAP32[r7>>2]=5247488;___cxa_throw(r7,5252700,514)}function __ZNKSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwm(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26;r2=0;r7=STACKTOP;STACKTOP=STACKTOP+120|0;r8=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r8>>2];r8=r7;r9=r7+8;r10=r7+20;r11=r7+104;r12=r7+108;r13=r7+112;r14=r7+116;r15=r8|0;HEAP8[r15]=HEAP8[5247320];HEAP8[r15+1|0]=HEAP8[5247321|0];HEAP8[r15+2|0]=HEAP8[5247322|0];HEAP8[r15+3|0]=HEAP8[5247323|0];HEAP8[r15+4|0]=HEAP8[5247324|0];HEAP8[r15+5|0]=HEAP8[5247325|0];r16=r8+1|0;r17=r4+4|0;r18=HEAP32[r17>>2];if((r18&2048|0)==0){r19=r16}else{HEAP8[r16]=43;r19=r8+2|0}if((r18&512|0)==0){r20=r19}else{HEAP8[r19]=35;r20=r19+1|0}HEAP8[r20]=108;r19=r20+1|0;r20=r18&74;do{if((r20|0)==64){HEAP8[r19]=111}else if((r20|0)==8){if((r18&16384|0)==0){HEAP8[r19]=120;break}else{HEAP8[r19]=88;break}}else{HEAP8[r19]=117}}while(0);r19=r9|0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r18=__ZNSt3__111__sprintf_lEPcPvPKcz(r19,HEAP32[1311652],r15,(tempInt=STACKTOP,STACKTOP=STACKTOP+4|0,HEAP32[tempInt>>2]=r6,tempInt));r6=r9+r18|0;r15=HEAP32[r17>>2]&176;do{if((r15|0)==16){r17=HEAP8[r19];if(r17<<24>>24==45|r17<<24>>24==43){r21=r9+1|0;break}if(!((r18|0)>1&r17<<24>>24==48)){r2=1708;break}r17=HEAP8[r9+1|0];if(!(r17<<24>>24==120|r17<<24>>24==88)){r2=1708;break}r21=r9+2|0;break}else if((r15|0)==32){r21=r6}else{r2=1708}}while(0);if(r2==1708){r21=r19}r2=r10|0;r10=r13|0;r15=HEAP32[r4+28>>2];HEAP32[r10>>2]=r15;r9=r15+4|0;tempValue=HEAP32[r9>>2],HEAP32[r9>>2]=tempValue+1,tempValue;__ZNSt3__19__num_putIwE21__widen_and_group_intEPcS2_S2_PwRS3_S4_RKNS_6localeE(r19,r21,r6,r2,r11,r12,r13);r13=HEAP32[r10>>2];r10=r13+4|0;if(((tempValue=HEAP32[r10>>2],HEAP32[r10>>2]=tempValue+ -1,tempValue)|0)!=0){r22=r3|0;r23=HEAP32[r22>>2];r24=r14|0;HEAP32[r24>>2]=r23;r25=HEAP32[r11>>2];r26=HEAP32[r12>>2];__ZNSt3__116__pad_and_outputIwNS_11char_traitsIwEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r14,r2,r25,r26,r4,r5);STACKTOP=r7;return}FUNCTION_TABLE[HEAP32[HEAP32[r13>>2]+8>>2]](r13|0);r22=r3|0;r23=HEAP32[r22>>2];r24=r14|0;HEAP32[r24>>2]=r23;r25=HEAP32[r11>>2];r26=HEAP32[r12>>2];__ZNSt3__116__pad_and_outputIwNS_11char_traitsIwEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r14,r2,r25,r26,r4,r5);STACKTOP=r7;return}function __ZNKSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwy(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26;r2=0;r8=STACKTOP;STACKTOP=STACKTOP+220|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r8;r10=r8+8;r11=r8+32;r12=r8+204;r13=r8+208;r14=r8+212;r15=r8+216;HEAP32[r9>>2]=37;HEAP32[r9+4>>2]=0;r16=r9;r9=r16+1|0;r17=r4+4|0;r18=HEAP32[r17>>2];if((r18&2048|0)==0){r19=r9}else{HEAP8[r9]=43;r19=r16+2|0}if((r18&512|0)==0){r20=r19}else{HEAP8[r19]=35;r20=r19+1|0}r19=r20+2|0;HEAP8[r20]=108;HEAP8[r20+1|0]=108;r20=r18&74;do{if((r20|0)==64){HEAP8[r19]=111}else if((r20|0)==8){if((r18&16384|0)==0){HEAP8[r19]=120;break}else{HEAP8[r19]=88;break}}else{HEAP8[r19]=117}}while(0);r19=r10|0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r18=__ZNSt3__111__sprintf_lEPcPvPKcz(r19,HEAP32[1311652],r16,(tempInt=STACKTOP,STACKTOP=STACKTOP+8|0,HEAP32[tempInt>>2]=r6,HEAP32[tempInt+4>>2]=r7,tempInt));r7=r10+r18|0;r6=HEAP32[r17>>2]&176;do{if((r6|0)==32){r21=r7}else if((r6|0)==16){r17=HEAP8[r19];if(r17<<24>>24==45|r17<<24>>24==43){r21=r10+1|0;break}if(!((r18|0)>1&r17<<24>>24==48)){r2=1739;break}r17=HEAP8[r10+1|0];if(!(r17<<24>>24==120|r17<<24>>24==88)){r2=1739;break}r21=r10+2|0;break}else{r2=1739}}while(0);if(r2==1739){r21=r19}r2=r11|0;r11=r14|0;r10=HEAP32[r4+28>>2];HEAP32[r11>>2]=r10;r18=r10+4|0;tempValue=HEAP32[r18>>2],HEAP32[r18>>2]=tempValue+1,tempValue;__ZNSt3__19__num_putIwE21__widen_and_group_intEPcS2_S2_PwRS3_S4_RKNS_6localeE(r19,r21,r7,r2,r12,r13,r14);r14=HEAP32[r11>>2];r11=r14+4|0;if(((tempValue=HEAP32[r11>>2],HEAP32[r11>>2]=tempValue+ -1,tempValue)|0)!=0){r22=r3|0;r23=HEAP32[r22>>2];r24=r15|0;HEAP32[r24>>2]=r23;r25=HEAP32[r12>>2];r26=HEAP32[r13>>2];__ZNSt3__116__pad_and_outputIwNS_11char_traitsIwEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r15,r2,r25,r26,r4,r5);STACKTOP=r8;return}FUNCTION_TABLE[HEAP32[HEAP32[r14>>2]+8>>2]](r14|0);r22=r3|0;r23=HEAP32[r22>>2];r24=r15|0;HEAP32[r24>>2]=r23;r25=HEAP32[r12>>2];r26=HEAP32[r13>>2];__ZNSt3__116__pad_and_outputIwNS_11char_traitsIwEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r15,r2,r25,r26,r4,r5);STACKTOP=r8;return}function __ZNKSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwd(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32;r2=0;r7=STACKTOP;STACKTOP=STACKTOP+292|0;r8=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r8>>2];r8=r7;r9=r7+8;r10=r7+40,r11=r10>>2;r12=r7+44;r13=r7+272;r14=r7+276;r15=r7+280;r16=r7+284;r17=r7+288;HEAP32[r8>>2]=37;HEAP32[r8+4>>2]=0;r18=r8;r8=r18+1|0;r19=r4+4|0;r20=HEAP32[r19>>2];if((r20&2048|0)==0){r21=r8}else{HEAP8[r8]=43;r21=r18+2|0}if((r20&1024|0)==0){r22=r21}else{HEAP8[r21]=35;r22=r21+1|0}r21=r20&260;r8=r20>>>14;do{if((r21|0)==260){if((r8&1|0)==0){HEAP8[r22]=97;r23=0;break}else{HEAP8[r22]=65;r23=0;break}}else{HEAP8[r22]=46;r20=r22+2|0;HEAP8[r22+1|0]=42;if((r21|0)==4){if((r8&1|0)==0){HEAP8[r20]=102;r23=1;break}else{HEAP8[r20]=70;r23=1;break}}else if((r21|0)==256){if((r8&1|0)==0){HEAP8[r20]=101;r23=1;break}else{HEAP8[r20]=69;r23=1;break}}else{if((r8&1|0)==0){HEAP8[r20]=103;r23=1;break}else{HEAP8[r20]=71;r23=1;break}}}}while(0);r8=r9|0;HEAP32[r11]=r8;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r9=HEAP32[1311652];if(r23){r24=__ZNSt3__112__snprintf_lEPcjPvPKcz(r8,30,r9,r18,(tempInt=STACKTOP,STACKTOP=STACKTOP+12|0,HEAP32[tempInt>>2]=HEAP32[r4+8>>2],HEAPF64[tempDoublePtr>>3]=r6,HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+8>>2]=HEAP32[tempDoublePtr+4>>2],tempInt))}else{r24=__ZNSt3__112__snprintf_lEPcjPvPKcz(r8,30,r9,r18,(tempInt=STACKTOP,STACKTOP=STACKTOP+8|0,HEAPF64[tempDoublePtr>>3]=r6,HEAP32[tempInt>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr+4>>2],tempInt))}do{if((r24|0)>29){r9=HEAP8[5255484]<<24>>24==0;if(r23){do{if(r9){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r25=__ZNSt3__112__asprintf_lEPPcPvPKcz(r10,HEAP32[1311652],r18,(tempInt=STACKTOP,STACKTOP=STACKTOP+12|0,HEAP32[tempInt>>2]=HEAP32[r4+8>>2],HEAPF64[tempDoublePtr>>3]=r6,HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+8>>2]=HEAP32[tempDoublePtr+4>>2],tempInt))}else{do{if(r9){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r25=__ZNSt3__112__asprintf_lEPPcPvPKcz(r10,HEAP32[1311652],r18,(tempInt=STACKTOP,STACKTOP=STACKTOP+12|0,HEAP32[tempInt>>2]=HEAP32[r4+8>>2],HEAPF64[tempDoublePtr>>3]=r6,HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+8>>2]=HEAP32[tempDoublePtr+4>>2],tempInt))}r9=HEAP32[r11];if((r9|0)!=0){r26=r25;r27=r9;r28=r9;break}r9=___cxa_allocate_exception(4);HEAP32[r9>>2]=5247464;___cxa_throw(r9,5252688,66)}else{r26=r24;r27=0;r28=HEAP32[r11]}}while(0);r24=r28+r26|0;r25=HEAP32[r19>>2]&176;do{if((r25|0)==16){r19=HEAP8[r28];if(r19<<24>>24==45|r19<<24>>24==43){r29=r28+1|0;break}if(!((r26|0)>1&r19<<24>>24==48)){r2=1801;break}r19=HEAP8[r28+1|0];if(!(r19<<24>>24==120|r19<<24>>24==88)){r2=1801;break}r29=r28+2|0;break}else if((r25|0)==32){r29=r24}else{r2=1801}}while(0);if(r2==1801){r29=r28}do{if((r28|0)==(r8|0)){r30=r12|0;r31=0;r32=r8}else{r2=_malloc(r26<<3);r25=r2;if((r2|0)!=0){r30=r25;r31=r25;r32=HEAP32[r11];break}r25=___cxa_allocate_exception(4);HEAP32[r25>>2]=5247464;___cxa_throw(r25,5252688,66)}}while(0);r11=r15|0;r26=HEAP32[r4+28>>2];HEAP32[r11>>2]=r26;r8=r26+4|0;tempValue=HEAP32[r8>>2],HEAP32[r8>>2]=tempValue+1,tempValue;__ZNSt3__19__num_putIwE23__widen_and_group_floatEPcS2_S2_PwRS3_S4_RKNS_6localeE(r32,r29,r24,r30,r13,r14,r15);r15=HEAP32[r11>>2];r11=r15+4|0;if(((tempValue=HEAP32[r11>>2],HEAP32[r11>>2]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r15>>2]+8>>2]](r15|0)}r15=r3|0;HEAP32[r17>>2]=HEAP32[r15>>2];__ZNSt3__116__pad_and_outputIwNS_11char_traitsIwEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r16,r17,r30,HEAP32[r13>>2],HEAP32[r14>>2],r4,r5);r5=HEAP32[r16>>2];HEAP32[r15>>2]=r5;HEAP32[r1>>2]=r5;if((r31|0)!=0){_free(r31)}if((r27|0)==0){STACKTOP=r7;return}_free(r27);STACKTOP=r7;return}function __ZNSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEED1Ev(r1){return}function __ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE13do_date_orderEv(r1){return 2}function __ZNSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEED1Ev(r1){return}function __ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE13do_date_orderEv(r1){return 2}function __ZNSt3__19money_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEED1Ev(r1){return}function __ZNSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEED0Ev(r1){__ZdlPv(r1);return}function __ZNSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEED0Ev(r1){__ZdlPv(r1);return}function __ZNSt3__18time_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEED1Ev(r1){var r2;r2=HEAP32[r1+8>>2];if((r2|0)==0){return}_freelocale(r2);return}function __ZNSt3__18time_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEED0Ev(r1){var r2,r3;r2=r1;r3=HEAP32[r1+8>>2];if((r3|0)==0){__ZdlPv(r2);return}_freelocale(r3);__ZdlPv(r2);return}function __ZNSt3__18time_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEED1Ev(r1){var r2;r2=HEAP32[r1+8>>2];if((r2|0)==0){return}_freelocale(r2);return}function __ZNSt3__18time_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEED0Ev(r1){var r2,r3;r2=r1;r3=HEAP32[r1+8>>2];if((r3|0)==0){__ZdlPv(r2);return}_freelocale(r3);__ZdlPv(r2);return}function __ZNSt3__19money_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEED0Ev(r1){__ZdlPv(r1);return}function __ZNKSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwe(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32;r2=0;r7=STACKTOP;STACKTOP=STACKTOP+292|0;r8=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r8>>2];r8=r7;r9=r7+8;r10=r7+40,r11=r10>>2;r12=r7+44;r13=r7+272;r14=r7+276;r15=r7+280;r16=r7+284;r17=r7+288;HEAP32[r8>>2]=37;HEAP32[r8+4>>2]=0;r18=r8;r8=r18+1|0;r19=r4+4|0;r20=HEAP32[r19>>2];if((r20&2048|0)==0){r21=r8}else{HEAP8[r8]=43;r21=r18+2|0}if((r20&1024|0)==0){r22=r21}else{HEAP8[r21]=35;r22=r21+1|0}r21=r20&260;r8=r20>>>14;do{if((r21|0)==260){HEAP8[r22]=76;r20=r22+1|0;if((r8&1|0)==0){HEAP8[r20]=97;r23=0;break}else{HEAP8[r20]=65;r23=0;break}}else{HEAP8[r22]=46;HEAP8[r22+1|0]=42;HEAP8[r22+2|0]=76;r20=r22+3|0;if((r21|0)==256){if((r8&1|0)==0){HEAP8[r20]=101;r23=1;break}else{HEAP8[r20]=69;r23=1;break}}else if((r21|0)==4){if((r8&1|0)==0){HEAP8[r20]=102;r23=1;break}else{HEAP8[r20]=70;r23=1;break}}else{if((r8&1|0)==0){HEAP8[r20]=103;r23=1;break}else{HEAP8[r20]=71;r23=1;break}}}}while(0);r8=r9|0;HEAP32[r11]=r8;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r9=HEAP32[1311652];if(r23){r24=__ZNSt3__112__snprintf_lEPcjPvPKcz(r8,30,r9,r18,(tempInt=STACKTOP,STACKTOP=STACKTOP+12|0,HEAP32[tempInt>>2]=HEAP32[r4+8>>2],HEAPF64[tempDoublePtr>>3]=r6,HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+8>>2]=HEAP32[tempDoublePtr+4>>2],tempInt))}else{r24=__ZNSt3__112__snprintf_lEPcjPvPKcz(r8,30,r9,r18,(tempInt=STACKTOP,STACKTOP=STACKTOP+8|0,HEAPF64[tempDoublePtr>>3]=r6,HEAP32[tempInt>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr+4>>2],tempInt))}do{if((r24|0)>29){r9=HEAP8[5255484]<<24>>24==0;if(r23){do{if(r9){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r25=__ZNSt3__112__asprintf_lEPPcPvPKcz(r10,HEAP32[1311652],r18,(tempInt=STACKTOP,STACKTOP=STACKTOP+12|0,HEAP32[tempInt>>2]=HEAP32[r4+8>>2],HEAPF64[tempDoublePtr>>3]=r6,HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+8>>2]=HEAP32[tempDoublePtr+4>>2],tempInt))}else{do{if(r9){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r25=__ZNSt3__112__asprintf_lEPPcPvPKcz(r10,HEAP32[1311652],r18,(tempInt=STACKTOP,STACKTOP=STACKTOP+8|0,HEAPF64[tempDoublePtr>>3]=r6,HEAP32[tempInt>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr+4>>2],tempInt))}r9=HEAP32[r11];if((r9|0)!=0){r26=r25;r27=r9;r28=r9;break}r9=___cxa_allocate_exception(4);HEAP32[r9>>2]=5247464;___cxa_throw(r9,5252688,66)}else{r26=r24;r27=0;r28=HEAP32[r11]}}while(0);r24=r28+r26|0;r25=HEAP32[r19>>2]&176;do{if((r25|0)==32){r29=r24}else if((r25|0)==16){r19=HEAP8[r28];if(r19<<24>>24==45|r19<<24>>24==43){r29=r28+1|0;break}if(!((r26|0)>1&r19<<24>>24==48)){r2=1908;break}r19=HEAP8[r28+1|0];if(!(r19<<24>>24==120|r19<<24>>24==88)){r2=1908;break}r29=r28+2|0;break}else{r2=1908}}while(0);if(r2==1908){r29=r28}do{if((r28|0)==(r8|0)){r30=r12|0;r31=0;r32=r8}else{r2=_malloc(r26<<3);r25=r2;if((r2|0)!=0){r30=r25;r31=r25;r32=HEAP32[r11];break}r25=___cxa_allocate_exception(4);HEAP32[r25>>2]=5247464;___cxa_throw(r25,5252688,66)}}while(0);r11=r15|0;r26=HEAP32[r4+28>>2];HEAP32[r11>>2]=r26;r8=r26+4|0;tempValue=HEAP32[r8>>2],HEAP32[r8>>2]=tempValue+1,tempValue;__ZNSt3__19__num_putIwE23__widen_and_group_floatEPcS2_S2_PwRS3_S4_RKNS_6localeE(r32,r29,r24,r30,r13,r14,r15);r15=HEAP32[r11>>2];r11=r15+4|0;if(((tempValue=HEAP32[r11>>2],HEAP32[r11>>2]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r15>>2]+8>>2]](r15|0)}r15=r3|0;HEAP32[r17>>2]=HEAP32[r15>>2];__ZNSt3__116__pad_and_outputIwNS_11char_traitsIwEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r16,r17,r30,HEAP32[r13>>2],HEAP32[r14>>2],r4,r5);r5=HEAP32[r16>>2];HEAP32[r15>>2]=r5;HEAP32[r1>>2]=r5;if((r31|0)!=0){_free(r31)}if((r27|0)==0){STACKTOP=r7;return}_free(r27);STACKTOP=r7;return}function __ZNKSt3__17num_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_RNS_8ios_baseEwPKv(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23;r2=0;r7=STACKTOP;STACKTOP=STACKTOP+192|0;r8=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r8>>2];r8=r7,r9=r8>>2;r10=r7+20;r11=r7+40;r12=r7+188;r13=r7+12|0;HEAP8[r13]=HEAP8[5247328];HEAP8[r13+1|0]=HEAP8[5247329|0];HEAP8[r13+2|0]=HEAP8[5247330|0];HEAP8[r13+3|0]=HEAP8[5247331|0];HEAP8[r13+4|0]=HEAP8[5247332|0];HEAP8[r13+5|0]=HEAP8[5247333|0];r14=r10|0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r15=__ZNSt3__111__sprintf_lEPcPvPKcz(r14,HEAP32[1311652],r13,(tempInt=STACKTOP,STACKTOP=STACKTOP+4|0,HEAP32[tempInt>>2]=r6,tempInt));r6=r10+r15|0;r13=HEAP32[r4+4>>2]&176;do{if((r13|0)==16){r16=HEAP8[r14];if(r16<<24>>24==45|r16<<24>>24==43){r17=r10+1|0;break}if(!((r15|0)>1&r16<<24>>24==48)){r2=1944;break}r16=HEAP8[r10+1|0];if(!(r16<<24>>24==120|r16<<24>>24==88)){r2=1944;break}r17=r10+2|0;break}else if((r13|0)==32){r17=r6}else{r2=1944}}while(0);if(r2==1944){r17=r14}r2=HEAP32[r4+28>>2],r13=r2>>2;r16=(r2+4|0)>>2;tempValue=HEAP32[r16],HEAP32[r16]=tempValue+1,tempValue;if((HEAP32[1313725]|0)!=-1){HEAP32[r9]=5254900;HEAP32[r9+1]=24;HEAP32[r9+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r8,406)}r8=HEAP32[1313726]-1|0;r9=HEAP32[r13+2];do{if(HEAP32[r13+3]-r9>>2>>>0>r8>>>0){r18=HEAP32[r9+(r8<<2)>>2];if((r18|0)==0){break}if(((tempValue=HEAP32[r16],HEAP32[r16]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r13]+8>>2]](r2)}r19=r11|0;FUNCTION_TABLE[HEAP32[HEAP32[r18>>2]+48>>2]](r18,r14,r6,r19);r18=(r15<<2)+r11|0;if((r17|0)==(r6|0)){r20=r18;r21=r3|0;r22=HEAP32[r21>>2];r23=r12|0;HEAP32[r23>>2]=r22;__ZNSt3__116__pad_and_outputIwNS_11char_traitsIwEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r12,r19,r20,r18,r4,r5);STACKTOP=r7;return}r20=(r17-r10<<2)+r11|0;r21=r3|0;r22=HEAP32[r21>>2];r23=r12|0;HEAP32[r23>>2]=r22;__ZNSt3__116__pad_and_outputIwNS_11char_traitsIwEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r12,r19,r20,r18,r4,r5);STACKTOP=r7;return}}while(0);r7=___cxa_allocate_exception(4);HEAP32[r7>>2]=5247488;___cxa_throw(r7,5252700,514)}function __ZNSt3__19money_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEED1Ev(r1){return}function __ZNSt3__19money_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEED0Ev(r1){__ZdlPv(r1);return}function __ZNKSt3__19money_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_bRNS_8ios_baseERjRe(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42;r2=0;r9=STACKTOP;STACKTOP=STACKTOP+248|0;r10=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r10>>2];r10=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r10>>2];r10=r9,r11=r10>>2;r12=r9+12;r13=r9+112;r14=r9+120;r15=r9+124;r16=r9+128;r17=r9+132;r18=r9+136;r19=r9+148;r20=(r13|0)>>2;HEAP32[r20]=r12|0;r21=r13+4|0;HEAP32[r21>>2]=414;r22=(r15|0)>>2;r23=HEAP32[r6+28>>2];HEAP32[r22]=r23;r24=r23+4|0;tempValue=HEAP32[r24>>2],HEAP32[r24>>2]=tempValue+1,tempValue;r24=HEAP32[r22];if((HEAP32[1313727]|0)!=-1){HEAP32[r11]=5254908;HEAP32[r11+1]=24;HEAP32[r11+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r10,406)}r10=HEAP32[1313728]-1|0;r11=HEAP32[r24+8>>2];do{if(HEAP32[r24+12>>2]-r11>>2>>>0>r10>>>0){r23=HEAP32[r11+(r10<<2)>>2];if((r23|0)==0){break}r25=r23;HEAP8[r16]=0;r26=(r4|0)>>2;HEAP32[r17>>2]=HEAP32[r26];do{if(__ZNSt3__19money_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE8__do_getERS4_S4_bRKNS_6localeEjRjRbRKNS_5ctypeIcEERNS_10unique_ptrIcPFvPvEEERPcSM_(r3,r17,r5,r15,HEAP32[r6+4>>2],r7,r16,r25,r13,r14,r12+100|0)){r27=r18|0;FUNCTION_TABLE[HEAP32[HEAP32[r23>>2]+32>>2]](r25,5247064,5247074,r27);r28=r19|0;r29=HEAP32[r14>>2];r30=HEAP32[r20];r31=r29-r30|0;do{if((r31|0)>98){r32=_malloc(r31+2|0);if((r32|0)!=0){r33=r32;r34=r32;break}r32=___cxa_allocate_exception(4);HEAP32[r32>>2]=5247464;___cxa_throw(r32,5252688,66)}else{r33=r28;r34=0}}while(0);if((HEAP8[r16]&1)<<24>>24==0){r35=r33}else{HEAP8[r33]=45;r35=r33+1|0}L2309:do{if(r30>>>0<r29>>>0){r31=r18+10|0;r32=r18;r36=r35;r37=r30;while(1){r38=r27;while(1){if((r38|0)==(r31|0)){r39=r31;break}if(HEAP8[r38]<<24>>24==HEAP8[r37]<<24>>24){r39=r38;break}else{r38=r38+1|0}}HEAP8[r36]=HEAP8[r39-r32+5247064|0];r38=r37+1|0;r40=r36+1|0;if(r38>>>0<HEAP32[r14>>2]>>>0){r36=r40;r37=r38}else{r41=r40;break L2309}}}else{r41=r35}}while(0);HEAP8[r41]=0;if((_sscanf(r28,5243472,(tempInt=STACKTOP,STACKTOP=STACKTOP+4|0,HEAP32[tempInt>>2]=r8,tempInt))|0)!=1){__ZNSt3__121__throw_runtime_errorEPKc(5243456)}if((r34|0)==0){break}_free(r34)}}while(0);r25=r3|0;r23=HEAP32[r25>>2],r27=r23>>2;do{if((r23|0)==0){r42=0}else{if((HEAP32[r27+3]|0)!=(HEAP32[r27+4]|0)){r42=r23;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r23)|0)!=-1){r42=r23;break}HEAP32[r25>>2]=0;r42=0}}while(0);r25=(r42|0)==0;r23=HEAP32[r26],r27=r23>>2;do{if((r23|0)==0){r2=2003}else{if((HEAP32[r27+3]|0)!=(HEAP32[r27+4]|0)){if(r25){break}else{r2=2005;break}}if((FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r23)|0)==-1){HEAP32[r26]=0;r2=2003;break}else{if(r25^(r23|0)==0){break}else{r2=2005;break}}}}while(0);do{if(r2==2003){if(r25){r2=2005;break}else{break}}}while(0);if(r2==2005){HEAP32[r7>>2]=HEAP32[r7>>2]|2}HEAP32[r1>>2]=r42;r25=HEAP32[r22];r23=r25+4|0;if(((tempValue=HEAP32[r23>>2],HEAP32[r23>>2]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r25>>2]+8>>2]](r25|0)}r25=HEAP32[r20];HEAP32[r20]=0;if((r25|0)==0){STACKTOP=r9;return}FUNCTION_TABLE[HEAP32[r21>>2]](r25);STACKTOP=r9;return}}while(0);r9=___cxa_allocate_exception(4);HEAP32[r9>>2]=5247488;___cxa_throw(r9,5252700,514)}function __ZNKSt3__19money_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_getES4_S4_bRNS_8ios_baseERjRNS_12basic_stringIcS3_NS_9allocatorIcEEEE(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37;r2=r8>>2;r9=0;r10=STACKTOP;STACKTOP=STACKTOP+136|0;r11=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r11>>2];r11=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r11>>2];r11=r10,r12=r11>>2;r13=r10+12;r14=r10+112;r15=r10+120;r16=r10+124;r17=r10+128;r18=r10+132;r19=(r14|0)>>2;HEAP32[r19]=r13|0;r20=r14+4|0;HEAP32[r20>>2]=414;r21=(r16|0)>>2;r22=HEAP32[r6+28>>2];HEAP32[r21]=r22;r23=r22+4|0;tempValue=HEAP32[r23>>2],HEAP32[r23>>2]=tempValue+1,tempValue;r23=HEAP32[r21];if((HEAP32[1313727]|0)!=-1){HEAP32[r12]=5254908;HEAP32[r12+1]=24;HEAP32[r12+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r11,406)}r11=HEAP32[1313728]-1|0;r12=HEAP32[r23+8>>2];do{if(HEAP32[r23+12>>2]-r12>>2>>>0>r11>>>0){r22=HEAP32[r12+(r11<<2)>>2];if((r22|0)==0){break}r24=r22;HEAP8[r17]=0;r25=r4|0;r26=HEAP32[r25>>2],r27=r26>>2;HEAP32[r18>>2]=r26;if(__ZNSt3__19money_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE8__do_getERS4_S4_bRKNS_6localeEjRjRbRKNS_5ctypeIcEERNS_10unique_ptrIcPFvPvEEERPcSM_(r3,r18,r5,r16,HEAP32[r6+4>>2],r7,r17,r24,r14,r15,r13+100|0)){r28=r8;if((HEAP8[r28]&1)<<24>>24==0){HEAP8[r8+1|0]=0;HEAP8[r28]=0}else{HEAP8[HEAP32[r2+2]]=0;HEAP32[r2+1]=0}r29=r22;do{if((HEAP8[r17]&1)<<24>>24!=0){r22=FUNCTION_TABLE[HEAP32[HEAP32[r29>>2]+28>>2]](r24,45);r30=HEAP8[r28];if((r30&1)<<24>>24==0){r31=10;r32=r30}else{r30=HEAP32[r2];r31=(r30&-2)-1|0;r32=r30&255}r30=r32&255;if((r30&1|0)==0){r33=r30>>>1}else{r33=HEAP32[r2+1]}if((r33|0)==(r31|0)){__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE9__grow_byEjjjjjj(r8,r31,1,r31,r31,0,0);r34=HEAP8[r28]}else{r34=r32}if((r34&1)<<24>>24==0){r35=r8+1|0}else{r35=HEAP32[r2+2]}HEAP8[r35+r33|0]=r22;r22=r33+1|0;HEAP8[r35+r22|0]=0;if((HEAP8[r28]&1)<<24>>24==0){HEAP8[r28]=r22<<1&255;break}else{HEAP32[r2+1]=r22;break}}}while(0);r28=FUNCTION_TABLE[HEAP32[HEAP32[r29>>2]+28>>2]](r24,48);r22=HEAP32[r15>>2];r30=r22-1|0;r36=HEAP32[r19];while(1){if(r36>>>0>=r30>>>0){break}if(HEAP8[r36]<<24>>24==r28<<24>>24){r36=r36+1|0}else{break}}__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendIPcEENS_9enable_ifIXsr21__is_forward_iteratorIT_EE5valueERS5_E4typeES9_S9_(r8,r36,r22)}r28=r3|0;r30=HEAP32[r28>>2],r24=r30>>2;do{if((r30|0)==0){r37=0}else{if((HEAP32[r24+3]|0)!=(HEAP32[r24+4]|0)){r37=r30;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r24]+36>>2]](r30)|0)!=-1){r37=r30;break}HEAP32[r28>>2]=0;r37=0}}while(0);r28=(r37|0)==0;do{if((r26|0)==0){r9=2066}else{if((HEAP32[r27+3]|0)!=(HEAP32[r27+4]|0)){if(r28){break}else{r9=2068;break}}if((FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r26)|0)==-1){HEAP32[r25>>2]=0;r9=2066;break}else{if(r28^(r26|0)==0){break}else{r9=2068;break}}}}while(0);do{if(r9==2066){if(r28){r9=2068;break}else{break}}}while(0);if(r9==2068){HEAP32[r7>>2]=HEAP32[r7>>2]|2}HEAP32[r1>>2]=r37;r28=HEAP32[r21];r26=r28+4|0;if(((tempValue=HEAP32[r26>>2],HEAP32[r26>>2]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r28>>2]+8>>2]](r28|0)}r28=HEAP32[r19];HEAP32[r19]=0;if((r28|0)==0){STACKTOP=r10;return}FUNCTION_TABLE[HEAP32[r20>>2]](r28);STACKTOP=r10;return}}while(0);r10=___cxa_allocate_exception(4);HEAP32[r10>>2]=5247488;___cxa_throw(r10,5252700,514)}function __ZNSt3__19money_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEED1Ev(r1){return}function __ZNSt3__19money_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEED0Ev(r1){__ZdlPv(r1);return}function __ZNKSt3__19money_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_bRNS_8ios_baseERjRe(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44;r2=0;r9=STACKTOP;STACKTOP=STACKTOP+576|0;r10=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r10>>2];r10=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r10>>2];r10=r9,r11=r10>>2;r12=r9+12;r13=r9+412;r14=r9+420;r15=r9+424;r16=r9+428;r17=r9+432;r18=r9+436;r19=r9+476;r20=(r13|0)>>2;HEAP32[r20]=r12|0;r21=r13+4|0;HEAP32[r21>>2]=414;r22=(r15|0)>>2;r23=HEAP32[r6+28>>2];HEAP32[r22]=r23;r24=r23+4|0;tempValue=HEAP32[r24>>2],HEAP32[r24>>2]=tempValue+1,tempValue;r24=HEAP32[r22];if((HEAP32[1313725]|0)!=-1){HEAP32[r11]=5254900;HEAP32[r11+1]=24;HEAP32[r11+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r10,406)}r10=HEAP32[1313726]-1|0;r11=HEAP32[r24+8>>2];do{if(HEAP32[r24+12>>2]-r11>>2>>>0>r10>>>0){r23=HEAP32[r11+(r10<<2)>>2];if((r23|0)==0){break}r25=r23;HEAP8[r16]=0;r26=(r4|0)>>2;HEAP32[r17>>2]=HEAP32[r26];do{if(__ZNSt3__19money_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE8__do_getERS4_S4_bRKNS_6localeEjRjRbRKNS_5ctypeIwEERNS_10unique_ptrIwPFvPvEEERPwSM_(r3,r17,r5,r15,HEAP32[r6+4>>2],r7,r16,r25,r13,r14,r12+400|0)){r27=r18|0;FUNCTION_TABLE[HEAP32[HEAP32[r23>>2]+48>>2]](r25,5247052,5247062,r27);r28=r19|0;r29=HEAP32[r14>>2];r30=HEAP32[r20];r31=r29-r30|0;do{if((r31|0)>392){r32=_malloc(r31+8>>2|0);if((r32|0)!=0){r33=r32;r34=r32;break}r32=___cxa_allocate_exception(4);HEAP32[r32>>2]=5247464;___cxa_throw(r32,5252688,66)}else{r33=r28;r34=0}}while(0);if((HEAP8[r16]&1)<<24>>24==0){r35=r33}else{HEAP8[r33]=45;r35=r33+1|0}L2444:do{if(r30>>>0<r29>>>0){r31=r18+160|0;r32=r18;r36=r35;r37=r30;while(1){r38=r27;while(1){if((r38|0)==(r31|0)){r39=r31;break}if((HEAP32[r38>>2]|0)==(HEAP32[r37>>2]|0)){r39=r38;break}else{r38=r38+4|0}}HEAP8[r36]=HEAP8[r39-r32+20988208>>2|0];r38=r37+4|0;r40=r36+1|0;if(r38>>>0<HEAP32[r14>>2]>>>0){r36=r40;r37=r38}else{r41=r40;break L2444}}}else{r41=r35}}while(0);HEAP8[r41]=0;if((_sscanf(r28,5243472,(tempInt=STACKTOP,STACKTOP=STACKTOP+4|0,HEAP32[tempInt>>2]=r8,tempInt))|0)!=1){__ZNSt3__121__throw_runtime_errorEPKc(5243456)}if((r34|0)==0){break}_free(r34)}}while(0);r25=r3|0;r23=HEAP32[r25>>2],r27=r23>>2;do{if((r23|0)==0){r42=0}else{r30=HEAP32[r27+3];if((r30|0)==(HEAP32[r27+4]|0)){r43=FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r23)}else{r43=HEAP32[r30>>2]}if((r43|0)!=-1){r42=r23;break}HEAP32[r25>>2]=0;r42=0}}while(0);r25=(r42|0)==0;r23=HEAP32[r26],r27=r23>>2;do{if((r23|0)==0){r2=2123}else{r30=HEAP32[r27+3];if((r30|0)==(HEAP32[r27+4]|0)){r44=FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r23)}else{r44=HEAP32[r30>>2]}if((r44|0)==-1){HEAP32[r26]=0;r2=2123;break}else{if(r25^(r23|0)==0){break}else{r2=2125;break}}}}while(0);do{if(r2==2123){if(r25){r2=2125;break}else{break}}}while(0);if(r2==2125){HEAP32[r7>>2]=HEAP32[r7>>2]|2}HEAP32[r1>>2]=r42;r25=HEAP32[r22];r23=r25+4|0;if(((tempValue=HEAP32[r23>>2],HEAP32[r23>>2]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r25>>2]+8>>2]](r25|0)}r25=HEAP32[r20];HEAP32[r20]=0;if((r25|0)==0){STACKTOP=r9;return}FUNCTION_TABLE[HEAP32[r21>>2]](r25);STACKTOP=r9;return}}while(0);r9=___cxa_allocate_exception(4);HEAP32[r9>>2]=5247488;___cxa_throw(r9,5252700,514)}function __ZNKSt3__19money_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_getES4_S4_bRNS_8ios_baseERjRNS_12basic_stringIwS3_NS_9allocatorIwEEEE(r1,r2,r3,r4,r5,r6,r7,r8){var r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39;r2=r8>>2;r9=0;r10=STACKTOP;STACKTOP=STACKTOP+436|0;r11=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r11>>2];r11=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r11>>2];r11=r10,r12=r11>>2;r13=r10+12;r14=r10+412;r15=r10+420;r16=r10+424;r17=r10+428;r18=r10+432;r19=(r14|0)>>2;HEAP32[r19]=r13|0;r20=r14+4|0;HEAP32[r20>>2]=414;r21=(r16|0)>>2;r22=HEAP32[r6+28>>2];HEAP32[r21]=r22;r23=r22+4|0;tempValue=HEAP32[r23>>2],HEAP32[r23>>2]=tempValue+1,tempValue;r23=HEAP32[r21];if((HEAP32[1313725]|0)!=-1){HEAP32[r12]=5254900;HEAP32[r12+1]=24;HEAP32[r12+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r11,406)}r11=HEAP32[1313726]-1|0;r12=HEAP32[r23+8>>2];do{if(HEAP32[r23+12>>2]-r12>>2>>>0>r11>>>0){r22=HEAP32[r12+(r11<<2)>>2];if((r22|0)==0){break}r24=r22;HEAP8[r17]=0;r25=r4|0;r26=HEAP32[r25>>2],r27=r26>>2;HEAP32[r18>>2]=r26;if(__ZNSt3__19money_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE8__do_getERS4_S4_bRKNS_6localeEjRjRbRKNS_5ctypeIwEERNS_10unique_ptrIwPFvPvEEERPwSM_(r3,r18,r5,r16,HEAP32[r6+4>>2],r7,r17,r24,r14,r15,r13+400|0)){r28=r8;if((HEAP8[r28]&1)<<24>>24==0){HEAP32[r2+1]=0;HEAP8[r28]=0}else{HEAP32[HEAP32[r2+2]>>2]=0;HEAP32[r2+1]=0}r29=r22;do{if((HEAP8[r17]&1)<<24>>24!=0){r22=FUNCTION_TABLE[HEAP32[HEAP32[r29>>2]+44>>2]](r24,45);r30=HEAP8[r28];if((r30&1)<<24>>24==0){r31=1;r32=r30}else{r30=HEAP32[r2];r31=(r30&-2)-1|0;r32=r30&255}r30=r32&255;if((r30&1|0)==0){r33=r30>>>1}else{r33=HEAP32[r2+1]}if((r33|0)==(r31|0)){__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE9__grow_byEjjjjjj(r8,r31,1,r31,r31,0,0);r34=HEAP8[r28]}else{r34=r32}if((r34&1)<<24>>24==0){r35=r8+4|0}else{r35=HEAP32[r2+2]}HEAP32[r35+(r33<<2)>>2]=r22;r22=r33+1|0;HEAP32[r35+(r22<<2)>>2]=0;if((HEAP8[r28]&1)<<24>>24==0){HEAP8[r28]=r22<<1&255;break}else{HEAP32[r2+1]=r22;break}}}while(0);r28=FUNCTION_TABLE[HEAP32[HEAP32[r29>>2]+44>>2]](r24,48);r22=HEAP32[r15>>2];r30=r22-4|0;r36=HEAP32[r19];while(1){if(r36>>>0>=r30>>>0){break}if((HEAP32[r36>>2]|0)==(r28|0)){r36=r36+4|0}else{break}}__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6appendIPwEENS_9enable_ifIXsr21__is_forward_iteratorIT_EE5valueERS5_E4typeES9_S9_(r8,r36,r22)}r28=r3|0;r30=HEAP32[r28>>2],r24=r30>>2;do{if((r30|0)==0){r37=0}else{r29=HEAP32[r24+3];if((r29|0)==(HEAP32[r24+4]|0)){r38=FUNCTION_TABLE[HEAP32[HEAP32[r24]+36>>2]](r30)}else{r38=HEAP32[r29>>2]}if((r38|0)!=-1){r37=r30;break}HEAP32[r28>>2]=0;r37=0}}while(0);r28=(r37|0)==0;do{if((r26|0)==0){r9=2187}else{r30=HEAP32[r27+3];if((r30|0)==(HEAP32[r27+4]|0)){r39=FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r26)}else{r39=HEAP32[r30>>2]}if((r39|0)==-1){HEAP32[r25>>2]=0;r9=2187;break}else{if(r28^(r26|0)==0){break}else{r9=2189;break}}}}while(0);do{if(r9==2187){if(r28){r9=2189;break}else{break}}}while(0);if(r9==2189){HEAP32[r7>>2]=HEAP32[r7>>2]|2}HEAP32[r1>>2]=r37;r28=HEAP32[r21];r26=r28+4|0;if(((tempValue=HEAP32[r26>>2],HEAP32[r26>>2]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r28>>2]+8>>2]](r28|0)}r28=HEAP32[r19];HEAP32[r19]=0;if((r28|0)==0){STACKTOP=r10;return}FUNCTION_TABLE[HEAP32[r20>>2]](r28);STACKTOP=r10;return}}while(0);r10=___cxa_allocate_exception(4);HEAP32[r10>>2]=5247488;___cxa_throw(r10,5252700,514)}function __ZNKSt3__19money_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_bRNS_8ios_baseEce(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48;r2=STACKTOP;STACKTOP=STACKTOP+244|0;r8=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r8>>2];r8=r2,r9=r8>>2;r10=r2+112,r11=r10>>2;r12=r2+216;r13=r2+220;r14=r2+224;r15=r2+228;r16=r2+232;r17=r16,r18=r17>>2;r19=STACKTOP,r20=r19>>2;STACKTOP=STACKTOP+12|0;r21=r19,r22=r21>>2;r23=STACKTOP,r24=r23>>2;STACKTOP=STACKTOP+12|0;r25=r23,r26=r25>>2;r27=STACKTOP;STACKTOP=STACKTOP+4|0;r28=STACKTOP;STACKTOP=STACKTOP+100|0;r29=STACKTOP;STACKTOP=STACKTOP+4|0;r30=STACKTOP;STACKTOP=STACKTOP+4|0;r31=STACKTOP;STACKTOP=STACKTOP+4|0;r32=r2+12|0;HEAP32[r11]=r32;r33=r2+116|0;r34=_snprintf(r32,100,5243448,(tempInt=STACKTOP,STACKTOP=STACKTOP+8|0,HEAPF64[tempDoublePtr>>3]=r7,HEAP32[tempInt>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr+4>>2],tempInt));do{if(r34>>>0>99){do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r32=__ZNSt3__112__asprintf_lEPPcPvPKcz(r10,HEAP32[1311652],5243448,(tempInt=STACKTOP,STACKTOP=STACKTOP+8|0,HEAPF64[tempDoublePtr>>3]=r7,HEAP32[tempInt>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr+4>>2],tempInt));r35=HEAP32[r11];if((r35|0)==0){r36=___cxa_allocate_exception(4);HEAP32[r36>>2]=5247464;___cxa_throw(r36,5252688,66)}r36=_malloc(r32);if((r36|0)!=0){r37=r36;r38=r32;r39=r35;r40=r36;break}r36=___cxa_allocate_exception(4);HEAP32[r36>>2]=5247464;___cxa_throw(r36,5252688,66)}else{r37=r33;r38=r34;r39=0;r40=0}}while(0);r34=(r12|0)>>2;r33=HEAP32[r5+28>>2];HEAP32[r34]=r33;r7=r33+4|0;tempValue=HEAP32[r7>>2],HEAP32[r7>>2]=tempValue+1,tempValue;r7=HEAP32[r34];if((HEAP32[1313727]|0)!=-1){HEAP32[r9]=5254908;HEAP32[r9+1]=24;HEAP32[r9+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r8,406)}r8=HEAP32[1313728]-1|0;r9=HEAP32[r7+8>>2];do{if(HEAP32[r7+12>>2]-r9>>2>>>0>r8>>>0){r33=HEAP32[r9+(r8<<2)>>2];if((r33|0)==0){break}r10=r33;r36=HEAP32[r11];FUNCTION_TABLE[HEAP32[HEAP32[r33>>2]+32>>2]](r10,r36,r36+r38|0,r37);if((r38|0)==0){r41=0}else{r41=HEAP8[HEAP32[r11]]<<24>>24==45}HEAP32[r18]=0;HEAP32[r18+1]=0;HEAP32[r18+2]=0;HEAP32[r22]=0;HEAP32[r22+1]=0;HEAP32[r22+2]=0;HEAP32[r26]=0;HEAP32[r26+1]=0;HEAP32[r26+2]=0;__ZNSt3__111__money_putIcE13__gather_infoEbbRKNS_6localeERNS_10money_base7patternERcS8_RNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEESF_SF_Ri(r4,r41,r12,r13,r14,r15,r16,r19,r23,r27);r36=r28|0;r33=HEAP32[r27>>2];if((r38|0)>(r33|0)){r35=HEAPU8[r25];if((r35&1|0)==0){r42=r35>>>1}else{r42=HEAP32[r24+1]}r35=HEAPU8[r21];if((r35&1|0)==0){r43=r35>>>1}else{r43=HEAP32[r20+1]}r44=(r38-r33<<1|1)+r42+r43|0}else{r35=HEAPU8[r25];if((r35&1|0)==0){r45=r35>>>1}else{r45=HEAP32[r24+1]}r35=HEAPU8[r21];if((r35&1|0)==0){r46=r35>>>1}else{r46=HEAP32[r20+1]}r44=r46+(r45+2)|0}r35=r44+r33|0;do{if(r35>>>0>100){r32=_malloc(r35);if((r32|0)!=0){r47=r32;r48=r32;break}r32=___cxa_allocate_exception(4);HEAP32[r32>>2]=5247464;___cxa_throw(r32,5252688,66)}else{r47=r36;r48=0}}while(0);__ZNSt3__111__money_putIcE8__formatEPcRS2_S3_jPKcS5_RKNS_5ctypeIcEEbRKNS_10money_base7patternEccRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEESL_SL_i(r47,r29,r30,HEAP32[r5+4>>2],r37,r37+r38|0,r10,r41,r13,HEAP8[r14],HEAP8[r15],r16,r19,r23,r33);HEAP32[r31>>2]=HEAP32[r3>>2];__ZNSt3__116__pad_and_outputIcNS_11char_traitsIcEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r31,r47,HEAP32[r29>>2],HEAP32[r30>>2],r5,r6);if((r48|0)!=0){_free(r48)}if((HEAP8[r25]&1)<<24>>24!=0){__ZdlPv(HEAP32[r24+2])}if((HEAP8[r21]&1)<<24>>24!=0){__ZdlPv(HEAP32[r20+2])}if((HEAP8[r17]&1)<<24>>24!=0){__ZdlPv(HEAP32[r16+8>>2])}r36=HEAP32[r34];r35=r36+4|0;if(((tempValue=HEAP32[r35>>2],HEAP32[r35>>2]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r36>>2]+8>>2]](r36|0)}if((r40|0)!=0){_free(r40)}if((r39|0)==0){STACKTOP=r2;return}_free(r39);STACKTOP=r2;return}}while(0);r2=___cxa_allocate_exception(4);HEAP32[r2>>2]=5247488;___cxa_throw(r2,5252700,514)}function __ZNKSt3__19money_putIcNS_19ostreambuf_iteratorIcNS_11char_traitsIcEEEEE6do_putES4_bRNS_8ios_baseEcRKNS_12basic_stringIcS3_NS_9allocatorIcEEEE(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48,r49,r50,r51,r52,r53,r54,r55,r56;r2=r7>>2;r8=STACKTOP;STACKTOP=STACKTOP+40|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r8,r10=r9>>2;r11=r8+12;r12=r8+16;r13=r8+20;r14=r8+24;r15=r8+28;r16=r15,r17=r16>>2;r18=STACKTOP,r19=r18>>2;STACKTOP=STACKTOP+12|0;r20=r18,r21=r20>>2;r22=STACKTOP,r23=r22>>2;STACKTOP=STACKTOP+12|0;r24=r22,r25=r24>>2;r26=STACKTOP;STACKTOP=STACKTOP+4|0;r27=STACKTOP;STACKTOP=STACKTOP+100|0;r28=STACKTOP;STACKTOP=STACKTOP+4|0;r29=STACKTOP;STACKTOP=STACKTOP+4|0;r30=STACKTOP;STACKTOP=STACKTOP+4|0;r31=(r11|0)>>2;r32=HEAP32[r5+28>>2];HEAP32[r31]=r32;r33=r32+4|0;tempValue=HEAP32[r33>>2],HEAP32[r33>>2]=tempValue+1,tempValue;r33=HEAP32[r31];if((HEAP32[1313727]|0)!=-1){HEAP32[r10]=5254908;HEAP32[r10+1]=24;HEAP32[r10+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r9,406)}r9=HEAP32[1313728]-1|0;r10=HEAP32[r33+8>>2];do{if(HEAP32[r33+12>>2]-r10>>2>>>0>r9>>>0){r32=HEAP32[r10+(r9<<2)>>2];if((r32|0)==0){break}r34=r32;r35=r7;r36=r7;r37=HEAP8[r36];r38=r37&255;if((r38&1|0)==0){r39=r38>>>1}else{r39=HEAP32[r2+1]}if((r39|0)==0){r40=0}else{if((r37&1)<<24>>24==0){r41=r35+1|0}else{r41=HEAP32[r2+2]}r40=HEAP8[r41]<<24>>24==FUNCTION_TABLE[HEAP32[HEAP32[r32>>2]+28>>2]](r34,45)<<24>>24}HEAP32[r17]=0;HEAP32[r17+1]=0;HEAP32[r17+2]=0;HEAP32[r21]=0;HEAP32[r21+1]=0;HEAP32[r21+2]=0;HEAP32[r25]=0;HEAP32[r25+1]=0;HEAP32[r25+2]=0;__ZNSt3__111__money_putIcE13__gather_infoEbbRKNS_6localeERNS_10money_base7patternERcS8_RNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEESF_SF_Ri(r4,r40,r11,r12,r13,r14,r15,r18,r22,r26);r32=r27|0;r37=HEAP8[r36];r38=r37&255;r42=(r38&1|0)==0;if(r42){r43=r38>>>1}else{r43=HEAP32[r2+1]}r44=HEAP32[r26>>2];if((r43|0)>(r44|0)){if(r42){r45=r38>>>1}else{r45=HEAP32[r2+1]}r38=HEAPU8[r24];if((r38&1|0)==0){r46=r38>>>1}else{r46=HEAP32[r23+1]}r38=HEAPU8[r20];if((r38&1|0)==0){r47=r38>>>1}else{r47=HEAP32[r19+1]}r48=(r45-r44<<1|1)+r46+r47|0}else{r38=HEAPU8[r24];if((r38&1|0)==0){r49=r38>>>1}else{r49=HEAP32[r23+1]}r38=HEAPU8[r20];if((r38&1|0)==0){r50=r38>>>1}else{r50=HEAP32[r19+1]}r48=r50+(r49+2)|0}r38=r48+r44|0;do{if(r38>>>0>100){r42=_malloc(r38);if((r42|0)!=0){r51=r42;r52=r42;r53=HEAP8[r36];break}r42=___cxa_allocate_exception(4);HEAP32[r42>>2]=5247464;___cxa_throw(r42,5252688,66)}else{r51=r32;r52=0;r53=r37}}while(0);if((r53&1)<<24>>24==0){r54=r35+1|0;r55=r35+1|0}else{r37=HEAP32[r2+2];r54=r37;r55=r37}r37=r53&255;if((r37&1|0)==0){r56=r37>>>1}else{r56=HEAP32[r2+1]}__ZNSt3__111__money_putIcE8__formatEPcRS2_S3_jPKcS5_RKNS_5ctypeIcEEbRKNS_10money_base7patternEccRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEESL_SL_i(r51,r28,r29,HEAP32[r5+4>>2],r55,r54+r56|0,r34,r40,r12,HEAP8[r13],HEAP8[r14],r15,r18,r22,r44);HEAP32[r30>>2]=HEAP32[r3>>2];__ZNSt3__116__pad_and_outputIcNS_11char_traitsIcEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r30,r51,HEAP32[r28>>2],HEAP32[r29>>2],r5,r6);if((r52|0)!=0){_free(r52)}if((HEAP8[r24]&1)<<24>>24!=0){__ZdlPv(HEAP32[r23+2])}if((HEAP8[r20]&1)<<24>>24!=0){__ZdlPv(HEAP32[r19+2])}if((HEAP8[r16]&1)<<24>>24!=0){__ZdlPv(HEAP32[r15+8>>2])}r37=HEAP32[r31];r32=r37+4|0;if(((tempValue=HEAP32[r32>>2],HEAP32[r32>>2]=tempValue+ -1,tempValue)|0)!=0){STACKTOP=r8;return}FUNCTION_TABLE[HEAP32[HEAP32[r37>>2]+8>>2]](r37|0);STACKTOP=r8;return}}while(0);r8=___cxa_allocate_exception(4);HEAP32[r8>>2]=5247488;___cxa_throw(r8,5252700,514)}function __ZNSt3__19money_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEED1Ev(r1){return}function __ZNSt3__19money_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEED0Ev(r1){__ZdlPv(r1);return}function __ZNKSt3__19money_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_bRNS_8ios_baseEwe(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48,r49;r2=STACKTOP;STACKTOP=STACKTOP+544|0;r8=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r8>>2];r8=r2,r9=r8>>2;r10=r2+112,r11=r10>>2;r12=r2+516;r13=r2+520;r14=r2+524;r15=r2+528;r16=r2+532;r17=r16,r18=r17>>2;r19=STACKTOP,r20=r19>>2;STACKTOP=STACKTOP+12|0;r21=r19,r22=r21>>2;r23=STACKTOP,r24=r23>>2;STACKTOP=STACKTOP+12|0;r25=r23,r26=r25>>2;r27=STACKTOP;STACKTOP=STACKTOP+4|0;r28=STACKTOP;STACKTOP=STACKTOP+400|0;r29=STACKTOP;STACKTOP=STACKTOP+4|0;r30=STACKTOP;STACKTOP=STACKTOP+4|0;r31=STACKTOP;STACKTOP=STACKTOP+4|0;r32=r2+12|0;HEAP32[r11]=r32;r33=r2+116|0;r34=_snprintf(r32,100,5243448,(tempInt=STACKTOP,STACKTOP=STACKTOP+8|0,HEAPF64[tempDoublePtr>>3]=r7,HEAP32[tempInt>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr+4>>2],tempInt));do{if(r34>>>0>99){do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r32=__ZNSt3__112__asprintf_lEPPcPvPKcz(r10,HEAP32[1311652],5243448,(tempInt=STACKTOP,STACKTOP=STACKTOP+8|0,HEAPF64[tempDoublePtr>>3]=r7,HEAP32[tempInt>>2]=HEAP32[tempDoublePtr>>2],HEAP32[tempInt+4>>2]=HEAP32[tempDoublePtr+4>>2],tempInt));r35=HEAP32[r11];if((r35|0)==0){r36=___cxa_allocate_exception(4);HEAP32[r36>>2]=5247464;___cxa_throw(r36,5252688,66)}r36=_malloc(r32<<2);r37=r36;if((r36|0)!=0){r38=r37;r39=r32;r40=r35;r41=r37;break}r37=___cxa_allocate_exception(4);HEAP32[r37>>2]=5247464;___cxa_throw(r37,5252688,66)}else{r38=r33;r39=r34;r40=0;r41=0}}while(0);r34=(r12|0)>>2;r33=HEAP32[r5+28>>2];HEAP32[r34]=r33;r7=r33+4|0;tempValue=HEAP32[r7>>2],HEAP32[r7>>2]=tempValue+1,tempValue;r7=HEAP32[r34];if((HEAP32[1313725]|0)!=-1){HEAP32[r9]=5254900;HEAP32[r9+1]=24;HEAP32[r9+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r8,406)}r8=HEAP32[1313726]-1|0;r9=HEAP32[r7+8>>2];do{if(HEAP32[r7+12>>2]-r9>>2>>>0>r8>>>0){r33=HEAP32[r9+(r8<<2)>>2];if((r33|0)==0){break}r10=r33;r37=HEAP32[r11];FUNCTION_TABLE[HEAP32[HEAP32[r33>>2]+48>>2]](r10,r37,r37+r39|0,r38);if((r39|0)==0){r42=0}else{r42=HEAP8[HEAP32[r11]]<<24>>24==45}HEAP32[r18]=0;HEAP32[r18+1]=0;HEAP32[r18+2]=0;HEAP32[r22]=0;HEAP32[r22+1]=0;HEAP32[r22+2]=0;HEAP32[r26]=0;HEAP32[r26+1]=0;HEAP32[r26+2]=0;__ZNSt3__111__money_putIwE13__gather_infoEbbRKNS_6localeERNS_10money_base7patternERwS8_RNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEERNS9_IwNSA_IwEENSC_IwEEEESJ_Ri(r4,r42,r12,r13,r14,r15,r16,r19,r23,r27);r37=r28|0;r33=HEAP32[r27>>2];if((r39|0)>(r33|0)){r35=HEAPU8[r25];if((r35&1|0)==0){r43=r35>>>1}else{r43=HEAP32[r24+1]}r35=HEAPU8[r21];if((r35&1|0)==0){r44=r35>>>1}else{r44=HEAP32[r20+1]}r45=(r39-r33<<1|1)+r43+r44|0}else{r35=HEAPU8[r25];if((r35&1|0)==0){r46=r35>>>1}else{r46=HEAP32[r24+1]}r35=HEAPU8[r21];if((r35&1|0)==0){r47=r35>>>1}else{r47=HEAP32[r20+1]}r45=r47+(r46+2)|0}r35=r45+r33|0;do{if(r35>>>0>100){r32=_malloc(r35<<2);r36=r32;if((r32|0)!=0){r48=r36;r49=r36;break}r36=___cxa_allocate_exception(4);HEAP32[r36>>2]=5247464;___cxa_throw(r36,5252688,66)}else{r48=r37;r49=0}}while(0);__ZNSt3__111__money_putIwE8__formatEPwRS2_S3_jPKwS5_RKNS_5ctypeIwEEbRKNS_10money_base7patternEwwRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEERKNSE_IwNSF_IwEENSH_IwEEEESQ_i(r48,r29,r30,HEAP32[r5+4>>2],r38,(r39<<2)+r38|0,r10,r42,r13,HEAP32[r14>>2],HEAP32[r15>>2],r16,r19,r23,r33);HEAP32[r31>>2]=HEAP32[r3>>2];__ZNSt3__116__pad_and_outputIwNS_11char_traitsIwEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r31,r48,HEAP32[r29>>2],HEAP32[r30>>2],r5,r6);if((r49|0)!=0){_free(r49)}if((HEAP8[r25]&1)<<24>>24!=0){__ZdlPv(HEAP32[r24+2])}if((HEAP8[r21]&1)<<24>>24!=0){__ZdlPv(HEAP32[r20+2])}if((HEAP8[r17]&1)<<24>>24!=0){__ZdlPv(HEAP32[r16+8>>2])}r37=HEAP32[r34];r35=r37+4|0;if(((tempValue=HEAP32[r35>>2],HEAP32[r35>>2]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r37>>2]+8>>2]](r37|0)}if((r41|0)!=0){_free(r41)}if((r40|0)==0){STACKTOP=r2;return}_free(r40);STACKTOP=r2;return}}while(0);r2=___cxa_allocate_exception(4);HEAP32[r2>>2]=5247488;___cxa_throw(r2,5252700,514)}function __ZNSt3__18messagesIcED1Ev(r1){return}function __ZNSt3__18messagesIwED1Ev(r1){return}function __ZNSt3__18messagesIcED0Ev(r1){__ZdlPv(r1);return}function __ZNSt3__18messagesIwED0Ev(r1){__ZdlPv(r1);return}function __ZNKSt3__19money_putIwNS_19ostreambuf_iteratorIwNS_11char_traitsIwEEEEE6do_putES4_bRNS_8ios_baseEwRKNS_12basic_stringIwS3_NS_9allocatorIwEEEE(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48,r49,r50,r51,r52,r53,r54,r55,r56;r2=r7>>2;r8=STACKTOP;STACKTOP=STACKTOP+40|0;r9=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r9>>2];r9=r8,r10=r9>>2;r11=r8+12;r12=r8+16;r13=r8+20;r14=r8+24;r15=r8+28;r16=r15,r17=r16>>2;r18=STACKTOP,r19=r18>>2;STACKTOP=STACKTOP+12|0;r20=r18,r21=r20>>2;r22=STACKTOP,r23=r22>>2;STACKTOP=STACKTOP+12|0;r24=r22,r25=r24>>2;r26=STACKTOP;STACKTOP=STACKTOP+4|0;r27=STACKTOP;STACKTOP=STACKTOP+400|0;r28=STACKTOP;STACKTOP=STACKTOP+4|0;r29=STACKTOP;STACKTOP=STACKTOP+4|0;r30=STACKTOP;STACKTOP=STACKTOP+4|0;r31=(r11|0)>>2;r32=HEAP32[r5+28>>2];HEAP32[r31]=r32;r33=r32+4|0;tempValue=HEAP32[r33>>2],HEAP32[r33>>2]=tempValue+1,tempValue;r33=HEAP32[r31];if((HEAP32[1313725]|0)!=-1){HEAP32[r10]=5254900;HEAP32[r10+1]=24;HEAP32[r10+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r9,406)}r9=HEAP32[1313726]-1|0;r10=HEAP32[r33+8>>2];do{if(HEAP32[r33+12>>2]-r10>>2>>>0>r9>>>0){r32=HEAP32[r10+(r9<<2)>>2];if((r32|0)==0){break}r34=r32;r35=r7;r36=HEAP8[r35];r37=r36&255;if((r37&1|0)==0){r38=r37>>>1}else{r38=HEAP32[r2+1]}if((r38|0)==0){r39=0}else{if((r36&1)<<24>>24==0){r40=r7+4|0}else{r40=HEAP32[r2+2]}r39=(HEAP32[r40>>2]|0)==(FUNCTION_TABLE[HEAP32[HEAP32[r32>>2]+44>>2]](r34,45)|0)}HEAP32[r17]=0;HEAP32[r17+1]=0;HEAP32[r17+2]=0;HEAP32[r21]=0;HEAP32[r21+1]=0;HEAP32[r21+2]=0;HEAP32[r25]=0;HEAP32[r25+1]=0;HEAP32[r25+2]=0;__ZNSt3__111__money_putIwE13__gather_infoEbbRKNS_6localeERNS_10money_base7patternERwS8_RNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEERNS9_IwNSA_IwEENSC_IwEEEESJ_Ri(r4,r39,r11,r12,r13,r14,r15,r18,r22,r26);r32=r27|0;r36=HEAP8[r35];r37=r36&255;r41=(r37&1|0)==0;if(r41){r42=r37>>>1}else{r42=HEAP32[r2+1]}r43=HEAP32[r26>>2];if((r42|0)>(r43|0)){if(r41){r44=r37>>>1}else{r44=HEAP32[r2+1]}r37=HEAPU8[r24];if((r37&1|0)==0){r45=r37>>>1}else{r45=HEAP32[r23+1]}r37=HEAPU8[r20];if((r37&1|0)==0){r46=r37>>>1}else{r46=HEAP32[r19+1]}r47=(r44-r43<<1|1)+r45+r46|0}else{r37=HEAPU8[r24];if((r37&1|0)==0){r48=r37>>>1}else{r48=HEAP32[r23+1]}r37=HEAPU8[r20];if((r37&1|0)==0){r49=r37>>>1}else{r49=HEAP32[r19+1]}r47=r49+(r48+2)|0}r37=r47+r43|0;do{if(r37>>>0>100){r41=_malloc(r37<<2);r50=r41;if((r41|0)!=0){r51=r50;r52=r50;r53=HEAP8[r35];break}r50=___cxa_allocate_exception(4);HEAP32[r50>>2]=5247464;___cxa_throw(r50,5252688,66)}else{r51=r32;r52=0;r53=r36}}while(0);if((r53&1)<<24>>24==0){r54=r7+4|0;r55=r7+4|0}else{r36=HEAP32[r2+2];r54=r36;r55=r36}r36=r53&255;if((r36&1|0)==0){r56=r36>>>1}else{r56=HEAP32[r2+1]}__ZNSt3__111__money_putIwE8__formatEPwRS2_S3_jPKwS5_RKNS_5ctypeIwEEbRKNS_10money_base7patternEwwRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEERKNSE_IwNSF_IwEENSH_IwEEEESQ_i(r51,r28,r29,HEAP32[r5+4>>2],r55,(r56<<2)+r54|0,r34,r39,r12,HEAP32[r13>>2],HEAP32[r14>>2],r15,r18,r22,r43);HEAP32[r30>>2]=HEAP32[r3>>2];__ZNSt3__116__pad_and_outputIwNS_11char_traitsIwEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r30,r51,HEAP32[r28>>2],HEAP32[r29>>2],r5,r6);if((r52|0)!=0){_free(r52)}if((HEAP8[r24]&1)<<24>>24!=0){__ZdlPv(HEAP32[r23+2])}if((HEAP8[r20]&1)<<24>>24!=0){__ZdlPv(HEAP32[r19+2])}if((HEAP8[r16]&1)<<24>>24!=0){__ZdlPv(HEAP32[r15+8>>2])}r36=HEAP32[r31];r32=r36+4|0;if(((tempValue=HEAP32[r32>>2],HEAP32[r32>>2]=tempValue+ -1,tempValue)|0)!=0){STACKTOP=r8;return}FUNCTION_TABLE[HEAP32[HEAP32[r36>>2]+8>>2]](r36|0);STACKTOP=r8;return}}while(0);r8=___cxa_allocate_exception(4);HEAP32[r8>>2]=5247488;___cxa_throw(r8,5252700,514)}function __ZNSt3__19__num_getIcE17__stage2_int_loopEciPcRS2_RjcRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSD_S2_(r1,r2,r3,r4,r5,r6,r7,r8,r9,r10){var r11,r12,r13,r14,r15,r16;r11=r5>>2;r5=r4>>2;r4=HEAP32[r5];r12=(r4|0)==(r3|0);do{if(r12){r13=HEAP8[r10+24|0]<<24>>24==r1<<24>>24;if(!r13){if(HEAP8[r10+25|0]<<24>>24!=r1<<24>>24){break}}HEAP32[r5]=r3+1|0;HEAP8[r3]=r13?43:45;HEAP32[r11]=0;r14=0;return r14}}while(0);r13=HEAPU8[r7];if((r13&1|0)==0){r15=r13>>>1}else{r15=HEAP32[r7+4>>2]}if((r15|0)!=0&r1<<24>>24==r6<<24>>24){r6=HEAP32[r9>>2];if((r6-r8|0)>=160){r14=0;return r14}r8=HEAP32[r11];HEAP32[r9>>2]=r6+4|0;HEAP32[r6>>2]=r8;HEAP32[r11]=0;r14=0;return r14}r8=r10+26|0;r6=r10;while(1){if((r6|0)==(r8|0)){r16=r8;break}if(HEAP8[r6]<<24>>24==r1<<24>>24){r16=r6;break}else{r6=r6+1|0}}r6=r16-r10|0;if((r6|0)>23){r14=-1;return r14}do{if((r2|0)==16){if((r6|0)<22){break}if(r12){r14=-1;return r14}if((r4-r3|0)>=3){r14=-1;return r14}if(HEAP8[r4-1|0]<<24>>24!=48){r14=-1;return r14}HEAP32[r11]=0;r10=HEAP8[r6+5255348|0];r16=HEAP32[r5];HEAP32[r5]=r16+1|0;HEAP8[r16]=r10;r14=0;return r14}else if((r2|0)==8|(r2|0)==10){if((r6|0)<(r2|0)){break}else{r14=-1}return r14}}while(0);if((r4-r3|0)<39){r3=HEAP8[r6+5255348|0];HEAP32[r5]=r4+1|0;HEAP8[r4]=r3}HEAP32[r11]=HEAP32[r11]+1|0;r14=0;return r14}function __ZNSt3__125__num_get_signed_integralIlEET_PKcS3_Rji(r1,r2,r3,r4){var r5,r6,r7,r8,r9;r5=STACKTOP;STACKTOP=STACKTOP+4|0;r6=r5;if((r1|0)==(r2|0)){HEAP32[r3>>2]=4;r7=0;STACKTOP=r5;return r7}r8=HEAP32[___errno_location()>>2];HEAP32[___errno_location()>>2]=0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r9=_strtoll(r1,r6,r4,HEAP32[1311652]);r4=tempRet0;r1=HEAP32[___errno_location()>>2];if((r1|0)==0){HEAP32[___errno_location()>>2]=r8}if((HEAP32[r6>>2]|0)!=(r2|0)){HEAP32[r3>>2]=4;r7=0;STACKTOP=r5;return r7}r2=-1;r6=0;if((r1|0)==34|((r4|0)<(r2|0)|(r4|0)==(r2|0)&r9>>>0<-2147483648>>>0)|((r4|0)>(r6|0)|(r4|0)==(r6|0)&r9>>>0>2147483647>>>0)){HEAP32[r3>>2]=4;r3=0;r7=(r4|0)>(r3|0)|(r4|0)==(r3|0)&r9>>>0>0>>>0?2147483647:-2147483648;STACKTOP=r5;return r7}else{r7=r9;STACKTOP=r5;return r7}}function __ZNSt3__114__scan_keywordINS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEPKNS_12basic_stringIcS3_NS_9allocatorIcEEEENS_5ctypeIcEEEET0_RT_SE_SD_SD_RKT1_Rjb(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46;r8=0;r9=STACKTOP;STACKTOP=STACKTOP+100|0;r10=r2;r2=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r2>>2]=HEAP32[r10>>2];r10=(r4-r3|0)/12&-1;r11=r9|0;do{if(r10>>>0>100){r12=_malloc(r10);if((r12|0)!=0){r13=r12;r14=r12;break}r12=___cxa_allocate_exception(4);HEAP32[r12>>2]=5247464;___cxa_throw(r12,5252688,66)}else{r13=r11;r14=0}}while(0);r11=(r3|0)==(r4|0);L2964:do{if(r11){r15=r10;r16=0}else{r12=r10;r17=0;r18=r13;r19=r3;while(1){r20=HEAPU8[r19];if((r20&1|0)==0){r21=r20>>>1}else{r21=HEAP32[r19+4>>2]}if((r21|0)==0){HEAP8[r18]=2;r22=r17+1|0;r23=r12-1|0}else{HEAP8[r18]=1;r22=r17;r23=r12}r20=r19+12|0;if((r20|0)==(r4|0)){r15=r23;r16=r22;break L2964}else{r12=r23;r17=r22;r18=r18+1|0;r19=r20}}}}while(0);r22=(r1|0)>>2;r1=(r2|0)>>2;r2=r5;r23=0;r21=r16;r16=r15;while(1){r15=HEAP32[r22],r10=r15>>2;do{if((r15|0)==0){r24=0}else{if((HEAP32[r10+3]|0)!=(HEAP32[r10+4]|0)){r24=r15;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r10]+36>>2]](r15)|0)==-1){HEAP32[r22]=0;r24=0;break}else{r24=HEAP32[r22];break}}}while(0);r15=(r24|0)==0;r10=HEAP32[r1],r19=r10>>2;if((r10|0)==0){r25=r24,r26=r25>>2;r27=0,r28=r27>>2}else{do{if((HEAP32[r19+3]|0)==(HEAP32[r19+4]|0)){if((FUNCTION_TABLE[HEAP32[HEAP32[r19]+36>>2]](r10)|0)!=-1){r29=r10;break}HEAP32[r1]=0;r29=0}else{r29=r10}}while(0);r25=HEAP32[r22],r26=r25>>2;r27=r29,r28=r27>>2}r30=(r27|0)==0;if(!((r15^r30)&(r16|0)!=0)){break}r10=HEAP32[r26+3];if((r10|0)==(HEAP32[r26+4]|0)){r31=FUNCTION_TABLE[HEAP32[HEAP32[r26]+36>>2]](r25)}else{r31=HEAPU8[r10]}r10=r31&255;if(r7){r32=r10}else{r32=FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+12>>2]](r5,r10)}L3002:do{if(r11){r33=r21;r34=r16}else{r10=r23+1|0;r19=r16;r18=r21;r17=r13;r12=0;r20=r3;while(1){do{if(HEAP8[r17]<<24>>24==1){r35=r20;if((HEAP8[r35]&1)<<24>>24==0){r36=r20+1|0}else{r36=HEAP32[r20+8>>2]}r37=HEAP8[r36+r23|0];if(r7){r38=r37}else{r38=FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+12>>2]](r5,r37)}if(r32<<24>>24!=r38<<24>>24){HEAP8[r17]=0;r39=r12;r40=r18;r41=r19-1|0;break}r37=HEAPU8[r35];if((r37&1|0)==0){r42=r37>>>1}else{r42=HEAP32[r20+4>>2]}if((r42|0)!=(r10|0)){r39=1;r40=r18;r41=r19;break}HEAP8[r17]=2;r39=1;r40=r18+1|0;r41=r19-1|0}else{r39=r12;r40=r18;r41=r19}}while(0);r37=r20+12|0;if((r37|0)==(r4|0)){break}r19=r41;r18=r40;r17=r17+1|0;r12=r39;r20=r37}if((r39&1)<<24>>24==0){r33=r40;r34=r41;break}r20=HEAP32[r22];r12=r20+12|0;r17=HEAP32[r12>>2];if((r17|0)==(HEAP32[r20+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r20>>2]+40>>2]](r20)}else{HEAP32[r12>>2]=r17+1|0}if((r40+r41|0)>>>0<2|r11){r33=r40;r34=r41;break}r17=r23+1|0;r12=r40;r20=r13;r18=r3;while(1){do{if(HEAP8[r20]<<24>>24==2){r19=HEAPU8[r18];if((r19&1|0)==0){r43=r19>>>1}else{r43=HEAP32[r18+4>>2]}if((r43|0)==(r17|0)){r44=r12;break}HEAP8[r20]=0;r44=r12-1|0}else{r44=r12}}while(0);r19=r18+12|0;if((r19|0)==(r4|0)){r33=r44;r34=r41;break L3002}else{r12=r44;r20=r20+1|0;r18=r19}}}}while(0);r23=r23+1|0;r21=r33;r16=r34}do{if((r25|0)==0){r45=0}else{if((HEAP32[r26+3]|0)!=(HEAP32[r26+4]|0)){r45=r25;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r26]+36>>2]](r25)|0)==-1){HEAP32[r22]=0;r45=0;break}else{r45=HEAP32[r22];break}}}while(0);r22=(r45|0)==0;do{if(r30){r8=2655}else{if((HEAP32[r28+3]|0)!=(HEAP32[r28+4]|0)){if(r22){break}else{r8=2657;break}}if((FUNCTION_TABLE[HEAP32[HEAP32[r28]+36>>2]](r27)|0)==-1){HEAP32[r1]=0;r8=2655;break}else{if(r22^(r27|0)==0){break}else{r8=2657;break}}}}while(0);do{if(r8==2655){if(r22){r8=2657;break}else{break}}}while(0);if(r8==2657){HEAP32[r6>>2]=HEAP32[r6>>2]|2}L3066:do{if(r11){r8=2662}else{r22=r3;r27=r13;while(1){if(HEAP8[r27]<<24>>24==2){r46=r22;break L3066}r1=r22+12|0;if((r1|0)==(r4|0)){r8=2662;break L3066}r22=r1;r27=r27+1|0}}}while(0);if(r8==2662){HEAP32[r6>>2]=HEAP32[r6>>2]|4;r46=r4}if((r14|0)==0){STACKTOP=r9;return r46}_free(r14);STACKTOP=r9;return r46}function __ZNSt3__19__num_getIcE17__stage2_int_prepERNS_8ios_baseEPcRc(r1,r2,r3,r4){var r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16;r5=STACKTOP;STACKTOP=STACKTOP+24|0;r6=r5,r7=r6>>2;r8=r5+12,r9=r8>>2;r10=HEAP32[r2+28>>2];r2=(r10+4|0)>>2;tempValue=HEAP32[r2],HEAP32[r2]=tempValue+1,tempValue;if((HEAP32[1313727]|0)!=-1){HEAP32[r9]=5254908;HEAP32[r9+1]=24;HEAP32[r9+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r8,406)}r8=HEAP32[1313728]-1|0;r9=r10+12|0;r11=r10+8|0;r12=HEAP32[r11>>2];do{if(HEAP32[r9>>2]-r12>>2>>>0>r8>>>0){r13=HEAP32[r12+(r8<<2)>>2];if((r13|0)==0){break}FUNCTION_TABLE[HEAP32[HEAP32[r13>>2]+32>>2]](r13,5255348,5255374,r3);if((HEAP32[1313635]|0)!=-1){HEAP32[r7]=5254540;HEAP32[r7+1]=24;HEAP32[r7+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254540,r6,406)}r13=HEAP32[1313636]-1|0;r14=HEAP32[r11>>2];do{if(HEAP32[r9>>2]-r14>>2>>>0>r13>>>0){r15=HEAP32[r14+(r13<<2)>>2];if((r15|0)==0){break}r16=r15;HEAP8[r4]=FUNCTION_TABLE[HEAP32[HEAP32[r15>>2]+16>>2]](r16);FUNCTION_TABLE[HEAP32[HEAP32[r15>>2]+20>>2]](r1,r16);if(((tempValue=HEAP32[r2],HEAP32[r2]=tempValue+ -1,tempValue)|0)!=0){STACKTOP=r5;return}FUNCTION_TABLE[HEAP32[HEAP32[r10>>2]+8>>2]](r10);STACKTOP=r5;return}}while(0);r13=___cxa_allocate_exception(4);HEAP32[r13>>2]=5247488;___cxa_throw(r13,5252700,514)}}while(0);r5=___cxa_allocate_exception(4);HEAP32[r5>>2]=5247488;___cxa_throw(r5,5252700,514)}function __ZNSt3__19__num_getIcE19__stage2_float_loopEcRbRcPcRS4_ccRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSE_RjS4_(r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12){var r13,r14,r15,r16,r17,r18;r13=r11>>2;r11=r10>>2;r10=r5>>2;if(r1<<24>>24==r6<<24>>24){if((HEAP8[r2]&1)<<24>>24==0){r14=-1;return r14}HEAP8[r2]=0;r6=HEAP32[r10];HEAP32[r10]=r6+1|0;HEAP8[r6]=46;r6=HEAPU8[r8];if((r6&1|0)==0){r15=r6>>>1}else{r15=HEAP32[r8+4>>2]}if((r15|0)==0){r14=0;return r14}r15=HEAP32[r11];if((r15-r9|0)>=160){r14=0;return r14}r6=HEAP32[r13];HEAP32[r11]=r15+4|0;HEAP32[r15>>2]=r6;r14=0;return r14}do{if(r1<<24>>24==r7<<24>>24){r6=HEAPU8[r8];if((r6&1|0)==0){r16=r6>>>1}else{r16=HEAP32[r8+4>>2]}if((r16|0)==0){break}if((HEAP8[r2]&1)<<24>>24==0){r14=-1;return r14}r6=HEAP32[r11];if((r6-r9|0)>=160){r14=0;return r14}r15=HEAP32[r13];HEAP32[r11]=r6+4|0;HEAP32[r6>>2]=r15;HEAP32[r13]=0;r14=0;return r14}}while(0);r16=r12+32|0;r7=r12;while(1){if((r7|0)==(r16|0)){r17=r16;break}if(HEAP8[r7]<<24>>24==r1<<24>>24){r17=r7;break}else{r7=r7+1|0}}r7=r17-r12|0;if((r7|0)>31){r14=-1;return r14}r12=HEAP8[r7+5255348|0];r17=r12&255;r1=HEAP32[r10];if((r7-24|0)>>>0<2){do{if((r1|0)!=(r4|0)){if((HEAP8[r1-1|0]&223|0)==(HEAP8[r3]<<24>>24|0)){break}else{r14=-1}return r14}}while(0);HEAP32[r10]=r1+1|0;HEAP8[r1]=r12;r14=0;return r14}if((r1-r4|0)<39){HEAP32[r10]=r1+1|0;HEAP8[r1]=r12}if((r7-22|0)>>>0<2){HEAP8[r3]=80;r14=0;return r14}do{if((r17&223|0)==(HEAP8[r3]<<24>>24|0)){HEAP8[r2]=0;r12=HEAPU8[r8];if((r12&1|0)==0){r18=r12>>>1}else{r18=HEAP32[r8+4>>2]}if((r18|0)==0){break}r12=HEAP32[r11];if((r12-r9|0)>=160){break}r1=HEAP32[r13];HEAP32[r11]=r12+4|0;HEAP32[r12>>2]=r1}}while(0);if((r7|0)>21){r14=0;return r14}HEAP32[r13]=HEAP32[r13]+1|0;r14=0;return r14}function __ZNSt3__125__num_get_signed_integralIxEET_PKcS3_Rji(r1,r2,r3,r4){var r5,r6,r7,r8,r9,r10,r11,r12,r13;r5=STACKTOP;STACKTOP=STACKTOP+4|0;r6=r5;do{if((r1|0)==(r2|0)){HEAP32[r3>>2]=4;r7=0;r8=0}else{r9=HEAP32[___errno_location()>>2];HEAP32[___errno_location()>>2]=0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r10=_strtoll(r1,r6,r4,HEAP32[1311652]);r11=tempRet0;r12=HEAP32[___errno_location()>>2];if((r12|0)==0){HEAP32[___errno_location()>>2]=r9}if((HEAP32[r6>>2]|0)!=(r2|0)){HEAP32[r3>>2]=4;r7=0;r8=0;break}if((r12|0)!=34){r7=r11;r8=r10;break}HEAP32[r3>>2]=4;r12=0;r13=(r11|0)>(r12|0)|(r11|0)==(r12|0)&r10>>>0>0>>>0;r7=r13?2147483647:-2147483648;r8=r13?-1:0}}while(0);STACKTOP=r5;return tempRet0=r7,r8}function __ZNSt3__127__num_get_unsigned_integralItEET_PKcS3_Rji(r1,r2,r3,r4){var r5,r6,r7,r8,r9;r5=STACKTOP;STACKTOP=STACKTOP+4|0;r6=r5;if((r1|0)==(r2|0)){HEAP32[r3>>2]=4;r7=0;STACKTOP=r5;return r7}if(HEAP8[r1]<<24>>24==45){HEAP32[r3>>2]=4;r7=0;STACKTOP=r5;return r7}r8=HEAP32[___errno_location()>>2];HEAP32[___errno_location()>>2]=0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r9=_strtoull(r1,r6,r4,HEAP32[1311652]);r4=tempRet0;r1=HEAP32[___errno_location()>>2];if((r1|0)==0){HEAP32[___errno_location()>>2]=r8}if((HEAP32[r6>>2]|0)!=(r2|0)){HEAP32[r3>>2]=4;r7=0;STACKTOP=r5;return r7}r2=0;if((r1|0)==34|(r4>>>0>r2>>>0|r4>>>0==r2>>>0&r9>>>0>65535>>>0)){HEAP32[r3>>2]=4;r7=-1;STACKTOP=r5;return r7}else{r7=r9&65535;STACKTOP=r5;return r7}}function __ZNSt3__127__num_get_unsigned_integralIjEET_PKcS3_Rji(r1,r2,r3,r4){var r5,r6,r7,r8,r9;r5=STACKTOP;STACKTOP=STACKTOP+4|0;r6=r5;if((r1|0)==(r2|0)){HEAP32[r3>>2]=4;r7=0;STACKTOP=r5;return r7}if(HEAP8[r1]<<24>>24==45){HEAP32[r3>>2]=4;r7=0;STACKTOP=r5;return r7}r8=HEAP32[___errno_location()>>2];HEAP32[___errno_location()>>2]=0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r9=_strtoull(r1,r6,r4,HEAP32[1311652]);r4=tempRet0;r1=HEAP32[___errno_location()>>2];if((r1|0)==0){HEAP32[___errno_location()>>2]=r8}if((HEAP32[r6>>2]|0)!=(r2|0)){HEAP32[r3>>2]=4;r7=0;STACKTOP=r5;return r7}r2=0;if((r1|0)==34|(r4>>>0>r2>>>0|r4>>>0==r2>>>0&r9>>>0>-1>>>0)){HEAP32[r3>>2]=4;r7=-1;STACKTOP=r5;return r7}else{r7=r9;STACKTOP=r5;return r7}}function __ZNSt3__127__num_get_unsigned_integralImEET_PKcS3_Rji(r1,r2,r3,r4){var r5,r6,r7,r8,r9;r5=STACKTOP;STACKTOP=STACKTOP+4|0;r6=r5;if((r1|0)==(r2|0)){HEAP32[r3>>2]=4;r7=0;STACKTOP=r5;return r7}if(HEAP8[r1]<<24>>24==45){HEAP32[r3>>2]=4;r7=0;STACKTOP=r5;return r7}r8=HEAP32[___errno_location()>>2];HEAP32[___errno_location()>>2]=0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r9=_strtoull(r1,r6,r4,HEAP32[1311652]);r4=tempRet0;r1=HEAP32[___errno_location()>>2];if((r1|0)==0){HEAP32[___errno_location()>>2]=r8}if((HEAP32[r6>>2]|0)!=(r2|0)){HEAP32[r3>>2]=4;r7=0;STACKTOP=r5;return r7}r2=0;if((r1|0)==34|(r4>>>0>r2>>>0|r4>>>0==r2>>>0&r9>>>0>-1>>>0)){HEAP32[r3>>2]=4;r7=-1;STACKTOP=r5;return r7}else{r7=r9;STACKTOP=r5;return r7}}function __ZNSt3__127__num_get_unsigned_integralIyEET_PKcS3_Rji(r1,r2,r3,r4){var r5,r6,r7,r8,r9,r10,r11,r12;r5=STACKTOP;STACKTOP=STACKTOP+4|0;r6=r5;do{if((r1|0)==(r2|0)){HEAP32[r3>>2]=4;r7=0;r8=0}else{if(HEAP8[r1]<<24>>24==45){HEAP32[r3>>2]=4;r7=0;r8=0;break}r9=HEAP32[___errno_location()>>2];HEAP32[___errno_location()>>2]=0;do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);r10=_strtoull(r1,r6,r4,HEAP32[1311652]);r11=tempRet0;r12=HEAP32[___errno_location()>>2];if((r12|0)==0){HEAP32[___errno_location()>>2]=r9}if((HEAP32[r6>>2]|0)!=(r2|0)){HEAP32[r3>>2]=4;r7=0;r8=0;break}if((r12|0)!=34){r7=r11;r8=r10;break}HEAP32[r3>>2]=4;r7=-1;r8=-1}}while(0);STACKTOP=r5;return tempRet0=r7,r8}function __ZNSt3__19__num_getIcE19__stage2_float_prepERNS_8ios_baseEPcRcS5_(r1,r2,r3,r4,r5){var r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18;r6=STACKTOP;STACKTOP=STACKTOP+24|0;r7=r6,r8=r7>>2;r9=r6+12,r10=r9>>2;r11=HEAP32[r2+28>>2];r2=(r11+4|0)>>2;tempValue=HEAP32[r2],HEAP32[r2]=tempValue+1,tempValue;if((HEAP32[1313727]|0)!=-1){HEAP32[r10]=5254908;HEAP32[r10+1]=24;HEAP32[r10+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r9,406)}r9=HEAP32[1313728]-1|0;r10=r11+12|0;r12=r11+8|0;r13=HEAP32[r12>>2];do{if(HEAP32[r10>>2]-r13>>2>>>0>r9>>>0){r14=HEAP32[r13+(r9<<2)>>2];if((r14|0)==0){break}FUNCTION_TABLE[HEAP32[HEAP32[r14>>2]+32>>2]](r14,5255348,5255380,r3);if((HEAP32[1313635]|0)!=-1){HEAP32[r8]=5254540;HEAP32[r8+1]=24;HEAP32[r8+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254540,r7,406)}r14=HEAP32[1313636]-1|0;r15=HEAP32[r12>>2];do{if(HEAP32[r10>>2]-r15>>2>>>0>r14>>>0){r16=HEAP32[r15+(r14<<2)>>2];if((r16|0)==0){break}r17=r16;r18=r16;HEAP8[r4]=FUNCTION_TABLE[HEAP32[HEAP32[r18>>2]+12>>2]](r17);HEAP8[r5]=FUNCTION_TABLE[HEAP32[HEAP32[r18>>2]+16>>2]](r17);FUNCTION_TABLE[HEAP32[HEAP32[r16>>2]+20>>2]](r1,r17);if(((tempValue=HEAP32[r2],HEAP32[r2]=tempValue+ -1,tempValue)|0)!=0){STACKTOP=r6;return}FUNCTION_TABLE[HEAP32[HEAP32[r11>>2]+8>>2]](r11);STACKTOP=r6;return}}while(0);r14=___cxa_allocate_exception(4);HEAP32[r14>>2]=5247488;___cxa_throw(r14,5252700,514)}}while(0);r6=___cxa_allocate_exception(4);HEAP32[r6>>2]=5247488;___cxa_throw(r6,5252700,514)}function __ZNSt3__110__sscanf_lEPKcPvS1_z(r1,r2,r3,r4){var r5,r6;r5=STACKTOP;STACKTOP=STACKTOP+4|0;r6=r5;HEAP32[r6>>2]=r4;r4=_uselocale(r2);r2=_sscanf(r1,r3,HEAP32[r6>>2]);if((r4|0)==0){STACKTOP=r5;return r2}_uselocale(r4);STACKTOP=r5;return r2}function __ZNSt3__19__num_getIwE17__stage2_int_loopEwiPcRS2_RjwRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSD_Pw(r1,r2,r3,r4,r5,r6,r7,r8,r9,r10){var r11,r12,r13,r14,r15,r16;r11=r5>>2;r5=r4>>2;r4=HEAP32[r5];r12=(r4|0)==(r3|0);do{if(r12){r13=(HEAP32[r10+96>>2]|0)==(r1|0);if(!r13){if((HEAP32[r10+100>>2]|0)!=(r1|0)){break}}HEAP32[r5]=r3+1|0;HEAP8[r3]=r13?43:45;HEAP32[r11]=0;r14=0;return r14}}while(0);r13=HEAPU8[r7];if((r13&1|0)==0){r15=r13>>>1}else{r15=HEAP32[r7+4>>2]}if((r15|0)!=0&(r1|0)==(r6|0)){r6=HEAP32[r9>>2];if((r6-r8|0)>=160){r14=0;return r14}r8=HEAP32[r11];HEAP32[r9>>2]=r6+4|0;HEAP32[r6>>2]=r8;HEAP32[r11]=0;r14=0;return r14}r8=r10+104|0;r6=r10;while(1){if((r6|0)==(r8|0)){r16=r8;break}if((HEAP32[r6>>2]|0)==(r1|0)){r16=r6;break}else{r6=r6+4|0}}r6=r16-r10|0;r10=r6>>2;if((r6|0)>92){r14=-1;return r14}do{if((r2|0)==16){if((r6|0)<88){break}if(r12){r14=-1;return r14}if((r4-r3|0)>=3){r14=-1;return r14}if(HEAP8[r4-1|0]<<24>>24!=48){r14=-1;return r14}HEAP32[r11]=0;r16=HEAP8[r10+5255348|0];r1=HEAP32[r5];HEAP32[r5]=r1+1|0;HEAP8[r1]=r16;r14=0;return r14}else if((r2|0)==8|(r2|0)==10){if((r10|0)<(r2|0)){break}else{r14=-1}return r14}}while(0);if((r4-r3|0)<39){r3=HEAP8[r10+5255348|0];HEAP32[r5]=r4+1|0;HEAP8[r4]=r3}HEAP32[r11]=HEAP32[r11]+1|0;r14=0;return r14}function __ZNSt3__114__scan_keywordINS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEPKNS_12basic_stringIwS3_NS_9allocatorIwEEEENS_5ctypeIwEEEET0_RT_SE_SD_SD_RKT1_Rjb(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48,r49;r8=0;r9=STACKTOP;STACKTOP=STACKTOP+100|0;r10=r2;r2=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r2>>2]=HEAP32[r10>>2];r10=(r4-r3|0)/12&-1;r11=r9|0;do{if(r10>>>0>100){r12=_malloc(r10);if((r12|0)!=0){r13=r12;r14=r12;break}r12=___cxa_allocate_exception(4);HEAP32[r12>>2]=5247464;___cxa_throw(r12,5252688,66)}else{r13=r11;r14=0}}while(0);r11=(r3|0)==(r4|0);L3367:do{if(r11){r15=r10;r16=0}else{r12=r10;r17=0;r18=r13;r19=r3;while(1){r20=HEAPU8[r19];if((r20&1|0)==0){r21=r20>>>1}else{r21=HEAP32[r19+4>>2]}if((r21|0)==0){HEAP8[r18]=2;r22=r17+1|0;r23=r12-1|0}else{HEAP8[r18]=1;r22=r17;r23=r12}r20=r19+12|0;if((r20|0)==(r4|0)){r15=r23;r16=r22;break L3367}else{r12=r23;r17=r22;r18=r18+1|0;r19=r20}}}}while(0);r22=(r1|0)>>2;r1=(r2|0)>>2;r2=r5;r23=0;r21=r16;r16=r15;while(1){r15=HEAP32[r22],r10=r15>>2;do{if((r15|0)==0){r24=0}else{r19=HEAP32[r10+3];if((r19|0)==(HEAP32[r10+4]|0)){r25=FUNCTION_TABLE[HEAP32[HEAP32[r10]+36>>2]](r15)}else{r25=HEAP32[r19>>2]}if((r25|0)==-1){HEAP32[r22]=0;r24=0;break}else{r24=HEAP32[r22];break}}}while(0);r15=(r24|0)==0;r10=HEAP32[r1],r19=r10>>2;if((r10|0)==0){r26=r24,r27=r26>>2;r28=0,r29=r28>>2}else{r18=HEAP32[r19+3];if((r18|0)==(HEAP32[r19+4]|0)){r30=FUNCTION_TABLE[HEAP32[HEAP32[r19]+36>>2]](r10)}else{r30=HEAP32[r18>>2]}if((r30|0)==-1){HEAP32[r1]=0;r31=0}else{r31=r10}r26=HEAP32[r22],r27=r26>>2;r28=r31,r29=r28>>2}r32=(r28|0)==0;if(!((r15^r32)&(r16|0)!=0)){break}r15=HEAP32[r27+3];if((r15|0)==(HEAP32[r27+4]|0)){r33=FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r26)}else{r33=HEAP32[r15>>2]}if(r7){r34=r33}else{r34=FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+28>>2]](r5,r33)}L3409:do{if(r11){r35=r21;r36=r16}else{r15=r23+1|0;r10=r16;r18=r21;r19=r13;r17=0;r12=r3;while(1){do{if(HEAP8[r19]<<24>>24==1){r20=r12;if((HEAP8[r20]&1)<<24>>24==0){r37=r12+4|0}else{r37=HEAP32[r12+8>>2]}r38=HEAP32[r37+(r23<<2)>>2];if(r7){r39=r38}else{r39=FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+28>>2]](r5,r38)}if((r34|0)!=(r39|0)){HEAP8[r19]=0;r40=r17;r41=r18;r42=r10-1|0;break}r38=HEAPU8[r20];if((r38&1|0)==0){r43=r38>>>1}else{r43=HEAP32[r12+4>>2]}if((r43|0)!=(r15|0)){r40=1;r41=r18;r42=r10;break}HEAP8[r19]=2;r40=1;r41=r18+1|0;r42=r10-1|0}else{r40=r17;r41=r18;r42=r10}}while(0);r38=r12+12|0;if((r38|0)==(r4|0)){break}r10=r42;r18=r41;r19=r19+1|0;r17=r40;r12=r38}if((r40&1)<<24>>24==0){r35=r41;r36=r42;break}r12=HEAP32[r22];r17=r12+12|0;r19=HEAP32[r17>>2];if((r19|0)==(HEAP32[r12+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r12>>2]+40>>2]](r12)}else{HEAP32[r17>>2]=r19+4|0}if((r41+r42|0)>>>0<2|r11){r35=r41;r36=r42;break}r19=r23+1|0;r17=r41;r12=r13;r18=r3;while(1){do{if(HEAP8[r12]<<24>>24==2){r10=HEAPU8[r18];if((r10&1|0)==0){r44=r10>>>1}else{r44=HEAP32[r18+4>>2]}if((r44|0)==(r19|0)){r45=r17;break}HEAP8[r12]=0;r45=r17-1|0}else{r45=r17}}while(0);r10=r18+12|0;if((r10|0)==(r4|0)){r35=r45;r36=r42;break L3409}else{r17=r45;r12=r12+1|0;r18=r10}}}}while(0);r23=r23+1|0;r21=r35;r16=r36}do{if((r26|0)==0){r46=1}else{r36=HEAP32[r27+3];if((r36|0)==(HEAP32[r27+4]|0)){r47=FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r26)}else{r47=HEAP32[r36>>2]}if((r47|0)==-1){HEAP32[r22]=0;r46=1;break}else{r46=(HEAP32[r22]|0)==0;break}}}while(0);do{if(r32){r8=2987}else{r22=HEAP32[r29+3];if((r22|0)==(HEAP32[r29+4]|0)){r48=FUNCTION_TABLE[HEAP32[HEAP32[r29]+36>>2]](r28)}else{r48=HEAP32[r22>>2]}if((r48|0)==-1){HEAP32[r1]=0;r8=2987;break}else{if(r46^(r28|0)==0){break}else{r8=2989;break}}}}while(0);do{if(r8==2987){if(r46){r8=2989;break}else{break}}}while(0);if(r8==2989){HEAP32[r6>>2]=HEAP32[r6>>2]|2}L3475:do{if(r11){r8=2994}else{r46=r3;r28=r13;while(1){if(HEAP8[r28]<<24>>24==2){r49=r46;break L3475}r1=r46+12|0;if((r1|0)==(r4|0)){r8=2994;break L3475}r46=r1;r28=r28+1|0}}}while(0);if(r8==2994){HEAP32[r6>>2]=HEAP32[r6>>2]|4;r49=r4}if((r14|0)==0){STACKTOP=r9;return r49}_free(r14);STACKTOP=r9;return r49}function __ZNSt3__19__num_getIwE17__stage2_int_prepERNS_8ios_baseEPwRw(r1,r2,r3,r4){var r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16;r5=STACKTOP;STACKTOP=STACKTOP+24|0;r6=r5,r7=r6>>2;r8=r5+12,r9=r8>>2;r10=HEAP32[r2+28>>2];r2=(r10+4|0)>>2;tempValue=HEAP32[r2],HEAP32[r2]=tempValue+1,tempValue;if((HEAP32[1313725]|0)!=-1){HEAP32[r9]=5254900;HEAP32[r9+1]=24;HEAP32[r9+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r8,406)}r8=HEAP32[1313726]-1|0;r9=r10+12|0;r11=r10+8|0;r12=HEAP32[r11>>2];do{if(HEAP32[r9>>2]-r12>>2>>>0>r8>>>0){r13=HEAP32[r12+(r8<<2)>>2];if((r13|0)==0){break}FUNCTION_TABLE[HEAP32[HEAP32[r13>>2]+48>>2]](r13,5255348,5255374,r3);if((HEAP32[1313633]|0)!=-1){HEAP32[r7]=5254532;HEAP32[r7+1]=24;HEAP32[r7+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254532,r6,406)}r13=HEAP32[1313634]-1|0;r14=HEAP32[r11>>2];do{if(HEAP32[r9>>2]-r14>>2>>>0>r13>>>0){r15=HEAP32[r14+(r13<<2)>>2];if((r15|0)==0){break}r16=r15;HEAP32[r4>>2]=FUNCTION_TABLE[HEAP32[HEAP32[r15>>2]+16>>2]](r16);FUNCTION_TABLE[HEAP32[HEAP32[r15>>2]+20>>2]](r1,r16);if(((tempValue=HEAP32[r2],HEAP32[r2]=tempValue+ -1,tempValue)|0)!=0){STACKTOP=r5;return}FUNCTION_TABLE[HEAP32[HEAP32[r10>>2]+8>>2]](r10);STACKTOP=r5;return}}while(0);r13=___cxa_allocate_exception(4);HEAP32[r13>>2]=5247488;___cxa_throw(r13,5252700,514)}}while(0);r5=___cxa_allocate_exception(4);HEAP32[r5>>2]=5247488;___cxa_throw(r5,5252700,514)}function __ZNSt3__19__num_getIwE19__stage2_float_loopEwRbRcPcRS4_wwRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjRSE_RjPw(r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12){var r13,r14,r15,r16,r17,r18;r13=r11>>2;r11=r10>>2;r10=r5>>2;if((r1|0)==(r6|0)){if((HEAP8[r2]&1)<<24>>24==0){r14=-1;return r14}HEAP8[r2]=0;r6=HEAP32[r10];HEAP32[r10]=r6+1|0;HEAP8[r6]=46;r6=HEAPU8[r8];if((r6&1|0)==0){r15=r6>>>1}else{r15=HEAP32[r8+4>>2]}if((r15|0)==0){r14=0;return r14}r15=HEAP32[r11];if((r15-r9|0)>=160){r14=0;return r14}r6=HEAP32[r13];HEAP32[r11]=r15+4|0;HEAP32[r15>>2]=r6;r14=0;return r14}do{if((r1|0)==(r7|0)){r6=HEAPU8[r8];if((r6&1|0)==0){r16=r6>>>1}else{r16=HEAP32[r8+4>>2]}if((r16|0)==0){break}if((HEAP8[r2]&1)<<24>>24==0){r14=-1;return r14}r6=HEAP32[r11];if((r6-r9|0)>=160){r14=0;return r14}r15=HEAP32[r13];HEAP32[r11]=r6+4|0;HEAP32[r6>>2]=r15;HEAP32[r13]=0;r14=0;return r14}}while(0);r16=r12+128|0;r7=r12;while(1){if((r7|0)==(r16|0)){r17=r16;break}if((HEAP32[r7>>2]|0)==(r1|0)){r17=r7;break}else{r7=r7+4|0}}r7=r17-r12|0;r12=r7>>2;if((r7|0)>124){r14=-1;return r14}r17=HEAP8[r12+5255348|0];r1=r17&255;r16=HEAP32[r10];if((r12-24|0)>>>0<2){do{if((r16|0)!=(r4|0)){if((HEAP8[r16-1|0]&223|0)==(HEAP8[r3]<<24>>24|0)){break}else{r14=-1}return r14}}while(0);HEAP32[r10]=r16+1|0;HEAP8[r16]=r17;r14=0;return r14}if((r16-r4|0)<39){HEAP32[r10]=r16+1|0;HEAP8[r16]=r17}do{if((r12-22|0)>>>0<2){HEAP8[r3]=80}else{if((r1&223|0)!=(HEAP8[r3]<<24>>24|0)){break}HEAP8[r2]=0;r17=HEAPU8[r8];if((r17&1|0)==0){r18=r17>>>1}else{r18=HEAP32[r8+4>>2]}if((r18|0)==0){break}r17=HEAP32[r11];if((r17-r9|0)>=160){break}r16=HEAP32[r13];HEAP32[r11]=r17+4|0;HEAP32[r17>>2]=r16}}while(0);if((r7|0)>84){r14=0;return r14}HEAP32[r13]=HEAP32[r13]+1|0;r14=0;return r14}function __ZNSt3__19__num_getIwE19__stage2_float_prepERNS_8ios_baseEPwRwS5_(r1,r2,r3,r4,r5){var r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18;r6=STACKTOP;STACKTOP=STACKTOP+24|0;r7=r6,r8=r7>>2;r9=r6+12,r10=r9>>2;r11=HEAP32[r2+28>>2];r2=(r11+4|0)>>2;tempValue=HEAP32[r2],HEAP32[r2]=tempValue+1,tempValue;if((HEAP32[1313725]|0)!=-1){HEAP32[r10]=5254900;HEAP32[r10+1]=24;HEAP32[r10+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r9,406)}r9=HEAP32[1313726]-1|0;r10=r11+12|0;r12=r11+8|0;r13=HEAP32[r12>>2];do{if(HEAP32[r10>>2]-r13>>2>>>0>r9>>>0){r14=HEAP32[r13+(r9<<2)>>2];if((r14|0)==0){break}FUNCTION_TABLE[HEAP32[HEAP32[r14>>2]+48>>2]](r14,5255348,5255380,r3);if((HEAP32[1313633]|0)!=-1){HEAP32[r8]=5254532;HEAP32[r8+1]=24;HEAP32[r8+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254532,r7,406)}r14=HEAP32[1313634]-1|0;r15=HEAP32[r12>>2];do{if(HEAP32[r10>>2]-r15>>2>>>0>r14>>>0){r16=HEAP32[r15+(r14<<2)>>2];if((r16|0)==0){break}r17=r16;r18=r16;HEAP32[r4>>2]=FUNCTION_TABLE[HEAP32[HEAP32[r18>>2]+12>>2]](r17);HEAP32[r5>>2]=FUNCTION_TABLE[HEAP32[HEAP32[r18>>2]+16>>2]](r17);FUNCTION_TABLE[HEAP32[HEAP32[r16>>2]+20>>2]](r1,r17);if(((tempValue=HEAP32[r2],HEAP32[r2]=tempValue+ -1,tempValue)|0)!=0){STACKTOP=r6;return}FUNCTION_TABLE[HEAP32[HEAP32[r11>>2]+8>>2]](r11);STACKTOP=r6;return}}while(0);r14=___cxa_allocate_exception(4);HEAP32[r14>>2]=5247488;___cxa_throw(r14,5252700,514)}}while(0);r6=___cxa_allocate_exception(4);HEAP32[r6>>2]=5247488;___cxa_throw(r6,5252700,514)}function __ZNSt3__111__sprintf_lEPcPvPKcz(r1,r2,r3,r4){var r5,r6;r5=STACKTOP;STACKTOP=STACKTOP+4|0;r6=r5;HEAP32[r6>>2]=r4;r4=_uselocale(r2);r2=_sprintf(r1,r3,HEAP32[r6>>2]);if((r4|0)==0){STACKTOP=r5;return r2}_uselocale(r4);STACKTOP=r5;return r2}function __ZNSt3__19__num_putIcE21__widen_and_group_intEPcS2_S2_S2_RS2_S3_RKNS_6localeE(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34;r8=r6>>2;r6=STACKTOP;STACKTOP=STACKTOP+36|0;r9=r6,r10=r9>>2;r11=r6+12,r12=r11>>2;r13=r6+24;r14=r7|0;r7=HEAP32[r14>>2];if((HEAP32[1313727]|0)!=-1){HEAP32[r12]=5254908;HEAP32[r12+1]=24;HEAP32[r12+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r11,406)}r11=HEAP32[1313728]-1|0;r12=HEAP32[r7+8>>2];if(HEAP32[r7+12>>2]-r12>>2>>>0<=r11>>>0){r15=___cxa_allocate_exception(4);r16=r15;HEAP32[r16>>2]=5247488;___cxa_throw(r15,5252700,514)}r7=HEAP32[r12+(r11<<2)>>2];if((r7|0)==0){r15=___cxa_allocate_exception(4);r16=r15;HEAP32[r16>>2]=5247488;___cxa_throw(r15,5252700,514)}r15=r7;r16=HEAP32[r14>>2];if((HEAP32[1313635]|0)!=-1){HEAP32[r10]=5254540;HEAP32[r10+1]=24;HEAP32[r10+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254540,r9,406)}r9=HEAP32[1313636]-1|0;r10=HEAP32[r16+8>>2];if(HEAP32[r16+12>>2]-r10>>2>>>0<=r9>>>0){r17=___cxa_allocate_exception(4);r18=r17;HEAP32[r18>>2]=5247488;___cxa_throw(r17,5252700,514)}r16=HEAP32[r10+(r9<<2)>>2];if((r16|0)==0){r17=___cxa_allocate_exception(4);r18=r17;HEAP32[r18>>2]=5247488;___cxa_throw(r17,5252700,514)}r17=r16;FUNCTION_TABLE[HEAP32[HEAP32[r16>>2]+20>>2]](r13,r17);r18=r13;r9=r13;r10=HEAPU8[r9];if((r10&1|0)==0){r19=r10>>>1}else{r19=HEAP32[r13+4>>2]}L3630:do{if((r19|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r7>>2]+32>>2]](r15,r1,r3,r4);HEAP32[r8]=r4+(r3-r1)|0}else{HEAP32[r8]=r4;r10=HEAP8[r1];if(r10<<24>>24==45|r10<<24>>24==43){r14=FUNCTION_TABLE[HEAP32[HEAP32[r7>>2]+28>>2]](r15,r10);r10=HEAP32[r8];HEAP32[r8]=r10+1|0;HEAP8[r10]=r14;r20=r1+1|0}else{r20=r1}do{if((r3-r20|0)>1){if(HEAP8[r20]<<24>>24!=48){r21=r20;break}r14=r20+1|0;r10=HEAP8[r14];if(!(r10<<24>>24==120|r10<<24>>24==88)){r21=r20;break}r10=r7;r11=FUNCTION_TABLE[HEAP32[HEAP32[r10>>2]+28>>2]](r15,48);r12=HEAP32[r8];HEAP32[r8]=r12+1|0;HEAP8[r12]=r11;r11=FUNCTION_TABLE[HEAP32[HEAP32[r10>>2]+28>>2]](r15,HEAP8[r14]);r14=HEAP32[r8];HEAP32[r8]=r14+1|0;HEAP8[r14]=r11;r21=r20+2|0}else{r21=r20}}while(0);L3643:do{if((r21|0)!=(r3|0)){r11=r3-1|0;if(r21>>>0<r11>>>0){r22=r21;r23=r11}else{break}while(1){r11=HEAP8[r22];HEAP8[r22]=HEAP8[r23];HEAP8[r23]=r11;r11=r22+1|0;r14=r23-1|0;if(r11>>>0<r14>>>0){r22=r11;r23=r14}else{break L3643}}}}while(0);r14=FUNCTION_TABLE[HEAP32[HEAP32[r16>>2]+16>>2]](r17);L3649:do{if(r21>>>0<r3>>>0){r11=r18+1|0;r10=r7;r12=r13+4|0;r24=r13+8|0;r25=0;r26=0;r27=r21;while(1){r28=(HEAP8[r9]&1)<<24>>24==0;do{if(HEAP8[(r28?r11:HEAP32[r24>>2])+r26|0]<<24>>24==0){r29=r26;r30=r25}else{if((r25|0)!=(HEAP8[(r28?r11:HEAP32[r24>>2])+r26|0]<<24>>24|0)){r29=r26;r30=r25;break}r31=HEAP32[r8];HEAP32[r8]=r31+1|0;HEAP8[r31]=r14;r31=HEAPU8[r9];r29=(r26>>>0<(((r31&1|0)==0?r31>>>1:HEAP32[r12>>2])-1|0)>>>0&1)+r26|0;r30=0}}while(0);r28=FUNCTION_TABLE[HEAP32[HEAP32[r10>>2]+28>>2]](r15,HEAP8[r27]);r31=HEAP32[r8];HEAP32[r8]=r31+1|0;HEAP8[r31]=r28;r28=r27+1|0;if(r28>>>0<r3>>>0){r25=r30+1|0;r26=r29;r27=r28}else{break L3649}}}}while(0);r14=r4+(r21-r1)|0;r27=HEAP32[r8];if((r14|0)==(r27|0)){break}r26=r27-1|0;if(r14>>>0<r26>>>0){r32=r14;r33=r26}else{break}while(1){r26=HEAP8[r32];HEAP8[r32]=HEAP8[r33];HEAP8[r33]=r26;r26=r32+1|0;r14=r33-1|0;if(r26>>>0<r14>>>0){r32=r26;r33=r14}else{break L3630}}}}while(0);if((r2|0)==(r3|0)){r34=HEAP32[r8]}else{r34=r4+(r2-r1)|0}HEAP32[r5>>2]=r34;if((HEAP8[r9]&1)<<24>>24==0){STACKTOP=r6;return}__ZdlPv(HEAP32[r13+8>>2]);STACKTOP=r6;return}function __ZNSt3__116__pad_and_outputIcNS_11char_traitsIcEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20;r8=r1>>2;r1=STACKTOP;STACKTOP=STACKTOP+12|0;r9=r2;r2=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r2>>2]=HEAP32[r9>>2];r9=r1,r10=r9>>2;r11=(r2|0)>>2;r2=HEAP32[r11],r12=r2>>2;if((r2|0)==0){HEAP32[r8]=0;STACKTOP=r1;return}r13=r5;r5=r3;r14=r13-r5|0;r15=r6+12|0;r6=HEAP32[r15>>2];r16=(r6|0)>(r14|0)?r6-r14|0:0;r14=r4;r6=r14-r5|0;do{if((r6|0)>0){if((FUNCTION_TABLE[HEAP32[HEAP32[r12]+48>>2]](r2,r3,r6)|0)==(r6|0)){break}HEAP32[r11]=0;HEAP32[r8]=0;STACKTOP=r1;return}}while(0);do{if((r16|0)>0){if(r16>>>0<11){r6=r16<<1&255;r3=r9;HEAP8[r3]=r6;r17=r9+1|0;r18=r6;r19=r3}else{r3=r16+16&-16;r6=__Znwj(r3);HEAP32[r10+2]=r6;r5=r3|1;HEAP32[r10]=r5;HEAP32[r10+1]=r16;r17=r6;r18=r5&255;r19=r9}_memset(r17,r7,r16);HEAP8[r17+r16|0]=0;if((r18&1)<<24>>24==0){r20=r9+1|0}else{r20=HEAP32[r10+2]}if((FUNCTION_TABLE[HEAP32[HEAP32[r12]+48>>2]](r2,r20,r16)|0)==(r16|0)){if((HEAP8[r19]&1)<<24>>24==0){break}__ZdlPv(HEAP32[r10+2]);break}HEAP32[r11]=0;HEAP32[r8]=0;if((HEAP8[r19]&1)<<24>>24==0){STACKTOP=r1;return}__ZdlPv(HEAP32[r10+2]);STACKTOP=r1;return}}while(0);r10=r13-r14|0;do{if((r10|0)>0){if((FUNCTION_TABLE[HEAP32[HEAP32[r12]+48>>2]](r2,r4,r10)|0)==(r10|0)){break}HEAP32[r11]=0;HEAP32[r8]=0;STACKTOP=r1;return}}while(0);HEAP32[r15>>2]=0;HEAP32[r8]=r2;STACKTOP=r1;return}function __ZNSt3__112__snprintf_lEPcjPvPKcz(r1,r2,r3,r4,r5){var r6,r7;r6=STACKTOP;STACKTOP=STACKTOP+4|0;r7=r6;HEAP32[r7>>2]=r5;r5=_uselocale(r3);r3=_snprintf(r1,r2,r4,HEAP32[r7>>2]);if((r5|0)==0){STACKTOP=r6;return r3}_uselocale(r5);STACKTOP=r6;return r3}function __ZNSt3__112__asprintf_lEPPcPvPKcz(r1,r2,r3,r4){var r5,r6;r5=STACKTOP;STACKTOP=STACKTOP+4|0;r6=r5;HEAP32[r6>>2]=r4;r4=_uselocale(r2);r2=_asprintf(r1,r3,HEAP32[r6>>2]);if((r4|0)==0){STACKTOP=r5;return r2}_uselocale(r4);STACKTOP=r5;return r2}function __ZNSt3__19__num_putIcE23__widen_and_group_floatEPcS2_S2_S2_RS2_S3_RKNS_6localeE(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37;r8=r6>>2;r6=0;r9=STACKTOP;STACKTOP=STACKTOP+36|0;r10=r9,r11=r10>>2;r12=r9+12,r13=r12>>2;r14=r9+24;r15=r7|0;r7=HEAP32[r15>>2];if((HEAP32[1313727]|0)!=-1){HEAP32[r13]=5254908;HEAP32[r13+1]=24;HEAP32[r13+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r12,406)}r12=HEAP32[1313728]-1|0;r13=HEAP32[r7+8>>2];if(HEAP32[r7+12>>2]-r13>>2>>>0<=r12>>>0){r16=___cxa_allocate_exception(4);r17=r16;HEAP32[r17>>2]=5247488;___cxa_throw(r16,5252700,514)}r7=HEAP32[r13+(r12<<2)>>2],r12=r7>>2;if((r7|0)==0){r16=___cxa_allocate_exception(4);r17=r16;HEAP32[r17>>2]=5247488;___cxa_throw(r16,5252700,514)}r16=r7;r17=HEAP32[r15>>2];if((HEAP32[1313635]|0)!=-1){HEAP32[r11]=5254540;HEAP32[r11+1]=24;HEAP32[r11+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254540,r10,406)}r10=HEAP32[1313636]-1|0;r11=HEAP32[r17+8>>2];if(HEAP32[r17+12>>2]-r11>>2>>>0<=r10>>>0){r18=___cxa_allocate_exception(4);r19=r18;HEAP32[r19>>2]=5247488;___cxa_throw(r18,5252700,514)}r17=HEAP32[r11+(r10<<2)>>2],r10=r17>>2;if((r17|0)==0){r18=___cxa_allocate_exception(4);r19=r18;HEAP32[r19>>2]=5247488;___cxa_throw(r18,5252700,514)}r18=r17;FUNCTION_TABLE[HEAP32[HEAP32[r10]+20>>2]](r14,r18);HEAP32[r8]=r4;r17=HEAP8[r1];if(r17<<24>>24==45|r17<<24>>24==43){r19=FUNCTION_TABLE[HEAP32[HEAP32[r12]+28>>2]](r16,r17);r17=HEAP32[r8];HEAP32[r8]=r17+1|0;HEAP8[r17]=r19;r20=r1+1|0}else{r20=r1}r19=r3;L70:do{if((r19-r20|0)>1){if(HEAP8[r20]<<24>>24!=48){r21=r20;r6=84;break}r17=r20+1|0;r11=HEAP8[r17];if(!(r11<<24>>24==120|r11<<24>>24==88)){r21=r20;r6=84;break}r11=r7;r15=FUNCTION_TABLE[HEAP32[HEAP32[r11>>2]+28>>2]](r16,48);r13=HEAP32[r8];HEAP32[r8]=r13+1|0;HEAP8[r13]=r15;r15=r20+2|0;r13=FUNCTION_TABLE[HEAP32[HEAP32[r11>>2]+28>>2]](r16,HEAP8[r17]);r17=HEAP32[r8];HEAP32[r8]=r17+1|0;HEAP8[r17]=r13;r13=r15;while(1){if(r13>>>0>=r3>>>0){r22=r13;r23=r15;break L70}r17=HEAP8[r13];do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);if((_isxdigit(r17<<24>>24,HEAP32[1311652])|0)==0){r22=r13;r23=r15;break L70}else{r13=r13+1|0}}}else{r21=r20;r6=84}}while(0);L85:do{if(r6==84){while(1){r6=0;if(r21>>>0>=r3>>>0){r22=r21;r23=r20;break L85}r13=HEAP8[r21];do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);if((_isdigit(r13<<24>>24,HEAP32[1311652])|0)==0){r22=r21;r23=r20;break L85}else{r21=r21+1|0;r6=84}}}}while(0);r6=r14;r21=r14;r20=HEAPU8[r21];if((r20&1|0)==0){r24=r20>>>1}else{r24=HEAP32[r14+4>>2]}L100:do{if((r24|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r12]+32>>2]](r16,r23,r22,HEAP32[r8]);HEAP32[r8]=HEAP32[r8]+(r22-r23)|0}else{L104:do{if((r23|0)!=(r22|0)){r20=r22-1|0;if(r23>>>0<r20>>>0){r25=r23;r26=r20}else{break}while(1){r20=HEAP8[r25];HEAP8[r25]=HEAP8[r26];HEAP8[r26]=r20;r20=r25+1|0;r17=r26-1|0;if(r20>>>0<r17>>>0){r25=r20;r26=r17}else{break L104}}}}while(0);r13=FUNCTION_TABLE[HEAP32[HEAP32[r10]+16>>2]](r18);L110:do{if(r23>>>0<r22>>>0){r17=r6+1|0;r20=r14+4|0;r15=r14+8|0;r11=r7;r27=0;r28=0;r29=r23;while(1){r30=(HEAP8[r21]&1)<<24>>24==0;do{if(HEAP8[(r30?r17:HEAP32[r15>>2])+r28|0]<<24>>24>0){if((r27|0)!=(HEAP8[(r30?r17:HEAP32[r15>>2])+r28|0]<<24>>24|0)){r31=r28;r32=r27;break}r33=HEAP32[r8];HEAP32[r8]=r33+1|0;HEAP8[r33]=r13;r33=HEAPU8[r21];r31=(r28>>>0<(((r33&1|0)==0?r33>>>1:HEAP32[r20>>2])-1|0)>>>0&1)+r28|0;r32=0}else{r31=r28;r32=r27}}while(0);r30=FUNCTION_TABLE[HEAP32[HEAP32[r11>>2]+28>>2]](r16,HEAP8[r29]);r33=HEAP32[r8];HEAP32[r8]=r33+1|0;HEAP8[r33]=r30;r30=r29+1|0;if(r30>>>0<r22>>>0){r27=r32+1|0;r28=r31;r29=r30}else{break L110}}}}while(0);r13=r4+(r23-r1)|0;r29=HEAP32[r8];if((r13|0)==(r29|0)){break}r28=r29-1|0;if(r13>>>0<r28>>>0){r34=r13;r35=r28}else{break}while(1){r28=HEAP8[r34];HEAP8[r34]=HEAP8[r35];HEAP8[r35]=r28;r28=r34+1|0;r13=r35-1|0;if(r28>>>0<r13>>>0){r34=r28;r35=r13}else{break L100}}}}while(0);L124:do{if(r22>>>0<r3>>>0){r35=r7;r34=r22;while(1){r23=HEAP8[r34];if(r23<<24>>24==46){break}r31=FUNCTION_TABLE[HEAP32[HEAP32[r35>>2]+28>>2]](r16,r23);r23=HEAP32[r8];HEAP32[r8]=r23+1|0;HEAP8[r23]=r31;r31=r34+1|0;if(r31>>>0<r3>>>0){r34=r31}else{r36=r31;break L124}}r35=FUNCTION_TABLE[HEAP32[HEAP32[r10]+12>>2]](r18);r31=HEAP32[r8];HEAP32[r8]=r31+1|0;HEAP8[r31]=r35;r36=r34+1|0}else{r36=r22}}while(0);FUNCTION_TABLE[HEAP32[HEAP32[r12]+32>>2]](r16,r36,r3,HEAP32[r8]);r16=HEAP32[r8]+(r19-r36)|0;HEAP32[r8]=r16;if((r2|0)==(r3|0)){r37=r16}else{r37=r4+(r2-r1)|0}HEAP32[r5>>2]=r37;if((HEAP8[r21]&1)<<24>>24==0){STACKTOP=r9;return}__ZdlPv(HEAP32[r14+8>>2]);STACKTOP=r9;return}function __ZNSt3__19__num_putIwE21__widen_and_group_intEPcS2_S2_PwRS3_S4_RKNS_6localeE(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34;r8=r6>>2;r6=STACKTOP;STACKTOP=STACKTOP+36|0;r9=r6,r10=r9>>2;r11=r6+12,r12=r11>>2;r13=r6+24;r14=r7|0;r7=HEAP32[r14>>2];if((HEAP32[1313725]|0)!=-1){HEAP32[r12]=5254900;HEAP32[r12+1]=24;HEAP32[r12+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r11,406)}r11=HEAP32[1313726]-1|0;r12=HEAP32[r7+8>>2];if(HEAP32[r7+12>>2]-r12>>2>>>0<=r11>>>0){r15=___cxa_allocate_exception(4);r16=r15;HEAP32[r16>>2]=5247488;___cxa_throw(r15,5252700,514)}r7=HEAP32[r12+(r11<<2)>>2];if((r7|0)==0){r15=___cxa_allocate_exception(4);r16=r15;HEAP32[r16>>2]=5247488;___cxa_throw(r15,5252700,514)}r15=r7;r16=HEAP32[r14>>2];if((HEAP32[1313633]|0)!=-1){HEAP32[r10]=5254532;HEAP32[r10+1]=24;HEAP32[r10+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254532,r9,406)}r9=HEAP32[1313634]-1|0;r10=HEAP32[r16+8>>2];if(HEAP32[r16+12>>2]-r10>>2>>>0<=r9>>>0){r17=___cxa_allocate_exception(4);r18=r17;HEAP32[r18>>2]=5247488;___cxa_throw(r17,5252700,514)}r16=HEAP32[r10+(r9<<2)>>2];if((r16|0)==0){r17=___cxa_allocate_exception(4);r18=r17;HEAP32[r18>>2]=5247488;___cxa_throw(r17,5252700,514)}r17=r16;FUNCTION_TABLE[HEAP32[HEAP32[r16>>2]+20>>2]](r13,r17);r18=r13;r9=r13;r10=HEAPU8[r9];if((r10&1|0)==0){r19=r10>>>1}else{r19=HEAP32[r13+4>>2]}L164:do{if((r19|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r7>>2]+48>>2]](r15,r1,r3,r4);HEAP32[r8]=(r3-r1<<2)+r4|0}else{HEAP32[r8]=r4;r10=HEAP8[r1];if(r10<<24>>24==45|r10<<24>>24==43){r14=FUNCTION_TABLE[HEAP32[HEAP32[r7>>2]+44>>2]](r15,r10);r10=HEAP32[r8];HEAP32[r8]=r10+4|0;HEAP32[r10>>2]=r14;r20=r1+1|0}else{r20=r1}do{if((r3-r20|0)>1){if(HEAP8[r20]<<24>>24!=48){r21=r20;break}r14=r20+1|0;r10=HEAP8[r14];if(!(r10<<24>>24==120|r10<<24>>24==88)){r21=r20;break}r10=r7;r11=FUNCTION_TABLE[HEAP32[HEAP32[r10>>2]+44>>2]](r15,48);r12=HEAP32[r8];HEAP32[r8]=r12+4|0;HEAP32[r12>>2]=r11;r11=FUNCTION_TABLE[HEAP32[HEAP32[r10>>2]+44>>2]](r15,HEAP8[r14]);r14=HEAP32[r8];HEAP32[r8]=r14+4|0;HEAP32[r14>>2]=r11;r21=r20+2|0}else{r21=r20}}while(0);L179:do{if((r21|0)!=(r3|0)){r11=r3-1|0;if(r21>>>0<r11>>>0){r22=r21;r23=r11}else{break}while(1){r11=HEAP8[r22];HEAP8[r22]=HEAP8[r23];HEAP8[r23]=r11;r11=r22+1|0;r14=r23-1|0;if(r11>>>0<r14>>>0){r22=r11;r23=r14}else{break L179}}}}while(0);r14=FUNCTION_TABLE[HEAP32[HEAP32[r16>>2]+16>>2]](r17);L185:do{if(r21>>>0<r3>>>0){r11=r18+1|0;r10=r7;r12=r13+4|0;r24=r13+8|0;r25=0;r26=0;r27=r21;while(1){r28=(HEAP8[r9]&1)<<24>>24==0;do{if(HEAP8[(r28?r11:HEAP32[r24>>2])+r26|0]<<24>>24==0){r29=r26;r30=r25}else{if((r25|0)!=(HEAP8[(r28?r11:HEAP32[r24>>2])+r26|0]<<24>>24|0)){r29=r26;r30=r25;break}r31=HEAP32[r8];HEAP32[r8]=r31+4|0;HEAP32[r31>>2]=r14;r31=HEAPU8[r9];r29=(r26>>>0<(((r31&1|0)==0?r31>>>1:HEAP32[r12>>2])-1|0)>>>0&1)+r26|0;r30=0}}while(0);r28=FUNCTION_TABLE[HEAP32[HEAP32[r10>>2]+44>>2]](r15,HEAP8[r27]);r31=HEAP32[r8];HEAP32[r8]=r31+4|0;HEAP32[r31>>2]=r28;r28=r27+1|0;if(r28>>>0<r3>>>0){r25=r30+1|0;r26=r29;r27=r28}else{break L185}}}}while(0);r14=(r21-r1<<2)+r4|0;r27=HEAP32[r8];if((r14|0)==(r27|0)){break}r26=r27-4|0;if(r14>>>0<r26>>>0){r32=r14;r33=r26}else{break}while(1){r26=HEAP32[r32>>2];HEAP32[r32>>2]=HEAP32[r33>>2];HEAP32[r33>>2]=r26;r26=r32+4|0;r14=r33-4|0;if(r26>>>0<r14>>>0){r32=r26;r33=r14}else{break L164}}}}while(0);if((r2|0)==(r3|0)){r34=HEAP32[r8]}else{r34=(r2-r1<<2)+r4|0}HEAP32[r5>>2]=r34;if((HEAP8[r9]&1)<<24>>24==0){STACKTOP=r6;return}__ZdlPv(HEAP32[r13+8>>2]);STACKTOP=r6;return}function __ZNSt3__116__pad_and_outputIwNS_11char_traitsIwEEEENS_19ostreambuf_iteratorIT_T0_EES6_PKS4_S8_S8_RNS_8ios_baseES4_(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19;r8=r1>>2;r1=STACKTOP;STACKTOP=STACKTOP+12|0;r9=r2;r2=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r2>>2]=HEAP32[r9>>2];r9=r1,r10=r9>>2;r11=(r2|0)>>2;r2=HEAP32[r11],r12=r2>>2;if((r2|0)==0){HEAP32[r8]=0;STACKTOP=r1;return}r13=r5;r5=r3;r14=r13-r5>>2;r15=r6+12|0;r6=HEAP32[r15>>2];r16=(r6|0)>(r14|0)?r6-r14|0:0;r14=r4;r6=r14-r5|0;r5=r6>>2;do{if((r6|0)>0){if((FUNCTION_TABLE[HEAP32[HEAP32[r12]+48>>2]](r2,r3,r5)|0)==(r5|0)){break}HEAP32[r11]=0;HEAP32[r8]=0;STACKTOP=r1;return}}while(0);do{if((r16|0)>0){if(r16>>>0>1073741822){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}if(r16>>>0<2){r5=r9;HEAP8[r5]=r16<<1&255;r17=r9+4|0;r18=r5}else{r5=r16+4&-4;r3=__Znwj(r5<<2);HEAP32[r10+2]=r3;HEAP32[r10]=r5|1;HEAP32[r10+1]=r16;r17=r3;r18=r9}_wmemset(r17,r7,r16);HEAP32[r17+(r16<<2)>>2]=0;if((HEAP8[r18]&1)<<24>>24==0){r19=r9+4|0}else{r19=HEAP32[r10+2]}if((FUNCTION_TABLE[HEAP32[HEAP32[r12]+48>>2]](r2,r19,r16)|0)==(r16|0)){if((HEAP8[r18]&1)<<24>>24==0){break}__ZdlPv(HEAP32[r10+2]);break}HEAP32[r11]=0;HEAP32[r8]=0;if((HEAP8[r18]&1)<<24>>24==0){STACKTOP=r1;return}__ZdlPv(HEAP32[r10+2]);STACKTOP=r1;return}}while(0);r10=r13-r14|0;r14=r10>>2;do{if((r10|0)>0){if((FUNCTION_TABLE[HEAP32[HEAP32[r12]+48>>2]](r2,r4,r14)|0)==(r14|0)){break}HEAP32[r11]=0;HEAP32[r8]=0;STACKTOP=r1;return}}while(0);HEAP32[r15>>2]=0;HEAP32[r8]=r2;STACKTOP=r1;return}function __ZNSt3__19__num_putIwE23__widen_and_group_floatEPcS2_S2_PwRS3_S4_RKNS_6localeE(r1,r2,r3,r4,r5,r6,r7){var r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37;r8=r6>>2;r6=0;r9=STACKTOP;STACKTOP=STACKTOP+36|0;r10=r9,r11=r10>>2;r12=r9+12,r13=r12>>2;r14=r9+24;r15=r7|0;r7=HEAP32[r15>>2];if((HEAP32[1313725]|0)!=-1){HEAP32[r13]=5254900;HEAP32[r13+1]=24;HEAP32[r13+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r12,406)}r12=HEAP32[1313726]-1|0;r13=HEAP32[r7+8>>2];if(HEAP32[r7+12>>2]-r13>>2>>>0<=r12>>>0){r16=___cxa_allocate_exception(4);r17=r16;HEAP32[r17>>2]=5247488;___cxa_throw(r16,5252700,514)}r7=HEAP32[r13+(r12<<2)>>2],r12=r7>>2;if((r7|0)==0){r16=___cxa_allocate_exception(4);r17=r16;HEAP32[r17>>2]=5247488;___cxa_throw(r16,5252700,514)}r16=r7;r17=HEAP32[r15>>2];if((HEAP32[1313633]|0)!=-1){HEAP32[r11]=5254532;HEAP32[r11+1]=24;HEAP32[r11+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254532,r10,406)}r10=HEAP32[1313634]-1|0;r11=HEAP32[r17+8>>2];if(HEAP32[r17+12>>2]-r11>>2>>>0<=r10>>>0){r18=___cxa_allocate_exception(4);r19=r18;HEAP32[r19>>2]=5247488;___cxa_throw(r18,5252700,514)}r17=HEAP32[r11+(r10<<2)>>2],r10=r17>>2;if((r17|0)==0){r18=___cxa_allocate_exception(4);r19=r18;HEAP32[r19>>2]=5247488;___cxa_throw(r18,5252700,514)}r18=r17;FUNCTION_TABLE[HEAP32[HEAP32[r10]+20>>2]](r14,r18);HEAP32[r8]=r4;r17=HEAP8[r1];if(r17<<24>>24==45|r17<<24>>24==43){r19=FUNCTION_TABLE[HEAP32[HEAP32[r12]+44>>2]](r16,r17);r17=HEAP32[r8];HEAP32[r8]=r17+4|0;HEAP32[r17>>2]=r19;r20=r1+1|0}else{r20=r1}r19=r3;L269:do{if((r19-r20|0)>1){if(HEAP8[r20]<<24>>24!=48){r21=r20;r6=256;break}r17=r20+1|0;r11=HEAP8[r17];if(!(r11<<24>>24==120|r11<<24>>24==88)){r21=r20;r6=256;break}r11=r7;r15=FUNCTION_TABLE[HEAP32[HEAP32[r11>>2]+44>>2]](r16,48);r13=HEAP32[r8];HEAP32[r8]=r13+4|0;HEAP32[r13>>2]=r15;r15=r20+2|0;r13=FUNCTION_TABLE[HEAP32[HEAP32[r11>>2]+44>>2]](r16,HEAP8[r17]);r17=HEAP32[r8];HEAP32[r8]=r17+4|0;HEAP32[r17>>2]=r13;r13=r15;while(1){if(r13>>>0>=r3>>>0){r22=r13;r23=r15;break L269}r17=HEAP8[r13];do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);if((_isxdigit(r17<<24>>24,HEAP32[1311652])|0)==0){r22=r13;r23=r15;break L269}else{r13=r13+1|0}}}else{r21=r20;r6=256}}while(0);L284:do{if(r6==256){while(1){r6=0;if(r21>>>0>=r3>>>0){r22=r21;r23=r20;break L284}r13=HEAP8[r21];do{if(HEAP8[5255484]<<24>>24==0){if((___cxa_guard_acquire(5255484)|0)==0){break}HEAP32[1311652]=_newlocale(1,5243860,0)}}while(0);if((_isdigit(r13<<24>>24,HEAP32[1311652])|0)==0){r22=r21;r23=r20;break L284}else{r21=r21+1|0;r6=256}}}}while(0);r6=r14;r21=r14;r20=HEAPU8[r21];if((r20&1|0)==0){r24=r20>>>1}else{r24=HEAP32[r14+4>>2]}L299:do{if((r24|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r12]+48>>2]](r16,r23,r22,HEAP32[r8]);HEAP32[r8]=(r22-r23<<2)+HEAP32[r8]|0}else{L303:do{if((r23|0)!=(r22|0)){r20=r22-1|0;if(r23>>>0<r20>>>0){r25=r23;r26=r20}else{break}while(1){r20=HEAP8[r25];HEAP8[r25]=HEAP8[r26];HEAP8[r26]=r20;r20=r25+1|0;r17=r26-1|0;if(r20>>>0<r17>>>0){r25=r20;r26=r17}else{break L303}}}}while(0);r13=FUNCTION_TABLE[HEAP32[HEAP32[r10]+16>>2]](r18);L309:do{if(r23>>>0<r22>>>0){r17=r6+1|0;r20=r14+4|0;r15=r14+8|0;r11=r7;r27=0;r28=0;r29=r23;while(1){r30=(HEAP8[r21]&1)<<24>>24==0;do{if(HEAP8[(r30?r17:HEAP32[r15>>2])+r28|0]<<24>>24>0){if((r27|0)!=(HEAP8[(r30?r17:HEAP32[r15>>2])+r28|0]<<24>>24|0)){r31=r28;r32=r27;break}r33=HEAP32[r8];HEAP32[r8]=r33+4|0;HEAP32[r33>>2]=r13;r33=HEAPU8[r21];r31=(r28>>>0<(((r33&1|0)==0?r33>>>1:HEAP32[r20>>2])-1|0)>>>0&1)+r28|0;r32=0}else{r31=r28;r32=r27}}while(0);r30=FUNCTION_TABLE[HEAP32[HEAP32[r11>>2]+44>>2]](r16,HEAP8[r29]);r33=HEAP32[r8];HEAP32[r8]=r33+4|0;HEAP32[r33>>2]=r30;r30=r29+1|0;if(r30>>>0<r22>>>0){r27=r32+1|0;r28=r31;r29=r30}else{break L309}}}}while(0);r13=(r23-r1<<2)+r4|0;r29=HEAP32[r8];if((r13|0)==(r29|0)){break}r28=r29-4|0;if(r13>>>0<r28>>>0){r34=r13;r35=r28}else{break}while(1){r28=HEAP32[r34>>2];HEAP32[r34>>2]=HEAP32[r35>>2];HEAP32[r35>>2]=r28;r28=r34+4|0;r13=r35-4|0;if(r28>>>0<r13>>>0){r34=r28;r35=r13}else{break L299}}}}while(0);L323:do{if(r22>>>0<r3>>>0){r35=r7;r34=r22;while(1){r23=HEAP8[r34];if(r23<<24>>24==46){break}r31=FUNCTION_TABLE[HEAP32[HEAP32[r35>>2]+44>>2]](r16,r23);r23=HEAP32[r8];HEAP32[r8]=r23+4|0;HEAP32[r23>>2]=r31;r31=r34+1|0;if(r31>>>0<r3>>>0){r34=r31}else{r36=r31;break L323}}r35=FUNCTION_TABLE[HEAP32[HEAP32[r10]+12>>2]](r18);r31=HEAP32[r8];HEAP32[r8]=r31+4|0;HEAP32[r31>>2]=r35;r36=r34+1|0}else{r36=r22}}while(0);FUNCTION_TABLE[HEAP32[HEAP32[r12]+48>>2]](r16,r36,r3,HEAP32[r8]);r16=(r19-r36<<2)+HEAP32[r8]|0;HEAP32[r8]=r16;if((r2|0)==(r3|0)){r37=r16}else{r37=(r2-r1<<2)+r4|0}HEAP32[r5>>2]=r37;if((HEAP8[r21]&1)<<24>>24==0){STACKTOP=r9;return}__ZdlPv(HEAP32[r14+8>>2]);STACKTOP=r9;return}function __ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE17__get_white_spaceERS4_S4_RjRKNS_5ctypeIcEE(r1,r2,r3,r4,r5){var r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18;r1=0;r6=STACKTOP;r7=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r7>>2];r7=r5+8|0;r5=(r2|0)>>2;r2=(r3|0)>>2;L341:while(1){r3=HEAP32[r5],r8=r3>>2;do{if((r3|0)==0){r9=0}else{if((HEAP32[r8+3]|0)!=(HEAP32[r8+4]|0)){r9=r3;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r8]+36>>2]](r3)|0)==-1){HEAP32[r5]=0;r9=0;break}else{r9=HEAP32[r5];break}}}while(0);r3=(r9|0)==0;r8=HEAP32[r2],r10=r8>>2;L350:do{if((r8|0)==0){r1=315}else{do{if((HEAP32[r10+3]|0)==(HEAP32[r10+4]|0)){if((FUNCTION_TABLE[HEAP32[HEAP32[r10]+36>>2]](r8)|0)!=-1){break}HEAP32[r2]=0;r1=315;break L350}}while(0);if(r3){r11=r8;r12=0;break}else{r13=r8,r14=r13>>2;r15=0;break L341}}}while(0);if(r1==315){r1=0;if(r3){r13=0,r14=r13>>2;r15=1;break}else{r11=0;r12=1}}r8=HEAP32[r5],r10=r8>>2;r16=HEAP32[r10+3];if((r16|0)==(HEAP32[r10+4]|0)){r17=FUNCTION_TABLE[HEAP32[HEAP32[r10]+36>>2]](r8)}else{r17=HEAPU8[r16]}r16=r17<<24>>24;if(r16>>>0>=128){r13=r11,r14=r13>>2;r15=r12;break}if((HEAP16[HEAP32[r7>>2]+(r16<<1)>>1]&8192)<<16>>16==0){r13=r11,r14=r13>>2;r15=r12;break}r16=HEAP32[r5];r8=r16+12|0;r10=HEAP32[r8>>2];if((r10|0)==(HEAP32[r16+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r16>>2]+40>>2]](r16);continue}else{HEAP32[r8>>2]=r10+1|0;continue}}r12=HEAP32[r5],r11=r12>>2;do{if((r12|0)==0){r18=0}else{if((HEAP32[r11+3]|0)!=(HEAP32[r11+4]|0)){r18=r12;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r11]+36>>2]](r12)|0)==-1){HEAP32[r5]=0;r18=0;break}else{r18=HEAP32[r5];break}}}while(0);r5=(r18|0)==0;do{if(r15){r1=334}else{if((HEAP32[r14+3]|0)!=(HEAP32[r14+4]|0)){if(!(r5^(r13|0)==0)){break}STACKTOP=r6;return}if((FUNCTION_TABLE[HEAP32[HEAP32[r14]+36>>2]](r13)|0)==-1){HEAP32[r2]=0;r1=334;break}if(!r5){break}STACKTOP=r6;return}}while(0);do{if(r1==334){if(r5){break}STACKTOP=r6;return}}while(0);HEAP32[r4>>2]=HEAP32[r4>>2]|2;STACKTOP=r6;return}function __ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE11__get_am_pmERiRS4_S4_RjRKNS_5ctypeIcEE(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11;r7=STACKTOP;STACKTOP=STACKTOP+4|0;r8=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r8>>2];r8=r7;r9=r1+8|0;r1=FUNCTION_TABLE[HEAP32[HEAP32[r9>>2]+8>>2]](r9);r9=HEAPU8[r1];if((r9&1|0)==0){r10=r9>>>1}else{r10=HEAP32[r1+4>>2]}r9=HEAPU8[r1+12|0];if((r9&1|0)==0){r11=r9>>>1}else{r11=HEAP32[r1+16>>2]}if((r10|0)==(-r11|0)){HEAP32[r5>>2]=HEAP32[r5>>2]|4;STACKTOP=r7;return}HEAP32[r8>>2]=HEAP32[r4>>2];r4=__ZNSt3__114__scan_keywordINS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEPKNS_12basic_stringIcS3_NS_9allocatorIcEEEENS_5ctypeIcEEEET0_RT_SE_SD_SD_RKT1_Rjb(r3,r8,r1,r1+24|0,r6,r5,0);r5=r4-r1|0;do{if((r4|0)==(r1|0)){if((HEAP32[r2>>2]|0)!=12){break}HEAP32[r2>>2]=0;STACKTOP=r7;return}}while(0);if((r5|0)!=12){STACKTOP=r7;return}r5=HEAP32[r2>>2];if((r5|0)>=12){STACKTOP=r7;return}HEAP32[r2>>2]=r5+12|0;STACKTOP=r7;return}function __ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKcSC_(r1,r2,r3,r4,r5,r6,r7,r8,r9){var r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48,r49,r50,r51,r52,r53,r54,r55,r56,r57,r58,r59,r60;r10=r6>>2;r11=0;r12=STACKTOP;STACKTOP=STACKTOP+24|0;r13=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r13>>2];r13=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r13>>2];r13=r12,r14=r13>>2;r15=r12+12;r16=r12+16;r17=r12+20;r18=HEAP32[r5+28>>2],r19=r18>>2;r20=(r18+4|0)>>2;tempValue=HEAP32[r20],HEAP32[r20]=tempValue+1,tempValue;if((HEAP32[1313727]|0)!=-1){HEAP32[r14]=5254908;HEAP32[r14+1]=24;HEAP32[r14+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254908,r13,406)}r13=HEAP32[1313728]-1|0;r14=HEAP32[r19+2];do{if(HEAP32[r19+3]-r14>>2>>>0>r13>>>0){r21=HEAP32[r14+(r13<<2)>>2];if((r21|0)==0){break}r22=r21;if(((tempValue=HEAP32[r20],HEAP32[r20]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r19]+8>>2]](r18)}HEAP32[r10]=0;r23=(r3|0)>>2;L426:do{if((r8|0)==(r9|0)){r11=432}else{r24=(r4|0)>>2;r25=r21>>2;r26=r21+8|0;r27=r21;r28=r2;r29=r16|0;r30=r17|0;r31=r15|0;r32=r8;r33=0;L428:while(1){r34=r33;while(1){if((r34|0)!=0){r11=432;break L426}r35=HEAP32[r23],r36=r35>>2;do{if((r35|0)==0){r37=0}else{if((HEAP32[r36+3]|0)!=(HEAP32[r36+4]|0)){r37=r35;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r36]+36>>2]](r35)|0)!=-1){r37=r35;break}HEAP32[r23]=0;r37=0}}while(0);r35=(r37|0)==0;r36=HEAP32[r24],r38=r36>>2;L438:do{if((r36|0)==0){r11=383}else{do{if((HEAP32[r38+3]|0)==(HEAP32[r38+4]|0)){if((FUNCTION_TABLE[HEAP32[HEAP32[r38]+36>>2]](r36)|0)!=-1){break}HEAP32[r24]=0;r11=383;break L438}}while(0);if(r35){r39=r36;break}else{r11=384;break L428}}}while(0);if(r11==383){r11=0;if(r35){r11=384;break L428}else{r39=0}}if(FUNCTION_TABLE[HEAP32[HEAP32[r25]+36>>2]](r22,HEAP8[r32],0)<<24>>24==37){r11=389;break}r36=HEAP8[r32];if(r36<<24>>24>-1){r40=HEAP32[r26>>2];if((HEAP16[r40+(r36<<24>>24<<1)>>1]&8192)<<16>>16!=0){r41=r32;r11=400;break}}r42=(r37+12|0)>>2;r36=HEAP32[r42];r43=r37+16|0;if((r36|0)==(HEAP32[r43>>2]|0)){r44=FUNCTION_TABLE[HEAP32[HEAP32[r37>>2]+36>>2]](r37)}else{r44=HEAPU8[r36]}if(FUNCTION_TABLE[HEAP32[HEAP32[r27>>2]+12>>2]](r22,r44&255)<<24>>24==FUNCTION_TABLE[HEAP32[HEAP32[r27>>2]+12>>2]](r22,HEAP8[r32])<<24>>24){r11=427;break}HEAP32[r10]=4;r34=4}L456:do{if(r11==427){r11=0;r34=HEAP32[r42];if((r34|0)==(HEAP32[r43>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r37>>2]+40>>2]](r37)}else{HEAP32[r42]=r34+1|0}r45=r32+1|0}else if(r11==400){while(1){r11=0;r34=r41+1|0;if((r34|0)==(r9|0)){r46=r9;break}r36=HEAP8[r34];if(r36<<24>>24<=-1){r46=r34;break}if((HEAP16[r40+(r36<<24>>24<<1)>>1]&8192)<<16>>16==0){r46=r34;break}else{r41=r34;r11=400}}r35=r37,r34=r35>>2;r36=r39,r38=r36>>2;while(1){do{if((r35|0)==0){r47=0}else{if((HEAP32[r34+3]|0)!=(HEAP32[r34+4]|0)){r47=r35;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r34]+36>>2]](r35)|0)!=-1){r47=r35;break}HEAP32[r23]=0;r47=0}}while(0);r48=(r47|0)==0;do{if((r36|0)==0){r11=413}else{if((HEAP32[r38+3]|0)!=(HEAP32[r38+4]|0)){if(r48){r49=r36;break}else{r45=r46;break L456}}if((FUNCTION_TABLE[HEAP32[HEAP32[r38]+36>>2]](r36)|0)==-1){HEAP32[r24]=0;r11=413;break}else{if(r48^(r36|0)==0){r49=r36;break}else{r45=r46;break L456}}}}while(0);if(r11==413){r11=0;if(r48){r45=r46;break L456}else{r49=0}}r50=(r47+12|0)>>2;r51=HEAP32[r50];r52=r47+16|0;if((r51|0)==(HEAP32[r52>>2]|0)){r53=FUNCTION_TABLE[HEAP32[HEAP32[r47>>2]+36>>2]](r47)}else{r53=HEAPU8[r51]}r51=r53<<24>>24;if(r51>>>0>=128){r45=r46;break L456}if((HEAP16[HEAP32[r26>>2]+(r51<<1)>>1]&8192)<<16>>16==0){r45=r46;break L456}r51=HEAP32[r50];if((r51|0)==(HEAP32[r52>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r47>>2]+40>>2]](r47);r35=r47,r34=r35>>2;r36=r49,r38=r36>>2;continue}else{HEAP32[r50]=r51+1|0;r35=r47,r34=r35>>2;r36=r49,r38=r36>>2;continue}}}else if(r11==389){r11=0;r36=r32+1|0;if((r36|0)==(r9|0)){r11=390;break L428}r38=FUNCTION_TABLE[HEAP32[HEAP32[r25]+36>>2]](r22,HEAP8[r36],0);if(r38<<24>>24==69|r38<<24>>24==48){r35=r32+2|0;if((r35|0)==(r9|0)){r11=393;break L428}r54=r38;r55=FUNCTION_TABLE[HEAP32[HEAP32[r25]+36>>2]](r22,HEAP8[r35],0);r56=r35}else{r54=0;r55=r38;r56=r36}r36=HEAP32[HEAP32[r28>>2]+36>>2];HEAP32[r29>>2]=r37;HEAP32[r30>>2]=r39;FUNCTION_TABLE[r36](r15,r2,r16,r17,r5,r6,r7,r55,r54);HEAP32[r23]=HEAP32[r31>>2];r45=r56+1|0}}while(0);if((r45|0)==(r9|0)){r11=432;break L426}r32=r45;r33=HEAP32[r10]}if(r11==384){HEAP32[r10]=4;r57=r37,r58=r57>>2;break}else if(r11==390){HEAP32[r10]=4;r57=r37,r58=r57>>2;break}else if(r11==393){HEAP32[r10]=4;r57=r37,r58=r57>>2;break}}}while(0);if(r11==432){r57=HEAP32[r23],r58=r57>>2}r22=r3|0;do{if((r57|0)!=0){if((HEAP32[r58+3]|0)!=(HEAP32[r58+4]|0)){break}if((FUNCTION_TABLE[HEAP32[HEAP32[r58]+36>>2]](r57)|0)!=-1){break}HEAP32[r22>>2]=0}}while(0);r23=HEAP32[r22>>2];r21=(r23|0)==0;r33=r4|0;r32=HEAP32[r33>>2],r31=r32>>2;L514:do{if((r32|0)==0){r11=442}else{do{if((HEAP32[r31+3]|0)==(HEAP32[r31+4]|0)){if((FUNCTION_TABLE[HEAP32[HEAP32[r31]+36>>2]](r32)|0)!=-1){break}HEAP32[r33>>2]=0;r11=442;break L514}}while(0);if(!r21){break}r59=r1|0,r60=r59>>2;HEAP32[r60]=r23;STACKTOP=r12;return}}while(0);do{if(r11==442){if(r21){break}r59=r1|0,r60=r59>>2;HEAP32[r60]=r23;STACKTOP=r12;return}}while(0);HEAP32[r10]=HEAP32[r10]|2;r59=r1|0,r60=r59>>2;HEAP32[r60]=r23;STACKTOP=r12;return}}while(0);r12=___cxa_allocate_exception(4);HEAP32[r12>>2]=5247488;___cxa_throw(r12,5252700,514)}function __ZNKSt3__18time_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE13__get_percentERS4_S4_RjRKNS_5ctypeIcEE(r1,r2,r3,r4,r5){var r6,r7,r8,r9,r10,r11,r12,r13,r14;r1=r4>>2;r4=0;r6=STACKTOP;r7=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r7>>2];r7=(r2|0)>>2;r2=HEAP32[r7],r8=r2>>2;do{if((r2|0)==0){r9=0}else{if((HEAP32[r8+3]|0)!=(HEAP32[r8+4]|0)){r9=r2;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r8]+36>>2]](r2)|0)==-1){HEAP32[r7]=0;r9=0;break}else{r9=HEAP32[r7];break}}}while(0);r2=(r9|0)==0;r9=(r3|0)>>2;r3=HEAP32[r9],r8=r3>>2;L536:do{if((r3|0)==0){r4=458}else{do{if((HEAP32[r8+3]|0)==(HEAP32[r8+4]|0)){if((FUNCTION_TABLE[HEAP32[HEAP32[r8]+36>>2]](r3)|0)!=-1){break}HEAP32[r9]=0;r4=458;break L536}}while(0);if(r2){r10=r3,r11=r10>>2;r12=0;break}else{r4=459;break}}}while(0);do{if(r4==458){if(r2){r4=459;break}else{r10=0,r11=r10>>2;r12=1;break}}}while(0);if(r4==459){HEAP32[r1]=HEAP32[r1]|6;STACKTOP=r6;return}r2=HEAP32[r7],r3=r2>>2;r8=HEAP32[r3+3];if((r8|0)==(HEAP32[r3+4]|0)){r13=FUNCTION_TABLE[HEAP32[HEAP32[r3]+36>>2]](r2)}else{r13=HEAPU8[r8]}if(FUNCTION_TABLE[HEAP32[HEAP32[r5>>2]+36>>2]](r5,r13&255,0)<<24>>24!=37){HEAP32[r1]=HEAP32[r1]|4;STACKTOP=r6;return}r13=HEAP32[r7];r5=r13+12|0;r8=HEAP32[r5>>2];if((r8|0)==(HEAP32[r13+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r13>>2]+40>>2]](r13)}else{HEAP32[r5>>2]=r8+1|0}r8=HEAP32[r7],r5=r8>>2;do{if((r8|0)==0){r14=0}else{if((HEAP32[r5+3]|0)!=(HEAP32[r5+4]|0)){r14=r8;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r5]+36>>2]](r8)|0)==-1){HEAP32[r7]=0;r14=0;break}else{r14=HEAP32[r7];break}}}while(0);r7=(r14|0)==0;do{if(r12){r4=478}else{if((HEAP32[r11+3]|0)!=(HEAP32[r11+4]|0)){if(!(r7^(r10|0)==0)){break}STACKTOP=r6;return}if((FUNCTION_TABLE[HEAP32[HEAP32[r11]+36>>2]](r10)|0)==-1){HEAP32[r9]=0;r4=478;break}if(!r7){break}STACKTOP=r6;return}}while(0);do{if(r4==478){if(r7){break}STACKTOP=r6;return}}while(0);HEAP32[r1]=HEAP32[r1]|2;STACKTOP=r6;return}function __ZNSt3__120__get_up_to_n_digitsIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r1,r2,r3,r4,r5){var r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33;r6=r3>>2;r3=0;r7=STACKTOP;r8=r2;r2=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r2>>2]=HEAP32[r8>>2];r8=(r1|0)>>2;r1=HEAP32[r8],r9=r1>>2;do{if((r1|0)==0){r10=0}else{if((HEAP32[r9+3]|0)!=(HEAP32[r9+4]|0)){r10=r1;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r9]+36>>2]](r1)|0)==-1){HEAP32[r8]=0;r10=0;break}else{r10=HEAP32[r8];break}}}while(0);r1=(r10|0)==0;r10=(r2|0)>>2;r2=HEAP32[r10],r9=r2>>2;L590:do{if((r2|0)==0){r3=498}else{do{if((HEAP32[r9+3]|0)==(HEAP32[r9+4]|0)){if((FUNCTION_TABLE[HEAP32[HEAP32[r9]+36>>2]](r2)|0)!=-1){break}HEAP32[r10]=0;r3=498;break L590}}while(0);if(r1){r11=r2;break}else{r3=499;break}}}while(0);do{if(r3==498){if(r1){r3=499;break}else{r11=0;break}}}while(0);if(r3==499){HEAP32[r6]=HEAP32[r6]|6;r12=0;STACKTOP=r7;return r12}r1=HEAP32[r8],r2=r1>>2;r9=HEAP32[r2+3];if((r9|0)==(HEAP32[r2+4]|0)){r13=FUNCTION_TABLE[HEAP32[HEAP32[r2]+36>>2]](r1)}else{r13=HEAPU8[r9]}r9=r13&255;r1=r13<<24>>24;do{if(r1>>>0<128){r13=r4+8|0;if((HEAP16[HEAP32[r13>>2]+(r1<<1)>>1]&2048)<<16>>16==0){break}r2=r4;r14=FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+36>>2]](r4,r9,0)<<24>>24;r15=HEAP32[r8];r16=r15+12|0;r17=HEAP32[r16>>2];do{if((r17|0)==(HEAP32[r15+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r15>>2]+40>>2]](r15);r18=r14;r19=r5;r20=r11,r21=r20>>2;break}else{HEAP32[r16>>2]=r17+1|0;r18=r14;r19=r5;r20=r11,r21=r20>>2;break}}while(0);while(1){r22=r18-48|0;r14=r19-1|0;r17=HEAP32[r8],r16=r17>>2;do{if((r17|0)==0){r23=0}else{if((HEAP32[r16+3]|0)!=(HEAP32[r16+4]|0)){r23=r17;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r16]+36>>2]](r17)|0)==-1){HEAP32[r8]=0;r23=0;break}else{r23=HEAP32[r8];break}}}while(0);r17=(r23|0)==0;if((r20|0)==0){r24=r23,r25=r24>>2;r26=0,r27=r26>>2}else{do{if((HEAP32[r21+3]|0)==(HEAP32[r21+4]|0)){if((FUNCTION_TABLE[HEAP32[HEAP32[r21]+36>>2]](r20)|0)!=-1){r28=r20;break}HEAP32[r10]=0;r28=0}else{r28=r20}}while(0);r24=HEAP32[r8],r25=r24>>2;r26=r28,r27=r26>>2}r29=(r26|0)==0;if(!((r17^r29)&(r14|0)>0)){r3=528;break}r16=HEAP32[r25+3];if((r16|0)==(HEAP32[r25+4]|0)){r30=FUNCTION_TABLE[HEAP32[HEAP32[r25]+36>>2]](r24)}else{r30=HEAPU8[r16]}r16=r30<<24>>24;if(r16>>>0>=128){r12=r22;r3=544;break}if((HEAP16[HEAP32[r13>>2]+(r16<<1)>>1]&2048)<<16>>16==0){r12=r22;r3=542;break}r16=(FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+36>>2]](r4,r30&255,0)<<24>>24)+(r22*10&-1)|0;r15=HEAP32[r8];r31=r15+12|0;r32=HEAP32[r31>>2];if((r32|0)==(HEAP32[r15+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r15>>2]+40>>2]](r15);r18=r16;r19=r14;r20=r26,r21=r20>>2;continue}else{HEAP32[r31>>2]=r32+1|0;r18=r16;r19=r14;r20=r26,r21=r20>>2;continue}}if(r3==542){STACKTOP=r7;return r12}else if(r3==544){STACKTOP=r7;return r12}else if(r3==528){do{if((r24|0)==0){r33=0}else{if((HEAP32[r25+3]|0)!=(HEAP32[r25+4]|0)){r33=r24;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r25]+36>>2]](r24)|0)==-1){HEAP32[r8]=0;r33=0;break}else{r33=HEAP32[r8];break}}}while(0);r2=(r33|0)==0;L649:do{if(r29){r3=538}else{do{if((HEAP32[r27+3]|0)==(HEAP32[r27+4]|0)){if((FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r26)|0)!=-1){break}HEAP32[r10]=0;r3=538;break L649}}while(0);if(r2){r12=r22}else{break}STACKTOP=r7;return r12}}while(0);do{if(r3==538){if(r2){break}else{r12=r22}STACKTOP=r7;return r12}}while(0);HEAP32[r6]=HEAP32[r6]|2;r12=r22;STACKTOP=r7;return r12}}}while(0);HEAP32[r6]=HEAP32[r6]|4;r12=0;STACKTOP=r7;return r12}function __ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE17__get_white_spaceERS4_S4_RjRKNS_5ctypeIwEE(r1,r2,r3,r4,r5){var r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22;r1=0;r6=STACKTOP;r7=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r7>>2];r7=(r2|0)>>2;r2=(r3|0)>>2;r3=r5;L664:while(1){r8=HEAP32[r7],r9=r8>>2;do{if((r8|0)==0){r10=1}else{r11=HEAP32[r9+3];if((r11|0)==(HEAP32[r9+4]|0)){r12=FUNCTION_TABLE[HEAP32[HEAP32[r9]+36>>2]](r8)}else{r12=HEAP32[r11>>2]}if((r12|0)==-1){HEAP32[r7]=0;r10=1;break}else{r10=(HEAP32[r7]|0)==0;break}}}while(0);r8=HEAP32[r2],r9=r8>>2;do{if((r8|0)==0){r1=562}else{r11=HEAP32[r9+3];if((r11|0)==(HEAP32[r9+4]|0)){r13=FUNCTION_TABLE[HEAP32[HEAP32[r9]+36>>2]](r8)}else{r13=HEAP32[r11>>2]}if((r13|0)==-1){HEAP32[r2]=0;r1=562;break}else{r11=(r8|0)==0;if(r10^r11){r14=r8;r15=r11;break}else{r16=r8,r17=r16>>2;r18=r11;break L664}}}}while(0);if(r1==562){r1=0;if(r10){r16=0,r17=r16>>2;r18=1;break}else{r14=0;r15=1}}r8=HEAP32[r7],r9=r8>>2;r11=HEAP32[r9+3];if((r11|0)==(HEAP32[r9+4]|0)){r19=FUNCTION_TABLE[HEAP32[HEAP32[r9]+36>>2]](r8)}else{r19=HEAP32[r11>>2]}if(!FUNCTION_TABLE[HEAP32[HEAP32[r3>>2]+12>>2]](r5,8192,r19)){r16=r14,r17=r16>>2;r18=r15;break}r11=HEAP32[r7];r8=r11+12|0;r9=HEAP32[r8>>2];if((r9|0)==(HEAP32[r11+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r11>>2]+40>>2]](r11);continue}else{HEAP32[r8>>2]=r9+4|0;continue}}r15=HEAP32[r7],r14=r15>>2;do{if((r15|0)==0){r20=1}else{r19=HEAP32[r14+3];if((r19|0)==(HEAP32[r14+4]|0)){r21=FUNCTION_TABLE[HEAP32[HEAP32[r14]+36>>2]](r15)}else{r21=HEAP32[r19>>2]}if((r21|0)==-1){HEAP32[r7]=0;r20=1;break}else{r20=(HEAP32[r7]|0)==0;break}}}while(0);do{if(r18){r1=584}else{r7=HEAP32[r17+3];if((r7|0)==(HEAP32[r17+4]|0)){r22=FUNCTION_TABLE[HEAP32[HEAP32[r17]+36>>2]](r16)}else{r22=HEAP32[r7>>2]}if((r22|0)==-1){HEAP32[r2]=0;r1=584;break}if(!(r20^(r16|0)==0)){break}STACKTOP=r6;return}}while(0);do{if(r1==584){if(r20){break}STACKTOP=r6;return}}while(0);HEAP32[r4>>2]=HEAP32[r4>>2]|2;STACKTOP=r6;return}function __ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE11__get_am_pmERiRS4_S4_RjRKNS_5ctypeIwEE(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11;r7=STACKTOP;STACKTOP=STACKTOP+4|0;r8=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r8>>2];r8=r7;r9=r1+8|0;r1=FUNCTION_TABLE[HEAP32[HEAP32[r9>>2]+8>>2]](r9);r9=HEAPU8[r1];if((r9&1|0)==0){r10=r9>>>1}else{r10=HEAP32[r1+4>>2]}r9=HEAPU8[r1+12|0];if((r9&1|0)==0){r11=r9>>>1}else{r11=HEAP32[r1+16>>2]}if((r10|0)==(-r11|0)){HEAP32[r5>>2]=HEAP32[r5>>2]|4;STACKTOP=r7;return}HEAP32[r8>>2]=HEAP32[r4>>2];r4=__ZNSt3__114__scan_keywordINS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEPKNS_12basic_stringIwS3_NS_9allocatorIwEEEENS_5ctypeIwEEEET0_RT_SE_SD_SD_RKT1_Rjb(r3,r8,r1,r1+24|0,r6,r5,0);r5=r4-r1|0;do{if((r4|0)==(r1|0)){if((HEAP32[r2>>2]|0)!=12){break}HEAP32[r2>>2]=0;STACKTOP=r7;return}}while(0);if((r5|0)!=12){STACKTOP=r7;return}r5=HEAP32[r2>>2];if((r5|0)>=12){STACKTOP=r7;return}HEAP32[r2>>2]=r5+12|0;STACKTOP=r7;return}function __ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE3getES4_S4_RNS_8ios_baseERjP2tmPKwSC_(r1,r2,r3,r4,r5,r6,r7,r8,r9){var r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48,r49,r50,r51,r52,r53,r54,r55,r56,r57,r58,r59,r60,r61,r62,r63,r64,r65,r66;r10=r6>>2;r11=0;r12=STACKTOP;STACKTOP=STACKTOP+24|0;r13=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r13>>2];r13=r4;r4=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r4>>2]=HEAP32[r13>>2];r13=r12,r14=r13>>2;r15=r12+12;r16=r12+16;r17=r12+20;r18=HEAP32[r5+28>>2],r19=r18>>2;r20=(r18+4|0)>>2;tempValue=HEAP32[r20],HEAP32[r20]=tempValue+1,tempValue;if((HEAP32[1313725]|0)!=-1){HEAP32[r14]=5254900;HEAP32[r14+1]=24;HEAP32[r14+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5254900,r13,406)}r13=HEAP32[1313726]-1|0;r14=HEAP32[r19+2];do{if(HEAP32[r19+3]-r14>>2>>>0>r13>>>0){r21=HEAP32[r14+(r13<<2)>>2];if((r21|0)==0){break}r22=r21;if(((tempValue=HEAP32[r20],HEAP32[r20]=tempValue+ -1,tempValue)|0)==0){FUNCTION_TABLE[HEAP32[HEAP32[r19]+8>>2]](r18)}HEAP32[r10]=0;r23=(r3|0)>>2;L757:do{if((r8|0)==(r9|0)){r11=685}else{r24=(r4|0)>>2;r25=r21>>2;r26=r21>>2;r27=r21;r28=r2;r29=r16|0;r30=r17|0;r31=r15|0;r32=r8,r33=r32>>2;r34=0;L759:while(1){r35=r34;while(1){if((r35|0)!=0){r11=685;break L757}r36=HEAP32[r23],r37=r36>>2;do{if((r36|0)==0){r38=0}else{r39=HEAP32[r37+3];if((r39|0)==(HEAP32[r37+4]|0)){r40=FUNCTION_TABLE[HEAP32[HEAP32[r37]+36>>2]](r36)}else{r40=HEAP32[r39>>2]}if((r40|0)!=-1){r38=r36;break}HEAP32[r23]=0;r38=0}}while(0);r36=(r38|0)==0;r37=HEAP32[r24],r39=r37>>2;do{if((r37|0)==0){r11=635}else{r41=HEAP32[r39+3];if((r41|0)==(HEAP32[r39+4]|0)){r42=FUNCTION_TABLE[HEAP32[HEAP32[r39]+36>>2]](r37)}else{r42=HEAP32[r41>>2]}if((r42|0)==-1){HEAP32[r24]=0;r11=635;break}else{if(r36^(r37|0)==0){r43=r37;break}else{r11=637;break L759}}}}while(0);if(r11==635){r11=0;if(r36){r11=637;break L759}else{r43=0}}if(FUNCTION_TABLE[HEAP32[HEAP32[r25]+52>>2]](r22,HEAP32[r33],0)<<24>>24==37){r11=642;break}if(FUNCTION_TABLE[HEAP32[HEAP32[r26]+12>>2]](r22,8192,HEAP32[r33])){r44=r32;r11=652;break}r45=(r38+12|0)>>2;r37=HEAP32[r45];r46=r38+16|0;if((r37|0)==(HEAP32[r46>>2]|0)){r47=FUNCTION_TABLE[HEAP32[HEAP32[r38>>2]+36>>2]](r38)}else{r47=HEAP32[r37>>2]}if((FUNCTION_TABLE[HEAP32[HEAP32[r27>>2]+28>>2]](r22,r47)|0)==(FUNCTION_TABLE[HEAP32[HEAP32[r27>>2]+28>>2]](r22,HEAP32[r33])|0)){r11=680;break}HEAP32[r10]=4;r35=4}L791:do{if(r11==642){r11=0;r35=r32+4|0;if((r35|0)==(r9|0)){r11=643;break L759}r37=FUNCTION_TABLE[HEAP32[HEAP32[r25]+52>>2]](r22,HEAP32[r35>>2],0);if(r37<<24>>24==69|r37<<24>>24==48){r39=r32+8|0;if((r39|0)==(r9|0)){r11=646;break L759}r48=r37;r49=FUNCTION_TABLE[HEAP32[HEAP32[r25]+52>>2]](r22,HEAP32[r39>>2],0);r50=r39}else{r48=0;r49=r37;r50=r35}r35=HEAP32[HEAP32[r28>>2]+36>>2];HEAP32[r29>>2]=r38;HEAP32[r30>>2]=r43;FUNCTION_TABLE[r35](r15,r2,r16,r17,r5,r6,r7,r49,r48);HEAP32[r23]=HEAP32[r31>>2];r51=r50+4|0}else if(r11==680){r11=0;r35=HEAP32[r45];if((r35|0)==(HEAP32[r46>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r38>>2]+40>>2]](r38)}else{HEAP32[r45]=r35+4|0}r51=r32+4|0}else if(r11==652){while(1){r11=0;r35=r44+4|0;if((r35|0)==(r9|0)){r52=r9;break}if(FUNCTION_TABLE[HEAP32[HEAP32[r26]+12>>2]](r22,8192,HEAP32[r35>>2])){r44=r35;r11=652}else{r52=r35;break}}r36=r38,r35=r36>>2;r37=r43,r39=r37>>2;while(1){do{if((r36|0)==0){r53=0}else{r41=HEAP32[r35+3];if((r41|0)==(HEAP32[r35+4]|0)){r54=FUNCTION_TABLE[HEAP32[HEAP32[r35]+36>>2]](r36)}else{r54=HEAP32[r41>>2]}if((r54|0)!=-1){r53=r36;break}HEAP32[r23]=0;r53=0}}while(0);r41=(r53|0)==0;do{if((r37|0)==0){r11=667}else{r55=HEAP32[r39+3];if((r55|0)==(HEAP32[r39+4]|0)){r56=FUNCTION_TABLE[HEAP32[HEAP32[r39]+36>>2]](r37)}else{r56=HEAP32[r55>>2]}if((r56|0)==-1){HEAP32[r24]=0;r11=667;break}else{if(r41^(r37|0)==0){r57=r37;break}else{r51=r52;break L791}}}}while(0);if(r11==667){r11=0;if(r41){r51=r52;break L791}else{r57=0}}r55=(r53+12|0)>>2;r58=HEAP32[r55];r59=r53+16|0;if((r58|0)==(HEAP32[r59>>2]|0)){r60=FUNCTION_TABLE[HEAP32[HEAP32[r53>>2]+36>>2]](r53)}else{r60=HEAP32[r58>>2]}if(!FUNCTION_TABLE[HEAP32[HEAP32[r26]+12>>2]](r22,8192,r60)){r51=r52;break L791}r58=HEAP32[r55];if((r58|0)==(HEAP32[r59>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r53>>2]+40>>2]](r53);r36=r53,r35=r36>>2;r37=r57,r39=r37>>2;continue}else{HEAP32[r55]=r58+4|0;r36=r53,r35=r36>>2;r37=r57,r39=r37>>2;continue}}}}while(0);if((r51|0)==(r9|0)){r11=685;break L757}r32=r51,r33=r32>>2;r34=HEAP32[r10]}if(r11==637){HEAP32[r10]=4;r61=r38,r62=r61>>2;break}else if(r11==643){HEAP32[r10]=4;r61=r38,r62=r61>>2;break}else if(r11==646){HEAP32[r10]=4;r61=r38,r62=r61>>2;break}}}while(0);if(r11==685){r61=HEAP32[r23],r62=r61>>2}r22=r3|0;do{if((r61|0)!=0){r21=HEAP32[r62+3];if((r21|0)==(HEAP32[r62+4]|0)){r63=FUNCTION_TABLE[HEAP32[HEAP32[r62]+36>>2]](r61)}else{r63=HEAP32[r21>>2]}if((r63|0)!=-1){break}HEAP32[r22>>2]=0}}while(0);r23=HEAP32[r22>>2];r21=(r23|0)==0;r34=r4|0;r32=HEAP32[r34>>2],r33=r32>>2;do{if((r32|0)==0){r11=698}else{r26=HEAP32[r33+3];if((r26|0)==(HEAP32[r33+4]|0)){r64=FUNCTION_TABLE[HEAP32[HEAP32[r33]+36>>2]](r32)}else{r64=HEAP32[r26>>2]}if((r64|0)==-1){HEAP32[r34>>2]=0;r11=698;break}if(!(r21^(r32|0)==0)){break}r65=r1|0,r66=r65>>2;HEAP32[r66]=r23;STACKTOP=r12;return}}while(0);do{if(r11==698){if(r21){break}r65=r1|0,r66=r65>>2;HEAP32[r66]=r23;STACKTOP=r12;return}}while(0);HEAP32[r10]=HEAP32[r10]|2;r65=r1|0,r66=r65>>2;HEAP32[r66]=r23;STACKTOP=r12;return}}while(0);r12=___cxa_allocate_exception(4);HEAP32[r12>>2]=5247488;___cxa_throw(r12,5252700,514)}function __ZNKSt3__18time_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE13__get_percentERS4_S4_RjRKNS_5ctypeIwEE(r1,r2,r3,r4,r5){var r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19;r1=r4>>2;r4=0;r6=STACKTOP;r7=r3;r3=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r3>>2]=HEAP32[r7>>2];r7=(r2|0)>>2;r2=HEAP32[r7],r8=r2>>2;do{if((r2|0)==0){r9=1}else{r10=HEAP32[r8+3];if((r10|0)==(HEAP32[r8+4]|0)){r11=FUNCTION_TABLE[HEAP32[HEAP32[r8]+36>>2]](r2)}else{r11=HEAP32[r10>>2]}if((r11|0)==-1){HEAP32[r7]=0;r9=1;break}else{r9=(HEAP32[r7]|0)==0;break}}}while(0);r11=(r3|0)>>2;r3=HEAP32[r11],r2=r3>>2;do{if((r3|0)==0){r4=718}else{r8=HEAP32[r2+3];if((r8|0)==(HEAP32[r2+4]|0)){r12=FUNCTION_TABLE[HEAP32[HEAP32[r2]+36>>2]](r3)}else{r12=HEAP32[r8>>2]}if((r12|0)==-1){HEAP32[r11]=0;r4=718;break}else{r8=(r3|0)==0;if(r9^r8){r13=r3,r14=r13>>2;r15=r8;break}else{r4=720;break}}}}while(0);do{if(r4==718){if(r9){r4=720;break}else{r13=0,r14=r13>>2;r15=1;break}}}while(0);if(r4==720){HEAP32[r1]=HEAP32[r1]|6;STACKTOP=r6;return}r9=HEAP32[r7],r3=r9>>2;r12=HEAP32[r3+3];if((r12|0)==(HEAP32[r3+4]|0)){r16=FUNCTION_TABLE[HEAP32[HEAP32[r3]+36>>2]](r9)}else{r16=HEAP32[r12>>2]}if(FUNCTION_TABLE[HEAP32[HEAP32[r5>>2]+52>>2]](r5,r16,0)<<24>>24!=37){HEAP32[r1]=HEAP32[r1]|4;STACKTOP=r6;return}r16=HEAP32[r7];r5=r16+12|0;r12=HEAP32[r5>>2];if((r12|0)==(HEAP32[r16+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r16>>2]+40>>2]](r16)}else{HEAP32[r5>>2]=r12+4|0}r12=HEAP32[r7],r5=r12>>2;do{if((r12|0)==0){r17=1}else{r16=HEAP32[r5+3];if((r16|0)==(HEAP32[r5+4]|0)){r18=FUNCTION_TABLE[HEAP32[HEAP32[r5]+36>>2]](r12)}else{r18=HEAP32[r16>>2]}if((r18|0)==-1){HEAP32[r7]=0;r17=1;break}else{r17=(HEAP32[r7]|0)==0;break}}}while(0);do{if(r15){r4=742}else{r7=HEAP32[r14+3];if((r7|0)==(HEAP32[r14+4]|0)){r19=FUNCTION_TABLE[HEAP32[HEAP32[r14]+36>>2]](r13)}else{r19=HEAP32[r7>>2]}if((r19|0)==-1){HEAP32[r11]=0;r4=742;break}if(!(r17^(r13|0)==0)){break}STACKTOP=r6;return}}while(0);do{if(r4==742){if(r17){break}STACKTOP=r6;return}}while(0);HEAP32[r1]=HEAP32[r1]|2;STACKTOP=r6;return}function __ZNSt3__120__get_up_to_n_digitsIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEEEiRT0_S5_RjRKNS_5ctypeIT_EEi(r1,r2,r3,r4,r5){var r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34;r6=r3>>2;r3=0;r7=STACKTOP;r8=r2;r2=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r2>>2]=HEAP32[r8>>2];r8=(r1|0)>>2;r1=HEAP32[r8],r9=r1>>2;do{if((r1|0)==0){r10=1}else{r11=HEAP32[r9+3];if((r11|0)==(HEAP32[r9+4]|0)){r12=FUNCTION_TABLE[HEAP32[HEAP32[r9]+36>>2]](r1)}else{r12=HEAP32[r11>>2]}if((r12|0)==-1){HEAP32[r8]=0;r10=1;break}else{r10=(HEAP32[r8]|0)==0;break}}}while(0);r12=(r2|0)>>2;r2=HEAP32[r12],r1=r2>>2;do{if((r2|0)==0){r3=764}else{r9=HEAP32[r1+3];if((r9|0)==(HEAP32[r1+4]|0)){r13=FUNCTION_TABLE[HEAP32[HEAP32[r1]+36>>2]](r2)}else{r13=HEAP32[r9>>2]}if((r13|0)==-1){HEAP32[r12]=0;r3=764;break}else{if(r10^(r2|0)==0){r14=r2;break}else{r3=766;break}}}}while(0);do{if(r3==764){if(r10){r3=766;break}else{r14=0;break}}}while(0);if(r3==766){HEAP32[r6]=HEAP32[r6]|6;r15=0;STACKTOP=r7;return r15}r10=HEAP32[r8],r2=r10>>2;r13=HEAP32[r2+3];if((r13|0)==(HEAP32[r2+4]|0)){r16=FUNCTION_TABLE[HEAP32[HEAP32[r2]+36>>2]](r10)}else{r16=HEAP32[r13>>2]}r13=r4;if(!FUNCTION_TABLE[HEAP32[HEAP32[r13>>2]+12>>2]](r4,2048,r16)){HEAP32[r6]=HEAP32[r6]|4;r15=0;STACKTOP=r7;return r15}r10=r4;r2=FUNCTION_TABLE[HEAP32[HEAP32[r10>>2]+52>>2]](r4,r16,0)<<24>>24;r16=HEAP32[r8];r1=r16+12|0;r9=HEAP32[r1>>2];do{if((r9|0)==(HEAP32[r16+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r16>>2]+40>>2]](r16);r17=r2;r18=r5;r19=r14,r20=r19>>2;break}else{HEAP32[r1>>2]=r9+4|0;r17=r2;r18=r5;r19=r14,r20=r19>>2;break}}while(0);while(1){r21=r17-48|0;r14=r18-1|0;r5=HEAP32[r8],r2=r5>>2;do{if((r5|0)==0){r22=0}else{r9=HEAP32[r2+3];if((r9|0)==(HEAP32[r2+4]|0)){r23=FUNCTION_TABLE[HEAP32[HEAP32[r2]+36>>2]](r5)}else{r23=HEAP32[r9>>2]}if((r23|0)==-1){HEAP32[r8]=0;r22=0;break}else{r22=HEAP32[r8];break}}}while(0);r5=(r22|0)==0;if((r19|0)==0){r24=r22,r25=r24>>2;r26=0,r27=r26>>2}else{r2=HEAP32[r20+3];if((r2|0)==(HEAP32[r20+4]|0)){r28=FUNCTION_TABLE[HEAP32[HEAP32[r20]+36>>2]](r19)}else{r28=HEAP32[r2>>2]}if((r28|0)==-1){HEAP32[r12]=0;r29=0}else{r29=r19}r24=HEAP32[r8],r25=r24>>2;r26=r29,r27=r26>>2}r30=(r26|0)==0;if(!((r5^r30)&(r14|0)>0)){break}r5=HEAP32[r25+3];if((r5|0)==(HEAP32[r25+4]|0)){r31=FUNCTION_TABLE[HEAP32[HEAP32[r25]+36>>2]](r24)}else{r31=HEAP32[r5>>2]}if(!FUNCTION_TABLE[HEAP32[HEAP32[r13>>2]+12>>2]](r4,2048,r31)){r15=r21;r3=817;break}r5=(FUNCTION_TABLE[HEAP32[HEAP32[r10>>2]+52>>2]](r4,r31,0)<<24>>24)+(r21*10&-1)|0;r2=HEAP32[r8];r9=r2+12|0;r1=HEAP32[r9>>2];if((r1|0)==(HEAP32[r2+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r2>>2]+40>>2]](r2);r17=r5;r18=r14;r19=r26,r20=r19>>2;continue}else{HEAP32[r9>>2]=r1+4|0;r17=r5;r18=r14;r19=r26,r20=r19>>2;continue}}if(r3==817){STACKTOP=r7;return r15}do{if((r24|0)==0){r32=1}else{r19=HEAP32[r25+3];if((r19|0)==(HEAP32[r25+4]|0)){r33=FUNCTION_TABLE[HEAP32[HEAP32[r25]+36>>2]](r24)}else{r33=HEAP32[r19>>2]}if((r33|0)==-1){HEAP32[r8]=0;r32=1;break}else{r32=(HEAP32[r8]|0)==0;break}}}while(0);do{if(r30){r3=810}else{r8=HEAP32[r27+3];if((r8|0)==(HEAP32[r27+4]|0)){r34=FUNCTION_TABLE[HEAP32[HEAP32[r27]+36>>2]](r26)}else{r34=HEAP32[r8>>2]}if((r34|0)==-1){HEAP32[r12]=0;r3=810;break}if(r32^(r26|0)==0){r15=r21}else{break}STACKTOP=r7;return r15}}while(0);do{if(r3==810){if(r32){break}else{r15=r21}STACKTOP=r7;return r15}}while(0);HEAP32[r6]=HEAP32[r6]|2;r15=r21;STACKTOP=r7;return r15}
function __ZNSt3__19money_getIcNS_19istreambuf_iteratorIcNS_11char_traitsIcEEEEE8__do_getERS4_S4_bRKNS_6localeEjRjRbRKNS_5ctypeIcEERNS_10unique_ptrIcPFvPvEEERPcSM_(r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11){var r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48,r49,r50,r51,r52,r53,r54,r55,r56,r57,r58,r59,r60,r61,r62,r63,r64,r65,r66,r67,r68,r69,r70,r71,r72,r73,r74,r75,r76,r77,r78,r79,r80,r81,r82,r83,r84,r85,r86,r87,r88,r89,r90,r91,r92,r93,r94,r95,r96,r97,r98,r99,r100,r101,r102,r103,r104,r105,r106,r107,r108,r109,r110,r111,r112,r113,r114,r115,r116,r117,r118,r119,r120,r121,r122,r123,r124,r125,r126,r127,r128,r129,r130,r131,r132,r133,r134,r135,r136,r137,r138,r139,r140,r141,r142,r143,r144,r145,r146,r147,r148,r149,r150,r151,r152,r153,r154,r155,r156,r157,r158,r159,r160,r161;r12=r10>>2;r10=r6>>2;r6=0;r13=STACKTOP;STACKTOP=STACKTOP+424|0;r14=r2;r2=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r2>>2]=HEAP32[r14>>2];r14=r13;r15=r13+400;r16=r13+404;r17=r13+408;r18=r13+412;r19=r18,r20=r19>>2;r21=STACKTOP;STACKTOP=STACKTOP+12|0;r22=STACKTOP;STACKTOP=STACKTOP+12|0;r23=STACKTOP;STACKTOP=STACKTOP+12|0;r24=STACKTOP;STACKTOP=STACKTOP+12|0;r25=STACKTOP;STACKTOP=STACKTOP+4|0;r26=STACKTOP;STACKTOP=STACKTOP+4|0;r27=r14|0;HEAP32[r20]=0;HEAP32[r20+1]=0;HEAP32[r20+2]=0;r20=r21,r28=r20>>2;r29=r22,r30=r29>>2;r31=r23,r32=r31>>2;r33=r24,r34=r33>>2;HEAP32[r28]=0;HEAP32[r28+1]=0;HEAP32[r28+2]=0;HEAP32[r30]=0;HEAP32[r30+1]=0;HEAP32[r30+2]=0;HEAP32[r32]=0;HEAP32[r32+1]=0;HEAP32[r32+2]=0;HEAP32[r34]=0;HEAP32[r34+1]=0;HEAP32[r34+2]=0;__ZNSt3__111__money_getIcE13__gather_infoEbRKNS_6localeERNS_10money_base7patternERcS8_RNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEESF_SF_SF_Ri(r3,r4,r15,r16,r17,r18,r21,r22,r23,r25);r4=(r9|0)>>2;HEAP32[r12]=HEAP32[r4];r3=(r1|0)>>2;r1=(r2|0)>>2;r2=(r8+8|0)>>2;r8=r24+1|0;r34=(r24+8|0)>>2;r32=(r24|0)>>2;r30=(r24+4|0)>>2;r24=r23+1|0;r28=(r23+4|0)>>2;r35=(r23+8|0)>>2;r36=r22+1|0;r37=(r22+4|0)>>2;r38=(r22+8|0)>>2;r39=(r5&512|0)!=0;r5=r21+1|0;r40=(r21+4|0)>>2;r41=(r21+8|0)>>2;r21=r15+3|0;r42=(r9+4|0)>>2;r9=r18+4|0;r43=r11;r11=414;r44=r27;r45=r27;r27=r14+400|0;r14=0;r46=0;L1032:while(1){r47=HEAP32[r3],r48=r47>>2;do{if((r47|0)==0){r49=0}else{if((HEAP32[r48+3]|0)!=(HEAP32[r48+4]|0)){r49=r47;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r48]+36>>2]](r47)|0)==-1){HEAP32[r3]=0;r49=0;break}else{r49=HEAP32[r3];break}}}while(0);r47=(r49|0)==0;r48=HEAP32[r1],r50=r48>>2;do{if((r48|0)==0){r6=834}else{if((HEAP32[r50+3]|0)!=(HEAP32[r50+4]|0)){if(r47){r51=r48;break}else{r52=r11;r53=r44;r54=r45;r55=r14;r6=1115;break L1032}}if((FUNCTION_TABLE[HEAP32[HEAP32[r50]+36>>2]](r48)|0)==-1){HEAP32[r1]=0;r6=834;break}else{if(r47^(r48|0)==0){r51=r48;break}else{r52=r11;r53=r44;r54=r45;r55=r14;r6=1115;break L1032}}}}while(0);if(r6==834){r6=0;if(r47){r52=r11;r53=r44;r54=r45;r55=r14;r6=1115;break}else{r51=0}}r48=HEAP8[r15+r46|0]<<24>>24;L1054:do{if((r48|0)==0){r6=873}else if((r48|0)==2){if(!((r14|0)!=0|r46>>>0<2)){if((r46|0)==2){r56=HEAP8[r21]<<24>>24!=0}else{r56=0}if(!(r39|r56)){r57=0;r58=r27;r59=r45;r60=r44;r61=r11;r62=r43;break}}r50=HEAP8[r20];r63=(r50&1)<<24>>24==0;r64=r63?r5:HEAP32[r41];L1062:do{if((r46|0)==0){r65=r64;r66=r50;r67=r51,r68=r67>>2}else{if(HEAPU8[r15+(r46-1)|0]>=2){r65=r64;r66=r50;r67=r51,r68=r67>>2;break}r69=r50&255;r70=(r69&1|0)==0;r71=r69>>>1;r69=HEAP32[r40];r72=HEAP32[r41];r73=r64;while(1){if((r73|0)==((r63?r5:r72)+(r70?r71:r69)|0)){break}r74=HEAP8[r73];if(r74<<24>>24<=-1){break}if((HEAP16[HEAP32[r2]+(r74<<24>>24<<1)>>1]&8192)<<16>>16==0){break}else{r73=r73+1|0}}r69=r73-(r63?r5:r72)|0;r71=HEAP8[r33];r70=r71&255;r74=(r70&1|0)==0;L1070:do{if(r69>>>0<=(r74?r70>>>1:HEAP32[r30])>>>0){r75=(r71&1)<<24>>24==0;r76=(r75?r8:HEAP32[r34])+((r74?r70>>>1:HEAP32[r30])-r69)|0;r77=(r75?r8:HEAP32[r34])+(r74?r70>>>1:HEAP32[r30])|0;if((r76|0)==(r77|0)){r65=r73;r66=r50;r67=r51,r68=r67>>2;break L1062}else{r78=r63?r5:r72;r79=r76}while(1){if(HEAP8[r79]<<24>>24!=HEAP8[r78]<<24>>24){break L1070}r76=r79+1|0;if((r76|0)==(r77|0)){r65=r73;r66=r50;r67=r51,r68=r67>>2;break L1062}else{r78=r78+1|0;r79=r76}}}}while(0);r65=r63?r5:r72;r66=r50;r67=r51,r68=r67>>2;break}}while(0);L1076:while(1){r50=r66&255;if((r65|0)==(((r66&1)<<24>>24==0?r5:HEAP32[r41])+((r50&1|0)==0?r50>>>1:HEAP32[r40])|0)){break}r50=HEAP32[r3],r63=r50>>2;do{if((r50|0)==0){r80=0}else{if((HEAP32[r63+3]|0)!=(HEAP32[r63+4]|0)){r80=r50;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r63]+36>>2]](r50)|0)==-1){HEAP32[r3]=0;r80=0;break}else{r80=HEAP32[r3];break}}}while(0);r50=(r80|0)==0;do{if((r67|0)==0){r6=981}else{if((HEAP32[r68+3]|0)!=(HEAP32[r68+4]|0)){if(r50){r81=r67;break}else{break L1076}}if((FUNCTION_TABLE[HEAP32[HEAP32[r68]+36>>2]](r67)|0)==-1){HEAP32[r1]=0;r6=981;break}else{if(r50^(r67|0)==0){r81=r67;break}else{break L1076}}}}while(0);if(r6==981){r6=0;if(r50){break}else{r81=0}}r63=HEAP32[r3],r72=r63>>2;r64=HEAP32[r72+3];if((r64|0)==(HEAP32[r72+4]|0)){r82=FUNCTION_TABLE[HEAP32[HEAP32[r72]+36>>2]](r63)}else{r82=HEAPU8[r64]}if((r82<<24>>24|0)!=(HEAP8[r65]<<24>>24|0)){break}r64=HEAP32[r3];r63=r64+12|0;r72=HEAP32[r63>>2];if((r72|0)==(HEAP32[r64+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r64>>2]+40>>2]](r64)}else{HEAP32[r63>>2]=r72+1|0}r65=r65+1|0;r66=HEAP8[r20];r67=r81,r68=r67>>2}if(!r39){r57=r14;r58=r27;r59=r45;r60=r44;r61=r11;r62=r43;break}r72=HEAP8[r20];r63=r72&255;if((r65|0)==(((r72&1)<<24>>24==0?r5:HEAP32[r41])+((r63&1|0)==0?r63>>>1:HEAP32[r40])|0)){r57=r14;r58=r27;r59=r45;r60=r44;r61=r11;r62=r43;break}else{r6=993;break L1032}}else if((r48|0)==3){r63=HEAP8[r29];r72=r63&255;r64=(r72&1|0)==0;r73=HEAP8[r31];r70=r73&255;r74=(r70&1|0)==0;if(((r64?r72>>>1:HEAP32[r37])|0)==(-(r74?r70>>>1:HEAP32[r28])|0)){r57=r14;r58=r27;r59=r45;r60=r44;r61=r11;r62=r43;break}do{if(((r64?r72>>>1:HEAP32[r37])|0)!=0){if(((r74?r70>>>1:HEAP32[r28])|0)==0){break}r69=HEAP32[r3],r71=r69>>2;r77=HEAP32[r71+3];if((r77|0)==(HEAP32[r71+4]|0)){r83=FUNCTION_TABLE[HEAP32[HEAP32[r71]+36>>2]](r69);r84=HEAP8[r29]}else{r83=HEAPU8[r77];r84=r63}r77=HEAP32[r3],r69=r77>>2;r71=r77+12|0;r76=HEAP32[r71>>2];r75=(r76|0)==(HEAP32[r69+4]|0);if((r83<<24>>24|0)==(HEAP8[(r84&1)<<24>>24==0?r36:HEAP32[r38]]<<24>>24|0)){if(r75){FUNCTION_TABLE[HEAP32[HEAP32[r69]+40>>2]](r77)}else{HEAP32[r71>>2]=r76+1|0}r71=HEAPU8[r29];r57=((r71&1|0)==0?r71>>>1:HEAP32[r37])>>>0>1?r22:r14;r58=r27;r59=r45;r60=r44;r61=r11;r62=r43;break L1054}if(r75){r85=FUNCTION_TABLE[HEAP32[HEAP32[r69]+36>>2]](r77)}else{r85=HEAPU8[r76]}if((r85<<24>>24|0)!=(HEAP8[(HEAP8[r31]&1)<<24>>24==0?r24:HEAP32[r35]]<<24>>24|0)){r6=952;break L1032}r76=HEAP32[r3];r77=r76+12|0;r69=HEAP32[r77>>2];if((r69|0)==(HEAP32[r76+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r76>>2]+40>>2]](r76)}else{HEAP32[r77>>2]=r69+1|0}HEAP8[r7]=1;r69=HEAPU8[r31];r57=((r69&1|0)==0?r69>>>1:HEAP32[r28])>>>0>1?r23:r14;r58=r27;r59=r45;r60=r44;r61=r11;r62=r43;break L1054}}while(0);r70=HEAP32[r3],r74=r70>>2;r69=HEAP32[r74+3];r77=(r69|0)==(HEAP32[r74+4]|0);if(((r64?r72>>>1:HEAP32[r37])|0)==0){if(r77){r86=FUNCTION_TABLE[HEAP32[HEAP32[r74]+36>>2]](r70);r87=HEAP8[r31]}else{r86=HEAPU8[r69];r87=r73}if((r86<<24>>24|0)!=(HEAP8[(r87&1)<<24>>24==0?r24:HEAP32[r35]]<<24>>24|0)){r57=r14;r58=r27;r59=r45;r60=r44;r61=r11;r62=r43;break}r76=HEAP32[r3];r75=r76+12|0;r71=HEAP32[r75>>2];if((r71|0)==(HEAP32[r76+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r76>>2]+40>>2]](r76)}else{HEAP32[r75>>2]=r71+1|0}HEAP8[r7]=1;r71=HEAPU8[r31];r57=((r71&1|0)==0?r71>>>1:HEAP32[r28])>>>0>1?r23:r14;r58=r27;r59=r45;r60=r44;r61=r11;r62=r43;break}if(r77){r88=FUNCTION_TABLE[HEAP32[HEAP32[r74]+36>>2]](r70);r89=HEAP8[r29]}else{r88=HEAPU8[r69];r89=r63}if((r88<<24>>24|0)!=(HEAP8[(r89&1)<<24>>24==0?r36:HEAP32[r38]]<<24>>24|0)){HEAP8[r7]=1;r57=r14;r58=r27;r59=r45;r60=r44;r61=r11;r62=r43;break}r69=HEAP32[r3];r70=r69+12|0;r74=HEAP32[r70>>2];if((r74|0)==(HEAP32[r69+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r69>>2]+40>>2]](r69)}else{HEAP32[r70>>2]=r74+1|0}r74=HEAPU8[r29];r57=((r74&1|0)==0?r74>>>1:HEAP32[r37])>>>0>1?r22:r14;r58=r27;r59=r45;r60=r44;r61=r11;r62=r43;break}else if((r48|0)==4){r74=HEAP8[r17]<<24>>24;r70=0;r69=r27;r77=r45;r71=r44;r75=r11;r76=r43;L1163:while(1){r90=HEAP32[r3],r91=r90>>2;do{if((r90|0)==0){r92=0}else{if((HEAP32[r91+3]|0)!=(HEAP32[r91+4]|0)){r92=r90;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r91]+36>>2]](r90)|0)==-1){HEAP32[r3]=0;r92=0;break}else{r92=HEAP32[r3];break}}}while(0);r90=(r92|0)==0;r91=HEAP32[r1],r50=r91>>2;do{if((r91|0)==0){r6=1006}else{if((HEAP32[r50+3]|0)!=(HEAP32[r50+4]|0)){if(r90){break}else{break L1163}}if((FUNCTION_TABLE[HEAP32[HEAP32[r50]+36>>2]](r91)|0)==-1){HEAP32[r1]=0;r6=1006;break}else{if(r90^(r91|0)==0){break}else{break L1163}}}}while(0);if(r6==1006){r6=0;if(r90){break}}r91=HEAP32[r3],r50=r91>>2;r93=HEAP32[r50+3];if((r93|0)==(HEAP32[r50+4]|0)){r94=FUNCTION_TABLE[HEAP32[HEAP32[r50]+36>>2]](r91)}else{r94=HEAPU8[r93]}r93=r94&255;r91=r94<<24>>24;do{if(r91>>>0<128){if((HEAP16[HEAP32[r2]+(r91<<1)>>1]&2048)<<16>>16==0){r6=1025;break}r50=HEAP32[r12];if((r50|0)==(r76|0)){r95=(HEAP32[r42]|0)!=414;r96=HEAP32[r4];r97=r76-r96|0;r98=r97>>>0<2147483647?r97<<1:-1;r99=_realloc(r95?r96:0,r98);if((r99|0)==0){r6=1015;break L1032}do{if(r95){HEAP32[r4]=r99;r100=r99}else{r96=HEAP32[r4];HEAP32[r4]=r99;if((r96|0)==0){r100=r99;break}FUNCTION_TABLE[HEAP32[r42]](r96);r100=HEAP32[r4]}}while(0);HEAP32[r42]=218;r99=r100+r97|0;HEAP32[r12]=r99;r101=HEAP32[r4]+r98|0;r102=r99}else{r101=r76;r102=r50}HEAP32[r12]=r102+1|0;HEAP8[r102]=r93;r103=r70+1|0;r104=r69;r105=r77;r106=r71;r107=r75;r108=r101;break}else{r6=1025}}while(0);if(r6==1025){r6=0;r93=HEAPU8[r19];if(!((r70|0)!=0&(((r93&1|0)==0?r93>>>1:HEAP32[r9>>2])|0)!=0&(r91|0)==(r74|0))){break}if((r77|0)==(r69|0)){r93=(r75|0)!=414;r90=r77-r71|0;r99=r90>>>0<2147483647?r90<<1:-1;if(r93){r109=r71}else{r109=0}r93=_realloc(r109,r99);r95=r93;if((r93|0)==0){r6=1030;break L1032}r110=(r99>>>2<<2)+r95|0;r111=(r90>>2<<2)+r95|0;r112=r95;r113=218}else{r110=r69;r111=r77;r112=r71;r113=r75}HEAP32[r111>>2]=r70;r103=0;r104=r110;r105=r111+4|0;r106=r112;r107=r113;r108=r76}r95=HEAP32[r3];r90=r95+12|0;r99=HEAP32[r90>>2];if((r99|0)==(HEAP32[r95+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r95>>2]+40>>2]](r95);r70=r103;r69=r104;r77=r105;r71=r106;r75=r107;r76=r108;continue}else{HEAP32[r90>>2]=r99+1|0;r70=r103;r69=r104;r77=r105;r71=r106;r75=r107;r76=r108;continue}}if((r71|0)==(r77|0)|(r70|0)==0){r114=r69;r115=r77;r116=r71;r117=r75}else{if((r77|0)==(r69|0)){r74=(r75|0)!=414;r63=r77-r71|0;r73=r63>>>0<2147483647?r63<<1:-1;if(r74){r118=r71}else{r118=0}r74=_realloc(r118,r73);r72=r74;if((r74|0)==0){r6=1044;break L1032}r119=(r73>>>2<<2)+r72|0;r120=(r63>>2<<2)+r72|0;r121=r72;r122=218}else{r119=r69;r120=r77;r121=r71;r122=r75}HEAP32[r120>>2]=r70;r114=r119;r115=r120+4|0;r116=r121;r117=r122}r72=HEAP32[r25>>2];L1233:do{if((r72|0)>0){r63=HEAP32[r3],r73=r63>>2;do{if((r63|0)==0){r123=0}else{if((HEAP32[r73+3]|0)!=(HEAP32[r73+4]|0)){r123=r63;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r73]+36>>2]](r63)|0)==-1){HEAP32[r3]=0;r123=0;break}else{r123=HEAP32[r3];break}}}while(0);r63=(r123|0)==0;r73=HEAP32[r1],r91=r73>>2;do{if((r73|0)==0){r6=1063}else{if((HEAP32[r91+3]|0)!=(HEAP32[r91+4]|0)){if(r63){r124=r73;break}else{r6=1069;break L1032}}if((FUNCTION_TABLE[HEAP32[HEAP32[r91]+36>>2]](r73)|0)==-1){HEAP32[r1]=0;r6=1063;break}else{if(r63^(r73|0)==0){r124=r73;break}else{r6=1069;break L1032}}}}while(0);if(r6==1063){r6=0;if(r63){r6=1069;break L1032}else{r124=0}}r73=HEAP32[r3],r91=r73>>2;r74=HEAP32[r91+3];if((r74|0)==(HEAP32[r91+4]|0)){r125=FUNCTION_TABLE[HEAP32[HEAP32[r91]+36>>2]](r73)}else{r125=HEAPU8[r74]}if((r125<<24>>24|0)!=(HEAP8[r16]<<24>>24|0)){r6=1069;break L1032}r74=HEAP32[r3];r73=r74+12|0;r91=HEAP32[r73>>2];do{if((r91|0)==(HEAP32[r74+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r74>>2]+40>>2]](r74);r126=r76;r127=r124,r128=r127>>2;r129=r72;break}else{HEAP32[r73>>2]=r91+1|0;r126=r76;r127=r124,r128=r127>>2;r129=r72;break}}while(0);while(1){r91=HEAP32[r3],r73=r91>>2;do{if((r91|0)==0){r130=0}else{if((HEAP32[r73+3]|0)!=(HEAP32[r73+4]|0)){r130=r91;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r73]+36>>2]](r91)|0)==-1){HEAP32[r3]=0;r130=0;break}else{r130=HEAP32[r3];break}}}while(0);r91=(r130|0)==0;do{if((r127|0)==0){r6=1085}else{if((HEAP32[r128+3]|0)!=(HEAP32[r128+4]|0)){if(r91){r131=r127;break}else{r6=1092;break L1032}}if((FUNCTION_TABLE[HEAP32[HEAP32[r128]+36>>2]](r127)|0)==-1){HEAP32[r1]=0;r6=1085;break}else{if(r91^(r127|0)==0){r131=r127;break}else{r6=1092;break L1032}}}}while(0);if(r6==1085){r6=0;if(r91){r6=1092;break L1032}else{r131=0}}r73=HEAP32[r3],r50=r73>>2;r98=HEAP32[r50+3];if((r98|0)==(HEAP32[r50+4]|0)){r132=FUNCTION_TABLE[HEAP32[HEAP32[r50]+36>>2]](r73)}else{r132=HEAPU8[r98]}r98=r132<<24>>24;if(r98>>>0>=128){r6=1092;break L1032}if((HEAP16[HEAP32[r2]+(r98<<1)>>1]&2048)<<16>>16==0){r6=1092;break L1032}r98=HEAP32[r12];if((r98|0)==(r126|0)){r73=(HEAP32[r42]|0)!=414;r50=HEAP32[r4];r97=r126-r50|0;r74=r97>>>0<2147483647?r97<<1:-1;r63=_realloc(r73?r50:0,r74);if((r63|0)==0){r6=1095;break L1032}do{if(r73){HEAP32[r4]=r63;r133=r63}else{r50=HEAP32[r4];HEAP32[r4]=r63;if((r50|0)==0){r133=r63;break}FUNCTION_TABLE[HEAP32[r42]](r50);r133=HEAP32[r4]}}while(0);HEAP32[r42]=218;r63=r133+r97|0;HEAP32[r12]=r63;r134=HEAP32[r4]+r74|0;r135=r63}else{r134=r126;r135=r98}r63=HEAP32[r3],r73=r63>>2;r91=HEAP32[r73+3];if((r91|0)==(HEAP32[r73+4]|0)){r136=FUNCTION_TABLE[HEAP32[HEAP32[r73]+36>>2]](r63);r137=HEAP32[r12]}else{r136=HEAPU8[r91];r137=r135}HEAP32[r12]=r137+1|0;HEAP8[r137]=r136&255;r91=r129-1|0;HEAP32[r25>>2]=r91;r63=HEAP32[r3];r73=r63+12|0;r50=HEAP32[r73>>2];if((r50|0)==(HEAP32[r63+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r63>>2]+40>>2]](r63)}else{HEAP32[r73>>2]=r50+1|0}if((r91|0)>0){r126=r134;r127=r131,r128=r127>>2;r129=r91}else{r138=r134;break L1233}}}else{r138=r76}}while(0);if((HEAP32[r12]|0)==(HEAP32[r4]|0)){r6=1113;break L1032}else{r57=r14;r58=r114;r59=r115;r60=r116;r61=r117;r62=r138;break}}else if((r48|0)==1){if((r46|0)==3){r52=r11;r53=r44;r54=r45;r55=r14;r6=1115;break L1032}r76=HEAP32[r3],r72=r76>>2;r70=HEAP32[r72+3];if((r70|0)==(HEAP32[r72+4]|0)){r139=FUNCTION_TABLE[HEAP32[HEAP32[r72]+36>>2]](r76)}else{r139=HEAPU8[r70]}r70=r139<<24>>24;if(r70>>>0>=128){r6=872;break L1032}if((HEAP16[HEAP32[r2]+(r70<<1)>>1]&8192)<<16>>16==0){r6=872;break L1032}r70=HEAP32[r3];r76=r70+12|0;r72=HEAP32[r76>>2];if((r72|0)==(HEAP32[r70+16>>2]|0)){r140=FUNCTION_TABLE[HEAP32[HEAP32[r70>>2]+40>>2]](r70)}else{HEAP32[r76>>2]=r72+1|0;r140=HEAPU8[r72]}r72=r140&255;r76=HEAP8[r33];if((r76&1)<<24>>24==0){r141=10;r142=r76}else{r76=HEAP32[r32];r141=(r76&-2)-1|0;r142=r76&255}r76=r142&255;r70=(r76&1|0)==0?r76>>>1:HEAP32[r30];if((r70|0)==(r141|0)){if((r141|0)==-3){r6=860;break L1032}r76=(r142&1)<<24>>24==0?r8:HEAP32[r34];do{if(r141>>>0<2147483631){r75=r141+1|0;r71=r141<<1;r77=r75>>>0<r71>>>0?r71:r75;if(r77>>>0<11){r143=11;break}r143=r77+16&-16}else{r143=-2}}while(0);r77=__Znwj(r143);_memcpy(r77,r76,r141);if((r141|0)!=10){__ZdlPv(r76)}HEAP32[r34]=r77;r77=r143|1;HEAP32[r32]=r77;r144=r77&255}else{r144=r142}r77=(r144&1)<<24>>24==0?r8:HEAP32[r34];HEAP8[r77+r70|0]=r72;r75=r70+1|0;HEAP8[r77+r75|0]=0;if((HEAP8[r33]&1)<<24>>24==0){HEAP8[r33]=r75<<1&255;r6=873;break}else{HEAP32[r30]=r75;r6=873;break}}else{r57=r14;r58=r27;r59=r45;r60=r44;r61=r11;r62=r43}}while(0);L1341:do{if(r6==873){r6=0;if((r46|0)==3){r52=r11;r53=r44;r54=r45;r55=r14;r6=1115;break L1032}else{r145=r51,r146=r145>>2}while(1){r48=HEAP32[r3],r47=r48>>2;do{if((r48|0)==0){r147=0}else{if((HEAP32[r47+3]|0)!=(HEAP32[r47+4]|0)){r147=r48;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r47]+36>>2]](r48)|0)==-1){HEAP32[r3]=0;r147=0;break}else{r147=HEAP32[r3];break}}}while(0);r48=(r147|0)==0;do{if((r145|0)==0){r6=886}else{if((HEAP32[r146+3]|0)!=(HEAP32[r146+4]|0)){if(r48){r148=r145;break}else{r57=r14;r58=r27;r59=r45;r60=r44;r61=r11;r62=r43;break L1341}}if((FUNCTION_TABLE[HEAP32[HEAP32[r146]+36>>2]](r145)|0)==-1){HEAP32[r1]=0;r6=886;break}else{if(r48^(r145|0)==0){r148=r145;break}else{r57=r14;r58=r27;r59=r45;r60=r44;r61=r11;r62=r43;break L1341}}}}while(0);if(r6==886){r6=0;if(r48){r57=r14;r58=r27;r59=r45;r60=r44;r61=r11;r62=r43;break L1341}else{r148=0}}r47=HEAP32[r3],r75=r47>>2;r77=HEAP32[r75+3];if((r77|0)==(HEAP32[r75+4]|0)){r149=FUNCTION_TABLE[HEAP32[HEAP32[r75]+36>>2]](r47)}else{r149=HEAPU8[r77]}r77=r149<<24>>24;if(r77>>>0>=128){r57=r14;r58=r27;r59=r45;r60=r44;r61=r11;r62=r43;break L1341}if((HEAP16[HEAP32[r2]+(r77<<1)>>1]&8192)<<16>>16==0){r57=r14;r58=r27;r59=r45;r60=r44;r61=r11;r62=r43;break L1341}r77=HEAP32[r3];r47=r77+12|0;r75=HEAP32[r47>>2];if((r75|0)==(HEAP32[r77+16>>2]|0)){r150=FUNCTION_TABLE[HEAP32[HEAP32[r77>>2]+40>>2]](r77)}else{HEAP32[r47>>2]=r75+1|0;r150=HEAPU8[r75]}r75=r150&255;r47=HEAP8[r33];if((r47&1)<<24>>24==0){r151=10;r152=r47}else{r47=HEAP32[r32];r151=(r47&-2)-1|0;r152=r47&255}r47=r152&255;r77=(r47&1|0)==0?r47>>>1:HEAP32[r30];if((r77|0)==(r151|0)){if((r151|0)==-3){r6=900;break L1032}r47=(r152&1)<<24>>24==0?r8:HEAP32[r34];do{if(r151>>>0<2147483631){r71=r151+1|0;r69=r151<<1;r91=r71>>>0<r69>>>0?r69:r71;if(r91>>>0<11){r153=11;break}r153=r91+16&-16}else{r153=-2}}while(0);r48=__Znwj(r153);_memcpy(r48,r47,r151);if((r151|0)!=10){__ZdlPv(r47)}HEAP32[r34]=r48;r48=r153|1;HEAP32[r32]=r48;r154=r48&255}else{r154=r152}r48=(r154&1)<<24>>24==0?r8:HEAP32[r34];HEAP8[r48+r77|0]=r75;r91=r77+1|0;HEAP8[r48+r91|0]=0;if((HEAP8[r33]&1)<<24>>24==0){HEAP8[r33]=r91<<1&255;r145=r148,r146=r145>>2;continue}else{HEAP32[r30]=r91;r145=r148,r146=r145>>2;continue}}}}while(0);r70=r46+1|0;if(r70>>>0<4){r43=r62;r11=r61;r44=r60;r45=r59;r27=r58;r14=r57;r46=r70}else{r52=r61;r53=r60;r54=r59;r55=r57;r6=1115;break}}L1394:do{if(r6==872){HEAP32[r10]=HEAP32[r10]|4;r155=0;r156=r44;r157=r11}else if(r6==900){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}else if(r6==952){HEAP32[r10]=HEAP32[r10]|4;r155=0;r156=r44;r157=r11}else if(r6==860){__ZNKSt3__121__basic_string_commonILb1EE20__throw_length_errorEv(0)}else if(r6==993){HEAP32[r10]=HEAP32[r10]|4;r155=0;r156=r44;r157=r11}else if(r6==1015){__ZSt17__throw_bad_allocv()}else if(r6==1030){__ZSt17__throw_bad_allocv()}else if(r6==1044){__ZSt17__throw_bad_allocv()}else if(r6==1069){HEAP32[r10]=HEAP32[r10]|4;r155=0;r156=r116;r157=r117}else if(r6==1092){HEAP32[r10]=HEAP32[r10]|4;r155=0;r156=r116;r157=r117}else if(r6==1095){__ZSt17__throw_bad_allocv()}else if(r6==1113){HEAP32[r10]=HEAP32[r10]|4;r155=0;r156=r116;r157=r117}else if(r6==1115){L1414:do{if((r55|0)!=0){r57=r55;r59=r55+1|0;r60=r55+8|0;r61=r55+4|0;r46=1;L1416:while(1){r14=HEAPU8[r57];if((r14&1|0)==0){r158=r14>>>1}else{r158=HEAP32[r61>>2]}if(r46>>>0>=r158>>>0){break L1414}r14=HEAP32[r3],r58=r14>>2;do{if((r14|0)==0){r159=0}else{if((HEAP32[r58+3]|0)!=(HEAP32[r58+4]|0)){r159=r14;break}if((FUNCTION_TABLE[HEAP32[HEAP32[r58]+36>>2]](r14)|0)==-1){HEAP32[r3]=0;r159=0;break}else{r159=HEAP32[r3];break}}}while(0);r14=(r159|0)==0;r58=HEAP32[r1],r77=r58>>2;do{if((r58|0)==0){r6=1133}else{if((HEAP32[r77+3]|0)!=(HEAP32[r77+4]|0)){if(r14){break}else{break L1416}}if((FUNCTION_TABLE[HEAP32[HEAP32[r77]+36>>2]](r58)|0)==-1){HEAP32[r1]=0;r6=1133;break}else{if(r14^(r58|0)==0){break}else{break L1416}}}}while(0);if(r6==1133){r6=0;if(r14){break}}r58=HEAP32[r3],r77=r58>>2;r75=HEAP32[r77+3];if((r75|0)==(HEAP32[r77+4]|0)){r160=FUNCTION_TABLE[HEAP32[HEAP32[r77]+36>>2]](r58)}else{r160=HEAPU8[r75]}if((HEAP8[r57]&1)<<24>>24==0){r161=r59}else{r161=HEAP32[r60>>2]}if((r160<<24>>24|0)!=(HEAP8[r161+r46|0]<<24>>24|0)){break}r75=r46+1|0;r58=HEAP32[r3];r77=r58+12|0;r47=HEAP32[r77>>2];if((r47|0)==(HEAP32[r58+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r58>>2]+40>>2]](r58);r46=r75;continue}else{HEAP32[r77>>2]=r47+1|0;r46=r75;continue}}HEAP32[r10]=HEAP32[r10]|4;r155=0;r156=r53;r157=r52;break L1394}}while(0);if((r53|0)==(r54|0)){r155=1;r156=r54;r157=r52;break}HEAP32[r26>>2]=0;__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r18,r53,r54,r26);if((HEAP32[r26>>2]|0)==0){r155=1;r156=r53;r157=r52;break}HEAP32[r10]=HEAP32[r10]|4;r155=0;r156=r53;r157=r52}}while(0);if((HEAP8[r33]&1)<<24>>24!=0){__ZdlPv(HEAP32[r34])}if((HEAP8[r31]&1)<<24>>24!=0){__ZdlPv(HEAP32[r35])}if((HEAP8[r29]&1)<<24>>24!=0){__ZdlPv(HEAP32[r38])}if((HEAP8[r20]&1)<<24>>24!=0){__ZdlPv(HEAP32[r41])}if((HEAP8[r19]&1)<<24>>24!=0){__ZdlPv(HEAP32[r18+8>>2])}if((r156|0)==0){STACKTOP=r13;return r155}FUNCTION_TABLE[r157](r156);STACKTOP=r13;return r155}function __ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendIPcEENS_9enable_ifIXsr21__is_forward_iteratorIT_EE5valueERS5_E4typeES9_S9_(r1,r2,r3){var r4,r5,r6,r7,r8,r9,r10,r11,r12;r4=r1;r5=r2;r6=HEAP8[r4];r7=r6&255;if((r7&1|0)==0){r8=r7>>>1}else{r8=HEAP32[r1+4>>2]}if((r6&1)<<24>>24==0){r9=10;r10=r6}else{r6=HEAP32[r1>>2];r9=(r6&-2)-1|0;r10=r6&255}r6=r3-r5|0;if((r3|0)==(r2|0)){return r1}if((r9-r8|0)>>>0<r6>>>0){__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE9__grow_byEjjjjjj(r1,r9,r8+r6-r9|0,r8,r8,0,0);r11=HEAP8[r4]}else{r11=r10}if((r11&1)<<24>>24==0){r12=r1+1|0}else{r12=HEAP32[r1+8>>2]}r11=r3+(r8-r5)|0;r5=r2;r2=r12+r8|0;while(1){HEAP8[r2]=HEAP8[r5];r10=r5+1|0;if((r10|0)==(r3|0)){break}else{r5=r10;r2=r2+1|0}}HEAP8[r12+r11|0]=0;r11=r8+r6|0;if((HEAP8[r4]&1)<<24>>24==0){HEAP8[r4]=r11<<1&255;return r1}else{HEAP32[r1+4>>2]=r11;return r1}}function __ZNSt3__111__money_getIcE13__gather_infoEbRKNS_6localeERNS_10money_base7patternERcS8_RNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEESF_SF_SF_Ri(r1,r2,r3,r4,r5,r6,r7,r8,r9,r10){var r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29;r11=STACKTOP;STACKTOP=STACKTOP+128|0;r12=r11,r13=r12>>2;r14=r11+12,r15=r14>>2;r16=r11+24;r17=r11+28;r18=r11+40;r19=r11+52;r20=r11+64;r21=r11+76;r22=r11+80;r23=r11+92;r24=r11+104;r25=r11+116;if(r1){r1=HEAP32[r2>>2];if((HEAP32[1313850]|0)!=-1){HEAP32[r15]=5255400;HEAP32[r15+1]=24;HEAP32[r15+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5255400,r14,406)}r14=HEAP32[1313851]-1|0;r15=HEAP32[r1+8>>2];if(HEAP32[r1+12>>2]-r15>>2>>>0<=r14>>>0){r26=___cxa_allocate_exception(4);r27=r26;HEAP32[r27>>2]=5247488;___cxa_throw(r26,5252700,514)}r1=HEAP32[r15+(r14<<2)>>2];if((r1|0)==0){r26=___cxa_allocate_exception(4);r27=r26;HEAP32[r27>>2]=5247488;___cxa_throw(r26,5252700,514)}r26=r1;FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+44>>2]](r16,r26);r27=r3;tempBigInt=HEAP32[r16>>2];HEAP8[r27]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r27+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r27+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r27+3|0]=tempBigInt&255;r27=r1>>2;FUNCTION_TABLE[HEAP32[HEAP32[r27]+32>>2]](r17,r26);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r9,r17);if((HEAP8[r17]&1)<<24>>24!=0){__ZdlPv(HEAP32[r17+8>>2])}FUNCTION_TABLE[HEAP32[HEAP32[r27]+28>>2]](r18,r26);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r8,r18);if((HEAP8[r18]&1)<<24>>24!=0){__ZdlPv(HEAP32[r18+8>>2])}r18=r1;HEAP8[r4]=FUNCTION_TABLE[HEAP32[HEAP32[r18>>2]+12>>2]](r26);HEAP8[r5]=FUNCTION_TABLE[HEAP32[HEAP32[r18>>2]+16>>2]](r26);FUNCTION_TABLE[HEAP32[HEAP32[r27]+20>>2]](r19,r26);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r6,r19);if((HEAP8[r19]&1)<<24>>24!=0){__ZdlPv(HEAP32[r19+8>>2])}FUNCTION_TABLE[HEAP32[HEAP32[r27]+24>>2]](r20,r26);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r7,r20);if((HEAP8[r20]&1)<<24>>24!=0){__ZdlPv(HEAP32[r20+8>>2])}r20=FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+36>>2]](r26);HEAP32[r10>>2]=r20;STACKTOP=r11;return}else{r26=HEAP32[r2>>2];if((HEAP32[1313852]|0)!=-1){HEAP32[r13]=5255408;HEAP32[r13+1]=24;HEAP32[r13+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5255408,r12,406)}r12=HEAP32[1313853]-1|0;r13=HEAP32[r26+8>>2];if(HEAP32[r26+12>>2]-r13>>2>>>0<=r12>>>0){r28=___cxa_allocate_exception(4);r29=r28;HEAP32[r29>>2]=5247488;___cxa_throw(r28,5252700,514)}r26=HEAP32[r13+(r12<<2)>>2];if((r26|0)==0){r28=___cxa_allocate_exception(4);r29=r28;HEAP32[r29>>2]=5247488;___cxa_throw(r28,5252700,514)}r28=r26;FUNCTION_TABLE[HEAP32[HEAP32[r26>>2]+44>>2]](r21,r28);r29=r3;tempBigInt=HEAP32[r21>>2];HEAP8[r29]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r29+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r29+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r29+3|0]=tempBigInt&255;r29=r26>>2;FUNCTION_TABLE[HEAP32[HEAP32[r29]+32>>2]](r22,r28);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r9,r22);if((HEAP8[r22]&1)<<24>>24!=0){__ZdlPv(HEAP32[r22+8>>2])}FUNCTION_TABLE[HEAP32[HEAP32[r29]+28>>2]](r23,r28);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r8,r23);if((HEAP8[r23]&1)<<24>>24!=0){__ZdlPv(HEAP32[r23+8>>2])}r23=r26;HEAP8[r4]=FUNCTION_TABLE[HEAP32[HEAP32[r23>>2]+12>>2]](r28);HEAP8[r5]=FUNCTION_TABLE[HEAP32[HEAP32[r23>>2]+16>>2]](r28);FUNCTION_TABLE[HEAP32[HEAP32[r29]+20>>2]](r24,r28);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r6,r24);if((HEAP8[r24]&1)<<24>>24!=0){__ZdlPv(HEAP32[r24+8>>2])}FUNCTION_TABLE[HEAP32[HEAP32[r29]+24>>2]](r25,r28);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r7,r25);if((HEAP8[r25]&1)<<24>>24!=0){__ZdlPv(HEAP32[r25+8>>2])}r20=FUNCTION_TABLE[HEAP32[HEAP32[r26>>2]+36>>2]](r28);HEAP32[r10>>2]=r20;STACKTOP=r11;return}}function __ZNSt3__19money_getIwNS_19istreambuf_iteratorIwNS_11char_traitsIwEEEEE8__do_getERS4_S4_bRKNS_6localeEjRjRbRKNS_5ctypeIwEERNS_10unique_ptrIwPFvPvEEERPwSM_(r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11){var r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48,r49,r50,r51,r52,r53,r54,r55,r56,r57,r58,r59,r60,r61,r62,r63,r64,r65,r66,r67,r68,r69,r70,r71,r72,r73,r74,r75,r76,r77,r78,r79,r80,r81,r82,r83,r84,r85,r86,r87,r88,r89,r90,r91,r92,r93,r94,r95,r96,r97,r98,r99,r100,r101,r102,r103,r104,r105,r106,r107,r108,r109,r110,r111,r112,r113,r114,r115,r116,r117,r118,r119,r120,r121,r122,r123,r124,r125,r126,r127,r128,r129,r130,r131,r132,r133,r134,r135,r136,r137,r138,r139,r140,r141,r142,r143,r144,r145,r146,r147,r148,r149,r150,r151,r152,r153,r154,r155,r156,r157,r158,r159,r160,r161,r162,r163;r12=r10>>2;r13=r6>>2;r6=0;r14=STACKTOP;STACKTOP=STACKTOP+428|0;r15=r2;r2=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r2>>2]=HEAP32[r15>>2];r15=r14,r16=r15>>2;r17=r14+4;r18=r14+404;r19=r14+408;r20=r14+412;r21=r14+416;r22=r21,r23=r22>>2;r24=STACKTOP;STACKTOP=STACKTOP+12|0;r25=STACKTOP;STACKTOP=STACKTOP+12|0;r26=STACKTOP;STACKTOP=STACKTOP+12|0;r27=STACKTOP;STACKTOP=STACKTOP+12|0;r28=STACKTOP;STACKTOP=STACKTOP+4|0;r29=STACKTOP;STACKTOP=STACKTOP+4|0;HEAP32[r16]=r11;r11=r17|0;HEAP32[r23]=0;HEAP32[r23+1]=0;HEAP32[r23+2]=0;r23=r24,r30=r23>>2;r31=r25,r32=r31>>2;r33=r26,r34=r33>>2;r35=r27,r36=r35>>2;HEAP32[r30]=0;HEAP32[r30+1]=0;HEAP32[r30+2]=0;HEAP32[r32]=0;HEAP32[r32+1]=0;HEAP32[r32+2]=0;HEAP32[r34]=0;HEAP32[r34+1]=0;HEAP32[r34+2]=0;HEAP32[r36]=0;HEAP32[r36+1]=0;HEAP32[r36+2]=0;__ZNSt3__111__money_getIwE13__gather_infoEbRKNS_6localeERNS_10money_base7patternERwS8_RNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEERNS9_IwNSA_IwEENSC_IwEEEESJ_SJ_Ri(r3,r4,r18,r19,r20,r21,r24,r25,r26,r28);r4=r9|0;HEAP32[r12]=HEAP32[r4>>2];r3=(r1|0)>>2;r1=(r2|0)>>2;r2=r8>>2;r36=r27+4|0,r34=r36>>2;r32=(r27+8|0)>>2;r30=r27|0;r37=r26+4|0,r38=r37>>2;r39=(r26+8|0)>>2;r40=r25+4|0,r41=r40>>2;r42=(r25+8|0)>>2;r43=(r5&512|0)!=0;r5=r24+4|0,r44=r5>>2;r45=(r24+8|0)>>2;r24=r18+3|0;r46=r21+4|0;r47=414;r48=r11;r49=r11;r11=r17+400|0;r17=0;r50=0;L1562:while(1){r51=HEAP32[r3],r52=r51>>2;do{if((r51|0)==0){r53=1}else{r54=HEAP32[r52+3];if((r54|0)==(HEAP32[r52+4]|0)){r55=FUNCTION_TABLE[HEAP32[HEAP32[r52]+36>>2]](r51)}else{r55=HEAP32[r54>>2]}if((r55|0)==-1){HEAP32[r3]=0;r53=1;break}else{r53=(HEAP32[r3]|0)==0;break}}}while(0);r51=HEAP32[r1],r52=r51>>2;do{if((r51|0)==0){r6=1272}else{r54=HEAP32[r52+3];if((r54|0)==(HEAP32[r52+4]|0)){r56=FUNCTION_TABLE[HEAP32[HEAP32[r52]+36>>2]](r51)}else{r56=HEAP32[r54>>2]}if((r56|0)==-1){HEAP32[r1]=0;r6=1272;break}else{if(r53^(r51|0)==0){r57=r51;break}else{r58=r47;r59=r48;r60=r49;r61=r17;r6=1527;break L1562}}}}while(0);if(r6==1272){r6=0;if(r53){r58=r47;r59=r48;r60=r49;r61=r17;r6=1527;break}else{r57=0}}r51=HEAP8[r18+r50|0]<<24>>24;L1586:do{if((r51|0)==3){r52=HEAP8[r31];r54=r52&255;r62=(r54&1|0)==0;r63=HEAP8[r33];r64=r63&255;r65=(r64&1|0)==0;if(((r62?r54>>>1:HEAP32[r41])|0)==(-(r65?r64>>>1:HEAP32[r38])|0)){r66=r17;r67=r11;r68=r49;r69=r48;r70=r47;break}do{if(((r62?r54>>>1:HEAP32[r41])|0)!=0){if(((r65?r64>>>1:HEAP32[r38])|0)==0){break}r71=HEAP32[r3],r72=r71>>2;r73=HEAP32[r72+3];if((r73|0)==(HEAP32[r72+4]|0)){r74=FUNCTION_TABLE[HEAP32[HEAP32[r72]+36>>2]](r71);r75=HEAP8[r31]}else{r74=HEAP32[r73>>2];r75=r52}r73=HEAP32[r3],r71=r73>>2;r72=r73+12|0;r76=HEAP32[r72>>2];r77=(r76|0)==(HEAP32[r71+4]|0);if((r74|0)==(HEAP32[((r75&1)<<24>>24==0?r40:HEAP32[r42])>>2]|0)){if(r77){FUNCTION_TABLE[HEAP32[HEAP32[r71]+40>>2]](r73)}else{HEAP32[r72>>2]=r76+4|0}r72=HEAPU8[r31];r66=((r72&1|0)==0?r72>>>1:HEAP32[r41])>>>0>1?r25:r17;r67=r11;r68=r49;r69=r48;r70=r47;break L1586}if(r77){r78=FUNCTION_TABLE[HEAP32[HEAP32[r71]+36>>2]](r73)}else{r78=HEAP32[r76>>2]}if((r78|0)!=(HEAP32[((HEAP8[r33]&1)<<24>>24==0?r37:HEAP32[r39])>>2]|0)){r6=1375;break L1562}r76=HEAP32[r3];r73=r76+12|0;r71=HEAP32[r73>>2];if((r71|0)==(HEAP32[r76+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r76>>2]+40>>2]](r76)}else{HEAP32[r73>>2]=r71+4|0}HEAP8[r7]=1;r71=HEAPU8[r33];r66=((r71&1|0)==0?r71>>>1:HEAP32[r38])>>>0>1?r26:r17;r67=r11;r68=r49;r69=r48;r70=r47;break L1586}}while(0);r64=HEAP32[r3],r65=r64>>2;r71=HEAP32[r65+3];r73=(r71|0)==(HEAP32[r65+4]|0);if(((r62?r54>>>1:HEAP32[r41])|0)==0){if(r73){r79=FUNCTION_TABLE[HEAP32[HEAP32[r65]+36>>2]](r64);r80=HEAP8[r33]}else{r79=HEAP32[r71>>2];r80=r63}if((r79|0)!=(HEAP32[((r80&1)<<24>>24==0?r37:HEAP32[r39])>>2]|0)){r66=r17;r67=r11;r68=r49;r69=r48;r70=r47;break}r76=HEAP32[r3];r77=r76+12|0;r72=HEAP32[r77>>2];if((r72|0)==(HEAP32[r76+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r76>>2]+40>>2]](r76)}else{HEAP32[r77>>2]=r72+4|0}HEAP8[r7]=1;r72=HEAPU8[r33];r66=((r72&1|0)==0?r72>>>1:HEAP32[r38])>>>0>1?r26:r17;r67=r11;r68=r49;r69=r48;r70=r47;break}if(r73){r81=FUNCTION_TABLE[HEAP32[HEAP32[r65]+36>>2]](r64);r82=HEAP8[r31]}else{r81=HEAP32[r71>>2];r82=r52}if((r81|0)!=(HEAP32[((r82&1)<<24>>24==0?r40:HEAP32[r42])>>2]|0)){HEAP8[r7]=1;r66=r17;r67=r11;r68=r49;r69=r48;r70=r47;break}r71=HEAP32[r3];r64=r71+12|0;r65=HEAP32[r64>>2];if((r65|0)==(HEAP32[r71+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r71>>2]+40>>2]](r71)}else{HEAP32[r64>>2]=r65+4|0}r65=HEAPU8[r31];r66=((r65&1|0)==0?r65>>>1:HEAP32[r41])>>>0>1?r25:r17;r67=r11;r68=r49;r69=r48;r70=r47;break}else if((r51|0)==1){if((r50|0)==3){r58=r47;r59=r48;r60=r49;r61=r17;r6=1527;break L1562}r65=HEAP32[r3],r64=r65>>2;r71=HEAP32[r64+3];if((r71|0)==(HEAP32[r64+4]|0)){r83=FUNCTION_TABLE[HEAP32[HEAP32[r64]+36>>2]](r65)}else{r83=HEAP32[r71>>2]}if(!FUNCTION_TABLE[HEAP32[HEAP32[r2]+12>>2]](r8,8192,r83)){r6=1302;break L1562}r71=HEAP32[r3];r65=r71+12|0;r64=HEAP32[r65>>2];if((r64|0)==(HEAP32[r71+16>>2]|0)){r84=FUNCTION_TABLE[HEAP32[HEAP32[r71>>2]+40>>2]](r71)}else{HEAP32[r65>>2]=r64+4|0;r84=HEAP32[r64>>2]}r64=HEAP8[r35];if((r64&1)<<24>>24==0){r85=1;r86=r64}else{r64=HEAP32[r30>>2];r85=(r64&-2)-1|0;r86=r64&255}r64=r86&255;r65=(r64&1|0)==0?r64>>>1:HEAP32[r34];if((r65|0)==(r85|0)){__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE9__grow_byEjjjjjj(r27,r85,1,r85,r85,0,0);r87=HEAP8[r35]}else{r87=r86}r64=(r87&1)<<24>>24==0?r36:HEAP32[r32];HEAP32[r64+(r65<<2)>>2]=r84;r71=r65+1|0;HEAP32[r64+(r71<<2)>>2]=0;if((HEAP8[r35]&1)<<24>>24==0){HEAP8[r35]=r71<<1&255;r6=1303;break}else{HEAP32[r34]=r71;r6=1303;break}}else if((r51|0)==0){r6=1303}else if((r51|0)==2){if(!((r17|0)!=0|r50>>>0<2)){if((r50|0)==2){r88=HEAP8[r24]<<24>>24!=0}else{r88=0}if(!(r43|r88)){r66=0;r67=r11;r68=r49;r69=r48;r70=r47;break}}r71=HEAP8[r23];r64=(r71&1)<<24>>24==0?r5:HEAP32[r45];L1668:do{if((r50|0)==0){r89=r64;r90=r71;r91=r57,r92=r91>>2}else{if(HEAPU8[r18+(r50-1)|0]<2){r93=r64;r94=r71}else{r89=r64;r90=r71;r91=r57,r92=r91>>2;break}while(1){r65=r94&255;if((r93|0)==((((r65&1|0)==0?r65>>>1:HEAP32[r44])<<2)+((r94&1)<<24>>24==0?r5:HEAP32[r45])|0)){r95=r94;break}if(!FUNCTION_TABLE[HEAP32[HEAP32[r2]+12>>2]](r8,8192,HEAP32[r93>>2])){r6=1386;break}r93=r93+4|0;r94=HEAP8[r23]}if(r6==1386){r6=0;r95=HEAP8[r23]}r65=(r95&1)<<24>>24==0;r73=r93-(r65?r5:HEAP32[r45])>>2;r72=HEAP8[r35];r77=r72&255;r76=(r77&1|0)==0;L1678:do{if(r73>>>0<=(r76?r77>>>1:HEAP32[r34])>>>0){r96=(r72&1)<<24>>24==0;r97=((r76?r77>>>1:HEAP32[r34])-r73<<2)+(r96?r36:HEAP32[r32])|0;r98=((r76?r77>>>1:HEAP32[r34])<<2)+(r96?r36:HEAP32[r32])|0;if((r97|0)==(r98|0)){r89=r93;r90=r95;r91=r57,r92=r91>>2;break L1668}else{r99=r97;r100=r65?r5:HEAP32[r45]}while(1){if((HEAP32[r99>>2]|0)!=(HEAP32[r100>>2]|0)){break L1678}r97=r99+4|0;if((r97|0)==(r98|0)){r89=r93;r90=r95;r91=r57,r92=r91>>2;break L1668}r99=r97;r100=r100+4|0}}}while(0);r89=r65?r5:HEAP32[r45];r90=r95;r91=r57,r92=r91>>2;break}}while(0);L1685:while(1){r71=r90&255;if((r89|0)==((((r71&1|0)==0?r71>>>1:HEAP32[r44])<<2)+((r90&1)<<24>>24==0?r5:HEAP32[r45])|0)){break}r71=HEAP32[r3],r64=r71>>2;do{if((r71|0)==0){r101=1}else{r52=HEAP32[r64+3];if((r52|0)==(HEAP32[r64+4]|0)){r102=FUNCTION_TABLE[HEAP32[HEAP32[r64]+36>>2]](r71)}else{r102=HEAP32[r52>>2]}if((r102|0)==-1){HEAP32[r3]=0;r101=1;break}else{r101=(HEAP32[r3]|0)==0;break}}}while(0);do{if((r91|0)==0){r6=1407}else{r71=HEAP32[r92+3];if((r71|0)==(HEAP32[r92+4]|0)){r103=FUNCTION_TABLE[HEAP32[HEAP32[r92]+36>>2]](r91)}else{r103=HEAP32[r71>>2]}if((r103|0)==-1){HEAP32[r1]=0;r6=1407;break}else{if(r101^(r91|0)==0){r104=r91;break}else{break L1685}}}}while(0);if(r6==1407){r6=0;if(r101){break}else{r104=0}}r71=HEAP32[r3],r64=r71>>2;r65=HEAP32[r64+3];if((r65|0)==(HEAP32[r64+4]|0)){r105=FUNCTION_TABLE[HEAP32[HEAP32[r64]+36>>2]](r71)}else{r105=HEAP32[r65>>2]}if((r105|0)!=(HEAP32[r89>>2]|0)){break}r65=HEAP32[r3];r71=r65+12|0;r64=HEAP32[r71>>2];if((r64|0)==(HEAP32[r65+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r65>>2]+40>>2]](r65)}else{HEAP32[r71>>2]=r64+4|0}r89=r89+4|0;r90=HEAP8[r23];r91=r104,r92=r91>>2}if(!r43){r66=r17;r67=r11;r68=r49;r69=r48;r70=r47;break}r64=HEAP8[r23];r71=r64&255;if((r89|0)==((((r71&1|0)==0?r71>>>1:HEAP32[r44])<<2)+((r64&1)<<24>>24==0?r5:HEAP32[r45])|0)){r66=r17;r67=r11;r68=r49;r69=r48;r70=r47;break}else{r6=1419;break L1562}}else if((r51|0)==4){r64=0;r71=r11;r65=r49;r52=r48;r63=r47;L1721:while(1){r54=HEAP32[r3],r62=r54>>2;do{if((r54|0)==0){r106=1}else{r77=HEAP32[r62+3];if((r77|0)==(HEAP32[r62+4]|0)){r107=FUNCTION_TABLE[HEAP32[HEAP32[r62]+36>>2]](r54)}else{r107=HEAP32[r77>>2]}if((r107|0)==-1){HEAP32[r3]=0;r106=1;break}else{r106=(HEAP32[r3]|0)==0;break}}}while(0);r54=HEAP32[r1],r62=r54>>2;do{if((r54|0)==0){r6=1433}else{r77=HEAP32[r62+3];if((r77|0)==(HEAP32[r62+4]|0)){r108=FUNCTION_TABLE[HEAP32[HEAP32[r62]+36>>2]](r54)}else{r108=HEAP32[r77>>2]}if((r108|0)==-1){HEAP32[r1]=0;r6=1433;break}else{if(r106^(r54|0)==0){break}else{break L1721}}}}while(0);if(r6==1433){r6=0;if(r106){break}}r54=HEAP32[r3],r62=r54>>2;r77=HEAP32[r62+3];if((r77|0)==(HEAP32[r62+4]|0)){r109=FUNCTION_TABLE[HEAP32[HEAP32[r62]+36>>2]](r54)}else{r109=HEAP32[r77>>2]}if(FUNCTION_TABLE[HEAP32[HEAP32[r2]+12>>2]](r8,2048,r109)){r77=HEAP32[r12];if((r77|0)==(HEAP32[r16]|0)){__ZNSt3__119__double_or_nothingIwEEvRNS_10unique_ptrIT_PFvPvEEERPS2_S9_(r9,r10,r15);r110=HEAP32[r12]}else{r110=r77}HEAP32[r12]=r110+4|0;HEAP32[r110>>2]=r109;r111=r64+1|0;r112=r71;r113=r65;r114=r52;r115=r63}else{r77=HEAPU8[r22];if((((r77&1|0)==0?r77>>>1:HEAP32[r46>>2])|0)==0|(r64|0)==0){break}if((r109|0)!=(HEAP32[r20>>2]|0)){break}if((r65|0)==(r71|0)){r77=(r63|0)!=414;r54=r65-r52|0;r62=r54>>>0<2147483647?r54<<1:-1;if(r77){r116=r52}else{r116=0}r77=_realloc(r116,r62);r76=r77;if((r77|0)==0){r6=1450;break L1562}r117=(r62>>>2<<2)+r76|0;r118=(r54>>2<<2)+r76|0;r119=r76;r120=218}else{r117=r71;r118=r65;r119=r52;r120=r63}HEAP32[r118>>2]=r64;r111=0;r112=r117;r113=r118+4|0;r114=r119;r115=r120}r76=HEAP32[r3];r54=r76+12|0;r62=HEAP32[r54>>2];if((r62|0)==(HEAP32[r76+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r76>>2]+40>>2]](r76);r64=r111;r71=r112;r65=r113;r52=r114;r63=r115;continue}else{HEAP32[r54>>2]=r62+4|0;r64=r111;r71=r112;r65=r113;r52=r114;r63=r115;continue}}if((r52|0)==(r65|0)|(r64|0)==0){r121=r71;r122=r65;r123=r52;r124=r63}else{if((r65|0)==(r71|0)){r62=(r63|0)!=414;r54=r65-r52|0;r76=r54>>>0<2147483647?r54<<1:-1;if(r62){r125=r52}else{r125=0}r62=_realloc(r125,r76);r77=r62;if((r62|0)==0){r6=1464;break L1562}r126=(r76>>>2<<2)+r77|0;r127=(r54>>2<<2)+r77|0;r128=r77;r129=218}else{r126=r71;r127=r65;r128=r52;r129=r63}HEAP32[r127>>2]=r64;r121=r126;r122=r127+4|0;r123=r128;r124=r129}r77=HEAP32[r28>>2];L1787:do{if((r77|0)>0){r54=HEAP32[r3],r76=r54>>2;do{if((r54|0)==0){r130=1}else{r62=HEAP32[r76+3];if((r62|0)==(HEAP32[r76+4]|0)){r131=FUNCTION_TABLE[HEAP32[HEAP32[r76]+36>>2]](r54)}else{r131=HEAP32[r62>>2]}if((r131|0)==-1){HEAP32[r3]=0;r130=1;break}else{r130=(HEAP32[r3]|0)==0;break}}}while(0);r54=HEAP32[r1],r76=r54>>2;do{if((r54|0)==0){r6=1484}else{r62=HEAP32[r76+3];if((r62|0)==(HEAP32[r76+4]|0)){r132=FUNCTION_TABLE[HEAP32[HEAP32[r76]+36>>2]](r54)}else{r132=HEAP32[r62>>2]}if((r132|0)==-1){HEAP32[r1]=0;r6=1484;break}else{if(r130^(r54|0)==0){r133=r54;break}else{r6=1490;break L1562}}}}while(0);if(r6==1484){r6=0;if(r130){r6=1490;break L1562}else{r133=0}}r54=HEAP32[r3],r76=r54>>2;r62=HEAP32[r76+3];if((r62|0)==(HEAP32[r76+4]|0)){r134=FUNCTION_TABLE[HEAP32[HEAP32[r76]+36>>2]](r54)}else{r134=HEAP32[r62>>2]}if((r134|0)!=(HEAP32[r19>>2]|0)){r6=1490;break L1562}r62=HEAP32[r3];r54=r62+12|0;r76=HEAP32[r54>>2];do{if((r76|0)==(HEAP32[r62+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r62>>2]+40>>2]](r62);r135=r133,r136=r135>>2;r137=r77;break}else{HEAP32[r54>>2]=r76+4|0;r135=r133,r136=r135>>2;r137=r77;break}}while(0);while(1){r76=HEAP32[r3],r54=r76>>2;do{if((r76|0)==0){r138=1}else{r62=HEAP32[r54+3];if((r62|0)==(HEAP32[r54+4]|0)){r139=FUNCTION_TABLE[HEAP32[HEAP32[r54]+36>>2]](r76)}else{r139=HEAP32[r62>>2]}if((r139|0)==-1){HEAP32[r3]=0;r138=1;break}else{r138=(HEAP32[r3]|0)==0;break}}}while(0);do{if((r135|0)==0){r6=1507}else{r76=HEAP32[r136+3];if((r76|0)==(HEAP32[r136+4]|0)){r140=FUNCTION_TABLE[HEAP32[HEAP32[r136]+36>>2]](r135)}else{r140=HEAP32[r76>>2]}if((r140|0)==-1){HEAP32[r1]=0;r6=1507;break}else{if(r138^(r135|0)==0){r141=r135;break}else{r6=1514;break L1562}}}}while(0);if(r6==1507){r6=0;if(r138){r6=1514;break L1562}else{r141=0}}r76=HEAP32[r3],r54=r76>>2;r62=HEAP32[r54+3];if((r62|0)==(HEAP32[r54+4]|0)){r142=FUNCTION_TABLE[HEAP32[HEAP32[r54]+36>>2]](r76)}else{r142=HEAP32[r62>>2]}if(!FUNCTION_TABLE[HEAP32[HEAP32[r2]+12>>2]](r8,2048,r142)){r6=1514;break L1562}if((HEAP32[r12]|0)==(HEAP32[r16]|0)){__ZNSt3__119__double_or_nothingIwEEvRNS_10unique_ptrIT_PFvPvEEERPS2_S9_(r9,r10,r15)}r62=HEAP32[r3],r76=r62>>2;r54=HEAP32[r76+3];if((r54|0)==(HEAP32[r76+4]|0)){r143=FUNCTION_TABLE[HEAP32[HEAP32[r76]+36>>2]](r62)}else{r143=HEAP32[r54>>2]}r54=HEAP32[r12];HEAP32[r12]=r54+4|0;HEAP32[r54>>2]=r143;r54=r137-1|0;HEAP32[r28>>2]=r54;r62=HEAP32[r3];r76=r62+12|0;r73=HEAP32[r76>>2];if((r73|0)==(HEAP32[r62+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r62>>2]+40>>2]](r62)}else{HEAP32[r76>>2]=r73+4|0}if((r54|0)>0){r135=r141,r136=r135>>2;r137=r54}else{break L1787}}}}while(0);if((HEAP32[r12]|0)==(HEAP32[r4>>2]|0)){r6=1525;break L1562}else{r66=r17;r67=r121;r68=r122;r69=r123;r70=r124;break}}else{r66=r17;r67=r11;r68=r49;r69=r48;r70=r47}}while(0);L1861:do{if(r6==1303){r6=0;if((r50|0)==3){r58=r47;r59=r48;r60=r49;r61=r17;r6=1527;break L1562}else{r144=r57,r145=r144>>2}while(1){r51=HEAP32[r3],r77=r51>>2;do{if((r51|0)==0){r146=1}else{r64=HEAP32[r77+3];if((r64|0)==(HEAP32[r77+4]|0)){r147=FUNCTION_TABLE[HEAP32[HEAP32[r77]+36>>2]](r51)}else{r147=HEAP32[r64>>2]}if((r147|0)==-1){HEAP32[r3]=0;r146=1;break}else{r146=(HEAP32[r3]|0)==0;break}}}while(0);do{if((r144|0)==0){r6=1317}else{r51=HEAP32[r145+3];if((r51|0)==(HEAP32[r145+4]|0)){r148=FUNCTION_TABLE[HEAP32[HEAP32[r145]+36>>2]](r144)}else{r148=HEAP32[r51>>2]}if((r148|0)==-1){HEAP32[r1]=0;r6=1317;break}else{if(r146^(r144|0)==0){r149=r144;break}else{r66=r17;r67=r11;r68=r49;r69=r48;r70=r47;break L1861}}}}while(0);if(r6==1317){r6=0;if(r146){r66=r17;r67=r11;r68=r49;r69=r48;r70=r47;break L1861}else{r149=0}}r51=HEAP32[r3],r77=r51>>2;r64=HEAP32[r77+3];if((r64|0)==(HEAP32[r77+4]|0)){r150=FUNCTION_TABLE[HEAP32[HEAP32[r77]+36>>2]](r51)}else{r150=HEAP32[r64>>2]}if(!FUNCTION_TABLE[HEAP32[HEAP32[r2]+12>>2]](r8,8192,r150)){r66=r17;r67=r11;r68=r49;r69=r48;r70=r47;break L1861}r64=HEAP32[r3];r51=r64+12|0;r77=HEAP32[r51>>2];if((r77|0)==(HEAP32[r64+16>>2]|0)){r151=FUNCTION_TABLE[HEAP32[HEAP32[r64>>2]+40>>2]](r64)}else{HEAP32[r51>>2]=r77+4|0;r151=HEAP32[r77>>2]}r77=HEAP8[r35];if((r77&1)<<24>>24==0){r152=1;r153=r77}else{r77=HEAP32[r30>>2];r152=(r77&-2)-1|0;r153=r77&255}r77=r153&255;r51=(r77&1|0)==0?r77>>>1:HEAP32[r34];if((r51|0)==(r152|0)){__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE9__grow_byEjjjjjj(r27,r152,1,r152,r152,0,0);r154=HEAP8[r35]}else{r154=r153}r77=(r154&1)<<24>>24==0?r36:HEAP32[r32];HEAP32[r77+(r51<<2)>>2]=r151;r64=r51+1|0;HEAP32[r77+(r64<<2)>>2]=0;if((HEAP8[r35]&1)<<24>>24==0){HEAP8[r35]=r64<<1&255;r144=r149,r145=r144>>2;continue}else{HEAP32[r34]=r64;r144=r149,r145=r144>>2;continue}}}}while(0);r64=r50+1|0;if(r64>>>0<4){r47=r70;r48=r69;r49=r68;r11=r67;r17=r66;r50=r64}else{r58=r70;r59=r69;r60=r68;r61=r66;r6=1527;break}}L1908:do{if(r6==1302){HEAP32[r13]=HEAP32[r13]|4;r155=0;r156=r48;r157=r47}else if(r6==1375){HEAP32[r13]=HEAP32[r13]|4;r155=0;r156=r48;r157=r47}else if(r6==1419){HEAP32[r13]=HEAP32[r13]|4;r155=0;r156=r48;r157=r47}else if(r6==1450){__ZSt17__throw_bad_allocv()}else if(r6==1464){__ZSt17__throw_bad_allocv()}else if(r6==1490){HEAP32[r13]=HEAP32[r13]|4;r155=0;r156=r123;r157=r124}else if(r6==1514){HEAP32[r13]=HEAP32[r13]|4;r155=0;r156=r123;r157=r124}else if(r6==1525){HEAP32[r13]=HEAP32[r13]|4;r155=0;r156=r123;r157=r124}else if(r6==1527){L1920:do{if((r61|0)!=0){r66=r61;r68=r61+4|0;r69=r61+8|0;r70=1;L1922:while(1){r50=HEAPU8[r66];if((r50&1|0)==0){r158=r50>>>1}else{r158=HEAP32[r68>>2]}if(r70>>>0>=r158>>>0){break L1920}r50=HEAP32[r3],r17=r50>>2;do{if((r50|0)==0){r159=1}else{r67=HEAP32[r17+3];if((r67|0)==(HEAP32[r17+4]|0)){r160=FUNCTION_TABLE[HEAP32[HEAP32[r17]+36>>2]](r50)}else{r160=HEAP32[r67>>2]}if((r160|0)==-1){HEAP32[r3]=0;r159=1;break}else{r159=(HEAP32[r3]|0)==0;break}}}while(0);r50=HEAP32[r1],r17=r50>>2;do{if((r50|0)==0){r6=1546}else{r67=HEAP32[r17+3];if((r67|0)==(HEAP32[r17+4]|0)){r161=FUNCTION_TABLE[HEAP32[HEAP32[r17]+36>>2]](r50)}else{r161=HEAP32[r67>>2]}if((r161|0)==-1){HEAP32[r1]=0;r6=1546;break}else{if(r159^(r50|0)==0){break}else{break L1922}}}}while(0);if(r6==1546){r6=0;if(r159){break}}r50=HEAP32[r3],r17=r50>>2;r67=HEAP32[r17+3];if((r67|0)==(HEAP32[r17+4]|0)){r162=FUNCTION_TABLE[HEAP32[HEAP32[r17]+36>>2]](r50)}else{r162=HEAP32[r67>>2]}if((HEAP8[r66]&1)<<24>>24==0){r163=r68}else{r163=HEAP32[r69>>2]}if((r162|0)!=(HEAP32[r163+(r70<<2)>>2]|0)){break}r67=r70+1|0;r50=HEAP32[r3];r17=r50+12|0;r11=HEAP32[r17>>2];if((r11|0)==(HEAP32[r50+16>>2]|0)){FUNCTION_TABLE[HEAP32[HEAP32[r50>>2]+40>>2]](r50);r70=r67;continue}else{HEAP32[r17>>2]=r11+4|0;r70=r67;continue}}HEAP32[r13]=HEAP32[r13]|4;r155=0;r156=r59;r157=r58;break L1908}}while(0);if((r59|0)==(r60|0)){r155=1;r156=r60;r157=r58;break}HEAP32[r29>>2]=0;__ZNSt3__116__check_groupingERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPjS8_Rj(r21,r59,r60,r29);if((HEAP32[r29>>2]|0)==0){r155=1;r156=r59;r157=r58;break}HEAP32[r13]=HEAP32[r13]|4;r155=0;r156=r59;r157=r58}}while(0);if((HEAP8[r35]&1)<<24>>24!=0){__ZdlPv(HEAP32[r32])}if((HEAP8[r33]&1)<<24>>24!=0){__ZdlPv(HEAP32[r39])}if((HEAP8[r31]&1)<<24>>24!=0){__ZdlPv(HEAP32[r42])}if((HEAP8[r23]&1)<<24>>24!=0){__ZdlPv(HEAP32[r45])}if((HEAP8[r22]&1)<<24>>24!=0){__ZdlPv(HEAP32[r21+8>>2])}if((r156|0)==0){STACKTOP=r14;return r155}FUNCTION_TABLE[r157](r156);STACKTOP=r14;return r155}function __ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6appendIPwEENS_9enable_ifIXsr21__is_forward_iteratorIT_EE5valueERS5_E4typeES9_S9_(r1,r2,r3){var r4,r5,r6,r7,r8,r9,r10,r11,r12,r13;r4=r1;r5=r2;r6=HEAP8[r4];r7=r6&255;if((r7&1|0)==0){r8=r7>>>1}else{r8=HEAP32[r1+4>>2]}if((r6&1)<<24>>24==0){r9=1;r10=r6}else{r6=HEAP32[r1>>2];r9=(r6&-2)-1|0;r10=r6&255}r6=r3-r5>>2;if((r6|0)==0){return r1}if((r9-r8|0)>>>0<r6>>>0){__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE9__grow_byEjjjjjj(r1,r9,r8+r6-r9|0,r8,r8,0,0);r11=HEAP8[r4]}else{r11=r10}if((r11&1)<<24>>24==0){r12=r1+4|0}else{r12=HEAP32[r1+8>>2]}r11=(r8<<2)+r12|0;if((r2|0)==(r3|0)){r13=r11}else{r10=r8+((r3-4+ -r5|0)>>>2)+1|0;r5=r2;r2=r11;while(1){HEAP32[r2>>2]=HEAP32[r5>>2];r11=r5+4|0;if((r11|0)==(r3|0)){break}else{r5=r11;r2=r2+4|0}}r13=(r10<<2)+r12|0}HEAP32[r13>>2]=0;r13=r8+r6|0;if((HEAP8[r4]&1)<<24>>24==0){HEAP8[r4]=r13<<1&255;return r1}else{HEAP32[r1+4>>2]=r13;return r1}}function __ZNSt3__111__money_getIwE13__gather_infoEbRKNS_6localeERNS_10money_base7patternERwS8_RNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEERNS9_IwNSA_IwEENSC_IwEEEESJ_SJ_Ri(r1,r2,r3,r4,r5,r6,r7,r8,r9,r10){var r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47;r11=STACKTOP;STACKTOP=STACKTOP+128|0;r12=r11,r13=r12>>2;r14=r11+12,r15=r14>>2;r16=r11+24;r17=r11+28,r18=r17>>2;r19=r11+40,r20=r19>>2;r21=r11+52;r22=r11+64,r23=r22>>2;r24=r11+76;r25=r11+80,r26=r25>>2;r27=r11+92,r28=r27>>2;r29=r11+104;r30=r11+116,r31=r30>>2;if(r1){r1=HEAP32[r2>>2];if((HEAP32[1313846]|0)!=-1){HEAP32[r15]=5255384;HEAP32[r15+1]=24;HEAP32[r15+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5255384,r14,406)}r14=HEAP32[1313847]-1|0;r15=HEAP32[r1+8>>2];if(HEAP32[r1+12>>2]-r15>>2>>>0<=r14>>>0){r32=___cxa_allocate_exception(4);r33=r32;HEAP32[r33>>2]=5247488;___cxa_throw(r32,5252700,514)}r1=HEAP32[r15+(r14<<2)>>2];if((r1|0)==0){r32=___cxa_allocate_exception(4);r33=r32;HEAP32[r33>>2]=5247488;___cxa_throw(r32,5252700,514)}r32=r1;FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+44>>2]](r16,r32);r33=r3;tempBigInt=HEAP32[r16>>2];HEAP8[r33]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r33+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r33+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r33+3|0]=tempBigInt&255;r33=r1>>2;FUNCTION_TABLE[HEAP32[HEAP32[r33]+32>>2]](r17,r32);r16=r17;r14=HEAP8[r16];if((r14&1)<<24>>24==0){r34=r17+4|0}else{r34=HEAP32[r18+2]}r17=r14&255;if((r17&1|0)==0){r35=r17>>>1}else{r35=HEAP32[r18+1]}__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(r9,r34,r35);if((HEAP8[r16]&1)<<24>>24!=0){__ZdlPv(HEAP32[r18+2])}FUNCTION_TABLE[HEAP32[HEAP32[r33]+28>>2]](r19,r32);r18=r19;r16=HEAP8[r18];if((r16&1)<<24>>24==0){r36=r19+4|0}else{r36=HEAP32[r20+2]}r19=r16&255;if((r19&1|0)==0){r37=r19>>>1}else{r37=HEAP32[r20+1]}__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(r8,r36,r37);if((HEAP8[r18]&1)<<24>>24!=0){__ZdlPv(HEAP32[r20+2])}r20=r1>>2;HEAP32[r4>>2]=FUNCTION_TABLE[HEAP32[HEAP32[r20]+12>>2]](r32);HEAP32[r5>>2]=FUNCTION_TABLE[HEAP32[HEAP32[r20]+16>>2]](r32);FUNCTION_TABLE[HEAP32[HEAP32[r1>>2]+20>>2]](r21,r32);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r6,r21);if((HEAP8[r21]&1)<<24>>24!=0){__ZdlPv(HEAP32[r21+8>>2])}FUNCTION_TABLE[HEAP32[HEAP32[r33]+24>>2]](r22,r32);r33=r22;r21=HEAP8[r33];if((r21&1)<<24>>24==0){r38=r22+4|0}else{r38=HEAP32[r23+2]}r22=r21&255;if((r22&1|0)==0){r39=r22>>>1}else{r39=HEAP32[r23+1]}__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(r7,r38,r39);if((HEAP8[r33]&1)<<24>>24!=0){__ZdlPv(HEAP32[r23+2])}r23=FUNCTION_TABLE[HEAP32[HEAP32[r20]+36>>2]](r32);HEAP32[r10>>2]=r23;STACKTOP=r11;return}else{r32=HEAP32[r2>>2];if((HEAP32[1313848]|0)!=-1){HEAP32[r13]=5255392;HEAP32[r13+1]=24;HEAP32[r13+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5255392,r12,406)}r12=HEAP32[1313849]-1|0;r13=HEAP32[r32+8>>2];if(HEAP32[r32+12>>2]-r13>>2>>>0<=r12>>>0){r40=___cxa_allocate_exception(4);r41=r40;HEAP32[r41>>2]=5247488;___cxa_throw(r40,5252700,514)}r32=HEAP32[r13+(r12<<2)>>2];if((r32|0)==0){r40=___cxa_allocate_exception(4);r41=r40;HEAP32[r41>>2]=5247488;___cxa_throw(r40,5252700,514)}r40=r32;FUNCTION_TABLE[HEAP32[HEAP32[r32>>2]+44>>2]](r24,r40);r41=r3;tempBigInt=HEAP32[r24>>2];HEAP8[r41]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r41+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r41+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r41+3|0]=tempBigInt&255;r41=r32>>2;FUNCTION_TABLE[HEAP32[HEAP32[r41]+32>>2]](r25,r40);r24=r25;r3=HEAP8[r24];if((r3&1)<<24>>24==0){r42=r25+4|0}else{r42=HEAP32[r26+2]}r25=r3&255;if((r25&1|0)==0){r43=r25>>>1}else{r43=HEAP32[r26+1]}__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(r9,r42,r43);if((HEAP8[r24]&1)<<24>>24!=0){__ZdlPv(HEAP32[r26+2])}FUNCTION_TABLE[HEAP32[HEAP32[r41]+28>>2]](r27,r40);r26=r27;r24=HEAP8[r26];if((r24&1)<<24>>24==0){r44=r27+4|0}else{r44=HEAP32[r28+2]}r27=r24&255;if((r27&1|0)==0){r45=r27>>>1}else{r45=HEAP32[r28+1]}__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(r8,r44,r45);if((HEAP8[r26]&1)<<24>>24!=0){__ZdlPv(HEAP32[r28+2])}r28=r32>>2;HEAP32[r4>>2]=FUNCTION_TABLE[HEAP32[HEAP32[r28]+12>>2]](r40);HEAP32[r5>>2]=FUNCTION_TABLE[HEAP32[HEAP32[r28]+16>>2]](r40);FUNCTION_TABLE[HEAP32[HEAP32[r32>>2]+20>>2]](r29,r40);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r6,r29);if((HEAP8[r29]&1)<<24>>24!=0){__ZdlPv(HEAP32[r29+8>>2])}FUNCTION_TABLE[HEAP32[HEAP32[r41]+24>>2]](r30,r40);r41=r30;r29=HEAP8[r41];if((r29&1)<<24>>24==0){r46=r30+4|0}else{r46=HEAP32[r31+2]}r30=r29&255;if((r30&1|0)==0){r47=r30>>>1}else{r47=HEAP32[r31+1]}__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(r7,r46,r47);if((HEAP8[r41]&1)<<24>>24!=0){__ZdlPv(HEAP32[r31+2])}r23=FUNCTION_TABLE[HEAP32[HEAP32[r28]+36>>2]](r40);HEAP32[r10>>2]=r23;STACKTOP=r11;return}}function __ZNSt3__119__double_or_nothingIwEEvRNS_10unique_ptrIT_PFvPvEEERPS2_S9_(r1,r2,r3){var r4,r5,r6,r7,r8,r9,r10,r11;r4=(r1+4|0)>>2;r5=(HEAP32[r4]|0)!=414;r6=(r1|0)>>2;r1=HEAP32[r6];r7=r1;r8=HEAP32[r3>>2]-r7|0;r9=r8>>>0<2147483647?r8<<1:-1;r8=HEAP32[r2>>2]-r7>>2;if(r5){r10=r1}else{r10=0}r1=_realloc(r10,r9);r10=r1;if((r1|0)==0){__ZSt17__throw_bad_allocv()}do{if(r5){HEAP32[r6]=r10;r11=r10}else{r1=HEAP32[r6];HEAP32[r6]=r10;if((r1|0)==0){r11=r10;break}FUNCTION_TABLE[HEAP32[r4]](r1);r11=HEAP32[r6]}}while(0);HEAP32[r4]=218;HEAP32[r2>>2]=(r8<<2)+r11|0;HEAP32[r3>>2]=(r9>>>2<<2)+HEAP32[r6]|0;return}function __ZNSt3__111__money_putIcE13__gather_infoEbbRKNS_6localeERNS_10money_base7patternERcS8_RNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEESF_SF_Ri(r1,r2,r3,r4,r5,r6,r7,r8,r9,r10){var r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36;r11=STACKTOP;STACKTOP=STACKTOP+28|0;r12=r11,r13=r12>>2;r14=r11+12,r15=r14>>2;r16=r11+24;r17=r16;r18=STACKTOP;STACKTOP=STACKTOP+12|0;r19=STACKTOP;STACKTOP=STACKTOP+4|0;r20=r19;r21=STACKTOP;STACKTOP=STACKTOP+12|0;r22=STACKTOP;STACKTOP=STACKTOP+12|0;r23=STACKTOP;STACKTOP=STACKTOP+12|0;r24=STACKTOP;STACKTOP=STACKTOP+4|0;r25=r24;r26=STACKTOP;STACKTOP=STACKTOP+12|0;r27=STACKTOP;STACKTOP=STACKTOP+4|0;r28=r27;r29=STACKTOP;STACKTOP=STACKTOP+12|0;r30=STACKTOP;STACKTOP=STACKTOP+12|0;r31=STACKTOP;STACKTOP=STACKTOP+12|0;r32=HEAP32[r3>>2]>>2;if(r1){if((HEAP32[1313850]|0)!=-1){HEAP32[r15]=5255400;HEAP32[r15+1]=24;HEAP32[r15+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5255400,r14,406)}r14=HEAP32[1313851]-1|0;r15=HEAP32[r32+2];if(HEAP32[r32+3]-r15>>2>>>0<=r14>>>0){r33=___cxa_allocate_exception(4);r34=r33;HEAP32[r34>>2]=5247488;___cxa_throw(r33,5252700,514)}r1=HEAP32[r15+(r14<<2)>>2],r14=r1>>2;if((r1|0)==0){r33=___cxa_allocate_exception(4);r34=r33;HEAP32[r34>>2]=5247488;___cxa_throw(r33,5252700,514)}r33=r1;r34=HEAP32[r14];do{if(r2){FUNCTION_TABLE[HEAP32[r34+44>>2]](r17,r33);r15=r4;tempBigInt=HEAP32[r16>>2];HEAP8[r15]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r15+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r15+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r15+3|0]=tempBigInt&255;FUNCTION_TABLE[HEAP32[HEAP32[r14]+32>>2]](r18,r33);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r9,r18);if((HEAP8[r18]&1)<<24>>24==0){break}__ZdlPv(HEAP32[r18+8>>2])}else{FUNCTION_TABLE[HEAP32[r34+40>>2]](r20,r33);r15=r4;tempBigInt=HEAP32[r19>>2];HEAP8[r15]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r15+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r15+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r15+3|0]=tempBigInt&255;FUNCTION_TABLE[HEAP32[HEAP32[r14]+28>>2]](r21,r33);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r9,r21);if((HEAP8[r21]&1)<<24>>24==0){break}__ZdlPv(HEAP32[r21+8>>2])}}while(0);r21=r1;HEAP8[r5]=FUNCTION_TABLE[HEAP32[HEAP32[r21>>2]+12>>2]](r33);HEAP8[r6]=FUNCTION_TABLE[HEAP32[HEAP32[r21>>2]+16>>2]](r33);r21=r1;FUNCTION_TABLE[HEAP32[HEAP32[r21>>2]+20>>2]](r22,r33);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r7,r22);if((HEAP8[r22]&1)<<24>>24!=0){__ZdlPv(HEAP32[r22+8>>2])}FUNCTION_TABLE[HEAP32[HEAP32[r21>>2]+24>>2]](r23,r33);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r8,r23);if((HEAP8[r23]&1)<<24>>24!=0){__ZdlPv(HEAP32[r23+8>>2])}r23=FUNCTION_TABLE[HEAP32[HEAP32[r14]+36>>2]](r33);HEAP32[r10>>2]=r23;STACKTOP=r11;return}else{if((HEAP32[1313852]|0)!=-1){HEAP32[r13]=5255408;HEAP32[r13+1]=24;HEAP32[r13+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5255408,r12,406)}r12=HEAP32[1313853]-1|0;r13=HEAP32[r32+2];if(HEAP32[r32+3]-r13>>2>>>0<=r12>>>0){r35=___cxa_allocate_exception(4);r36=r35;HEAP32[r36>>2]=5247488;___cxa_throw(r35,5252700,514)}r32=HEAP32[r13+(r12<<2)>>2],r12=r32>>2;if((r32|0)==0){r35=___cxa_allocate_exception(4);r36=r35;HEAP32[r36>>2]=5247488;___cxa_throw(r35,5252700,514)}r35=r32;r36=HEAP32[r12];do{if(r2){FUNCTION_TABLE[HEAP32[r36+44>>2]](r25,r35);r13=r4;tempBigInt=HEAP32[r24>>2];HEAP8[r13]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r13+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r13+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r13+3|0]=tempBigInt&255;FUNCTION_TABLE[HEAP32[HEAP32[r12]+32>>2]](r26,r35);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r9,r26);if((HEAP8[r26]&1)<<24>>24==0){break}__ZdlPv(HEAP32[r26+8>>2])}else{FUNCTION_TABLE[HEAP32[r36+40>>2]](r28,r35);r13=r4;tempBigInt=HEAP32[r27>>2];HEAP8[r13]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r13+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r13+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r13+3|0]=tempBigInt&255;FUNCTION_TABLE[HEAP32[HEAP32[r12]+28>>2]](r29,r35);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r9,r29);if((HEAP8[r29]&1)<<24>>24==0){break}__ZdlPv(HEAP32[r29+8>>2])}}while(0);r29=r32;HEAP8[r5]=FUNCTION_TABLE[HEAP32[HEAP32[r29>>2]+12>>2]](r35);HEAP8[r6]=FUNCTION_TABLE[HEAP32[HEAP32[r29>>2]+16>>2]](r35);r29=r32;FUNCTION_TABLE[HEAP32[HEAP32[r29>>2]+20>>2]](r30,r35);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r7,r30);if((HEAP8[r30]&1)<<24>>24!=0){__ZdlPv(HEAP32[r30+8>>2])}FUNCTION_TABLE[HEAP32[HEAP32[r29>>2]+24>>2]](r31,r35);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r8,r31);if((HEAP8[r31]&1)<<24>>24!=0){__ZdlPv(HEAP32[r31+8>>2])}r23=FUNCTION_TABLE[HEAP32[HEAP32[r12]+36>>2]](r35);HEAP32[r10>>2]=r23;STACKTOP=r11;return}}function __ZNSt3__111__money_putIcE8__formatEPcRS2_S3_jPKcS5_RKNS_5ctypeIcEEbRKNS_10money_base7patternEccRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEESL_SL_i(r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15){var r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48,r49,r50,r51,r52,r53,r54,r55,r56,r57,r58,r59,r60,r61,r62,r63,r64,r65,r66,r67,r68,r69,r70,r71,r72,r73,r74,r75,r76,r77,r78;r16=r3>>2;r3=0;HEAP32[r16]=r1;r17=r7>>2;r18=r14;r19=r14+1|0;r20=r14+8|0;r21=(r14+4|0)>>2;r14=r13;r22=(r4&512|0)==0;r23=r13+1|0;r24=r13+4|0;r25=r13+8|0;r13=r7+8|0;r26=(r15|0)>0;r27=r12;r28=r12+1|0;r29=(r12+8|0)>>2;r30=r12+4|0;r12=-r15|0;r31=r5;r5=0;while(1){r32=HEAP8[r9+r5|0]<<24>>24;L2191:do{if((r32|0)==1){HEAP32[r2>>2]=HEAP32[r16];r33=FUNCTION_TABLE[HEAP32[HEAP32[r17]+28>>2]](r7,32);r34=HEAP32[r16];HEAP32[r16]=r34+1|0;HEAP8[r34]=r33;r35=r31}else if((r32|0)==2){r33=HEAP8[r14];r34=r33&255;r36=(r34&1|0)==0;if(r36){r37=r34>>>1}else{r37=HEAP32[r24>>2]}if((r37|0)==0|r22){r35=r31;break}if((r33&1)<<24>>24==0){r38=r23;r39=r23}else{r33=HEAP32[r25>>2];r38=r33;r39=r33}if(r36){r40=r34>>>1}else{r40=HEAP32[r24>>2]}r34=r38+r40|0;r36=HEAP32[r16];L2206:do{if((r39|0)==(r34|0)){r41=r36}else{r33=r39;r42=r36;while(1){HEAP8[r42]=HEAP8[r33];r43=r33+1|0;r44=r42+1|0;if((r43|0)==(r34|0)){r41=r44;break L2206}else{r33=r43;r42=r44}}}}while(0);HEAP32[r16]=r41;r35=r31}else if((r32|0)==0){HEAP32[r2>>2]=HEAP32[r16];r35=r31}else if((r32|0)==3){r34=HEAP8[r18];r36=r34&255;if((r36&1|0)==0){r45=r36>>>1}else{r45=HEAP32[r21]}if((r45|0)==0){r35=r31;break}if((r34&1)<<24>>24==0){r46=r19}else{r46=HEAP32[r20>>2]}r34=HEAP8[r46];r36=HEAP32[r16];HEAP32[r16]=r36+1|0;HEAP8[r36]=r34;r35=r31}else if((r32|0)==4){r34=HEAP32[r16];r36=r8?r31+1|0:r31;r42=r36;while(1){if(r42>>>0>=r6>>>0){break}r33=HEAP8[r42];if(r33<<24>>24<=-1){break}if((HEAP16[HEAP32[r13>>2]+(r33<<24>>24<<1)>>1]&2048)<<16>>16==0){break}else{r42=r42+1|0}}r33=r42;if(r26){do{if(r42>>>0>r36>>>0){r44=r36+ -r33|0;r43=r44>>>0<r12>>>0?r12:r44;r44=r43+r15|0;r47=r42;r48=r15;r49=r34;while(1){r50=r47-1|0;r51=HEAP8[r50];HEAP32[r16]=r49+1|0;HEAP8[r49]=r51;r51=r48-1|0;r52=(r51|0)>0;if(!(r50>>>0>r36>>>0&r52)){break}r47=r50;r48=r51;r49=HEAP32[r16]}r49=r42+r43|0;if(r52){r53=r44;r54=r49;r3=1815;break}else{r55=0;r56=r44;r57=r49;break}}else{r53=r15;r54=r42;r3=1815}}while(0);if(r3==1815){r3=0;r55=FUNCTION_TABLE[HEAP32[HEAP32[r17]+28>>2]](r7,48);r56=r53;r57=r54}r33=HEAP32[r16];HEAP32[r16]=r33+1|0;L2237:do{if((r56|0)>0){r49=r56;r48=r33;while(1){HEAP8[r48]=r55;r47=r49-1|0;r51=HEAP32[r16];HEAP32[r16]=r51+1|0;if((r47|0)>0){r49=r47;r48=r51}else{r58=r51;break L2237}}}else{r58=r33}}while(0);HEAP8[r58]=r10;r59=r57}else{r59=r42}L2242:do{if((r59|0)==(r36|0)){r33=FUNCTION_TABLE[HEAP32[HEAP32[r17]+28>>2]](r7,48);r48=HEAP32[r16];HEAP32[r16]=r48+1|0;HEAP8[r48]=r33}else{r33=HEAP8[r27];r48=r33&255;if((r48&1|0)==0){r60=r48>>>1}else{r60=HEAP32[r30>>2]}do{if((r60|0)==0){r61=r59;r62=0;r63=0;r64=-1}else{if((r33&1)<<24>>24==0){r65=r28}else{r65=HEAP32[r29]}r61=r59;r62=0;r63=0;r64=HEAP8[r65]<<24>>24;break}}while(0);while(1){do{if((r62|0)==(r64|0)){r33=HEAP32[r16];HEAP32[r16]=r33+1|0;HEAP8[r33]=r11;r33=r63+1|0;r48=HEAP8[r27];r49=r48&255;if((r49&1|0)==0){r66=r49>>>1}else{r66=HEAP32[r30>>2]}if(r33>>>0>=r66>>>0){r67=r64;r68=r33;r69=0;break}r49=(r48&1)<<24>>24==0;if(r49){r70=r28}else{r70=HEAP32[r29]}if(HEAP8[r70+r33|0]<<24>>24==127){r67=-1;r68=r33;r69=0;break}if(r49){r71=r28}else{r71=HEAP32[r29]}r67=HEAP8[r71+r33|0]<<24>>24;r68=r33;r69=0}else{r67=r64;r68=r63;r69=r62}}while(0);r33=r61-1|0;r49=HEAP8[r33];r48=HEAP32[r16];HEAP32[r16]=r48+1|0;HEAP8[r48]=r49;if((r33|0)==(r36|0)){break L2242}else{r61=r33;r62=r69+1|0;r63=r68;r64=r67}}}}while(0);r42=HEAP32[r16];if((r34|0)==(r42|0)){r35=r36;break}r33=r42-1|0;if(r34>>>0<r33>>>0){r72=r34;r73=r33}else{r35=r36;break}while(1){r33=HEAP8[r72];HEAP8[r72]=HEAP8[r73];HEAP8[r73]=r33;r33=r72+1|0;r42=r73-1|0;if(r33>>>0<r42>>>0){r72=r33;r73=r42}else{r35=r36;break L2191}}}else{r35=r31}}while(0);r32=r5+1|0;if((r32|0)==4){break}else{r31=r35;r5=r32}}r5=HEAP8[r18];r18=r5&255;r35=(r18&1|0)==0;if(r35){r74=r18>>>1}else{r74=HEAP32[r21]}if(r74>>>0>1){if((r5&1)<<24>>24==0){r75=r19;r76=r19}else{r19=HEAP32[r20>>2];r75=r19;r76=r19}if(r35){r77=r18>>>1}else{r77=HEAP32[r21]}r21=r75+r77|0;r77=HEAP32[r16];r75=r76+1|0;L2290:do{if((r75|0)==(r21|0)){r78=r77}else{r76=r77;r18=r75;while(1){HEAP8[r76]=HEAP8[r18];r35=r76+1|0;r19=r18+1|0;if((r19|0)==(r21|0)){r78=r35;break L2290}else{r76=r35;r18=r19}}}}while(0);HEAP32[r16]=r78}r78=r4&176;if((r78|0)==16){return}else if((r78|0)==32){HEAP32[r2>>2]=HEAP32[r16];return}else{HEAP32[r2>>2]=r1;return}}function __ZNSt3__111__money_putIwE13__gather_infoEbbRKNS_6localeERNS_10money_base7patternERwS8_RNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEERNS9_IwNSA_IwEENSC_IwEEEESJ_Ri(r1,r2,r3,r4,r5,r6,r7,r8,r9,r10){var r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48,r49,r50,r51,r52,r53,r54,r55;r11=STACKTOP;STACKTOP=STACKTOP+28|0;r12=r11,r13=r12>>2;r14=r11+12,r15=r14>>2;r16=r11+24;r17=r16;r18=STACKTOP,r19=r18>>2;STACKTOP=STACKTOP+12|0;r20=STACKTOP;STACKTOP=STACKTOP+4|0;r21=r20;r22=STACKTOP,r23=r22>>2;STACKTOP=STACKTOP+12|0;r24=STACKTOP;STACKTOP=STACKTOP+12|0;r25=STACKTOP,r26=r25>>2;STACKTOP=STACKTOP+12|0;r27=STACKTOP;STACKTOP=STACKTOP+4|0;r28=r27;r29=STACKTOP,r30=r29>>2;STACKTOP=STACKTOP+12|0;r31=STACKTOP;STACKTOP=STACKTOP+4|0;r32=r31;r33=STACKTOP,r34=r33>>2;STACKTOP=STACKTOP+12|0;r35=STACKTOP;STACKTOP=STACKTOP+12|0;r36=STACKTOP,r37=r36>>2;STACKTOP=STACKTOP+12|0;r38=HEAP32[r3>>2]>>2;if(r1){if((HEAP32[1313846]|0)!=-1){HEAP32[r15]=5255384;HEAP32[r15+1]=24;HEAP32[r15+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5255384,r14,406)}r14=HEAP32[1313847]-1|0;r15=HEAP32[r38+2];if(HEAP32[r38+3]-r15>>2>>>0<=r14>>>0){r39=___cxa_allocate_exception(4);r40=r39;HEAP32[r40>>2]=5247488;___cxa_throw(r39,5252700,514)}r1=HEAP32[r15+(r14<<2)>>2],r14=r1>>2;if((r1|0)==0){r39=___cxa_allocate_exception(4);r40=r39;HEAP32[r40>>2]=5247488;___cxa_throw(r39,5252700,514)}r39=r1;r40=HEAP32[r14];do{if(r2){FUNCTION_TABLE[HEAP32[r40+44>>2]](r17,r39);r15=r4;tempBigInt=HEAP32[r16>>2];HEAP8[r15]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r15+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r15+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r15+3|0]=tempBigInt&255;FUNCTION_TABLE[HEAP32[HEAP32[r14]+32>>2]](r18,r39);r15=r18;r3=HEAP8[r15];if((r3&1)<<24>>24==0){r41=r18+4|0}else{r41=HEAP32[r19+2]}r42=r3&255;if((r42&1|0)==0){r43=r42>>>1}else{r43=HEAP32[r19+1]}__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(r9,r41,r43);if((HEAP8[r15]&1)<<24>>24==0){break}__ZdlPv(HEAP32[r19+2])}else{FUNCTION_TABLE[HEAP32[r40+40>>2]](r21,r39);r15=r4;tempBigInt=HEAP32[r20>>2];HEAP8[r15]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r15+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r15+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r15+3|0]=tempBigInt&255;FUNCTION_TABLE[HEAP32[HEAP32[r14]+28>>2]](r22,r39);r15=r22;r42=HEAP8[r15];if((r42&1)<<24>>24==0){r44=r22+4|0}else{r44=HEAP32[r23+2]}r3=r42&255;if((r3&1|0)==0){r45=r3>>>1}else{r45=HEAP32[r23+1]}__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(r9,r44,r45);if((HEAP8[r15]&1)<<24>>24==0){break}__ZdlPv(HEAP32[r23+2])}}while(0);r23=r1>>2;HEAP32[r5>>2]=FUNCTION_TABLE[HEAP32[HEAP32[r23]+12>>2]](r39);HEAP32[r6>>2]=FUNCTION_TABLE[HEAP32[HEAP32[r23]+16>>2]](r39);FUNCTION_TABLE[HEAP32[HEAP32[r14]+20>>2]](r24,r39);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r7,r24);if((HEAP8[r24]&1)<<24>>24!=0){__ZdlPv(HEAP32[r24+8>>2])}FUNCTION_TABLE[HEAP32[HEAP32[r14]+24>>2]](r25,r39);r14=r25;r24=HEAP8[r14];if((r24&1)<<24>>24==0){r46=r25+4|0}else{r46=HEAP32[r26+2]}r25=r24&255;if((r25&1|0)==0){r47=r25>>>1}else{r47=HEAP32[r26+1]}__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(r8,r46,r47);if((HEAP8[r14]&1)<<24>>24!=0){__ZdlPv(HEAP32[r26+2])}r26=FUNCTION_TABLE[HEAP32[HEAP32[r23]+36>>2]](r39);HEAP32[r10>>2]=r26;STACKTOP=r11;return}else{if((HEAP32[1313848]|0)!=-1){HEAP32[r13]=5255392;HEAP32[r13+1]=24;HEAP32[r13+2]=0;__ZNSt3__111__call_onceERVmPvPFvS2_E(5255392,r12,406)}r12=HEAP32[1313849]-1|0;r13=HEAP32[r38+2];if(HEAP32[r38+3]-r13>>2>>>0<=r12>>>0){r48=___cxa_allocate_exception(4);r49=r48;HEAP32[r49>>2]=5247488;___cxa_throw(r48,5252700,514)}r38=HEAP32[r13+(r12<<2)>>2],r12=r38>>2;if((r38|0)==0){r48=___cxa_allocate_exception(4);r49=r48;HEAP32[r49>>2]=5247488;___cxa_throw(r48,5252700,514)}r48=r38;r49=HEAP32[r12];do{if(r2){FUNCTION_TABLE[HEAP32[r49+44>>2]](r28,r48);r13=r4;tempBigInt=HEAP32[r27>>2];HEAP8[r13]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r13+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r13+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r13+3|0]=tempBigInt&255;FUNCTION_TABLE[HEAP32[HEAP32[r12]+32>>2]](r29,r48);r13=r29;r39=HEAP8[r13];if((r39&1)<<24>>24==0){r50=r29+4|0}else{r50=HEAP32[r30+2]}r23=r39&255;if((r23&1|0)==0){r51=r23>>>1}else{r51=HEAP32[r30+1]}__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(r9,r50,r51);if((HEAP8[r13]&1)<<24>>24==0){break}__ZdlPv(HEAP32[r30+2])}else{FUNCTION_TABLE[HEAP32[r49+40>>2]](r32,r48);r13=r4;tempBigInt=HEAP32[r31>>2];HEAP8[r13]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r13+1|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r13+2|0]=tempBigInt&255;tempBigInt=tempBigInt>>8;HEAP8[r13+3|0]=tempBigInt&255;FUNCTION_TABLE[HEAP32[HEAP32[r12]+28>>2]](r33,r48);r13=r33;r23=HEAP8[r13];if((r23&1)<<24>>24==0){r52=r33+4|0}else{r52=HEAP32[r34+2]}r39=r23&255;if((r39&1|0)==0){r53=r39>>>1}else{r53=HEAP32[r34+1]}__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(r9,r52,r53);if((HEAP8[r13]&1)<<24>>24==0){break}__ZdlPv(HEAP32[r34+2])}}while(0);r34=r38>>2;HEAP32[r5>>2]=FUNCTION_TABLE[HEAP32[HEAP32[r34]+12>>2]](r48);HEAP32[r6>>2]=FUNCTION_TABLE[HEAP32[HEAP32[r34]+16>>2]](r48);FUNCTION_TABLE[HEAP32[HEAP32[r12]+20>>2]](r35,r48);__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEaSERKS5_(r7,r35);if((HEAP8[r35]&1)<<24>>24!=0){__ZdlPv(HEAP32[r35+8>>2])}FUNCTION_TABLE[HEAP32[HEAP32[r12]+24>>2]](r36,r48);r12=r36;r35=HEAP8[r12];if((r35&1)<<24>>24==0){r54=r36+4|0}else{r54=HEAP32[r37+2]}r36=r35&255;if((r36&1|0)==0){r55=r36>>>1}else{r55=HEAP32[r37+1]}__ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEE6assignEPKwj(r8,r54,r55);if((HEAP8[r12]&1)<<24>>24!=0){__ZdlPv(HEAP32[r37+2])}r26=FUNCTION_TABLE[HEAP32[HEAP32[r34]+36>>2]](r48);HEAP32[r10>>2]=r26;STACKTOP=r11;return}}function __ZNSt3__111__money_putIwE8__formatEPwRS2_S3_jPKwS5_RKNS_5ctypeIwEEbRKNS_10money_base7patternEwwRKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEERKNSE_IwNSF_IwEENSH_IwEEEESQ_i(r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15){var r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48,r49,r50,r51,r52,r53,r54,r55,r56,r57,r58,r59,r60,r61,r62,r63,r64,r65,r66,r67,r68,r69,r70,r71,r72,r73,r74,r75,r76,r77;r16=r3>>2;r3=0;HEAP32[r16]=r1;r17=r7>>2;r18=r14;r19=r14+4|0,r20=r19>>2;r21=r14+8|0;r14=r13;r22=(r4&512|0)==0;r23=r13+4|0;r24=r13+8|0;r13=r7;r25=(r15|0)>0;r26=r12;r27=r12+1|0;r28=(r12+8|0)>>2;r29=r12+4|0;r12=r5;r5=0;while(1){r30=HEAP8[r9+r5|0]<<24>>24;L2408:do{if((r30|0)==0){HEAP32[r2>>2]=HEAP32[r16];r31=r12}else if((r30|0)==3){r32=HEAP8[r18];r33=r32&255;if((r33&1|0)==0){r34=r33>>>1}else{r34=HEAP32[r20]}if((r34|0)==0){r31=r12;break}if((r32&1)<<24>>24==0){r35=r19}else{r35=HEAP32[r21>>2]}r32=HEAP32[r35>>2];r33=HEAP32[r16];HEAP32[r16]=r33+4|0;HEAP32[r33>>2]=r32;r31=r12}else if((r30|0)==2){r32=HEAP8[r14];r33=r32&255;r36=(r33&1|0)==0;if(r36){r37=r33>>>1}else{r37=HEAP32[r23>>2]}if((r37|0)==0|r22){r31=r12;break}if((r32&1)<<24>>24==0){r38=r23;r39=r23;r40=r23}else{r32=HEAP32[r24>>2];r38=r32;r39=r32;r40=r32}if(r36){r41=r33>>>1}else{r41=HEAP32[r23>>2]}r33=(r41<<2)+r38|0;r36=HEAP32[r16];if((r39|0)==(r33|0)){r42=r36}else{r32=((r41-1<<2)+r38+ -r40|0)>>>2;r43=r39;r44=r36;while(1){HEAP32[r44>>2]=HEAP32[r43>>2];r45=r43+4|0;if((r45|0)==(r33|0)){break}r43=r45;r44=r44+4|0}r42=(r32+1<<2)+r36|0}HEAP32[r16]=r42;r31=r12}else if((r30|0)==4){r44=HEAP32[r16];r43=r8?r12+4|0:r12;r33=r43;while(1){if(r33>>>0>=r6>>>0){break}if(FUNCTION_TABLE[HEAP32[HEAP32[r13>>2]+12>>2]](r7,2048,HEAP32[r33>>2])){r33=r33+4|0}else{break}}if(r25){do{if(r33>>>0>r43>>>0){r36=r33;r32=r15;while(1){r46=r36-4|0;r45=HEAP32[r46>>2];r47=HEAP32[r16];HEAP32[r16]=r47+4|0;HEAP32[r47>>2]=r45;r48=r32-1|0;r49=(r48|0)>0;if(r46>>>0>r43>>>0&r49){r36=r46;r32=r48}else{break}}if(r49){r50=r48;r51=r46;r3=1996;break}else{r52=0;r53=r48;r54=r46;break}}else{r50=r15;r51=r33;r3=1996}}while(0);if(r3==1996){r3=0;r52=FUNCTION_TABLE[HEAP32[HEAP32[r17]+44>>2]](r7,48);r53=r50;r54=r51}r32=HEAP32[r16];HEAP32[r16]=r32+4|0;L2454:do{if((r53|0)>0){r36=r53;r45=r32;while(1){HEAP32[r45>>2]=r52;r47=r36-1|0;r55=HEAP32[r16];HEAP32[r16]=r55+4|0;if((r47|0)>0){r36=r47;r45=r55}else{r56=r55;break L2454}}}else{r56=r32}}while(0);HEAP32[r56>>2]=r10;r57=r54}else{r57=r33}L2459:do{if((r57|0)==(r43|0)){r32=FUNCTION_TABLE[HEAP32[HEAP32[r17]+44>>2]](r7,48);r45=HEAP32[r16];HEAP32[r16]=r45+4|0;HEAP32[r45>>2]=r32}else{r32=HEAP8[r26];r45=r32&255;if((r45&1|0)==0){r58=r45>>>1}else{r58=HEAP32[r29>>2]}do{if((r58|0)==0){r59=r57;r60=0;r61=0;r62=-1}else{if((r32&1)<<24>>24==0){r63=r27}else{r63=HEAP32[r28]}r59=r57;r60=0;r61=0;r62=HEAP8[r63]<<24>>24;break}}while(0);while(1){do{if((r60|0)==(r62|0)){r32=HEAP32[r16];HEAP32[r16]=r32+4|0;HEAP32[r32>>2]=r11;r32=r61+1|0;r45=HEAP8[r26];r36=r45&255;if((r36&1|0)==0){r64=r36>>>1}else{r64=HEAP32[r29>>2]}if(r32>>>0>=r64>>>0){r65=r62;r66=r32;r67=0;break}r36=(r45&1)<<24>>24==0;if(r36){r68=r27}else{r68=HEAP32[r28]}if(HEAP8[r68+r32|0]<<24>>24==127){r65=-1;r66=r32;r67=0;break}if(r36){r69=r27}else{r69=HEAP32[r28]}r65=HEAP8[r69+r32|0]<<24>>24;r66=r32;r67=0}else{r65=r62;r66=r61;r67=r60}}while(0);r32=r59-4|0;r36=HEAP32[r32>>2];r45=HEAP32[r16];HEAP32[r16]=r45+4|0;HEAP32[r45>>2]=r36;if((r32|0)==(r43|0)){break L2459}else{r59=r32;r60=r67+1|0;r61=r66;r62=r65}}}}while(0);r33=HEAP32[r16];if((r44|0)==(r33|0)){r31=r43;break}r32=r33-4|0;if(r44>>>0<r32>>>0){r70=r44;r71=r32}else{r31=r43;break}while(1){r32=HEAP32[r70>>2];HEAP32[r70>>2]=HEAP32[r71>>2];HEAP32[r71>>2]=r32;r32=r70+4|0;r33=r71-4|0;if(r32>>>0<r33>>>0){r70=r32;r71=r33}else{r31=r43;break L2408}}}else if((r30|0)==1){HEAP32[r2>>2]=HEAP32[r16];r43=FUNCTION_TABLE[HEAP32[HEAP32[r17]+44>>2]](r7,32);r44=HEAP32[r16];HEAP32[r16]=r44+4|0;HEAP32[r44>>2]=r43;r31=r12}else{r31=r12}}while(0);r30=r5+1|0;if((r30|0)==4){break}else{r12=r31;r5=r30}}r5=HEAP8[r18];r18=r5&255;r31=(r18&1|0)==0;if(r31){r72=r18>>>1}else{r72=HEAP32[r20]}if(r72>>>0>1){if((r5&1)<<24>>24==0){r73=r19;r74=r19;r75=r19}else{r19=HEAP32[r21>>2];r73=r19;r74=r19;r75=r19}if(r31){r76=r18>>>1}else{r76=HEAP32[r20]}r20=(r76<<2)+r73|0;r18=HEAP32[r16];r31=r74+4|0;if((r31|0)==(r20|0)){r77=r18}else{r74=(((r76-2<<2)+r73+ -r75|0)>>>2)+1|0;r75=r18;r73=r31;while(1){HEAP32[r75>>2]=HEAP32[r73>>2];r31=r73+4|0;if((r31|0)==(r20|0)){break}else{r75=r75+4|0;r73=r31}}r77=(r74<<2)+r18|0}HEAP32[r16]=r77}r77=r4&176;if((r77|0)==16){return}else if((r77|0)==32){HEAP32[r2>>2]=HEAP32[r16];return}else{HEAP32[r2>>2]=r1;return}}function __ZNSt3__117__call_once_proxyINS_12_GLOBAL__N_111__fake_bindEEEvPv(r1){var r2,r3,r4;r2=r1+4|0;r3=HEAP32[r1>>2]+HEAP32[r2+4>>2]|0;r1=r3;r4=HEAP32[r2>>2];if((r4&1|0)==0){r2=r4;FUNCTION_TABLE[r2](r1);return}else{r2=HEAP32[HEAP32[r3>>2]+(r4-1)>>2];FUNCTION_TABLE[r2](r1);return}}function ___cxx_global_array_dtor(r1){var r2;r1=5245848;while(1){r2=r1-12|0;if((HEAP8[r2]&1)<<24>>24!=0){__ZdlPv(HEAP32[r1-12+8>>2])}if((r2|0)==5245560){break}else{r1=r2}}return}function ___cxx_global_array_dtor80(r1){var r2;r1=5246592;while(1){r2=r1-12|0;if((HEAP8[r2]&1)<<24>>24!=0){__ZdlPv(HEAP32[r1-12+8>>2])}if((r2|0)==5246304){break}else{r1=r2}}return}function ___cxx_global_array_dtor83(r1){var r2;r1=5245392;while(1){r2=r1-12|0;if((HEAP8[r2]&1)<<24>>24!=0){__ZdlPv(HEAP32[r1-12+8>>2])}if((r2|0)==5245104){break}else{r1=r2}}return}function ___cxx_global_array_dtor108(r1){var r2;r1=5246136;while(1){r2=r1-12|0;if((HEAP8[r2]&1)<<24>>24!=0){__ZdlPv(HEAP32[r1-12+8>>2])}if((r2|0)==5245848){break}else{r1=r2}}return}function __ZNKSt3__120__vector_base_commonILb1EE20__throw_length_errorEv(r1){var r2,r3,r4,r5,r6;r1=___cxa_allocate_exception(8);HEAP32[r1>>2]=5247560;r2=r1+4|0;if((r2|0)==0){r3=r1;HEAP32[r3>>2]=5247536;___cxa_throw(r1,5252724,248)}r4=__Znaj(19),r5=r4>>2;HEAP32[r5+1]=6;HEAP32[r5]=6;r6=r4+12|0;HEAP32[r2>>2]=r6;HEAP32[r5+2]=0;HEAP8[r6]=HEAP8[5243440];HEAP8[r6+1|0]=HEAP8[5243441|0];HEAP8[r6+2|0]=HEAP8[5243442|0];HEAP8[r6+3|0]=HEAP8[5243443|0];HEAP8[r6+4|0]=HEAP8[5243444|0];HEAP8[r6+5|0]=HEAP8[5243445|0];HEAP8[r6+6|0]=HEAP8[5243446|0];r3=r1;HEAP32[r3>>2]=5247536;___cxa_throw(r1,5252724,248)}function __ZNSt3__16vectorIPNS_6locale5facetENS_15__sso_allocatorIS3_Lj28EEEE8__appendEj(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17;r3=0;r4=r1+8|0;r5=(r1+4|0)>>2;r6=HEAP32[r5];r7=HEAP32[r4>>2];r8=r6;if(r7-r8>>2>>>0>=r2>>>0){r9=r2;r10=r6;while(1){if((r10|0)==0){r11=0}else{HEAP32[r10>>2]=0;r11=HEAP32[r5]}r6=r11+4|0;HEAP32[r5]=r6;r12=r9-1|0;if((r12|0)==0){break}else{r9=r12;r10=r6}}return}r10=r1+12|0;r9=(r1|0)>>2;r11=HEAP32[r9];r6=r8-r11>>2;r8=r6+r2|0;if(r8>>>0>1073741823){__ZNKSt3__120__vector_base_commonILb1EE20__throw_length_errorEv(0)}r12=r7-r11|0;do{if(r12>>2>>>0>536870910){r13=1073741823;r3=2090}else{r11=r12>>1;r7=r11>>>0<r8>>>0?r8:r11;if((r7|0)==0){r14=0;r15=0;break}r11=r1+124|0;if(!((HEAP8[r11]&1)<<24>>24==0&r7>>>0<29)){r13=r7;r3=2090;break}HEAP8[r11]=1;r14=r10;r15=r7;break}}while(0);if(r3==2090){r14=__Znwj(r13<<2);r15=r13}r13=r2;r2=(r6<<2)+r14|0;while(1){if((r2|0)==0){r16=0}else{HEAP32[r2>>2]=0;r16=r2}r17=r16+4|0;r3=r13-1|0;if((r3|0)==0){break}else{r13=r3;r2=r17}}r2=HEAP32[r9];r13=HEAP32[r5]-r2|0;r16=(r6-(r13>>2)<<2)+r14|0;r6=r2;_memcpy(r16,r6,r13);HEAP32[r9]=r16;HEAP32[r5]=r17;HEAP32[r4>>2]=(r15<<2)+r14|0;if((r2|0)==0){return}if((r2|0)==(r10|0)){HEAP8[r1+124|0]=0;return}else{__ZdlPv(r6);return}}function __ZNKSt8bad_cast4whatEv(r1){return 5243648}function __ZN10__cxxabiv116__shim_type_infoD2Ev(r1){return}function __ZNK10__cxxabiv116__shim_type_info5noop1Ev(r1){return}function __ZNK10__cxxabiv116__shim_type_info5noop2Ev(r1){return}function __GLOBAL__I_a263(){if(HEAP8[5255732]<<24>>24==0){HEAP32[1313651]=0;HEAP32[1313652]=0;HEAP32[1313933]=1;HEAP32[1313934]=0}if(HEAP8[5255724]<<24>>24==0){HEAP32[1313649]=0;HEAP32[1313650]=0;HEAP32[1313931]=1;HEAP32[1313932]=0}if(HEAP8[5255716]<<24>>24==0){HEAP32[1313647]=0;HEAP32[1313648]=0;HEAP32[1313929]=1;HEAP32[1313930]=0}if(HEAP8[5255708]<<24>>24==0){HEAP32[1313645]=0;HEAP32[1313646]=0;HEAP32[1313927]=1;HEAP32[1313928]=0}if(HEAP8[5255700]<<24>>24==0){HEAP32[1313643]=0;HEAP32[1313644]=0;HEAP32[1313925]=1;HEAP32[1313926]=0}if(HEAP8[5255692]<<24>>24==0){HEAP32[1313641]=0;HEAP32[1313642]=0;HEAP32[1313923]=1;HEAP32[1313924]=0}if(HEAP8[5255668]<<24>>24==0){HEAP32[1313631]=0;HEAP32[1313632]=0;HEAP32[1313917]=1;HEAP32[1313918]=0}if(HEAP8[5255660]<<24>>24==0){HEAP32[1313629]=0;HEAP32[1313630]=0;HEAP32[1313915]=1;HEAP32[1313916]=0}if(HEAP8[5255652]<<24>>24==0){HEAP32[1313627]=0;HEAP32[1313628]=0;HEAP32[1313913]=1;HEAP32[1313914]=0}if(HEAP8[5255644]<<24>>24==0){HEAP32[1313625]=0;HEAP32[1313626]=0;HEAP32[1313911]=1;HEAP32[1313912]=0}if(HEAP8[5255764]<<24>>24==0){HEAP32[1313852]=0;HEAP32[1313853]=0;HEAP32[1313941]=1;HEAP32[1313942]=0}if(HEAP8[5255756]<<24>>24==0){HEAP32[1313850]=0;HEAP32[1313851]=0;HEAP32[1313939]=1;HEAP32[1313940]=0}if(HEAP8[5255748]<<24>>24==0){HEAP32[1313848]=0;HEAP32[1313849]=0;HEAP32[1313937]=1;HEAP32[1313938]=0}if(HEAP8[5255740]<<24>>24==0){HEAP32[1313846]=0;HEAP32[1313847]=0;HEAP32[1313935]=1;HEAP32[1313936]=0}if(HEAP8[5255636]<<24>>24==0){HEAP32[1313623]=0;HEAP32[1313624]=0;HEAP32[1313909]=1;HEAP32[1313910]=0}if(HEAP8[5255628]<<24>>24==0){HEAP32[1313621]=0;HEAP32[1313622]=0;HEAP32[1313907]=1;HEAP32[1313908]=0}if(HEAP8[5255620]<<24>>24==0){HEAP32[1313619]=0;HEAP32[1313620]=0;HEAP32[1313905]=1;HEAP32[1313906]=0}if(HEAP8[5255612]<<24>>24==0){HEAP32[1313617]=0;HEAP32[1313618]=0;HEAP32[1313903]=1;HEAP32[1313904]=0}if(HEAP8[5255684]<<24>>24==0){HEAP32[1313639]=0;HEAP32[1313640]=0;HEAP32[1313921]=1;HEAP32[1313922]=0}if(HEAP8[5255676]<<24>>24!=0){HEAP32[1313725]=0;HEAP32[1313726]=0;HEAP32[1313727]=0;HEAP32[1313728]=0;HEAP32[1313655]=0;HEAP32[1313656]=0;HEAP32[1313653]=0;HEAP32[1313654]=0;HEAP32[1313657]=0;HEAP32[1313658]=0;HEAP32[1313659]=0;HEAP32[1313660]=0;HEAP32[1313635]=0;HEAP32[1313636]=0;HEAP32[1313633]=0;HEAP32[1313634]=0;return}HEAP32[1313637]=0;HEAP32[1313638]=0;HEAP32[1313919]=1;HEAP32[1313920]=0;HEAP32[1313725]=0;HEAP32[1313726]=0;HEAP32[1313727]=0;HEAP32[1313728]=0;HEAP32[1313655]=0;HEAP32[1313656]=0;HEAP32[1313653]=0;HEAP32[1313654]=0;HEAP32[1313657]=0;HEAP32[1313658]=0;HEAP32[1313659]=0;HEAP32[1313660]=0;HEAP32[1313635]=0;HEAP32[1313636]=0;HEAP32[1313633]=0;HEAP32[1313634]=0;return}function __ZNK10__cxxabiv117__class_type_info27has_unambiguous_public_baseEPNS_19__dynamic_cast_infoEPvi(r1,r2,r3,r4){var r5;if((HEAP32[r2+8>>2]|0)!=(r1|0)){return}r1=r2+16|0;r5=HEAP32[r1>>2];if((r5|0)==0){HEAP32[r1>>2]=r3;HEAP32[r2+24>>2]=r4;HEAP32[r2+36>>2]=1;return}if((r5|0)!=(r3|0)){r3=r2+36|0;HEAP32[r3>>2]=HEAP32[r3>>2]+1|0;HEAP32[r2+24>>2]=2;HEAP8[r2+54|0]=1;return}r3=r2+24|0;if((HEAP32[r3>>2]|0)!=2){return}HEAP32[r3>>2]=r4;return}function ___cxx_global_array_dtor132(r1){if((HEAP8[5245548]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311389])}if((HEAP8[5245536]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311386])}if((HEAP8[5245524]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311383])}if((HEAP8[5245512]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311380])}if((HEAP8[5245500]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311377])}if((HEAP8[5245488]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311374])}if((HEAP8[5245476]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311371])}if((HEAP8[5245464]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311368])}if((HEAP8[5245452]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311365])}if((HEAP8[5245440]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311362])}if((HEAP8[5245428]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311359])}if((HEAP8[5245416]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311356])}if((HEAP8[5245404]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311353])}if((HEAP8[5245392]&1)<<24>>24==0){return}__ZdlPv(HEAP32[1311350]);return}function ___cxx_global_array_dtor147(r1){if((HEAP8[5246292]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311575])}if((HEAP8[5246280]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311572])}if((HEAP8[5246268]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311569])}if((HEAP8[5246256]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311566])}if((HEAP8[5246244]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311563])}if((HEAP8[5246232]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311560])}if((HEAP8[5246220]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311557])}if((HEAP8[5246208]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311554])}if((HEAP8[5246196]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311551])}if((HEAP8[5246184]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311548])}if((HEAP8[5246172]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311545])}if((HEAP8[5246160]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311542])}if((HEAP8[5246148]&1)<<24>>24!=0){__ZdlPv(HEAP32[1311539])}if((HEAP8[5246136]&1)<<24>>24==0){return}__ZdlPv(HEAP32[1311536]);return}function __ZNSt8bad_castD0Ev(r1){__ZdlPv(r1);return}function __ZNSt8bad_castD2Ev(r1){return}function __ZN10__cxxabiv117__class_type_infoD0Ev(r1){__ZdlPv(r1);return}function __ZN10__cxxabiv120__si_class_type_infoD0Ev(r1){__ZdlPv(r1);return}function __ZN10__cxxabiv121__vmi_class_type_infoD0Ev(r1){__ZdlPv(r1);return}function __ZNK10__cxxabiv117__class_type_info9can_catchEPKNS_16__shim_type_infoERPv(r1,r2,r3){var r4,r5,r6,r7,r8,r9;r4=STACKTOP;STACKTOP=STACKTOP+56|0;r5=r4,r6=r5>>2;do{if((r1|0)==(r2|0)){r7=1}else{if((r2|0)==0){r7=0;break}r8=___dynamic_cast(r2,5254128,5254116,-1);r9=r8;if((r8|0)==0){r7=0;break}_memset(r5,0,56);HEAP32[r6]=r9;HEAP32[r6+2]=r1;HEAP32[r6+3]=-1;HEAP32[r6+12]=1;FUNCTION_TABLE[HEAP32[HEAP32[r8>>2]+28>>2]](r9,r5,HEAP32[r3>>2],1);if((HEAP32[r6+6]|0)!=1){r7=0;break}HEAP32[r3>>2]=HEAP32[r6+4];r7=1}}while(0);STACKTOP=r4;return r7}function __ZNK10__cxxabiv120__si_class_type_info27has_unambiguous_public_baseEPNS_19__dynamic_cast_infoEPvi(r1,r2,r3,r4){var r5;if((r1|0)!=(HEAP32[r2+8>>2]|0)){r5=HEAP32[r1+8>>2];FUNCTION_TABLE[HEAP32[HEAP32[r5>>2]+28>>2]](r5,r2,r3,r4);return}r5=r2+16|0;r1=HEAP32[r5>>2];if((r1|0)==0){HEAP32[r5>>2]=r3;HEAP32[r2+24>>2]=r4;HEAP32[r2+36>>2]=1;return}if((r1|0)!=(r3|0)){r3=r2+36|0;HEAP32[r3>>2]=HEAP32[r3>>2]+1|0;HEAP32[r2+24>>2]=2;HEAP8[r2+54|0]=1;return}r3=r2+24|0;if((HEAP32[r3>>2]|0)!=2){return}HEAP32[r3>>2]=r4;return}function __ZNK10__cxxabiv121__vmi_class_type_info27has_unambiguous_public_baseEPNS_19__dynamic_cast_infoEPvi(r1,r2,r3,r4){var r5,r6,r7,r8,r9,r10,r11;r5=0;if((r1|0)==(HEAP32[r2+8>>2]|0)){r6=r2+16|0;r7=HEAP32[r6>>2];if((r7|0)==0){HEAP32[r6>>2]=r3;HEAP32[r2+24>>2]=r4;HEAP32[r2+36>>2]=1;return}if((r7|0)!=(r3|0)){r7=r2+36|0;HEAP32[r7>>2]=HEAP32[r7>>2]+1|0;HEAP32[r2+24>>2]=2;HEAP8[r2+54|0]=1;return}r7=r2+24|0;if((HEAP32[r7>>2]|0)!=2){return}HEAP32[r7>>2]=r4;return}r7=HEAP32[r1+12>>2];r6=(r7<<3)+r1+16|0;r8=HEAP32[r1+20>>2];r9=r8>>8;if((r8&1|0)==0){r10=r9}else{r10=HEAP32[HEAP32[r3>>2]+r9>>2]}r9=HEAP32[r1+16>>2];FUNCTION_TABLE[HEAP32[HEAP32[r9>>2]+28>>2]](r9,r2,r3+r10|0,(r8&2|0)!=0?r4:2);if((r7|0)<=1){return}r7=r2+54|0;r8=r3;r10=r1+24|0;while(1){r1=HEAP32[r10+4>>2];r9=r1>>8;if((r1&1|0)==0){r11=r9}else{r11=HEAP32[HEAP32[r8>>2]+r9>>2]}r9=HEAP32[r10>>2];FUNCTION_TABLE[HEAP32[HEAP32[r9>>2]+28>>2]](r9,r2,r3+r11|0,(r1&2|0)!=0?r4:2);if((HEAP8[r7]&1)<<24>>24!=0){r5=2267;break}r1=r10+8|0;if(r1>>>0<r6>>>0){r10=r1}else{r5=2272;break}}if(r5==2272){return}else if(r5==2267){return}}function ___dynamic_cast(r1,r2,r3,r4){var r5,r6,r7,r8,r9,r10,r11,r12,r13,r14;r5=STACKTOP;STACKTOP=STACKTOP+56|0;r6=r5,r7=r6>>2;r8=HEAP32[r1>>2];r9=r1+HEAP32[r8-8>>2]|0;r10=HEAP32[r8-4>>2];r8=r10;HEAP32[r7]=r3;HEAP32[r7+1]=r1;HEAP32[r7+2]=r2;HEAP32[r7+3]=r4;r4=r6+16|0;r2=r6+20|0;r1=r6+24|0;r11=r6+28|0;r12=r6+32|0;r13=r6+40|0;_memset(r4,0,39);if((r10|0)==(r3|0)){HEAP32[r7+12]=1;FUNCTION_TABLE[HEAP32[HEAP32[r10>>2]+20>>2]](r8,r6,r9,r9,1,0);STACKTOP=r5;return(HEAP32[r1>>2]|0)==1?r9:0}FUNCTION_TABLE[HEAP32[HEAP32[r10>>2]+24>>2]](r8,r6,r9,1,0);r9=HEAP32[r7+9];do{if((r9|0)==0){if((HEAP32[r13>>2]|0)!=1){r14=0;break}if((HEAP32[r11>>2]|0)!=1){r14=0;break}r14=(HEAP32[r12>>2]|0)==1?HEAP32[r2>>2]:0}else if((r9|0)==1){if((HEAP32[r1>>2]|0)!=1){if((HEAP32[r13>>2]|0)!=0){r14=0;break}if((HEAP32[r11>>2]|0)!=1){r14=0;break}if((HEAP32[r12>>2]|0)!=1){r14=0;break}}r14=HEAP32[r4>>2]}else{r14=0}}while(0);STACKTOP=r5;return r14}function __ZNK10__cxxabiv117__class_type_info16search_below_dstEPNS_19__dynamic_cast_infoEPKvib(r1,r2,r3,r4,r5){var r6;r5=r2>>2;if((HEAP32[r5+2]|0)==(r1|0)){if((HEAP32[r5+1]|0)!=(r3|0)){return}r6=r2+28|0;if((HEAP32[r6>>2]|0)==1){return}HEAP32[r6>>2]=r4;return}if((HEAP32[r5]|0)!=(r1|0)){return}do{if((HEAP32[r5+4]|0)!=(r3|0)){r1=r2+20|0;if((HEAP32[r1>>2]|0)==(r3|0)){break}HEAP32[r5+8]=r4;HEAP32[r1>>2]=r3;r1=r2+40|0;HEAP32[r1>>2]=HEAP32[r1>>2]+1|0;do{if((HEAP32[r5+9]|0)==1){if((HEAP32[r5+6]|0)!=2){break}HEAP8[r2+54|0]=1}}while(0);HEAP32[r5+11]=4;return}}while(0);if((r4|0)!=1){return}HEAP32[r5+8]=1;return}function __ZNK10__cxxabiv117__class_type_info16search_above_dstEPNS_19__dynamic_cast_infoEPKvS4_ib(r1,r2,r3,r4,r5,r6){var r7;r6=r2>>2;if((HEAP32[r6+2]|0)!=(r1|0)){return}HEAP8[r2+53|0]=1;if((HEAP32[r6+1]|0)!=(r4|0)){return}HEAP8[r2+52|0]=1;r4=r2+16|0;r1=HEAP32[r4>>2];if((r1|0)==0){HEAP32[r4>>2]=r3;HEAP32[r6+6]=r5;HEAP32[r6+9]=1;if(!((HEAP32[r6+12]|0)==1&(r5|0)==1)){return}HEAP8[r2+54|0]=1;return}if((r1|0)!=(r3|0)){r3=r2+36|0;HEAP32[r3>>2]=HEAP32[r3>>2]+1|0;HEAP8[r2+54|0]=1;return}r3=r2+24|0;r1=HEAP32[r3>>2];if((r1|0)==2){HEAP32[r3>>2]=r5;r7=r5}else{r7=r1}if(!((HEAP32[r6+12]|0)==1&(r7|0)==1)){return}HEAP8[r2+54|0]=1;return}function __ZNK10__cxxabiv121__vmi_class_type_info16search_below_dstEPNS_19__dynamic_cast_infoEPKvib(r1,r2,r3,r4,r5){var r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34;r6=r2>>2;r7=r1>>2;r8=0;r9=r1|0;if((r9|0)==(HEAP32[r6+2]|0)){if((HEAP32[r6+1]|0)!=(r3|0)){return}r10=r2+28|0;if((HEAP32[r10>>2]|0)==1){return}HEAP32[r10>>2]=r4;return}if((r9|0)==(HEAP32[r6]|0)){do{if((HEAP32[r6+4]|0)!=(r3|0)){r9=r2+20|0;if((HEAP32[r9>>2]|0)==(r3|0)){break}HEAP32[r6+8]=r4;r10=(r2+44|0)>>2;if((HEAP32[r10]|0)==4){return}r11=HEAP32[r7+3];r12=(r11<<3)+r1+16|0;L2911:do{if((r11|0)>0){r13=r2+52|0;r14=r2+53|0;r15=r2+54|0;r16=r1+8|0;r17=r2+24|0;r18=r3;r19=0;r20=r1+16|0;r21=0;L2913:while(1){HEAP8[r13]=0;HEAP8[r14]=0;r22=HEAP32[r20+4>>2];r23=r22>>8;if((r22&1|0)==0){r24=r23}else{r24=HEAP32[HEAP32[r18>>2]+r23>>2]}r23=HEAP32[r20>>2];FUNCTION_TABLE[HEAP32[HEAP32[r23>>2]+20>>2]](r23,r2,r3,r3+r24|0,2-(r22>>>1&1)|0,r5);if((HEAP8[r15]&1)<<24>>24!=0){r25=r21;r26=r19;break}do{if((HEAP8[r14]&1)<<24>>24==0){r27=r21;r28=r19}else{if((HEAP8[r13]&1)<<24>>24==0){if((HEAP32[r16>>2]&1|0)==0){r25=1;r26=r19;break L2913}else{r27=1;r28=r19;break}}if((HEAP32[r17>>2]|0)==1){break L2911}if((HEAP32[r16>>2]&2|0)==0){break L2911}else{r27=1;r28=1}}}while(0);r22=r20+8|0;if(r22>>>0<r12>>>0){r19=r28;r20=r22;r21=r27}else{r25=r27;r26=r28;break}}if((r26&1)<<24>>24==0){r29=r25;r8=2348;break}else{r30=r25;r8=2351;break}}else{r29=0;r8=2348}}while(0);do{if(r8==2348){HEAP32[r9>>2]=r3;r12=r2+40|0;HEAP32[r12>>2]=HEAP32[r12>>2]+1|0;if((HEAP32[r6+9]|0)!=1){r30=r29;r8=2351;break}if((HEAP32[r6+6]|0)!=2){r30=r29;r8=2351;break}HEAP8[r2+54|0]=1;r30=r29;r8=2351;break}}while(0);do{if(r8==2351){if((r30&1)<<24>>24!=0){break}HEAP32[r10]=4;return}}while(0);HEAP32[r10]=3;return}}while(0);if((r4|0)!=1){return}HEAP32[r6+8]=1;return}r6=HEAP32[r7+3];r30=(r6<<3)+r1+16|0;r29=HEAP32[r7+5];r25=r29>>8;if((r29&1|0)==0){r31=r25}else{r31=HEAP32[HEAP32[r3>>2]+r25>>2]}r25=HEAP32[r7+4];FUNCTION_TABLE[HEAP32[HEAP32[r25>>2]+24>>2]](r25,r2,r3+r31|0,(r29&2|0)!=0?r4:2,r5);r29=r1+24|0;if((r6|0)<=1){return}r6=HEAP32[r7+2];do{if((r6&2|0)==0){r7=(r2+36|0)>>2;if((HEAP32[r7]|0)==1){break}if((r6&1|0)==0){r1=r2+54|0;r31=r3;r25=r29;while(1){if((HEAP8[r1]&1)<<24>>24!=0){r8=2386;break}if((HEAP32[r7]|0)==1){r8=2379;break}r26=HEAP32[r25+4>>2];r28=r26>>8;if((r26&1|0)==0){r32=r28}else{r32=HEAP32[HEAP32[r31>>2]+r28>>2]}r28=HEAP32[r25>>2];FUNCTION_TABLE[HEAP32[HEAP32[r28>>2]+24>>2]](r28,r2,r3+r32|0,(r26&2|0)!=0?r4:2,r5);r26=r25+8|0;if(r26>>>0<r30>>>0){r25=r26}else{r8=2388;break}}if(r8==2386){return}else if(r8==2388){return}else if(r8==2379){return}}r25=r2+24|0;r31=r2+54|0;r1=r3;r10=r29;while(1){if((HEAP8[r31]&1)<<24>>24!=0){r8=2384;break}if((HEAP32[r7]|0)==1){if((HEAP32[r25>>2]|0)==1){r8=2393;break}}r26=HEAP32[r10+4>>2];r28=r26>>8;if((r26&1|0)==0){r33=r28}else{r33=HEAP32[HEAP32[r1>>2]+r28>>2]}r28=HEAP32[r10>>2];FUNCTION_TABLE[HEAP32[HEAP32[r28>>2]+24>>2]](r28,r2,r3+r33|0,(r26&2|0)!=0?r4:2,r5);r26=r10+8|0;if(r26>>>0<r30>>>0){r10=r26}else{r8=2381;break}}if(r8==2384){return}else if(r8==2393){return}else if(r8==2381){return}}}while(0);r33=r2+54|0;r32=r3;r6=r29;while(1){if((HEAP8[r33]&1)<<24>>24!=0){r8=2389;break}r29=HEAP32[r6+4>>2];r10=r29>>8;if((r29&1|0)==0){r34=r10}else{r34=HEAP32[HEAP32[r32>>2]+r10>>2]}r10=HEAP32[r6>>2];FUNCTION_TABLE[HEAP32[HEAP32[r10>>2]+24>>2]](r10,r2,r3+r34|0,(r29&2|0)!=0?r4:2,r5);r29=r6+8|0;if(r29>>>0<r30>>>0){r6=r29}else{r8=2391;break}}if(r8==2389){return}else if(r8==2391){return}}function __ZNK10__cxxabiv120__si_class_type_info16search_below_dstEPNS_19__dynamic_cast_infoEPKvib(r1,r2,r3,r4,r5){var r6,r7,r8,r9,r10,r11,r12,r13;r6=r2>>2;r7=0;r8=r1|0;if((r8|0)==(HEAP32[r6+2]|0)){if((HEAP32[r6+1]|0)!=(r3|0)){return}r9=r2+28|0;if((HEAP32[r9>>2]|0)==1){return}HEAP32[r9>>2]=r4;return}if((r8|0)!=(HEAP32[r6]|0)){r8=HEAP32[r1+8>>2];FUNCTION_TABLE[HEAP32[HEAP32[r8>>2]+24>>2]](r8,r2,r3,r4,r5);return}do{if((HEAP32[r6+4]|0)!=(r3|0)){r8=r2+20|0;if((HEAP32[r8>>2]|0)==(r3|0)){break}HEAP32[r6+8]=r4;r9=(r2+44|0)>>2;if((HEAP32[r9]|0)==4){return}r10=r2+52|0;HEAP8[r10]=0;r11=r2+53|0;HEAP8[r11]=0;r12=HEAP32[r1+8>>2];FUNCTION_TABLE[HEAP32[HEAP32[r12>>2]+20>>2]](r12,r2,r3,r3,1,r5);do{if((HEAP8[r11]&1)<<24>>24==0){r13=0;r7=2408}else{if((HEAP8[r10]&1)<<24>>24==0){r13=1;r7=2408;break}else{break}}}while(0);L3012:do{if(r7==2408){HEAP32[r8>>2]=r3;r10=r2+40|0;HEAP32[r10>>2]=HEAP32[r10>>2]+1|0;do{if((HEAP32[r6+9]|0)==1){if((HEAP32[r6+6]|0)!=2){r7=2411;break}HEAP8[r2+54|0]=1;if(r13){break L3012}else{break}}else{r7=2411}}while(0);if(r7==2411){if(r13){break}}HEAP32[r9]=4;return}}while(0);HEAP32[r9]=3;return}}while(0);if((r4|0)!=1){return}HEAP32[r6+8]=1;return}function __ZNK10__cxxabiv121__vmi_class_type_info16search_above_dstEPNS_19__dynamic_cast_infoEPKvS4_ib(r1,r2,r3,r4,r5,r6){var r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22;r7=r2>>2;if((r1|0)!=(HEAP32[r7+2]|0)){r8=r2+52|0;r9=HEAP8[r8]&1;r10=r2+53|0;r11=HEAP8[r10]&1;r12=HEAP32[r1+12>>2];r13=(r12<<3)+r1+16|0;HEAP8[r8]=0;HEAP8[r10]=0;r14=HEAP32[r1+20>>2];r15=r14>>8;if((r14&1|0)==0){r16=r15}else{r16=HEAP32[HEAP32[r4>>2]+r15>>2]}r15=HEAP32[r1+16>>2];FUNCTION_TABLE[HEAP32[HEAP32[r15>>2]+20>>2]](r15,r2,r3,r4+r16|0,(r14&2|0)!=0?r5:2,r6);L3034:do{if((r12|0)>1){r14=r2+24|0;r16=r1+8|0;r15=r2+54|0;r17=r4;r18=r1+24|0;while(1){if((HEAP8[r15]&1)<<24>>24!=0){break L3034}do{if((HEAP8[r8]&1)<<24>>24==0){if((HEAP8[r10]&1)<<24>>24==0){break}if((HEAP32[r16>>2]&1|0)==0){break L3034}}else{if((HEAP32[r14>>2]|0)==1){break L3034}if((HEAP32[r16>>2]&2|0)==0){break L3034}}}while(0);HEAP8[r8]=0;HEAP8[r10]=0;r19=HEAP32[r18+4>>2];r20=r19>>8;if((r19&1|0)==0){r21=r20}else{r21=HEAP32[HEAP32[r17>>2]+r20>>2]}r20=HEAP32[r18>>2];FUNCTION_TABLE[HEAP32[HEAP32[r20>>2]+20>>2]](r20,r2,r3,r4+r21|0,(r19&2|0)!=0?r5:2,r6);r19=r18+8|0;if(r19>>>0<r13>>>0){r18=r19}else{break L3034}}}}while(0);HEAP8[r8]=r9;HEAP8[r10]=r11;return}HEAP8[r2+53|0]=1;if((HEAP32[r7+1]|0)!=(r4|0)){return}HEAP8[r2+52|0]=1;r4=r2+16|0;r11=HEAP32[r4>>2];if((r11|0)==0){HEAP32[r4>>2]=r3;HEAP32[r7+6]=r5;HEAP32[r7+9]=1;if(!((HEAP32[r7+12]|0)==1&(r5|0)==1)){return}HEAP8[r2+54|0]=1;return}if((r11|0)!=(r3|0)){r3=r2+36|0;HEAP32[r3>>2]=HEAP32[r3>>2]+1|0;HEAP8[r2+54|0]=1;return}r3=r2+24|0;r11=HEAP32[r3>>2];if((r11|0)==2){HEAP32[r3>>2]=r5;r22=r5}else{r22=r11}if(!((HEAP32[r7+12]|0)==1&(r22|0)==1)){return}HEAP8[r2+54|0]=1;return}function __ZNK10__cxxabiv120__si_class_type_info16search_above_dstEPNS_19__dynamic_cast_infoEPKvS4_ib(r1,r2,r3,r4,r5,r6){var r7,r8,r9;r7=r2>>2;if((r1|0)!=(HEAP32[r7+2]|0)){r8=HEAP32[r1+8>>2];FUNCTION_TABLE[HEAP32[HEAP32[r8>>2]+20>>2]](r8,r2,r3,r4,r5,r6);return}HEAP8[r2+53|0]=1;if((HEAP32[r7+1]|0)!=(r4|0)){return}HEAP8[r2+52|0]=1;r4=r2+16|0;r6=HEAP32[r4>>2];if((r6|0)==0){HEAP32[r4>>2]=r3;HEAP32[r7+6]=r5;HEAP32[r7+9]=1;if(!((HEAP32[r7+12]|0)==1&(r5|0)==1)){return}HEAP8[r2+54|0]=1;return}if((r6|0)!=(r3|0)){r3=r2+36|0;HEAP32[r3>>2]=HEAP32[r3>>2]+1|0;HEAP8[r2+54|0]=1;return}r3=r2+24|0;r6=HEAP32[r3>>2];if((r6|0)==2){HEAP32[r3>>2]=r5;r9=r5}else{r9=r6}if(!((HEAP32[r7+12]|0)==1&(r9|0)==1)){return}HEAP8[r2+54|0]=1;return}function _malloc(r1){var r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46,r47,r48,r49,r50,r51,r52,r53,r54,r55,r56,r57,r58,r59,r60,r61,r62,r63,r64,r65,r66,r67,r68,r69,r70,r71,r72,r73,r74,r75,r76,r77,r78,r79,r80,r81,r82,r83,r84,r85,r86,r87,r88,r89,r90,r91,r92,r93,r94,r95;r2=0;do{if(r1>>>0<245){if(r1>>>0<11){r3=16}else{r3=r1+11&-8}r4=r3>>>3;r5=HEAP32[1311158];r6=r5>>>(r4>>>0);if((r6&3|0)!=0){r7=(r6&1^1)+r4|0;r8=r7<<1;r9=(r8<<2)+5244672|0;r10=(r8+2<<2)+5244672|0;r8=HEAP32[r10>>2];r11=r8+8|0;r12=HEAP32[r11>>2];do{if((r9|0)==(r12|0)){HEAP32[1311158]=r5&(1<<r7^-1)}else{if(r12>>>0<HEAP32[1311162]>>>0){_abort()}r13=r12+12|0;if((HEAP32[r13>>2]|0)==(r8|0)){HEAP32[r13>>2]=r9;HEAP32[r10>>2]=r12;break}else{_abort()}}}while(0);r12=r7<<3;HEAP32[r8+4>>2]=r12|3;r10=r8+(r12|4)|0;HEAP32[r10>>2]=HEAP32[r10>>2]|1;r14=r11;return r14}if(r3>>>0<=HEAP32[1311160]>>>0){r15=r3,r16=r15>>2;break}if((r6|0)!=0){r10=2<<r4;r12=r6<<r4&(r10|-r10);r10=(r12&-r12)-1|0;r12=r10>>>12&16;r9=r10>>>(r12>>>0);r10=r9>>>5&8;r13=r9>>>(r10>>>0);r9=r13>>>2&4;r17=r13>>>(r9>>>0);r13=r17>>>1&2;r18=r17>>>(r13>>>0);r17=r18>>>1&1;r19=(r10|r12|r9|r13|r17)+(r18>>>(r17>>>0))|0;r17=r19<<1;r18=(r17<<2)+5244672|0;r13=(r17+2<<2)+5244672|0;r17=HEAP32[r13>>2];r9=r17+8|0;r12=HEAP32[r9>>2];do{if((r18|0)==(r12|0)){HEAP32[1311158]=r5&(1<<r19^-1)}else{if(r12>>>0<HEAP32[1311162]>>>0){_abort()}r10=r12+12|0;if((HEAP32[r10>>2]|0)==(r17|0)){HEAP32[r10>>2]=r18;HEAP32[r13>>2]=r12;break}else{_abort()}}}while(0);r12=r19<<3;r13=r12-r3|0;HEAP32[r17+4>>2]=r3|3;r18=r17;r5=r18+r3|0;HEAP32[r18+(r3|4)>>2]=r13|1;HEAP32[r18+r12>>2]=r13;r12=HEAP32[1311160];if((r12|0)!=0){r18=HEAP32[1311163];r4=r12>>>3;r12=r4<<1;r6=(r12<<2)+5244672|0;r11=HEAP32[1311158];r8=1<<r4;do{if((r11&r8|0)==0){HEAP32[1311158]=r11|r8;r20=r6;r21=(r12+2<<2)+5244672|0}else{r4=(r12+2<<2)+5244672|0;r7=HEAP32[r4>>2];if(r7>>>0>=HEAP32[1311162]>>>0){r20=r7;r21=r4;break}_abort()}}while(0);HEAP32[r21>>2]=r18;HEAP32[r20+12>>2]=r18;HEAP32[r18+8>>2]=r20;HEAP32[r18+12>>2]=r6}HEAP32[1311160]=r13;HEAP32[1311163]=r5;r14=r9;return r14}r12=HEAP32[1311159];if((r12|0)==0){r15=r3,r16=r15>>2;break}r8=(r12&-r12)-1|0;r12=r8>>>12&16;r11=r8>>>(r12>>>0);r8=r11>>>5&8;r17=r11>>>(r8>>>0);r11=r17>>>2&4;r19=r17>>>(r11>>>0);r17=r19>>>1&2;r4=r19>>>(r17>>>0);r19=r4>>>1&1;r7=HEAP32[((r8|r12|r11|r17|r19)+(r4>>>(r19>>>0))<<2)+5244936>>2];r19=r7;r4=r7,r17=r4>>2;r11=(HEAP32[r7+4>>2]&-8)-r3|0;while(1){r7=HEAP32[r19+16>>2];if((r7|0)==0){r12=HEAP32[r19+20>>2];if((r12|0)==0){break}else{r22=r12}}else{r22=r7}r7=(HEAP32[r22+4>>2]&-8)-r3|0;r12=r7>>>0<r11>>>0;r19=r22;r4=r12?r22:r4,r17=r4>>2;r11=r12?r7:r11}r19=r4;r9=HEAP32[1311162];if(r19>>>0<r9>>>0){_abort()}r5=r19+r3|0;r13=r5;if(r19>>>0>=r5>>>0){_abort()}r5=HEAP32[r17+6];r6=HEAP32[r17+3];L56:do{if((r6|0)==(r4|0)){r18=r4+20|0;r7=HEAP32[r18>>2];do{if((r7|0)==0){r12=r4+16|0;r8=HEAP32[r12>>2];if((r8|0)==0){r23=0,r24=r23>>2;break L56}else{r25=r8;r26=r12;break}}else{r25=r7;r26=r18}}while(0);while(1){r18=r25+20|0;r7=HEAP32[r18>>2];if((r7|0)!=0){r25=r7;r26=r18;continue}r18=r25+16|0;r7=HEAP32[r18>>2];if((r7|0)==0){break}else{r25=r7;r26=r18}}if(r26>>>0<r9>>>0){_abort()}else{HEAP32[r26>>2]=0;r23=r25,r24=r23>>2;break}}else{r18=HEAP32[r17+2];if(r18>>>0<r9>>>0){_abort()}r7=r18+12|0;if((HEAP32[r7>>2]|0)!=(r4|0)){_abort()}r12=r6+8|0;if((HEAP32[r12>>2]|0)==(r4|0)){HEAP32[r7>>2]=r6;HEAP32[r12>>2]=r18;r23=r6,r24=r23>>2;break}else{_abort()}}}while(0);L78:do{if((r5|0)!=0){r6=r4+28|0;r9=(HEAP32[r6>>2]<<2)+5244936|0;do{if((r4|0)==(HEAP32[r9>>2]|0)){HEAP32[r9>>2]=r23;if((r23|0)!=0){break}HEAP32[1311159]=HEAP32[1311159]&(1<<HEAP32[r6>>2]^-1);break L78}else{if(r5>>>0<HEAP32[1311162]>>>0){_abort()}r18=r5+16|0;if((HEAP32[r18>>2]|0)==(r4|0)){HEAP32[r18>>2]=r23}else{HEAP32[r5+20>>2]=r23}if((r23|0)==0){break L78}}}while(0);if(r23>>>0<HEAP32[1311162]>>>0){_abort()}HEAP32[r24+6]=r5;r6=HEAP32[r17+4];do{if((r6|0)!=0){if(r6>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r24+4]=r6;HEAP32[r6+24>>2]=r23;break}}}while(0);r6=HEAP32[r17+5];if((r6|0)==0){break}if(r6>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r24+5]=r6;HEAP32[r6+24>>2]=r23;break}}}while(0);if(r11>>>0<16){r5=r11+r3|0;HEAP32[r17+1]=r5|3;r6=r5+(r19+4)|0;HEAP32[r6>>2]=HEAP32[r6>>2]|1}else{HEAP32[r17+1]=r3|3;HEAP32[r19+(r3|4)>>2]=r11|1;HEAP32[r19+r11+r3>>2]=r11;r6=HEAP32[1311160];if((r6|0)!=0){r5=HEAP32[1311163];r9=r6>>>3;r6=r9<<1;r18=(r6<<2)+5244672|0;r12=HEAP32[1311158];r7=1<<r9;do{if((r12&r7|0)==0){HEAP32[1311158]=r12|r7;r27=r18;r28=(r6+2<<2)+5244672|0}else{r9=(r6+2<<2)+5244672|0;r8=HEAP32[r9>>2];if(r8>>>0>=HEAP32[1311162]>>>0){r27=r8;r28=r9;break}_abort()}}while(0);HEAP32[r28>>2]=r5;HEAP32[r27+12>>2]=r5;HEAP32[r5+8>>2]=r27;HEAP32[r5+12>>2]=r18}HEAP32[1311160]=r11;HEAP32[1311163]=r13}r6=r4+8|0;if((r6|0)==0){r15=r3,r16=r15>>2;break}else{r14=r6}return r14}else{if(r1>>>0>4294967231){r15=-1,r16=r15>>2;break}r6=r1+11|0;r7=r6&-8,r12=r7>>2;r19=HEAP32[1311159];if((r19|0)==0){r15=r7,r16=r15>>2;break}r17=-r7|0;r9=r6>>>8;do{if((r9|0)==0){r29=0}else{if(r7>>>0>16777215){r29=31;break}r6=(r9+1048320|0)>>>16&8;r8=r9<<r6;r10=(r8+520192|0)>>>16&4;r30=r8<<r10;r8=(r30+245760|0)>>>16&2;r31=14-(r10|r6|r8)+(r30<<r8>>>15)|0;r29=r7>>>((r31+7|0)>>>0)&1|r31<<1}}while(0);r9=HEAP32[(r29<<2)+5244936>>2];L126:do{if((r9|0)==0){r32=0;r33=r17;r34=0}else{if((r29|0)==31){r35=0}else{r35=25-(r29>>>1)|0}r4=0;r13=r17;r11=r9,r18=r11>>2;r5=r7<<r35;r31=0;while(1){r8=HEAP32[r18+1]&-8;r30=r8-r7|0;if(r30>>>0<r13>>>0){if((r8|0)==(r7|0)){r32=r11;r33=r30;r34=r11;break L126}else{r36=r11;r37=r30}}else{r36=r4;r37=r13}r30=HEAP32[r18+5];r8=HEAP32[((r5>>>31<<2)+16>>2)+r18];r6=(r30|0)==0|(r30|0)==(r8|0)?r31:r30;if((r8|0)==0){r32=r36;r33=r37;r34=r6;break L126}else{r4=r36;r13=r37;r11=r8,r18=r11>>2;r5=r5<<1;r31=r6}}}}while(0);if((r34|0)==0&(r32|0)==0){r9=2<<r29;r17=r19&(r9|-r9);if((r17|0)==0){r15=r7,r16=r15>>2;break}r9=(r17&-r17)-1|0;r17=r9>>>12&16;r31=r9>>>(r17>>>0);r9=r31>>>5&8;r5=r31>>>(r9>>>0);r31=r5>>>2&4;r11=r5>>>(r31>>>0);r5=r11>>>1&2;r18=r11>>>(r5>>>0);r11=r18>>>1&1;r38=HEAP32[((r9|r17|r31|r5|r11)+(r18>>>(r11>>>0))<<2)+5244936>>2]}else{r38=r34}L141:do{if((r38|0)==0){r39=r33;r40=r32,r41=r40>>2}else{r11=r38,r18=r11>>2;r5=r33;r31=r32;while(1){r17=(HEAP32[r18+1]&-8)-r7|0;r9=r17>>>0<r5>>>0;r13=r9?r17:r5;r17=r9?r11:r31;r9=HEAP32[r18+4];if((r9|0)!=0){r11=r9,r18=r11>>2;r5=r13;r31=r17;continue}r9=HEAP32[r18+5];if((r9|0)==0){r39=r13;r40=r17,r41=r40>>2;break L141}else{r11=r9,r18=r11>>2;r5=r13;r31=r17}}}}while(0);if((r40|0)==0){r15=r7,r16=r15>>2;break}if(r39>>>0>=(HEAP32[1311160]-r7|0)>>>0){r15=r7,r16=r15>>2;break}r19=r40,r31=r19>>2;r5=HEAP32[1311162];if(r19>>>0<r5>>>0){_abort()}r11=r19+r7|0;r18=r11;if(r19>>>0>=r11>>>0){_abort()}r17=HEAP32[r41+6];r13=HEAP32[r41+3];L154:do{if((r13|0)==(r40|0)){r9=r40+20|0;r4=HEAP32[r9>>2];do{if((r4|0)==0){r6=r40+16|0;r8=HEAP32[r6>>2];if((r8|0)==0){r42=0,r43=r42>>2;break L154}else{r44=r8;r45=r6;break}}else{r44=r4;r45=r9}}while(0);while(1){r9=r44+20|0;r4=HEAP32[r9>>2];if((r4|0)!=0){r44=r4;r45=r9;continue}r9=r44+16|0;r4=HEAP32[r9>>2];if((r4|0)==0){break}else{r44=r4;r45=r9}}if(r45>>>0<r5>>>0){_abort()}else{HEAP32[r45>>2]=0;r42=r44,r43=r42>>2;break}}else{r9=HEAP32[r41+2];if(r9>>>0<r5>>>0){_abort()}r4=r9+12|0;if((HEAP32[r4>>2]|0)!=(r40|0)){_abort()}r6=r13+8|0;if((HEAP32[r6>>2]|0)==(r40|0)){HEAP32[r4>>2]=r13;HEAP32[r6>>2]=r9;r42=r13,r43=r42>>2;break}else{_abort()}}}while(0);L176:do{if((r17|0)!=0){r13=r40+28|0;r5=(HEAP32[r13>>2]<<2)+5244936|0;do{if((r40|0)==(HEAP32[r5>>2]|0)){HEAP32[r5>>2]=r42;if((r42|0)!=0){break}HEAP32[1311159]=HEAP32[1311159]&(1<<HEAP32[r13>>2]^-1);break L176}else{if(r17>>>0<HEAP32[1311162]>>>0){_abort()}r9=r17+16|0;if((HEAP32[r9>>2]|0)==(r40|0)){HEAP32[r9>>2]=r42}else{HEAP32[r17+20>>2]=r42}if((r42|0)==0){break L176}}}while(0);if(r42>>>0<HEAP32[1311162]>>>0){_abort()}HEAP32[r43+6]=r17;r13=HEAP32[r41+4];do{if((r13|0)!=0){if(r13>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r43+4]=r13;HEAP32[r13+24>>2]=r42;break}}}while(0);r13=HEAP32[r41+5];if((r13|0)==0){break}if(r13>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r43+5]=r13;HEAP32[r13+24>>2]=r42;break}}}while(0);do{if(r39>>>0<16){r17=r39+r7|0;HEAP32[r41+1]=r17|3;r13=r17+(r19+4)|0;HEAP32[r13>>2]=HEAP32[r13>>2]|1}else{HEAP32[r41+1]=r7|3;HEAP32[((r7|4)>>2)+r31]=r39|1;HEAP32[(r39>>2)+r31+r12]=r39;r13=r39>>>3;if(r39>>>0<256){r17=r13<<1;r5=(r17<<2)+5244672|0;r9=HEAP32[1311158];r6=1<<r13;do{if((r9&r6|0)==0){HEAP32[1311158]=r9|r6;r46=r5;r47=(r17+2<<2)+5244672|0}else{r13=(r17+2<<2)+5244672|0;r4=HEAP32[r13>>2];if(r4>>>0>=HEAP32[1311162]>>>0){r46=r4;r47=r13;break}_abort()}}while(0);HEAP32[r47>>2]=r18;HEAP32[r46+12>>2]=r18;HEAP32[r12+(r31+2)]=r46;HEAP32[r12+(r31+3)]=r5;break}r17=r11;r6=r39>>>8;do{if((r6|0)==0){r48=0}else{if(r39>>>0>16777215){r48=31;break}r9=(r6+1048320|0)>>>16&8;r13=r6<<r9;r4=(r13+520192|0)>>>16&4;r8=r13<<r4;r13=(r8+245760|0)>>>16&2;r30=14-(r4|r9|r13)+(r8<<r13>>>15)|0;r48=r39>>>((r30+7|0)>>>0)&1|r30<<1}}while(0);r6=(r48<<2)+5244936|0;HEAP32[r12+(r31+7)]=r48;HEAP32[r12+(r31+5)]=0;HEAP32[r12+(r31+4)]=0;r5=HEAP32[1311159];r30=1<<r48;if((r5&r30|0)==0){HEAP32[1311159]=r5|r30;HEAP32[r6>>2]=r17;HEAP32[r12+(r31+6)]=r6;HEAP32[r12+(r31+3)]=r17;HEAP32[r12+(r31+2)]=r17;break}if((r48|0)==31){r49=0}else{r49=25-(r48>>>1)|0}r30=r39<<r49;r5=HEAP32[r6>>2];while(1){if((HEAP32[r5+4>>2]&-8|0)==(r39|0)){break}r50=(r30>>>31<<2)+r5+16|0;r6=HEAP32[r50>>2];if((r6|0)==0){r2=151;break}else{r30=r30<<1;r5=r6}}if(r2==151){if(r50>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r50>>2]=r17;HEAP32[r12+(r31+6)]=r5;HEAP32[r12+(r31+3)]=r17;HEAP32[r12+(r31+2)]=r17;break}}r30=r5+8|0;r6=HEAP32[r30>>2];r13=HEAP32[1311162];if(r5>>>0<r13>>>0){_abort()}if(r6>>>0<r13>>>0){_abort()}else{HEAP32[r6+12>>2]=r17;HEAP32[r30>>2]=r17;HEAP32[r12+(r31+2)]=r6;HEAP32[r12+(r31+3)]=r5;HEAP32[r12+(r31+6)]=0;break}}}while(0);r31=r40+8|0;if((r31|0)==0){r15=r7,r16=r15>>2;break}else{r14=r31}return r14}}while(0);r40=HEAP32[1311160];if(r15>>>0<=r40>>>0){r50=r40-r15|0;r39=HEAP32[1311163];if(r50>>>0>15){r49=r39;HEAP32[1311163]=r49+r15|0;HEAP32[1311160]=r50;HEAP32[(r49+4>>2)+r16]=r50|1;HEAP32[r49+r40>>2]=r50;HEAP32[r39+4>>2]=r15|3}else{HEAP32[1311160]=0;HEAP32[1311163]=0;HEAP32[r39+4>>2]=r40|3;r50=r40+(r39+4)|0;HEAP32[r50>>2]=HEAP32[r50>>2]|1}r14=r39+8|0;return r14}r39=HEAP32[1311161];if(r15>>>0<r39>>>0){r50=r39-r15|0;HEAP32[1311161]=r50;r39=HEAP32[1311164];r40=r39;HEAP32[1311164]=r40+r15|0;HEAP32[(r40+4>>2)+r16]=r50|1;HEAP32[r39+4>>2]=r15|3;r14=r39+8|0;return r14}do{if((HEAP32[1310738]|0)==0){r39=_sysconf(8);if((r39-1&r39|0)==0){HEAP32[1310740]=r39;HEAP32[1310739]=r39;HEAP32[1310741]=-1;HEAP32[1310742]=2097152;HEAP32[1310743]=0;HEAP32[1311269]=0;HEAP32[1310738]=_time(0)&-16^1431655768;break}else{_abort()}}}while(0);r39=r15+48|0;r50=HEAP32[1310740];r40=r15+47|0;r49=r50+r40|0;r48=-r50|0;r50=r49&r48;if(r50>>>0<=r15>>>0){r14=0;return r14}r46=HEAP32[1311268];do{if((r46|0)!=0){r47=HEAP32[1311266];r41=r47+r50|0;if(r41>>>0<=r47>>>0|r41>>>0>r46>>>0){r14=0}else{break}return r14}}while(0);L268:do{if((HEAP32[1311269]&4|0)==0){r46=HEAP32[1311164];L270:do{if((r46|0)==0){r2=181}else{r41=r46;r47=5245080;while(1){r51=r47|0;r42=HEAP32[r51>>2];if(r42>>>0<=r41>>>0){r52=r47+4|0;if((r42+HEAP32[r52>>2]|0)>>>0>r41>>>0){break}}r42=HEAP32[r47+8>>2];if((r42|0)==0){r2=181;break L270}else{r47=r42}}if((r47|0)==0){r2=181;break}r41=r49-HEAP32[1311161]&r48;if(r41>>>0>=2147483647){r53=0;break}r5=_sbrk(r41);r17=(r5|0)==(HEAP32[r51>>2]+HEAP32[r52>>2]|0);r54=r17?r5:-1;r55=r17?r41:0;r56=r5;r57=r41;r2=190;break}}while(0);do{if(r2==181){r46=_sbrk(0);if((r46|0)==-1){r53=0;break}r7=r46;r41=HEAP32[1310739];r5=r41-1|0;if((r5&r7|0)==0){r58=r50}else{r58=r50-r7+(r5+r7&-r41)|0}r41=HEAP32[1311266];r7=r41+r58|0;if(!(r58>>>0>r15>>>0&r58>>>0<2147483647)){r53=0;break}r5=HEAP32[1311268];if((r5|0)!=0){if(r7>>>0<=r41>>>0|r7>>>0>r5>>>0){r53=0;break}}r5=_sbrk(r58);r7=(r5|0)==(r46|0);r54=r7?r46:-1;r55=r7?r58:0;r56=r5;r57=r58;r2=190;break}}while(0);L290:do{if(r2==190){r5=-r57|0;if((r54|0)!=-1){r59=r55,r60=r59>>2;r61=r54,r62=r61>>2;r2=201;break L268}do{if((r56|0)!=-1&r57>>>0<2147483647&r57>>>0<r39>>>0){r7=HEAP32[1310740];r46=r40-r57+r7&-r7;if(r46>>>0>=2147483647){r63=r57;break}if((_sbrk(r46)|0)==-1){_sbrk(r5);r53=r55;break L290}else{r63=r46+r57|0;break}}else{r63=r57}}while(0);if((r56|0)==-1){r53=r55}else{r59=r63,r60=r59>>2;r61=r56,r62=r61>>2;r2=201;break L268}}}while(0);HEAP32[1311269]=HEAP32[1311269]|4;r64=r53;r2=198;break}else{r64=0;r2=198}}while(0);do{if(r2==198){if(r50>>>0>=2147483647){break}r53=_sbrk(r50);r56=_sbrk(0);if(!((r56|0)!=-1&(r53|0)!=-1&r53>>>0<r56>>>0)){break}r63=r56-r53|0;r56=r63>>>0>(r15+40|0)>>>0;r55=r56?r53:-1;if((r55|0)==-1){break}else{r59=r56?r63:r64,r60=r59>>2;r61=r55,r62=r61>>2;r2=201;break}}}while(0);do{if(r2==201){r64=HEAP32[1311266]+r59|0;HEAP32[1311266]=r64;if(r64>>>0>HEAP32[1311267]>>>0){HEAP32[1311267]=r64}r64=HEAP32[1311164],r50=r64>>2;L310:do{if((r64|0)==0){r55=HEAP32[1311162];if((r55|0)==0|r61>>>0<r55>>>0){HEAP32[1311162]=r61}HEAP32[1311270]=r61;HEAP32[1311271]=r59;HEAP32[1311273]=0;HEAP32[1311167]=HEAP32[1310738];HEAP32[1311166]=-1;r55=0;while(1){r63=r55<<1;r56=(r63<<2)+5244672|0;HEAP32[(r63+3<<2)+5244672>>2]=r56;HEAP32[(r63+2<<2)+5244672>>2]=r56;r56=r55+1|0;if((r56|0)==32){break}else{r55=r56}}r55=r61+8|0;if((r55&7|0)==0){r65=0}else{r65=-r55&7}r55=r59-40-r65|0;HEAP32[1311164]=r61+r65|0;HEAP32[1311161]=r55;HEAP32[(r65+4>>2)+r62]=r55|1;HEAP32[(r59-36>>2)+r62]=40;HEAP32[1311165]=HEAP32[1310742]}else{r55=5245080,r56=r55>>2;while(1){r66=HEAP32[r56];r67=r55+4|0;r68=HEAP32[r67>>2];if((r61|0)==(r66+r68|0)){r2=213;break}r63=HEAP32[r56+2];if((r63|0)==0){break}else{r55=r63,r56=r55>>2}}do{if(r2==213){if((HEAP32[r56+3]&8|0)!=0){break}r55=r64;if(!(r55>>>0>=r66>>>0&r55>>>0<r61>>>0)){break}HEAP32[r67>>2]=r68+r59|0;r55=HEAP32[1311164];r63=HEAP32[1311161]+r59|0;r53=r55;r57=r55+8|0;if((r57&7|0)==0){r69=0}else{r69=-r57&7}r57=r63-r69|0;HEAP32[1311164]=r53+r69|0;HEAP32[1311161]=r57;HEAP32[r69+(r53+4)>>2]=r57|1;HEAP32[r63+(r53+4)>>2]=40;HEAP32[1311165]=HEAP32[1310742];break L310}}while(0);if(r61>>>0<HEAP32[1311162]>>>0){HEAP32[1311162]=r61}r56=r61+r59|0;r53=5245080;while(1){r70=r53|0;if((HEAP32[r70>>2]|0)==(r56|0)){r2=223;break}r63=HEAP32[r53+8>>2];if((r63|0)==0){break}else{r53=r63}}do{if(r2==223){if((HEAP32[r53+12>>2]&8|0)!=0){break}HEAP32[r70>>2]=r61;r56=r53+4|0;HEAP32[r56>>2]=HEAP32[r56>>2]+r59|0;r56=r61+8|0;if((r56&7|0)==0){r71=0}else{r71=-r56&7}r56=r59+(r61+8)|0;if((r56&7|0)==0){r72=0,r73=r72>>2}else{r72=-r56&7,r73=r72>>2}r56=r61+r72+r59|0;r63=r56;r57=r71+r15|0,r55=r57>>2;r40=r61+r57|0;r57=r40;r39=r56-(r61+r71)-r15|0;HEAP32[(r71+4>>2)+r62]=r15|3;do{if((r63|0)==(HEAP32[1311164]|0)){r54=HEAP32[1311161]+r39|0;HEAP32[1311161]=r54;HEAP32[1311164]=r57;HEAP32[r55+(r62+1)]=r54|1}else{if((r63|0)==(HEAP32[1311163]|0)){r54=HEAP32[1311160]+r39|0;HEAP32[1311160]=r54;HEAP32[1311163]=r57;HEAP32[r55+(r62+1)]=r54|1;HEAP32[(r54>>2)+r62+r55]=r54;break}r54=r59+4|0;r58=HEAP32[(r54>>2)+r62+r73];if((r58&3|0)==1){r52=r58&-8;r51=r58>>>3;L355:do{if(r58>>>0<256){r48=HEAP32[((r72|8)>>2)+r62+r60];r49=HEAP32[r73+(r62+(r60+3))];r5=(r51<<3)+5244672|0;do{if((r48|0)!=(r5|0)){if(r48>>>0<HEAP32[1311162]>>>0){_abort()}if((HEAP32[r48+12>>2]|0)==(r63|0)){break}_abort()}}while(0);if((r49|0)==(r48|0)){HEAP32[1311158]=HEAP32[1311158]&(1<<r51^-1);break}do{if((r49|0)==(r5|0)){r74=r49+8|0}else{if(r49>>>0<HEAP32[1311162]>>>0){_abort()}r47=r49+8|0;if((HEAP32[r47>>2]|0)==(r63|0)){r74=r47;break}_abort()}}while(0);HEAP32[r48+12>>2]=r49;HEAP32[r74>>2]=r48}else{r5=r56;r47=HEAP32[((r72|24)>>2)+r62+r60];r46=HEAP32[r73+(r62+(r60+3))];L376:do{if((r46|0)==(r5|0)){r7=r72|16;r41=r61+r54+r7|0;r17=HEAP32[r41>>2];do{if((r17|0)==0){r42=r61+r7+r59|0;r43=HEAP32[r42>>2];if((r43|0)==0){r75=0,r76=r75>>2;break L376}else{r77=r43;r78=r42;break}}else{r77=r17;r78=r41}}while(0);while(1){r41=r77+20|0;r17=HEAP32[r41>>2];if((r17|0)!=0){r77=r17;r78=r41;continue}r41=r77+16|0;r17=HEAP32[r41>>2];if((r17|0)==0){break}else{r77=r17;r78=r41}}if(r78>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r78>>2]=0;r75=r77,r76=r75>>2;break}}else{r41=HEAP32[((r72|8)>>2)+r62+r60];if(r41>>>0<HEAP32[1311162]>>>0){_abort()}r17=r41+12|0;if((HEAP32[r17>>2]|0)!=(r5|0)){_abort()}r7=r46+8|0;if((HEAP32[r7>>2]|0)==(r5|0)){HEAP32[r17>>2]=r46;HEAP32[r7>>2]=r41;r75=r46,r76=r75>>2;break}else{_abort()}}}while(0);if((r47|0)==0){break}r46=r72+(r61+(r59+28))|0;r48=(HEAP32[r46>>2]<<2)+5244936|0;do{if((r5|0)==(HEAP32[r48>>2]|0)){HEAP32[r48>>2]=r75;if((r75|0)!=0){break}HEAP32[1311159]=HEAP32[1311159]&(1<<HEAP32[r46>>2]^-1);break L355}else{if(r47>>>0<HEAP32[1311162]>>>0){_abort()}r49=r47+16|0;if((HEAP32[r49>>2]|0)==(r5|0)){HEAP32[r49>>2]=r75}else{HEAP32[r47+20>>2]=r75}if((r75|0)==0){break L355}}}while(0);if(r75>>>0<HEAP32[1311162]>>>0){_abort()}HEAP32[r76+6]=r47;r5=r72|16;r46=HEAP32[(r5>>2)+r62+r60];do{if((r46|0)!=0){if(r46>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r76+4]=r46;HEAP32[r46+24>>2]=r75;break}}}while(0);r46=HEAP32[(r54+r5>>2)+r62];if((r46|0)==0){break}if(r46>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r76+5]=r46;HEAP32[r46+24>>2]=r75;break}}}while(0);r79=r61+(r52|r72)+r59|0;r80=r52+r39|0}else{r79=r63;r80=r39}r54=r79+4|0;HEAP32[r54>>2]=HEAP32[r54>>2]&-2;HEAP32[r55+(r62+1)]=r80|1;HEAP32[(r80>>2)+r62+r55]=r80;r54=r80>>>3;if(r80>>>0<256){r51=r54<<1;r58=(r51<<2)+5244672|0;r46=HEAP32[1311158];r47=1<<r54;do{if((r46&r47|0)==0){HEAP32[1311158]=r46|r47;r81=r58;r82=(r51+2<<2)+5244672|0}else{r54=(r51+2<<2)+5244672|0;r48=HEAP32[r54>>2];if(r48>>>0>=HEAP32[1311162]>>>0){r81=r48;r82=r54;break}_abort()}}while(0);HEAP32[r82>>2]=r57;HEAP32[r81+12>>2]=r57;HEAP32[r55+(r62+2)]=r81;HEAP32[r55+(r62+3)]=r58;break}r51=r40;r47=r80>>>8;do{if((r47|0)==0){r83=0}else{if(r80>>>0>16777215){r83=31;break}r46=(r47+1048320|0)>>>16&8;r52=r47<<r46;r54=(r52+520192|0)>>>16&4;r48=r52<<r54;r52=(r48+245760|0)>>>16&2;r49=14-(r54|r46|r52)+(r48<<r52>>>15)|0;r83=r80>>>((r49+7|0)>>>0)&1|r49<<1}}while(0);r47=(r83<<2)+5244936|0;HEAP32[r55+(r62+7)]=r83;HEAP32[r55+(r62+5)]=0;HEAP32[r55+(r62+4)]=0;r58=HEAP32[1311159];r49=1<<r83;if((r58&r49|0)==0){HEAP32[1311159]=r58|r49;HEAP32[r47>>2]=r51;HEAP32[r55+(r62+6)]=r47;HEAP32[r55+(r62+3)]=r51;HEAP32[r55+(r62+2)]=r51;break}if((r83|0)==31){r84=0}else{r84=25-(r83>>>1)|0}r49=r80<<r84;r58=HEAP32[r47>>2];while(1){if((HEAP32[r58+4>>2]&-8|0)==(r80|0)){break}r85=(r49>>>31<<2)+r58+16|0;r47=HEAP32[r85>>2];if((r47|0)==0){r2=296;break}else{r49=r49<<1;r58=r47}}if(r2==296){if(r85>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r85>>2]=r51;HEAP32[r55+(r62+6)]=r58;HEAP32[r55+(r62+3)]=r51;HEAP32[r55+(r62+2)]=r51;break}}r49=r58+8|0;r47=HEAP32[r49>>2];r52=HEAP32[1311162];if(r58>>>0<r52>>>0){_abort()}if(r47>>>0<r52>>>0){_abort()}else{HEAP32[r47+12>>2]=r51;HEAP32[r49>>2]=r51;HEAP32[r55+(r62+2)]=r47;HEAP32[r55+(r62+3)]=r58;HEAP32[r55+(r62+6)]=0;break}}}while(0);r14=r61+(r71|8)|0;return r14}}while(0);r53=r64;r55=5245080,r40=r55>>2;while(1){r86=HEAP32[r40];if(r86>>>0<=r53>>>0){r87=HEAP32[r40+1];r88=r86+r87|0;if(r88>>>0>r53>>>0){break}}r55=HEAP32[r40+2],r40=r55>>2}r55=r86+(r87-39)|0;if((r55&7|0)==0){r89=0}else{r89=-r55&7}r55=r86+(r87-47)+r89|0;r40=r55>>>0<(r64+16|0)>>>0?r53:r55;r55=r40+8|0,r57=r55>>2;r39=r61+8|0;if((r39&7|0)==0){r90=0}else{r90=-r39&7}r39=r59-40-r90|0;HEAP32[1311164]=r61+r90|0;HEAP32[1311161]=r39;HEAP32[(r90+4>>2)+r62]=r39|1;HEAP32[(r59-36>>2)+r62]=40;HEAP32[1311165]=HEAP32[1310742];HEAP32[r40+4>>2]=27;HEAP32[r57]=HEAP32[1311270];HEAP32[r57+1]=HEAP32[1311271];HEAP32[r57+2]=HEAP32[1311272];HEAP32[r57+3]=HEAP32[1311273];HEAP32[1311270]=r61;HEAP32[1311271]=r59;HEAP32[1311273]=0;HEAP32[1311272]=r55;r55=r40+28|0;HEAP32[r55>>2]=7;L474:do{if((r40+32|0)>>>0<r88>>>0){r57=r55;while(1){r39=r57+4|0;HEAP32[r39>>2]=7;if((r57+8|0)>>>0<r88>>>0){r57=r39}else{break L474}}}}while(0);if((r40|0)==(r53|0)){break}r55=r40-r64|0;r57=r55+(r53+4)|0;HEAP32[r57>>2]=HEAP32[r57>>2]&-2;HEAP32[r50+1]=r55|1;HEAP32[r53+r55>>2]=r55;r57=r55>>>3;if(r55>>>0<256){r39=r57<<1;r63=(r39<<2)+5244672|0;r56=HEAP32[1311158];r47=1<<r57;do{if((r56&r47|0)==0){HEAP32[1311158]=r56|r47;r91=r63;r92=(r39+2<<2)+5244672|0}else{r57=(r39+2<<2)+5244672|0;r49=HEAP32[r57>>2];if(r49>>>0>=HEAP32[1311162]>>>0){r91=r49;r92=r57;break}_abort()}}while(0);HEAP32[r92>>2]=r64;HEAP32[r91+12>>2]=r64;HEAP32[r50+2]=r91;HEAP32[r50+3]=r63;break}r39=r64;r47=r55>>>8;do{if((r47|0)==0){r93=0}else{if(r55>>>0>16777215){r93=31;break}r56=(r47+1048320|0)>>>16&8;r53=r47<<r56;r40=(r53+520192|0)>>>16&4;r57=r53<<r40;r53=(r57+245760|0)>>>16&2;r49=14-(r40|r56|r53)+(r57<<r53>>>15)|0;r93=r55>>>((r49+7|0)>>>0)&1|r49<<1}}while(0);r47=(r93<<2)+5244936|0;HEAP32[r50+7]=r93;HEAP32[r50+5]=0;HEAP32[r50+4]=0;r63=HEAP32[1311159];r49=1<<r93;if((r63&r49|0)==0){HEAP32[1311159]=r63|r49;HEAP32[r47>>2]=r39;HEAP32[r50+6]=r47;HEAP32[r50+3]=r64;HEAP32[r50+2]=r64;break}if((r93|0)==31){r94=0}else{r94=25-(r93>>>1)|0}r49=r55<<r94;r63=HEAP32[r47>>2];while(1){if((HEAP32[r63+4>>2]&-8|0)==(r55|0)){break}r95=(r49>>>31<<2)+r63+16|0;r47=HEAP32[r95>>2];if((r47|0)==0){r2=331;break}else{r49=r49<<1;r63=r47}}if(r2==331){if(r95>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r95>>2]=r39;HEAP32[r50+6]=r63;HEAP32[r50+3]=r64;HEAP32[r50+2]=r64;break}}r49=r63+8|0;r55=HEAP32[r49>>2];r47=HEAP32[1311162];if(r63>>>0<r47>>>0){_abort()}if(r55>>>0<r47>>>0){_abort()}else{HEAP32[r55+12>>2]=r39;HEAP32[r49>>2]=r39;HEAP32[r50+2]=r55;HEAP32[r50+3]=r63;HEAP32[r50+6]=0;break}}}while(0);r50=HEAP32[1311161];if(r50>>>0<=r15>>>0){break}r64=r50-r15|0;HEAP32[1311161]=r64;r50=HEAP32[1311164];r55=r50;HEAP32[1311164]=r55+r15|0;HEAP32[(r55+4>>2)+r16]=r64|1;HEAP32[r50+4>>2]=r15|3;r14=r50+8|0;return r14}}while(0);HEAP32[___errno_location()>>2]=12;r14=0;return r14}function _free(r1){var r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43,r44,r45,r46;r2=r1>>2;r3=0;if((r1|0)==0){return}r4=r1-8|0;r5=r4;r6=HEAP32[1311162];if(r4>>>0<r6>>>0){_abort()}r7=HEAP32[r1-4>>2];r8=r7&3;if((r8|0)==1){_abort()}r9=r7&-8,r10=r9>>2;r11=r1+(r9-8)|0;r12=r11;L527:do{if((r7&1|0)==0){r13=HEAP32[r4>>2];if((r8|0)==0){return}r14=-8-r13|0,r15=r14>>2;r16=r1+r14|0;r17=r16;r18=r13+r9|0;if(r16>>>0<r6>>>0){_abort()}if((r17|0)==(HEAP32[1311163]|0)){r19=(r1+(r9-4)|0)>>2;if((HEAP32[r19]&3|0)!=3){r20=r17,r21=r20>>2;r22=r18;break}HEAP32[1311160]=r18;HEAP32[r19]=HEAP32[r19]&-2;HEAP32[r15+(r2+1)]=r18|1;HEAP32[r11>>2]=r18;return}r19=r13>>>3;if(r13>>>0<256){r13=HEAP32[r15+(r2+2)];r23=HEAP32[r15+(r2+3)];r24=(r19<<3)+5244672|0;do{if((r13|0)!=(r24|0)){if(r13>>>0<r6>>>0){_abort()}if((HEAP32[r13+12>>2]|0)==(r17|0)){break}_abort()}}while(0);if((r23|0)==(r13|0)){HEAP32[1311158]=HEAP32[1311158]&(1<<r19^-1);r20=r17,r21=r20>>2;r22=r18;break}do{if((r23|0)==(r24|0)){r25=r23+8|0}else{if(r23>>>0<r6>>>0){_abort()}r26=r23+8|0;if((HEAP32[r26>>2]|0)==(r17|0)){r25=r26;break}_abort()}}while(0);HEAP32[r13+12>>2]=r23;HEAP32[r25>>2]=r13;r20=r17,r21=r20>>2;r22=r18;break}r24=r16;r19=HEAP32[r15+(r2+6)];r26=HEAP32[r15+(r2+3)];L561:do{if((r26|0)==(r24|0)){r27=r14+(r1+20)|0;r28=HEAP32[r27>>2];do{if((r28|0)==0){r29=r14+(r1+16)|0;r30=HEAP32[r29>>2];if((r30|0)==0){r31=0,r32=r31>>2;break L561}else{r33=r30;r34=r29;break}}else{r33=r28;r34=r27}}while(0);while(1){r27=r33+20|0;r28=HEAP32[r27>>2];if((r28|0)!=0){r33=r28;r34=r27;continue}r27=r33+16|0;r28=HEAP32[r27>>2];if((r28|0)==0){break}else{r33=r28;r34=r27}}if(r34>>>0<r6>>>0){_abort()}else{HEAP32[r34>>2]=0;r31=r33,r32=r31>>2;break}}else{r27=HEAP32[r15+(r2+2)];if(r27>>>0<r6>>>0){_abort()}r28=r27+12|0;if((HEAP32[r28>>2]|0)!=(r24|0)){_abort()}r29=r26+8|0;if((HEAP32[r29>>2]|0)==(r24|0)){HEAP32[r28>>2]=r26;HEAP32[r29>>2]=r27;r31=r26,r32=r31>>2;break}else{_abort()}}}while(0);if((r19|0)==0){r20=r17,r21=r20>>2;r22=r18;break}r26=r14+(r1+28)|0;r16=(HEAP32[r26>>2]<<2)+5244936|0;do{if((r24|0)==(HEAP32[r16>>2]|0)){HEAP32[r16>>2]=r31;if((r31|0)!=0){break}HEAP32[1311159]=HEAP32[1311159]&(1<<HEAP32[r26>>2]^-1);r20=r17,r21=r20>>2;r22=r18;break L527}else{if(r19>>>0<HEAP32[1311162]>>>0){_abort()}r13=r19+16|0;if((HEAP32[r13>>2]|0)==(r24|0)){HEAP32[r13>>2]=r31}else{HEAP32[r19+20>>2]=r31}if((r31|0)==0){r20=r17,r21=r20>>2;r22=r18;break L527}}}while(0);if(r31>>>0<HEAP32[1311162]>>>0){_abort()}HEAP32[r32+6]=r19;r24=HEAP32[r15+(r2+4)];do{if((r24|0)!=0){if(r24>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r32+4]=r24;HEAP32[r24+24>>2]=r31;break}}}while(0);r24=HEAP32[r15+(r2+5)];if((r24|0)==0){r20=r17,r21=r20>>2;r22=r18;break}if(r24>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r32+5]=r24;HEAP32[r24+24>>2]=r31;r20=r17,r21=r20>>2;r22=r18;break}}else{r20=r5,r21=r20>>2;r22=r9}}while(0);r5=r20,r31=r5>>2;if(r5>>>0>=r11>>>0){_abort()}r5=r1+(r9-4)|0;r32=HEAP32[r5>>2];if((r32&1|0)==0){_abort()}do{if((r32&2|0)==0){if((r12|0)==(HEAP32[1311164]|0)){r6=HEAP32[1311161]+r22|0;HEAP32[1311161]=r6;HEAP32[1311164]=r20;HEAP32[r21+1]=r6|1;if((r20|0)==(HEAP32[1311163]|0)){HEAP32[1311163]=0;HEAP32[1311160]=0}if(r6>>>0<=HEAP32[1311165]>>>0){return}_sys_trim(0);return}if((r12|0)==(HEAP32[1311163]|0)){r6=HEAP32[1311160]+r22|0;HEAP32[1311160]=r6;HEAP32[1311163]=r20;HEAP32[r21+1]=r6|1;HEAP32[(r6>>2)+r31]=r6;return}r6=(r32&-8)+r22|0;r33=r32>>>3;L632:do{if(r32>>>0<256){r34=HEAP32[r2+r10];r25=HEAP32[((r9|4)>>2)+r2];r8=(r33<<3)+5244672|0;do{if((r34|0)!=(r8|0)){if(r34>>>0<HEAP32[1311162]>>>0){_abort()}if((HEAP32[r34+12>>2]|0)==(r12|0)){break}_abort()}}while(0);if((r25|0)==(r34|0)){HEAP32[1311158]=HEAP32[1311158]&(1<<r33^-1);break}do{if((r25|0)==(r8|0)){r35=r25+8|0}else{if(r25>>>0<HEAP32[1311162]>>>0){_abort()}r4=r25+8|0;if((HEAP32[r4>>2]|0)==(r12|0)){r35=r4;break}_abort()}}while(0);HEAP32[r34+12>>2]=r25;HEAP32[r35>>2]=r34}else{r8=r11;r4=HEAP32[r10+(r2+4)];r7=HEAP32[((r9|4)>>2)+r2];L653:do{if((r7|0)==(r8|0)){r24=r9+(r1+12)|0;r19=HEAP32[r24>>2];do{if((r19|0)==0){r26=r9+(r1+8)|0;r16=HEAP32[r26>>2];if((r16|0)==0){r36=0,r37=r36>>2;break L653}else{r38=r16;r39=r26;break}}else{r38=r19;r39=r24}}while(0);while(1){r24=r38+20|0;r19=HEAP32[r24>>2];if((r19|0)!=0){r38=r19;r39=r24;continue}r24=r38+16|0;r19=HEAP32[r24>>2];if((r19|0)==0){break}else{r38=r19;r39=r24}}if(r39>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r39>>2]=0;r36=r38,r37=r36>>2;break}}else{r24=HEAP32[r2+r10];if(r24>>>0<HEAP32[1311162]>>>0){_abort()}r19=r24+12|0;if((HEAP32[r19>>2]|0)!=(r8|0)){_abort()}r26=r7+8|0;if((HEAP32[r26>>2]|0)==(r8|0)){HEAP32[r19>>2]=r7;HEAP32[r26>>2]=r24;r36=r7,r37=r36>>2;break}else{_abort()}}}while(0);if((r4|0)==0){break}r7=r9+(r1+20)|0;r34=(HEAP32[r7>>2]<<2)+5244936|0;do{if((r8|0)==(HEAP32[r34>>2]|0)){HEAP32[r34>>2]=r36;if((r36|0)!=0){break}HEAP32[1311159]=HEAP32[1311159]&(1<<HEAP32[r7>>2]^-1);break L632}else{if(r4>>>0<HEAP32[1311162]>>>0){_abort()}r25=r4+16|0;if((HEAP32[r25>>2]|0)==(r8|0)){HEAP32[r25>>2]=r36}else{HEAP32[r4+20>>2]=r36}if((r36|0)==0){break L632}}}while(0);if(r36>>>0<HEAP32[1311162]>>>0){_abort()}HEAP32[r37+6]=r4;r8=HEAP32[r10+(r2+2)];do{if((r8|0)!=0){if(r8>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r37+4]=r8;HEAP32[r8+24>>2]=r36;break}}}while(0);r8=HEAP32[r10+(r2+3)];if((r8|0)==0){break}if(r8>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r37+5]=r8;HEAP32[r8+24>>2]=r36;break}}}while(0);HEAP32[r21+1]=r6|1;HEAP32[(r6>>2)+r31]=r6;if((r20|0)!=(HEAP32[1311163]|0)){r40=r6;break}HEAP32[1311160]=r6;return}else{HEAP32[r5>>2]=r32&-2;HEAP32[r21+1]=r22|1;HEAP32[(r22>>2)+r31]=r22;r40=r22}}while(0);r22=r40>>>3;if(r40>>>0<256){r31=r22<<1;r32=(r31<<2)+5244672|0;r5=HEAP32[1311158];r36=1<<r22;do{if((r5&r36|0)==0){HEAP32[1311158]=r5|r36;r41=r32;r42=(r31+2<<2)+5244672|0}else{r22=(r31+2<<2)+5244672|0;r37=HEAP32[r22>>2];if(r37>>>0>=HEAP32[1311162]>>>0){r41=r37;r42=r22;break}_abort()}}while(0);HEAP32[r42>>2]=r20;HEAP32[r41+12>>2]=r20;HEAP32[r21+2]=r41;HEAP32[r21+3]=r32;return}r32=r20;r41=r40>>>8;do{if((r41|0)==0){r43=0}else{if(r40>>>0>16777215){r43=31;break}r42=(r41+1048320|0)>>>16&8;r31=r41<<r42;r36=(r31+520192|0)>>>16&4;r5=r31<<r36;r31=(r5+245760|0)>>>16&2;r22=14-(r36|r42|r31)+(r5<<r31>>>15)|0;r43=r40>>>((r22+7|0)>>>0)&1|r22<<1}}while(0);r41=(r43<<2)+5244936|0;HEAP32[r21+7]=r43;HEAP32[r21+5]=0;HEAP32[r21+4]=0;r22=HEAP32[1311159];r31=1<<r43;do{if((r22&r31|0)==0){HEAP32[1311159]=r22|r31;HEAP32[r41>>2]=r32;HEAP32[r21+6]=r41;HEAP32[r21+3]=r20;HEAP32[r21+2]=r20}else{if((r43|0)==31){r44=0}else{r44=25-(r43>>>1)|0}r5=r40<<r44;r42=HEAP32[r41>>2];while(1){if((HEAP32[r42+4>>2]&-8|0)==(r40|0)){break}r45=(r5>>>31<<2)+r42+16|0;r36=HEAP32[r45>>2];if((r36|0)==0){r3=510;break}else{r5=r5<<1;r42=r36}}if(r3==510){if(r45>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r45>>2]=r32;HEAP32[r21+6]=r42;HEAP32[r21+3]=r20;HEAP32[r21+2]=r20;break}}r5=r42+8|0;r6=HEAP32[r5>>2];r36=HEAP32[1311162];if(r42>>>0<r36>>>0){_abort()}if(r6>>>0<r36>>>0){_abort()}else{HEAP32[r6+12>>2]=r32;HEAP32[r5>>2]=r32;HEAP32[r21+2]=r6;HEAP32[r21+3]=r42;HEAP32[r21+6]=0;break}}}while(0);r21=HEAP32[1311166]-1|0;HEAP32[1311166]=r21;if((r21|0)==0){r46=5245088}else{return}while(1){r21=HEAP32[r46>>2];if((r21|0)==0){break}else{r46=r21+8|0}}HEAP32[1311166]=-1;return}function _realloc(r1,r2){var r3,r4,r5,r6;if((r1|0)==0){r3=_malloc(r2);return r3}if(r2>>>0>4294967231){HEAP32[___errno_location()>>2]=12;r3=0;return r3}if(r2>>>0<11){r4=16}else{r4=r2+11&-8}r5=_try_realloc_chunk(r1-8|0,r4);if((r5|0)!=0){r3=r5+8|0;return r3}r5=_malloc(r2);if((r5|0)==0){r3=0;return r3}r4=HEAP32[r1-4>>2];r6=(r4&-8)-((r4&3|0)==0?8:4)|0;_memcpy(r5,r1,r6>>>0<r2>>>0?r6:r2);_free(r1);r3=r5;return r3}function _sys_trim(r1){var r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15;do{if((HEAP32[1310738]|0)==0){r2=_sysconf(8);if((r2-1&r2|0)==0){HEAP32[1310740]=r2;HEAP32[1310739]=r2;HEAP32[1310741]=-1;HEAP32[1310742]=2097152;HEAP32[1310743]=0;HEAP32[1311269]=0;HEAP32[1310738]=_time(0)&-16^1431655768;break}else{_abort()}}}while(0);if(r1>>>0>=4294967232){r3=0;r4=r3&1;return r4}r2=HEAP32[1311164];if((r2|0)==0){r3=0;r4=r3&1;return r4}r5=HEAP32[1311161];do{if(r5>>>0>(r1+40|0)>>>0){r6=HEAP32[1310740];r7=Math.imul(Math.floor(((-40-r1-1+r5+r6|0)>>>0)/(r6>>>0))-1|0,r6);r8=r2;r9=5245080,r10=r9>>2;while(1){r11=HEAP32[r10];if(r11>>>0<=r8>>>0){if((r11+HEAP32[r10+1]|0)>>>0>r8>>>0){r12=r9;break}}r11=HEAP32[r10+2];if((r11|0)==0){r12=0;break}else{r9=r11,r10=r9>>2}}if((HEAP32[r12+12>>2]&8|0)!=0){break}r9=_sbrk(0);r10=(r12+4|0)>>2;if((r9|0)!=(HEAP32[r12>>2]+HEAP32[r10]|0)){break}r8=_sbrk(-(r7>>>0>2147483646?-2147483648-r6|0:r7)|0);r11=_sbrk(0);if(!((r8|0)!=-1&r11>>>0<r9>>>0)){break}r8=r9-r11|0;if((r9|0)==(r11|0)){break}HEAP32[r10]=HEAP32[r10]-r8|0;HEAP32[1311266]=HEAP32[1311266]-r8|0;r10=HEAP32[1311164];r13=HEAP32[1311161]-r8|0;r8=r10;r14=r10+8|0;if((r14&7|0)==0){r15=0}else{r15=-r14&7}r14=r13-r15|0;HEAP32[1311164]=r8+r15|0;HEAP32[1311161]=r14;HEAP32[r15+(r8+4)>>2]=r14|1;HEAP32[r13+(r8+4)>>2]=40;HEAP32[1311165]=HEAP32[1310742];r3=(r9|0)!=(r11|0);r4=r3&1;return r4}}while(0);if(HEAP32[1311161]>>>0<=HEAP32[1311165]>>>0){r3=0;r4=r3&1;return r4}HEAP32[1311165]=-1;r3=0;r4=r3&1;return r4}function _try_realloc_chunk(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29;r3=(r1+4|0)>>2;r4=HEAP32[r3];r5=r4&-8,r6=r5>>2;r7=r1,r8=r7>>2;r9=r7+r5|0;r10=r9;r11=HEAP32[1311162];if(r7>>>0<r11>>>0){_abort()}r12=r4&3;if(!((r12|0)!=1&r7>>>0<r9>>>0)){_abort()}r13=(r7+(r5|4)|0)>>2;r14=HEAP32[r13];if((r14&1|0)==0){_abort()}if((r12|0)==0){if(r2>>>0<256){r15=0;return r15}do{if(r5>>>0>=(r2+4|0)>>>0){if((r5-r2|0)>>>0>HEAP32[1310740]<<1>>>0){break}else{r15=r1}return r15}}while(0);r15=0;return r15}if(r5>>>0>=r2>>>0){r12=r5-r2|0;if(r12>>>0<=15){r15=r1;return r15}HEAP32[r3]=r4&1|r2|2;HEAP32[(r2+4>>2)+r8]=r12|3;HEAP32[r13]=HEAP32[r13]|1;_dispose_chunk(r7+r2|0,r12);r15=r1;return r15}if((r10|0)==(HEAP32[1311164]|0)){r12=HEAP32[1311161]+r5|0;if(r12>>>0<=r2>>>0){r15=0;return r15}r13=r12-r2|0;HEAP32[r3]=r4&1|r2|2;HEAP32[(r2+4>>2)+r8]=r13|1;HEAP32[1311164]=r7+r2|0;HEAP32[1311161]=r13;r15=r1;return r15}if((r10|0)==(HEAP32[1311163]|0)){r13=HEAP32[1311160]+r5|0;if(r13>>>0<r2>>>0){r15=0;return r15}r12=r13-r2|0;if(r12>>>0>15){HEAP32[r3]=r4&1|r2|2;HEAP32[(r2+4>>2)+r8]=r12|1;HEAP32[(r13>>2)+r8]=r12;r16=r13+(r7+4)|0;HEAP32[r16>>2]=HEAP32[r16>>2]&-2;r17=r7+r2|0;r18=r12}else{HEAP32[r3]=r4&1|r13|2;r4=r13+(r7+4)|0;HEAP32[r4>>2]=HEAP32[r4>>2]|1;r17=0;r18=0}HEAP32[1311160]=r18;HEAP32[1311163]=r17;r15=r1;return r15}if((r14&2|0)!=0){r15=0;return r15}r17=(r14&-8)+r5|0;if(r17>>>0<r2>>>0){r15=0;return r15}r18=r17-r2|0;r4=r14>>>3;L853:do{if(r14>>>0<256){r13=HEAP32[r6+(r8+2)];r12=HEAP32[r6+(r8+3)];r16=(r4<<3)+5244672|0;do{if((r13|0)!=(r16|0)){if(r13>>>0<r11>>>0){_abort()}if((HEAP32[r13+12>>2]|0)==(r10|0)){break}_abort()}}while(0);if((r12|0)==(r13|0)){HEAP32[1311158]=HEAP32[1311158]&(1<<r4^-1);break}do{if((r12|0)==(r16|0)){r19=r12+8|0}else{if(r12>>>0<r11>>>0){_abort()}r20=r12+8|0;if((HEAP32[r20>>2]|0)==(r10|0)){r19=r20;break}_abort()}}while(0);HEAP32[r13+12>>2]=r12;HEAP32[r19>>2]=r13}else{r16=r9;r20=HEAP32[r6+(r8+6)];r21=HEAP32[r6+(r8+3)];L874:do{if((r21|0)==(r16|0)){r22=r5+(r7+20)|0;r23=HEAP32[r22>>2];do{if((r23|0)==0){r24=r5+(r7+16)|0;r25=HEAP32[r24>>2];if((r25|0)==0){r26=0,r27=r26>>2;break L874}else{r28=r25;r29=r24;break}}else{r28=r23;r29=r22}}while(0);while(1){r22=r28+20|0;r23=HEAP32[r22>>2];if((r23|0)!=0){r28=r23;r29=r22;continue}r22=r28+16|0;r23=HEAP32[r22>>2];if((r23|0)==0){break}else{r28=r23;r29=r22}}if(r29>>>0<r11>>>0){_abort()}else{HEAP32[r29>>2]=0;r26=r28,r27=r26>>2;break}}else{r22=HEAP32[r6+(r8+2)];if(r22>>>0<r11>>>0){_abort()}r23=r22+12|0;if((HEAP32[r23>>2]|0)!=(r16|0)){_abort()}r24=r21+8|0;if((HEAP32[r24>>2]|0)==(r16|0)){HEAP32[r23>>2]=r21;HEAP32[r24>>2]=r22;r26=r21,r27=r26>>2;break}else{_abort()}}}while(0);if((r20|0)==0){break}r21=r5+(r7+28)|0;r13=(HEAP32[r21>>2]<<2)+5244936|0;do{if((r16|0)==(HEAP32[r13>>2]|0)){HEAP32[r13>>2]=r26;if((r26|0)!=0){break}HEAP32[1311159]=HEAP32[1311159]&(1<<HEAP32[r21>>2]^-1);break L853}else{if(r20>>>0<HEAP32[1311162]>>>0){_abort()}r12=r20+16|0;if((HEAP32[r12>>2]|0)==(r16|0)){HEAP32[r12>>2]=r26}else{HEAP32[r20+20>>2]=r26}if((r26|0)==0){break L853}}}while(0);if(r26>>>0<HEAP32[1311162]>>>0){_abort()}HEAP32[r27+6]=r20;r16=HEAP32[r6+(r8+4)];do{if((r16|0)!=0){if(r16>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r27+4]=r16;HEAP32[r16+24>>2]=r26;break}}}while(0);r16=HEAP32[r6+(r8+5)];if((r16|0)==0){break}if(r16>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r27+5]=r16;HEAP32[r16+24>>2]=r26;break}}}while(0);if(r18>>>0<16){HEAP32[r3]=r17|HEAP32[r3]&1|2;r26=r7+(r17|4)|0;HEAP32[r26>>2]=HEAP32[r26>>2]|1;r15=r1;return r15}else{HEAP32[r3]=HEAP32[r3]&1|r2|2;HEAP32[(r2+4>>2)+r8]=r18|3;r8=r7+(r17|4)|0;HEAP32[r8>>2]=HEAP32[r8>>2]|1;_dispose_chunk(r7+r2|0,r18);r15=r1;return r15}}function _dispose_chunk(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41,r42,r43;r3=r2>>2;r4=0;r5=r1,r6=r5>>2;r7=r5+r2|0;r8=r7;r9=HEAP32[r1+4>>2];L929:do{if((r9&1|0)==0){r10=HEAP32[r1>>2];if((r9&3|0)==0){return}r11=r5+ -r10|0;r12=r11;r13=r10+r2|0;r14=HEAP32[1311162];if(r11>>>0<r14>>>0){_abort()}if((r12|0)==(HEAP32[1311163]|0)){r15=(r2+(r5+4)|0)>>2;if((HEAP32[r15]&3|0)!=3){r16=r12,r17=r16>>2;r18=r13;break}HEAP32[1311160]=r13;HEAP32[r15]=HEAP32[r15]&-2;HEAP32[(4-r10>>2)+r6]=r13|1;HEAP32[r7>>2]=r13;return}r15=r10>>>3;if(r10>>>0<256){r19=HEAP32[(8-r10>>2)+r6];r20=HEAP32[(12-r10>>2)+r6];r21=(r15<<3)+5244672|0;do{if((r19|0)!=(r21|0)){if(r19>>>0<r14>>>0){_abort()}if((HEAP32[r19+12>>2]|0)==(r12|0)){break}_abort()}}while(0);if((r20|0)==(r19|0)){HEAP32[1311158]=HEAP32[1311158]&(1<<r15^-1);r16=r12,r17=r16>>2;r18=r13;break}do{if((r20|0)==(r21|0)){r22=r20+8|0}else{if(r20>>>0<r14>>>0){_abort()}r23=r20+8|0;if((HEAP32[r23>>2]|0)==(r12|0)){r22=r23;break}_abort()}}while(0);HEAP32[r19+12>>2]=r20;HEAP32[r22>>2]=r19;r16=r12,r17=r16>>2;r18=r13;break}r21=r11;r15=HEAP32[(24-r10>>2)+r6];r23=HEAP32[(12-r10>>2)+r6];L963:do{if((r23|0)==(r21|0)){r24=16-r10|0;r25=r24+(r5+4)|0;r26=HEAP32[r25>>2];do{if((r26|0)==0){r27=r5+r24|0;r28=HEAP32[r27>>2];if((r28|0)==0){r29=0,r30=r29>>2;break L963}else{r31=r28;r32=r27;break}}else{r31=r26;r32=r25}}while(0);while(1){r25=r31+20|0;r26=HEAP32[r25>>2];if((r26|0)!=0){r31=r26;r32=r25;continue}r25=r31+16|0;r26=HEAP32[r25>>2];if((r26|0)==0){break}else{r31=r26;r32=r25}}if(r32>>>0<r14>>>0){_abort()}else{HEAP32[r32>>2]=0;r29=r31,r30=r29>>2;break}}else{r25=HEAP32[(8-r10>>2)+r6];if(r25>>>0<r14>>>0){_abort()}r26=r25+12|0;if((HEAP32[r26>>2]|0)!=(r21|0)){_abort()}r24=r23+8|0;if((HEAP32[r24>>2]|0)==(r21|0)){HEAP32[r26>>2]=r23;HEAP32[r24>>2]=r25;r29=r23,r30=r29>>2;break}else{_abort()}}}while(0);if((r15|0)==0){r16=r12,r17=r16>>2;r18=r13;break}r23=r5+(28-r10)|0;r14=(HEAP32[r23>>2]<<2)+5244936|0;do{if((r21|0)==(HEAP32[r14>>2]|0)){HEAP32[r14>>2]=r29;if((r29|0)!=0){break}HEAP32[1311159]=HEAP32[1311159]&(1<<HEAP32[r23>>2]^-1);r16=r12,r17=r16>>2;r18=r13;break L929}else{if(r15>>>0<HEAP32[1311162]>>>0){_abort()}r11=r15+16|0;if((HEAP32[r11>>2]|0)==(r21|0)){HEAP32[r11>>2]=r29}else{HEAP32[r15+20>>2]=r29}if((r29|0)==0){r16=r12,r17=r16>>2;r18=r13;break L929}}}while(0);if(r29>>>0<HEAP32[1311162]>>>0){_abort()}HEAP32[r30+6]=r15;r21=16-r10|0;r23=HEAP32[(r21>>2)+r6];do{if((r23|0)!=0){if(r23>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r30+4]=r23;HEAP32[r23+24>>2]=r29;break}}}while(0);r23=HEAP32[(r21+4>>2)+r6];if((r23|0)==0){r16=r12,r17=r16>>2;r18=r13;break}if(r23>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r30+5]=r23;HEAP32[r23+24>>2]=r29;r16=r12,r17=r16>>2;r18=r13;break}}else{r16=r1,r17=r16>>2;r18=r2}}while(0);r1=HEAP32[1311162];if(r7>>>0<r1>>>0){_abort()}r29=r2+(r5+4)|0;r30=HEAP32[r29>>2];do{if((r30&2|0)==0){if((r8|0)==(HEAP32[1311164]|0)){r31=HEAP32[1311161]+r18|0;HEAP32[1311161]=r31;HEAP32[1311164]=r16;HEAP32[r17+1]=r31|1;if((r16|0)!=(HEAP32[1311163]|0)){return}HEAP32[1311163]=0;HEAP32[1311160]=0;return}if((r8|0)==(HEAP32[1311163]|0)){r31=HEAP32[1311160]+r18|0;HEAP32[1311160]=r31;HEAP32[1311163]=r16;HEAP32[r17+1]=r31|1;HEAP32[(r31>>2)+r17]=r31;return}r31=(r30&-8)+r18|0;r32=r30>>>3;L1028:do{if(r30>>>0<256){r22=HEAP32[r3+(r6+2)];r9=HEAP32[r3+(r6+3)];r23=(r32<<3)+5244672|0;do{if((r22|0)!=(r23|0)){if(r22>>>0<r1>>>0){_abort()}if((HEAP32[r22+12>>2]|0)==(r8|0)){break}_abort()}}while(0);if((r9|0)==(r22|0)){HEAP32[1311158]=HEAP32[1311158]&(1<<r32^-1);break}do{if((r9|0)==(r23|0)){r33=r9+8|0}else{if(r9>>>0<r1>>>0){_abort()}r10=r9+8|0;if((HEAP32[r10>>2]|0)==(r8|0)){r33=r10;break}_abort()}}while(0);HEAP32[r22+12>>2]=r9;HEAP32[r33>>2]=r22}else{r23=r7;r10=HEAP32[r3+(r6+6)];r15=HEAP32[r3+(r6+3)];L1049:do{if((r15|0)==(r23|0)){r14=r2+(r5+20)|0;r11=HEAP32[r14>>2];do{if((r11|0)==0){r19=r2+(r5+16)|0;r20=HEAP32[r19>>2];if((r20|0)==0){r34=0,r35=r34>>2;break L1049}else{r36=r20;r37=r19;break}}else{r36=r11;r37=r14}}while(0);while(1){r14=r36+20|0;r11=HEAP32[r14>>2];if((r11|0)!=0){r36=r11;r37=r14;continue}r14=r36+16|0;r11=HEAP32[r14>>2];if((r11|0)==0){break}else{r36=r11;r37=r14}}if(r37>>>0<r1>>>0){_abort()}else{HEAP32[r37>>2]=0;r34=r36,r35=r34>>2;break}}else{r14=HEAP32[r3+(r6+2)];if(r14>>>0<r1>>>0){_abort()}r11=r14+12|0;if((HEAP32[r11>>2]|0)!=(r23|0)){_abort()}r19=r15+8|0;if((HEAP32[r19>>2]|0)==(r23|0)){HEAP32[r11>>2]=r15;HEAP32[r19>>2]=r14;r34=r15,r35=r34>>2;break}else{_abort()}}}while(0);if((r10|0)==0){break}r15=r2+(r5+28)|0;r22=(HEAP32[r15>>2]<<2)+5244936|0;do{if((r23|0)==(HEAP32[r22>>2]|0)){HEAP32[r22>>2]=r34;if((r34|0)!=0){break}HEAP32[1311159]=HEAP32[1311159]&(1<<HEAP32[r15>>2]^-1);break L1028}else{if(r10>>>0<HEAP32[1311162]>>>0){_abort()}r9=r10+16|0;if((HEAP32[r9>>2]|0)==(r23|0)){HEAP32[r9>>2]=r34}else{HEAP32[r10+20>>2]=r34}if((r34|0)==0){break L1028}}}while(0);if(r34>>>0<HEAP32[1311162]>>>0){_abort()}HEAP32[r35+6]=r10;r23=HEAP32[r3+(r6+4)];do{if((r23|0)!=0){if(r23>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r35+4]=r23;HEAP32[r23+24>>2]=r34;break}}}while(0);r23=HEAP32[r3+(r6+5)];if((r23|0)==0){break}if(r23>>>0<HEAP32[1311162]>>>0){_abort()}else{HEAP32[r35+5]=r23;HEAP32[r23+24>>2]=r34;break}}}while(0);HEAP32[r17+1]=r31|1;HEAP32[(r31>>2)+r17]=r31;if((r16|0)!=(HEAP32[1311163]|0)){r38=r31;break}HEAP32[1311160]=r31;return}else{HEAP32[r29>>2]=r30&-2;HEAP32[r17+1]=r18|1;HEAP32[(r18>>2)+r17]=r18;r38=r18}}while(0);r18=r38>>>3;if(r38>>>0<256){r30=r18<<1;r29=(r30<<2)+5244672|0;r34=HEAP32[1311158];r35=1<<r18;do{if((r34&r35|0)==0){HEAP32[1311158]=r34|r35;r39=r29;r40=(r30+2<<2)+5244672|0}else{r18=(r30+2<<2)+5244672|0;r6=HEAP32[r18>>2];if(r6>>>0>=HEAP32[1311162]>>>0){r39=r6;r40=r18;break}_abort()}}while(0);HEAP32[r40>>2]=r16;HEAP32[r39+12>>2]=r16;HEAP32[r17+2]=r39;HEAP32[r17+3]=r29;return}r29=r16;r39=r38>>>8;do{if((r39|0)==0){r41=0}else{if(r38>>>0>16777215){r41=31;break}r40=(r39+1048320|0)>>>16&8;r30=r39<<r40;r35=(r30+520192|0)>>>16&4;r34=r30<<r35;r30=(r34+245760|0)>>>16&2;r18=14-(r35|r40|r30)+(r34<<r30>>>15)|0;r41=r38>>>((r18+7|0)>>>0)&1|r18<<1}}while(0);r39=(r41<<2)+5244936|0;HEAP32[r17+7]=r41;HEAP32[r17+5]=0;HEAP32[r17+4]=0;r18=HEAP32[1311159];r30=1<<r41;if((r18&r30|0)==0){HEAP32[1311159]=r18|r30;HEAP32[r39>>2]=r29;HEAP32[r17+6]=r39;HEAP32[r17+3]=r16;HEAP32[r17+2]=r16;return}if((r41|0)==31){r42=0}else{r42=25-(r41>>>1)|0}r41=r38<<r42;r42=HEAP32[r39>>2];while(1){if((HEAP32[r42+4>>2]&-8|0)==(r38|0)){break}r43=(r41>>>31<<2)+r42+16|0;r39=HEAP32[r43>>2];if((r39|0)==0){r4=816;break}else{r41=r41<<1;r42=r39}}if(r4==816){if(r43>>>0<HEAP32[1311162]>>>0){_abort()}HEAP32[r43>>2]=r29;HEAP32[r17+6]=r42;HEAP32[r17+3]=r16;HEAP32[r17+2]=r16;return}r16=r42+8|0;r43=HEAP32[r16>>2];r4=HEAP32[1311162];if(r42>>>0<r4>>>0){_abort()}if(r43>>>0<r4>>>0){_abort()}HEAP32[r43+12>>2]=r29;HEAP32[r16>>2]=r29;HEAP32[r17+2]=r43;HEAP32[r17+3]=r42;HEAP32[r17+6]=0;return}function __Znwj(r1){var r2,r3,r4;r2=0;r3=(r1|0)==0?1:r1;while(1){r4=_malloc(r3);if((r4|0)!=0){r2=860;break}r1=(tempValue=HEAP32[1313854],HEAP32[1313854]=tempValue,tempValue);if((r1|0)==0){break}FUNCTION_TABLE[r1]()}if(r2==860){return r4}r4=___cxa_allocate_exception(4);HEAP32[r4>>2]=5247464;___cxa_throw(r4,5252688,66)}function __Znaj(r1){return __Znwj(r1)}function __ZNKSt9bad_alloc4whatEv(r1){return 5243812}function __ZdlPv(r1){if((r1|0)==0){return}_free(r1);return}function __ZdaPv(r1){__ZdlPv(r1);return}function __ZNSt9bad_allocD0Ev(r1){__ZdlPv(r1);return}function __ZNSt9bad_allocD2Ev(r1){return}function _strtod(r1,r2){var r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39,r40,r41;r3=0;r4=r1;while(1){r5=r4+1|0;if((_isspace(HEAP8[r4]<<24>>24)|0)==0){break}else{r4=r5}}r6=HEAP8[r4];if(r6<<24>>24==45){r7=r5;r8=1}else if(r6<<24>>24==43){r7=r5;r8=0}else{r7=r4;r8=0}r4=-1;r5=0;r6=r7;while(1){r9=HEAP8[r6];if(((r9<<24>>24)-48|0)>>>0<10){r10=r4}else{if(r9<<24>>24!=46|(r4|0)>-1){break}else{r10=r5}}r4=r10;r5=r5+1|0;r6=r6+1|0}r10=r6+ -r5|0;r7=(r4|0)<0;r11=((r7^1)<<31>>31)+r5|0;r12=(r11|0)>18;r13=(r12?-18:-r11|0)+(r7?r5:r4)|0;r4=r12?18:r11;do{if((r4|0)==0){r14=r1;r15=0}else{do{if((r4|0)>9){r11=r10;r12=r4;r5=0;while(1){r7=HEAP8[r11];r16=r11+1|0;if(r7<<24>>24==46){r17=HEAP8[r16];r18=r11+2|0}else{r17=r7;r18=r16}r19=(r17<<24>>24)+((r5*10&-1)-48)|0;r16=r12-1|0;if((r16|0)>9){r11=r18;r12=r16;r5=r19}else{break}}r20=(r19|0)*1e9;r21=9;r22=r18;r3=890;break}else{if((r4|0)>0){r20=0;r21=r4;r22=r10;r3=890;break}else{r23=0;r24=0;break}}}while(0);if(r3==890){r5=r22;r12=r21;r11=0;while(1){r16=HEAP8[r5];r7=r5+1|0;if(r16<<24>>24==46){r25=HEAP8[r7];r26=r5+2|0}else{r25=r16;r26=r7}r27=(r25<<24>>24)+((r11*10&-1)-48)|0;r7=r12-1|0;if((r7|0)>0){r5=r26;r12=r7;r11=r27}else{break}}r23=r27|0;r24=r20}r11=r24+r23;L1192:do{if(r9<<24>>24==69|r9<<24>>24==101){r12=r6+1|0;r5=HEAP8[r12];if(r5<<24>>24==45){r28=r6+2|0;r29=1}else if(r5<<24>>24==43){r28=r6+2|0;r29=0}else{r28=r12;r29=0}r12=HEAP8[r28];if(((r12<<24>>24)-48|0)>>>0<10){r30=r28;r31=0;r32=r12}else{r33=0;r34=r28;r35=r29;break}while(1){r12=(r32<<24>>24)+((r31*10&-1)-48)|0;r5=r30+1|0;r7=HEAP8[r5];if(((r7<<24>>24)-48|0)>>>0<10){r30=r5;r31=r12;r32=r7}else{r33=r12;r34=r5;r35=r29;break L1192}}}else{r33=0;r34=r6;r35=0}}while(0);r5=r13+((r35|0)==0?r33:-r33|0)|0;r12=(r5|0)<0?-r5|0:r5;do{if((r12|0)>511){HEAP32[___errno_location()>>2]=34;r36=1;r37=5242880;r38=511;r3=907;break}else{if((r12|0)==0){r39=1;break}else{r36=1;r37=5242880;r38=r12;r3=907;break}}}while(0);L1204:do{if(r3==907){while(1){r3=0;if((r38&1|0)==0){r40=r36}else{r40=r36*(HEAP32[tempDoublePtr>>2]=HEAP32[r37>>2],HEAP32[tempDoublePtr+4>>2]=HEAP32[r37+4>>2],HEAPF64[tempDoublePtr>>3])}r12=r38>>1;if((r12|0)==0){r39=r40;break L1204}else{r36=r40;r37=r37+8|0;r38=r12;r3=907}}}}while(0);if((r5|0)>-1){r14=r34;r15=r11*r39;break}else{r14=r34;r15=r11/r39;break}}}while(0);if((r2|0)!=0){HEAP32[r2>>2]=r14}if((r8|0)==0){r41=r15;return r41}r41=-r15;return r41}function __ZSt17__throw_bad_allocv(){var r1;r1=___cxa_allocate_exception(4);HEAP32[r1>>2]=5247464;___cxa_throw(r1,5252688,66)}function _i64Add(r1,r2,r3,r4){var r5,r6;r1=r1|0;r2=r2|0;r3=r3|0;r4=r4|0;r5=0,r6=0;r5=r1+r3>>>0;r6=r2+r4>>>0;if(r5>>>0<r1>>>0){r6=r6+1>>>0}return tempRet0=r6,r5|0}function _bitshift64Shl(r1,r2,r3){var r4;r1=r1|0;r2=r2|0;r3=r3|0;r4=0;if((r3|0)<32){r4=(1<<r3)-1|0;tempRet0=r2<<r3|(r1&r4<<32-r3)>>>32-r3;return r1<<r3}tempRet0=r1<<r3-32;return 0}function _bitshift64Lshr(r1,r2,r3){var r4;r1=r1|0;r2=r2|0;r3=r3|0;r4=0;if((r3|0)<32){r4=(1<<r3)-1|0;tempRet0=r2>>>r3;return r1>>>r3|(r2&r4)<<32-r3}tempRet0=0;return r2>>>r3-32|0}function _bitshift64Ashr(r1,r2,r3){var r4;r1=r1|0;r2=r2|0;r3=r3|0;r4=0;if((r3|0)<32){r4=(1<<r3)-1|0;tempRet0=r2>>r3;return r1>>>r3|(r2&r4)<<32-r3}tempRet0=(r2|0)<0?-1:0;return r2>>r3-32|0}
// EMSCRIPTEN_END_FUNCS
Module["_realloc"] = _realloc;
// TODO: strip out parts of this we do not need
//======= begin closure i64 code =======
// Copyright 2009 The Closure Library Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/**
 * @fileoverview Defines a Long class for representing a 64-bit two's-complement
 * integer value, which faithfully simulates the behavior of a Java "long". This
 * implementation is derived from LongLib in GWT.
 *
 */
var i64Math = (function() { // Emscripten wrapper
  var goog = { math: {} };
  /**
   * Constructs a 64-bit two's-complement integer, given its low and high 32-bit
   * values as *signed* integers.  See the from* functions below for more
   * convenient ways of constructing Longs.
   *
   * The internal representation of a long is the two given signed, 32-bit values.
   * We use 32-bit pieces because these are the size of integers on which
   * Javascript performs bit-operations.  For operations like addition and
   * multiplication, we split each number into 16-bit pieces, which can easily be
   * multiplied within Javascript's floating-point representation without overflow
   * or change in sign.
   *
   * In the algorithms below, we frequently reduce the negative case to the
   * positive case by negating the input(s) and then post-processing the result.
   * Note that we must ALWAYS check specially whether those values are MIN_VALUE
   * (-2^63) because -MIN_VALUE == MIN_VALUE (since 2^63 cannot be represented as
   * a positive number, it overflows back into a negative).  Not handling this
   * case would often result in infinite recursion.
   *
   * @param {number} low  The low (signed) 32 bits of the long.
   * @param {number} high  The high (signed) 32 bits of the long.
   * @constructor
   */
  goog.math.Long = function(low, high) {
    /**
     * @type {number}
     * @private
     */
    this.low_ = low | 0;  // force into 32 signed bits.
    /**
     * @type {number}
     * @private
     */
    this.high_ = high | 0;  // force into 32 signed bits.
  };
  // NOTE: Common constant values ZERO, ONE, NEG_ONE, etc. are defined below the
  // from* methods on which they depend.
  /**
   * A cache of the Long representations of small integer values.
   * @type {!Object}
   * @private
   */
  goog.math.Long.IntCache_ = {};
  /**
   * Returns a Long representing the given (32-bit) integer value.
   * @param {number} value The 32-bit integer in question.
   * @return {!goog.math.Long} The corresponding Long value.
   */
  goog.math.Long.fromInt = function(value) {
    if (-128 <= value && value < 128) {
      var cachedObj = goog.math.Long.IntCache_[value];
      if (cachedObj) {
        return cachedObj;
      }
    }
    var obj = new goog.math.Long(value | 0, value < 0 ? -1 : 0);
    if (-128 <= value && value < 128) {
      goog.math.Long.IntCache_[value] = obj;
    }
    return obj;
  };
  /**
   * Returns a Long representing the given value, provided that it is a finite
   * number.  Otherwise, zero is returned.
   * @param {number} value The number in question.
   * @return {!goog.math.Long} The corresponding Long value.
   */
  goog.math.Long.fromNumber = function(value) {
    if (isNaN(value) || !isFinite(value)) {
      return goog.math.Long.ZERO;
    } else if (value <= -goog.math.Long.TWO_PWR_63_DBL_) {
      return goog.math.Long.MIN_VALUE;
    } else if (value + 1 >= goog.math.Long.TWO_PWR_63_DBL_) {
      return goog.math.Long.MAX_VALUE;
    } else if (value < 0) {
      return goog.math.Long.fromNumber(-value).negate();
    } else {
      return new goog.math.Long(
          (value % goog.math.Long.TWO_PWR_32_DBL_) | 0,
          (value / goog.math.Long.TWO_PWR_32_DBL_) | 0);
    }
  };
  /**
   * Returns a Long representing the 64-bit integer that comes by concatenating
   * the given high and low bits.  Each is assumed to use 32 bits.
   * @param {number} lowBits The low 32-bits.
   * @param {number} highBits The high 32-bits.
   * @return {!goog.math.Long} The corresponding Long value.
   */
  goog.math.Long.fromBits = function(lowBits, highBits) {
    return new goog.math.Long(lowBits, highBits);
  };
  /**
   * Returns a Long representation of the given string, written using the given
   * radix.
   * @param {string} str The textual representation of the Long.
   * @param {number=} opt_radix The radix in which the text is written.
   * @return {!goog.math.Long} The corresponding Long value.
   */
  goog.math.Long.fromString = function(str, opt_radix) {
    if (str.length == 0) {
      throw Error('number format error: empty string');
    }
    var radix = opt_radix || 10;
    if (radix < 2 || 36 < radix) {
      throw Error('radix out of range: ' + radix);
    }
    if (str.charAt(0) == '-') {
      return goog.math.Long.fromString(str.substring(1), radix).negate();
    } else if (str.indexOf('-') >= 0) {
      throw Error('number format error: interior "-" character: ' + str);
    }
    // Do several (8) digits each time through the loop, so as to
    // minimize the calls to the very expensive emulated div.
    var radixToPower = goog.math.Long.fromNumber(Math.pow(radix, 8));
    var result = goog.math.Long.ZERO;
    for (var i = 0; i < str.length; i += 8) {
      var size = Math.min(8, str.length - i);
      var value = parseInt(str.substring(i, i + size), radix);
      if (size < 8) {
        var power = goog.math.Long.fromNumber(Math.pow(radix, size));
        result = result.multiply(power).add(goog.math.Long.fromNumber(value));
      } else {
        result = result.multiply(radixToPower);
        result = result.add(goog.math.Long.fromNumber(value));
      }
    }
    return result;
  };
  // NOTE: the compiler should inline these constant values below and then remove
  // these variables, so there should be no runtime penalty for these.
  /**
   * Number used repeated below in calculations.  This must appear before the
   * first call to any from* function below.
   * @type {number}
   * @private
   */
  goog.math.Long.TWO_PWR_16_DBL_ = 1 << 16;
  /**
   * @type {number}
   * @private
   */
  goog.math.Long.TWO_PWR_24_DBL_ = 1 << 24;
  /**
   * @type {number}
   * @private
   */
  goog.math.Long.TWO_PWR_32_DBL_ =
      goog.math.Long.TWO_PWR_16_DBL_ * goog.math.Long.TWO_PWR_16_DBL_;
  /**
   * @type {number}
   * @private
   */
  goog.math.Long.TWO_PWR_31_DBL_ =
      goog.math.Long.TWO_PWR_32_DBL_ / 2;
  /**
   * @type {number}
   * @private
   */
  goog.math.Long.TWO_PWR_48_DBL_ =
      goog.math.Long.TWO_PWR_32_DBL_ * goog.math.Long.TWO_PWR_16_DBL_;
  /**
   * @type {number}
   * @private
   */
  goog.math.Long.TWO_PWR_64_DBL_ =
      goog.math.Long.TWO_PWR_32_DBL_ * goog.math.Long.TWO_PWR_32_DBL_;
  /**
   * @type {number}
   * @private
   */
  goog.math.Long.TWO_PWR_63_DBL_ =
      goog.math.Long.TWO_PWR_64_DBL_ / 2;
  /** @type {!goog.math.Long} */
  goog.math.Long.ZERO = goog.math.Long.fromInt(0);
  /** @type {!goog.math.Long} */
  goog.math.Long.ONE = goog.math.Long.fromInt(1);
  /** @type {!goog.math.Long} */
  goog.math.Long.NEG_ONE = goog.math.Long.fromInt(-1);
  /** @type {!goog.math.Long} */
  goog.math.Long.MAX_VALUE =
      goog.math.Long.fromBits(0xFFFFFFFF | 0, 0x7FFFFFFF | 0);
  /** @type {!goog.math.Long} */
  goog.math.Long.MIN_VALUE = goog.math.Long.fromBits(0, 0x80000000 | 0);
  /**
   * @type {!goog.math.Long}
   * @private
   */
  goog.math.Long.TWO_PWR_24_ = goog.math.Long.fromInt(1 << 24);
  /** @return {number} The value, assuming it is a 32-bit integer. */
  goog.math.Long.prototype.toInt = function() {
    return this.low_;
  };
  /** @return {number} The closest floating-point representation to this value. */
  goog.math.Long.prototype.toNumber = function() {
    return this.high_ * goog.math.Long.TWO_PWR_32_DBL_ +
           this.getLowBitsUnsigned();
  };
  /**
   * @param {number=} opt_radix The radix in which the text should be written.
   * @return {string} The textual representation of this value.
   */
  goog.math.Long.prototype.toString = function(opt_radix) {
    var radix = opt_radix || 10;
    if (radix < 2 || 36 < radix) {
      throw Error('radix out of range: ' + radix);
    }
    if (this.isZero()) {
      return '0';
    }
    if (this.isNegative()) {
      if (this.equals(goog.math.Long.MIN_VALUE)) {
        // We need to change the Long value before it can be negated, so we remove
        // the bottom-most digit in this base and then recurse to do the rest.
        var radixLong = goog.math.Long.fromNumber(radix);
        var div = this.div(radixLong);
        var rem = div.multiply(radixLong).subtract(this);
        return div.toString(radix) + rem.toInt().toString(radix);
      } else {
        return '-' + this.negate().toString(radix);
      }
    }
    // Do several (6) digits each time through the loop, so as to
    // minimize the calls to the very expensive emulated div.
    var radixToPower = goog.math.Long.fromNumber(Math.pow(radix, 6));
    var rem = this;
    var result = '';
    while (true) {
      var remDiv = rem.div(radixToPower);
      var intval = rem.subtract(remDiv.multiply(radixToPower)).toInt();
      var digits = intval.toString(radix);
      rem = remDiv;
      if (rem.isZero()) {
        return digits + result;
      } else {
        while (digits.length < 6) {
          digits = '0' + digits;
        }
        result = '' + digits + result;
      }
    }
  };
  /** @return {number} The high 32-bits as a signed value. */
  goog.math.Long.prototype.getHighBits = function() {
    return this.high_;
  };
  /** @return {number} The low 32-bits as a signed value. */
  goog.math.Long.prototype.getLowBits = function() {
    return this.low_;
  };
  /** @return {number} The low 32-bits as an unsigned value. */
  goog.math.Long.prototype.getLowBitsUnsigned = function() {
    return (this.low_ >= 0) ?
        this.low_ : goog.math.Long.TWO_PWR_32_DBL_ + this.low_;
  };
  /**
   * @return {number} Returns the number of bits needed to represent the absolute
   *     value of this Long.
   */
  goog.math.Long.prototype.getNumBitsAbs = function() {
    if (this.isNegative()) {
      if (this.equals(goog.math.Long.MIN_VALUE)) {
        return 64;
      } else {
        return this.negate().getNumBitsAbs();
      }
    } else {
      var val = this.high_ != 0 ? this.high_ : this.low_;
      for (var bit = 31; bit > 0; bit--) {
        if ((val & (1 << bit)) != 0) {
          break;
        }
      }
      return this.high_ != 0 ? bit + 33 : bit + 1;
    }
  };
  /** @return {boolean} Whether this value is zero. */
  goog.math.Long.prototype.isZero = function() {
    return this.high_ == 0 && this.low_ == 0;
  };
  /** @return {boolean} Whether this value is negative. */
  goog.math.Long.prototype.isNegative = function() {
    return this.high_ < 0;
  };
  /** @return {boolean} Whether this value is odd. */
  goog.math.Long.prototype.isOdd = function() {
    return (this.low_ & 1) == 1;
  };
  /**
   * @param {goog.math.Long} other Long to compare against.
   * @return {boolean} Whether this Long equals the other.
   */
  goog.math.Long.prototype.equals = function(other) {
    return (this.high_ == other.high_) && (this.low_ == other.low_);
  };
  /**
   * @param {goog.math.Long} other Long to compare against.
   * @return {boolean} Whether this Long does not equal the other.
   */
  goog.math.Long.prototype.notEquals = function(other) {
    return (this.high_ != other.high_) || (this.low_ != other.low_);
  };
  /**
   * @param {goog.math.Long} other Long to compare against.
   * @return {boolean} Whether this Long is less than the other.
   */
  goog.math.Long.prototype.lessThan = function(other) {
    return this.compare(other) < 0;
  };
  /**
   * @param {goog.math.Long} other Long to compare against.
   * @return {boolean} Whether this Long is less than or equal to the other.
   */
  goog.math.Long.prototype.lessThanOrEqual = function(other) {
    return this.compare(other) <= 0;
  };
  /**
   * @param {goog.math.Long} other Long to compare against.
   * @return {boolean} Whether this Long is greater than the other.
   */
  goog.math.Long.prototype.greaterThan = function(other) {
    return this.compare(other) > 0;
  };
  /**
   * @param {goog.math.Long} other Long to compare against.
   * @return {boolean} Whether this Long is greater than or equal to the other.
   */
  goog.math.Long.prototype.greaterThanOrEqual = function(other) {
    return this.compare(other) >= 0;
  };
  /**
   * Compares this Long with the given one.
   * @param {goog.math.Long} other Long to compare against.
   * @return {number} 0 if they are the same, 1 if the this is greater, and -1
   *     if the given one is greater.
   */
  goog.math.Long.prototype.compare = function(other) {
    if (this.equals(other)) {
      return 0;
    }
    var thisNeg = this.isNegative();
    var otherNeg = other.isNegative();
    if (thisNeg && !otherNeg) {
      return -1;
    }
    if (!thisNeg && otherNeg) {
      return 1;
    }
    // at this point, the signs are the same, so subtraction will not overflow
    if (this.subtract(other).isNegative()) {
      return -1;
    } else {
      return 1;
    }
  };
  /** @return {!goog.math.Long} The negation of this value. */
  goog.math.Long.prototype.negate = function() {
    if (this.equals(goog.math.Long.MIN_VALUE)) {
      return goog.math.Long.MIN_VALUE;
    } else {
      return this.not().add(goog.math.Long.ONE);
    }
  };
  /**
   * Returns the sum of this and the given Long.
   * @param {goog.math.Long} other Long to add to this one.
   * @return {!goog.math.Long} The sum of this and the given Long.
   */
  goog.math.Long.prototype.add = function(other) {
    // Divide each number into 4 chunks of 16 bits, and then sum the chunks.
    var a48 = this.high_ >>> 16;
    var a32 = this.high_ & 0xFFFF;
    var a16 = this.low_ >>> 16;
    var a00 = this.low_ & 0xFFFF;
    var b48 = other.high_ >>> 16;
    var b32 = other.high_ & 0xFFFF;
    var b16 = other.low_ >>> 16;
    var b00 = other.low_ & 0xFFFF;
    var c48 = 0, c32 = 0, c16 = 0, c00 = 0;
    c00 += a00 + b00;
    c16 += c00 >>> 16;
    c00 &= 0xFFFF;
    c16 += a16 + b16;
    c32 += c16 >>> 16;
    c16 &= 0xFFFF;
    c32 += a32 + b32;
    c48 += c32 >>> 16;
    c32 &= 0xFFFF;
    c48 += a48 + b48;
    c48 &= 0xFFFF;
    return goog.math.Long.fromBits((c16 << 16) | c00, (c48 << 16) | c32);
  };
  /**
   * Returns the difference of this and the given Long.
   * @param {goog.math.Long} other Long to subtract from this.
   * @return {!goog.math.Long} The difference of this and the given Long.
   */
  goog.math.Long.prototype.subtract = function(other) {
    return this.add(other.negate());
  };
  /**
   * Returns the product of this and the given long.
   * @param {goog.math.Long} other Long to multiply with this.
   * @return {!goog.math.Long} The product of this and the other.
   */
  goog.math.Long.prototype.multiply = function(other) {
    if (this.isZero()) {
      return goog.math.Long.ZERO;
    } else if (other.isZero()) {
      return goog.math.Long.ZERO;
    }
    if (this.equals(goog.math.Long.MIN_VALUE)) {
      return other.isOdd() ? goog.math.Long.MIN_VALUE : goog.math.Long.ZERO;
    } else if (other.equals(goog.math.Long.MIN_VALUE)) {
      return this.isOdd() ? goog.math.Long.MIN_VALUE : goog.math.Long.ZERO;
    }
    if (this.isNegative()) {
      if (other.isNegative()) {
        return this.negate().multiply(other.negate());
      } else {
        return this.negate().multiply(other).negate();
      }
    } else if (other.isNegative()) {
      return this.multiply(other.negate()).negate();
    }
    // If both longs are small, use float multiplication
    if (this.lessThan(goog.math.Long.TWO_PWR_24_) &&
        other.lessThan(goog.math.Long.TWO_PWR_24_)) {
      return goog.math.Long.fromNumber(this.toNumber() * other.toNumber());
    }
    // Divide each long into 4 chunks of 16 bits, and then add up 4x4 products.
    // We can skip products that would overflow.
    var a48 = this.high_ >>> 16;
    var a32 = this.high_ & 0xFFFF;
    var a16 = this.low_ >>> 16;
    var a00 = this.low_ & 0xFFFF;
    var b48 = other.high_ >>> 16;
    var b32 = other.high_ & 0xFFFF;
    var b16 = other.low_ >>> 16;
    var b00 = other.low_ & 0xFFFF;
    var c48 = 0, c32 = 0, c16 = 0, c00 = 0;
    c00 += a00 * b00;
    c16 += c00 >>> 16;
    c00 &= 0xFFFF;
    c16 += a16 * b00;
    c32 += c16 >>> 16;
    c16 &= 0xFFFF;
    c16 += a00 * b16;
    c32 += c16 >>> 16;
    c16 &= 0xFFFF;
    c32 += a32 * b00;
    c48 += c32 >>> 16;
    c32 &= 0xFFFF;
    c32 += a16 * b16;
    c48 += c32 >>> 16;
    c32 &= 0xFFFF;
    c32 += a00 * b32;
    c48 += c32 >>> 16;
    c32 &= 0xFFFF;
    c48 += a48 * b00 + a32 * b16 + a16 * b32 + a00 * b48;
    c48 &= 0xFFFF;
    return goog.math.Long.fromBits((c16 << 16) | c00, (c48 << 16) | c32);
  };
  /**
   * Returns this Long divided by the given one.
   * @param {goog.math.Long} other Long by which to divide.
   * @return {!goog.math.Long} This Long divided by the given one.
   */
  goog.math.Long.prototype.div = function(other) {
    if (other.isZero()) {
      throw Error('division by zero');
    } else if (this.isZero()) {
      return goog.math.Long.ZERO;
    }
    if (this.equals(goog.math.Long.MIN_VALUE)) {
      if (other.equals(goog.math.Long.ONE) ||
          other.equals(goog.math.Long.NEG_ONE)) {
        return goog.math.Long.MIN_VALUE;  // recall that -MIN_VALUE == MIN_VALUE
      } else if (other.equals(goog.math.Long.MIN_VALUE)) {
        return goog.math.Long.ONE;
      } else {
        // At this point, we have |other| >= 2, so |this/other| < |MIN_VALUE|.
        var halfThis = this.shiftRight(1);
        var approx = halfThis.div(other).shiftLeft(1);
        if (approx.equals(goog.math.Long.ZERO)) {
          return other.isNegative() ? goog.math.Long.ONE : goog.math.Long.NEG_ONE;
        } else {
          var rem = this.subtract(other.multiply(approx));
          var result = approx.add(rem.div(other));
          return result;
        }
      }
    } else if (other.equals(goog.math.Long.MIN_VALUE)) {
      return goog.math.Long.ZERO;
    }
    if (this.isNegative()) {
      if (other.isNegative()) {
        return this.negate().div(other.negate());
      } else {
        return this.negate().div(other).negate();
      }
    } else if (other.isNegative()) {
      return this.div(other.negate()).negate();
    }
    // Repeat the following until the remainder is less than other:  find a
    // floating-point that approximates remainder / other *from below*, add this
    // into the result, and subtract it from the remainder.  It is critical that
    // the approximate value is less than or equal to the real value so that the
    // remainder never becomes negative.
    var res = goog.math.Long.ZERO;
    var rem = this;
    while (rem.greaterThanOrEqual(other)) {
      // Approximate the result of division. This may be a little greater or
      // smaller than the actual value.
      var approx = Math.max(1, Math.floor(rem.toNumber() / other.toNumber()));
      // We will tweak the approximate result by changing it in the 48-th digit or
      // the smallest non-fractional digit, whichever is larger.
      var log2 = Math.ceil(Math.log(approx) / Math.LN2);
      var delta = (log2 <= 48) ? 1 : Math.pow(2, log2 - 48);
      // Decrease the approximation until it is smaller than the remainder.  Note
      // that if it is too large, the product overflows and is negative.
      var approxRes = goog.math.Long.fromNumber(approx);
      var approxRem = approxRes.multiply(other);
      while (approxRem.isNegative() || approxRem.greaterThan(rem)) {
        approx -= delta;
        approxRes = goog.math.Long.fromNumber(approx);
        approxRem = approxRes.multiply(other);
      }
      // We know the answer can't be zero... and actually, zero would cause
      // infinite recursion since we would make no progress.
      if (approxRes.isZero()) {
        approxRes = goog.math.Long.ONE;
      }
      res = res.add(approxRes);
      rem = rem.subtract(approxRem);
    }
    return res;
  };
  /**
   * Returns this Long modulo the given one.
   * @param {goog.math.Long} other Long by which to mod.
   * @return {!goog.math.Long} This Long modulo the given one.
   */
  goog.math.Long.prototype.modulo = function(other) {
    return this.subtract(this.div(other).multiply(other));
  };
  /** @return {!goog.math.Long} The bitwise-NOT of this value. */
  goog.math.Long.prototype.not = function() {
    return goog.math.Long.fromBits(~this.low_, ~this.high_);
  };
  /**
   * Returns the bitwise-AND of this Long and the given one.
   * @param {goog.math.Long} other The Long with which to AND.
   * @return {!goog.math.Long} The bitwise-AND of this and the other.
   */
  goog.math.Long.prototype.and = function(other) {
    return goog.math.Long.fromBits(this.low_ & other.low_,
                                   this.high_ & other.high_);
  };
  /**
   * Returns the bitwise-OR of this Long and the given one.
   * @param {goog.math.Long} other The Long with which to OR.
   * @return {!goog.math.Long} The bitwise-OR of this and the other.
   */
  goog.math.Long.prototype.or = function(other) {
    return goog.math.Long.fromBits(this.low_ | other.low_,
                                   this.high_ | other.high_);
  };
  /**
   * Returns the bitwise-XOR of this Long and the given one.
   * @param {goog.math.Long} other The Long with which to XOR.
   * @return {!goog.math.Long} The bitwise-XOR of this and the other.
   */
  goog.math.Long.prototype.xor = function(other) {
    return goog.math.Long.fromBits(this.low_ ^ other.low_,
                                   this.high_ ^ other.high_);
  };
  /**
   * Returns this Long with bits shifted to the left by the given amount.
   * @param {number} numBits The number of bits by which to shift.
   * @return {!goog.math.Long} This shifted to the left by the given amount.
   */
  goog.math.Long.prototype.shiftLeft = function(numBits) {
    numBits &= 63;
    if (numBits == 0) {
      return this;
    } else {
      var low = this.low_;
      if (numBits < 32) {
        var high = this.high_;
        return goog.math.Long.fromBits(
            low << numBits,
            (high << numBits) | (low >>> (32 - numBits)));
      } else {
        return goog.math.Long.fromBits(0, low << (numBits - 32));
      }
    }
  };
  /**
   * Returns this Long with bits shifted to the right by the given amount.
   * @param {number} numBits The number of bits by which to shift.
   * @return {!goog.math.Long} This shifted to the right by the given amount.
   */
  goog.math.Long.prototype.shiftRight = function(numBits) {
    numBits &= 63;
    if (numBits == 0) {
      return this;
    } else {
      var high = this.high_;
      if (numBits < 32) {
        var low = this.low_;
        return goog.math.Long.fromBits(
            (low >>> numBits) | (high << (32 - numBits)),
            high >> numBits);
      } else {
        return goog.math.Long.fromBits(
            high >> (numBits - 32),
            high >= 0 ? 0 : -1);
      }
    }
  };
  /**
   * Returns this Long with bits shifted to the right by the given amount, with
   * the new top bits matching the current sign bit.
   * @param {number} numBits The number of bits by which to shift.
   * @return {!goog.math.Long} This shifted to the right by the given amount, with
   *     zeros placed into the new leading bits.
   */
  goog.math.Long.prototype.shiftRightUnsigned = function(numBits) {
    numBits &= 63;
    if (numBits == 0) {
      return this;
    } else {
      var high = this.high_;
      if (numBits < 32) {
        var low = this.low_;
        return goog.math.Long.fromBits(
            (low >>> numBits) | (high << (32 - numBits)),
            high >>> numBits);
      } else if (numBits == 32) {
        return goog.math.Long.fromBits(high, 0);
      } else {
        return goog.math.Long.fromBits(high >>> (numBits - 32), 0);
      }
    }
  };
  //======= begin jsbn =======
  var navigator = { appName: 'Modern Browser' }; // polyfill a little
  // Copyright (c) 2005  Tom Wu
  // All Rights Reserved.
  // http://www-cs-students.stanford.edu/~tjw/jsbn/
  /*
   * Copyright (c) 2003-2005  Tom Wu
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining
   * a copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sublicense, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
   * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
   * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
   *
   * IN NO EVENT SHALL TOM WU BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
   * INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
   * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF
   * THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT
   * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
   *
   * In addition, the following condition applies:
   *
   * All redistributions must retain an intact copy of this copyright notice
   * and disclaimer.
   */
  // Basic JavaScript BN library - subset useful for RSA encryption.
  // Bits per digit
  var dbits;
  // JavaScript engine analysis
  var canary = 0xdeadbeefcafe;
  var j_lm = ((canary&0xffffff)==0xefcafe);
  // (public) Constructor
  function BigInteger(a,b,c) {
    if(a != null)
      if("number" == typeof a) this.fromNumber(a,b,c);
      else if(b == null && "string" != typeof a) this.fromString(a,256);
      else this.fromString(a,b);
  }
  // return new, unset BigInteger
  function nbi() { return new BigInteger(null); }
  // am: Compute w_j += (x*this_i), propagate carries,
  // c is initial carry, returns final carry.
  // c < 3*dvalue, x < 2*dvalue, this_i < dvalue
  // We need to select the fastest one that works in this environment.
  // am1: use a single mult and divide to get the high bits,
  // max digit bits should be 26 because
  // max internal value = 2*dvalue^2-2*dvalue (< 2^53)
  function am1(i,x,w,j,c,n) {
    while(--n >= 0) {
      var v = x*this[i++]+w[j]+c;
      c = Math.floor(v/0x4000000);
      w[j++] = v&0x3ffffff;
    }
    return c;
  }
  // am2 avoids a big mult-and-extract completely.
  // Max digit bits should be <= 30 because we do bitwise ops
  // on values up to 2*hdvalue^2-hdvalue-1 (< 2^31)
  function am2(i,x,w,j,c,n) {
    var xl = x&0x7fff, xh = x>>15;
    while(--n >= 0) {
      var l = this[i]&0x7fff;
      var h = this[i++]>>15;
      var m = xh*l+h*xl;
      l = xl*l+((m&0x7fff)<<15)+w[j]+(c&0x3fffffff);
      c = (l>>>30)+(m>>>15)+xh*h+(c>>>30);
      w[j++] = l&0x3fffffff;
    }
    return c;
  }
  // Alternately, set max digit bits to 28 since some
  // browsers slow down when dealing with 32-bit numbers.
  function am3(i,x,w,j,c,n) {
    var xl = x&0x3fff, xh = x>>14;
    while(--n >= 0) {
      var l = this[i]&0x3fff;
      var h = this[i++]>>14;
      var m = xh*l+h*xl;
      l = xl*l+((m&0x3fff)<<14)+w[j]+c;
      c = (l>>28)+(m>>14)+xh*h;
      w[j++] = l&0xfffffff;
    }
    return c;
  }
  if(j_lm && (navigator.appName == "Microsoft Internet Explorer")) {
    BigInteger.prototype.am = am2;
    dbits = 30;
  }
  else if(j_lm && (navigator.appName != "Netscape")) {
    BigInteger.prototype.am = am1;
    dbits = 26;
  }
  else { // Mozilla/Netscape seems to prefer am3
    BigInteger.prototype.am = am3;
    dbits = 28;
  }
  BigInteger.prototype.DB = dbits;
  BigInteger.prototype.DM = ((1<<dbits)-1);
  BigInteger.prototype.DV = (1<<dbits);
  var BI_FP = 52;
  BigInteger.prototype.FV = Math.pow(2,BI_FP);
  BigInteger.prototype.F1 = BI_FP-dbits;
  BigInteger.prototype.F2 = 2*dbits-BI_FP;
  // Digit conversions
  var BI_RM = "0123456789abcdefghijklmnopqrstuvwxyz";
  var BI_RC = new Array();
  var rr,vv;
  rr = "0".charCodeAt(0);
  for(vv = 0; vv <= 9; ++vv) BI_RC[rr++] = vv;
  rr = "a".charCodeAt(0);
  for(vv = 10; vv < 36; ++vv) BI_RC[rr++] = vv;
  rr = "A".charCodeAt(0);
  for(vv = 10; vv < 36; ++vv) BI_RC[rr++] = vv;
  function int2char(n) { return BI_RM.charAt(n); }
  function intAt(s,i) {
    var c = BI_RC[s.charCodeAt(i)];
    return (c==null)?-1:c;
  }
  // (protected) copy this to r
  function bnpCopyTo(r) {
    for(var i = this.t-1; i >= 0; --i) r[i] = this[i];
    r.t = this.t;
    r.s = this.s;
  }
  // (protected) set from integer value x, -DV <= x < DV
  function bnpFromInt(x) {
    this.t = 1;
    this.s = (x<0)?-1:0;
    if(x > 0) this[0] = x;
    else if(x < -1) this[0] = x+DV;
    else this.t = 0;
  }
  // return bigint initialized to value
  function nbv(i) { var r = nbi(); r.fromInt(i); return r; }
  // (protected) set from string and radix
  function bnpFromString(s,b) {
    var k;
    if(b == 16) k = 4;
    else if(b == 8) k = 3;
    else if(b == 256) k = 8; // byte array
    else if(b == 2) k = 1;
    else if(b == 32) k = 5;
    else if(b == 4) k = 2;
    else { this.fromRadix(s,b); return; }
    this.t = 0;
    this.s = 0;
    var i = s.length, mi = false, sh = 0;
    while(--i >= 0) {
      var x = (k==8)?s[i]&0xff:intAt(s,i);
      if(x < 0) {
        if(s.charAt(i) == "-") mi = true;
        continue;
      }
      mi = false;
      if(sh == 0)
        this[this.t++] = x;
      else if(sh+k > this.DB) {
        this[this.t-1] |= (x&((1<<(this.DB-sh))-1))<<sh;
        this[this.t++] = (x>>(this.DB-sh));
      }
      else
        this[this.t-1] |= x<<sh;
      sh += k;
      if(sh >= this.DB) sh -= this.DB;
    }
    if(k == 8 && (s[0]&0x80) != 0) {
      this.s = -1;
      if(sh > 0) this[this.t-1] |= ((1<<(this.DB-sh))-1)<<sh;
    }
    this.clamp();
    if(mi) BigInteger.ZERO.subTo(this,this);
  }
  // (protected) clamp off excess high words
  function bnpClamp() {
    var c = this.s&this.DM;
    while(this.t > 0 && this[this.t-1] == c) --this.t;
  }
  // (public) return string representation in given radix
  function bnToString(b) {
    if(this.s < 0) return "-"+this.negate().toString(b);
    var k;
    if(b == 16) k = 4;
    else if(b == 8) k = 3;
    else if(b == 2) k = 1;
    else if(b == 32) k = 5;
    else if(b == 4) k = 2;
    else return this.toRadix(b);
    var km = (1<<k)-1, d, m = false, r = "", i = this.t;
    var p = this.DB-(i*this.DB)%k;
    if(i-- > 0) {
      if(p < this.DB && (d = this[i]>>p) > 0) { m = true; r = int2char(d); }
      while(i >= 0) {
        if(p < k) {
          d = (this[i]&((1<<p)-1))<<(k-p);
          d |= this[--i]>>(p+=this.DB-k);
        }
        else {
          d = (this[i]>>(p-=k))&km;
          if(p <= 0) { p += this.DB; --i; }
        }
        if(d > 0) m = true;
        if(m) r += int2char(d);
      }
    }
    return m?r:"0";
  }
  // (public) -this
  function bnNegate() { var r = nbi(); BigInteger.ZERO.subTo(this,r); return r; }
  // (public) |this|
  function bnAbs() { return (this.s<0)?this.negate():this; }
  // (public) return + if this > a, - if this < a, 0 if equal
  function bnCompareTo(a) {
    var r = this.s-a.s;
    if(r != 0) return r;
    var i = this.t;
    r = i-a.t;
    if(r != 0) return (this.s<0)?-r:r;
    while(--i >= 0) if((r=this[i]-a[i]) != 0) return r;
    return 0;
  }
  // returns bit length of the integer x
  function nbits(x) {
    var r = 1, t;
    if((t=x>>>16) != 0) { x = t; r += 16; }
    if((t=x>>8) != 0) { x = t; r += 8; }
    if((t=x>>4) != 0) { x = t; r += 4; }
    if((t=x>>2) != 0) { x = t; r += 2; }
    if((t=x>>1) != 0) { x = t; r += 1; }
    return r;
  }
  // (public) return the number of bits in "this"
  function bnBitLength() {
    if(this.t <= 0) return 0;
    return this.DB*(this.t-1)+nbits(this[this.t-1]^(this.s&this.DM));
  }
  // (protected) r = this << n*DB
  function bnpDLShiftTo(n,r) {
    var i;
    for(i = this.t-1; i >= 0; --i) r[i+n] = this[i];
    for(i = n-1; i >= 0; --i) r[i] = 0;
    r.t = this.t+n;
    r.s = this.s;
  }
  // (protected) r = this >> n*DB
  function bnpDRShiftTo(n,r) {
    for(var i = n; i < this.t; ++i) r[i-n] = this[i];
    r.t = Math.max(this.t-n,0);
    r.s = this.s;
  }
  // (protected) r = this << n
  function bnpLShiftTo(n,r) {
    var bs = n%this.DB;
    var cbs = this.DB-bs;
    var bm = (1<<cbs)-1;
    var ds = Math.floor(n/this.DB), c = (this.s<<bs)&this.DM, i;
    for(i = this.t-1; i >= 0; --i) {
      r[i+ds+1] = (this[i]>>cbs)|c;
      c = (this[i]&bm)<<bs;
    }
    for(i = ds-1; i >= 0; --i) r[i] = 0;
    r[ds] = c;
    r.t = this.t+ds+1;
    r.s = this.s;
    r.clamp();
  }
  // (protected) r = this >> n
  function bnpRShiftTo(n,r) {
    r.s = this.s;
    var ds = Math.floor(n/this.DB);
    if(ds >= this.t) { r.t = 0; return; }
    var bs = n%this.DB;
    var cbs = this.DB-bs;
    var bm = (1<<bs)-1;
    r[0] = this[ds]>>bs;
    for(var i = ds+1; i < this.t; ++i) {
      r[i-ds-1] |= (this[i]&bm)<<cbs;
      r[i-ds] = this[i]>>bs;
    }
    if(bs > 0) r[this.t-ds-1] |= (this.s&bm)<<cbs;
    r.t = this.t-ds;
    r.clamp();
  }
  // (protected) r = this - a
  function bnpSubTo(a,r) {
    var i = 0, c = 0, m = Math.min(a.t,this.t);
    while(i < m) {
      c += this[i]-a[i];
      r[i++] = c&this.DM;
      c >>= this.DB;
    }
    if(a.t < this.t) {
      c -= a.s;
      while(i < this.t) {
        c += this[i];
        r[i++] = c&this.DM;
        c >>= this.DB;
      }
      c += this.s;
    }
    else {
      c += this.s;
      while(i < a.t) {
        c -= a[i];
        r[i++] = c&this.DM;
        c >>= this.DB;
      }
      c -= a.s;
    }
    r.s = (c<0)?-1:0;
    if(c < -1) r[i++] = this.DV+c;
    else if(c > 0) r[i++] = c;
    r.t = i;
    r.clamp();
  }
  // (protected) r = this * a, r != this,a (HAC 14.12)
  // "this" should be the larger one if appropriate.
  function bnpMultiplyTo(a,r) {
    var x = this.abs(), y = a.abs();
    var i = x.t;
    r.t = i+y.t;
    while(--i >= 0) r[i] = 0;
    for(i = 0; i < y.t; ++i) r[i+x.t] = x.am(0,y[i],r,i,0,x.t);
    r.s = 0;
    r.clamp();
    if(this.s != a.s) BigInteger.ZERO.subTo(r,r);
  }
  // (protected) r = this^2, r != this (HAC 14.16)
  function bnpSquareTo(r) {
    var x = this.abs();
    var i = r.t = 2*x.t;
    while(--i >= 0) r[i] = 0;
    for(i = 0; i < x.t-1; ++i) {
      var c = x.am(i,x[i],r,2*i,0,1);
      if((r[i+x.t]+=x.am(i+1,2*x[i],r,2*i+1,c,x.t-i-1)) >= x.DV) {
        r[i+x.t] -= x.DV;
        r[i+x.t+1] = 1;
      }
    }
    if(r.t > 0) r[r.t-1] += x.am(i,x[i],r,2*i,0,1);
    r.s = 0;
    r.clamp();
  }
  // (protected) divide this by m, quotient and remainder to q, r (HAC 14.20)
  // r != q, this != m.  q or r may be null.
  function bnpDivRemTo(m,q,r) {
    var pm = m.abs();
    if(pm.t <= 0) return;
    var pt = this.abs();
    if(pt.t < pm.t) {
      if(q != null) q.fromInt(0);
      if(r != null) this.copyTo(r);
      return;
    }
    if(r == null) r = nbi();
    var y = nbi(), ts = this.s, ms = m.s;
    var nsh = this.DB-nbits(pm[pm.t-1]);	// normalize modulus
    if(nsh > 0) { pm.lShiftTo(nsh,y); pt.lShiftTo(nsh,r); }
    else { pm.copyTo(y); pt.copyTo(r); }
    var ys = y.t;
    var y0 = y[ys-1];
    if(y0 == 0) return;
    var yt = y0*(1<<this.F1)+((ys>1)?y[ys-2]>>this.F2:0);
    var d1 = this.FV/yt, d2 = (1<<this.F1)/yt, e = 1<<this.F2;
    var i = r.t, j = i-ys, t = (q==null)?nbi():q;
    y.dlShiftTo(j,t);
    if(r.compareTo(t) >= 0) {
      r[r.t++] = 1;
      r.subTo(t,r);
    }
    BigInteger.ONE.dlShiftTo(ys,t);
    t.subTo(y,y);	// "negative" y so we can replace sub with am later
    while(y.t < ys) y[y.t++] = 0;
    while(--j >= 0) {
      // Estimate quotient digit
      var qd = (r[--i]==y0)?this.DM:Math.floor(r[i]*d1+(r[i-1]+e)*d2);
      if((r[i]+=y.am(0,qd,r,j,0,ys)) < qd) {	// Try it out
        y.dlShiftTo(j,t);
        r.subTo(t,r);
        while(r[i] < --qd) r.subTo(t,r);
      }
    }
    if(q != null) {
      r.drShiftTo(ys,q);
      if(ts != ms) BigInteger.ZERO.subTo(q,q);
    }
    r.t = ys;
    r.clamp();
    if(nsh > 0) r.rShiftTo(nsh,r);	// Denormalize remainder
    if(ts < 0) BigInteger.ZERO.subTo(r,r);
  }
  // (public) this mod a
  function bnMod(a) {
    var r = nbi();
    this.abs().divRemTo(a,null,r);
    if(this.s < 0 && r.compareTo(BigInteger.ZERO) > 0) a.subTo(r,r);
    return r;
  }
  // Modular reduction using "classic" algorithm
  function Classic(m) { this.m = m; }
  function cConvert(x) {
    if(x.s < 0 || x.compareTo(this.m) >= 0) return x.mod(this.m);
    else return x;
  }
  function cRevert(x) { return x; }
  function cReduce(x) { x.divRemTo(this.m,null,x); }
  function cMulTo(x,y,r) { x.multiplyTo(y,r); this.reduce(r); }
  function cSqrTo(x,r) { x.squareTo(r); this.reduce(r); }
  Classic.prototype.convert = cConvert;
  Classic.prototype.revert = cRevert;
  Classic.prototype.reduce = cReduce;
  Classic.prototype.mulTo = cMulTo;
  Classic.prototype.sqrTo = cSqrTo;
  // (protected) return "-1/this % 2^DB"; useful for Mont. reduction
  // justification:
  //         xy == 1 (mod m)
  //         xy =  1+km
  //   xy(2-xy) = (1+km)(1-km)
  // x[y(2-xy)] = 1-k^2m^2
  // x[y(2-xy)] == 1 (mod m^2)
  // if y is 1/x mod m, then y(2-xy) is 1/x mod m^2
  // should reduce x and y(2-xy) by m^2 at each step to keep size bounded.
  // JS multiply "overflows" differently from C/C++, so care is needed here.
  function bnpInvDigit() {
    if(this.t < 1) return 0;
    var x = this[0];
    if((x&1) == 0) return 0;
    var y = x&3;		// y == 1/x mod 2^2
    y = (y*(2-(x&0xf)*y))&0xf;	// y == 1/x mod 2^4
    y = (y*(2-(x&0xff)*y))&0xff;	// y == 1/x mod 2^8
    y = (y*(2-(((x&0xffff)*y)&0xffff)))&0xffff;	// y == 1/x mod 2^16
    // last step - calculate inverse mod DV directly;
    // assumes 16 < DB <= 32 and assumes ability to handle 48-bit ints
    y = (y*(2-x*y%this.DV))%this.DV;		// y == 1/x mod 2^dbits
    // we really want the negative inverse, and -DV < y < DV
    return (y>0)?this.DV-y:-y;
  }
  // Montgomery reduction
  function Montgomery(m) {
    this.m = m;
    this.mp = m.invDigit();
    this.mpl = this.mp&0x7fff;
    this.mph = this.mp>>15;
    this.um = (1<<(m.DB-15))-1;
    this.mt2 = 2*m.t;
  }
  // xR mod m
  function montConvert(x) {
    var r = nbi();
    x.abs().dlShiftTo(this.m.t,r);
    r.divRemTo(this.m,null,r);
    if(x.s < 0 && r.compareTo(BigInteger.ZERO) > 0) this.m.subTo(r,r);
    return r;
  }
  // x/R mod m
  function montRevert(x) {
    var r = nbi();
    x.copyTo(r);
    this.reduce(r);
    return r;
  }
  // x = x/R mod m (HAC 14.32)
  function montReduce(x) {
    while(x.t <= this.mt2)	// pad x so am has enough room later
      x[x.t++] = 0;
    for(var i = 0; i < this.m.t; ++i) {
      // faster way of calculating u0 = x[i]*mp mod DV
      var j = x[i]&0x7fff;
      var u0 = (j*this.mpl+(((j*this.mph+(x[i]>>15)*this.mpl)&this.um)<<15))&x.DM;
      // use am to combine the multiply-shift-add into one call
      j = i+this.m.t;
      x[j] += this.m.am(0,u0,x,i,0,this.m.t);
      // propagate carry
      while(x[j] >= x.DV) { x[j] -= x.DV; x[++j]++; }
    }
    x.clamp();
    x.drShiftTo(this.m.t,x);
    if(x.compareTo(this.m) >= 0) x.subTo(this.m,x);
  }
  // r = "x^2/R mod m"; x != r
  function montSqrTo(x,r) { x.squareTo(r); this.reduce(r); }
  // r = "xy/R mod m"; x,y != r
  function montMulTo(x,y,r) { x.multiplyTo(y,r); this.reduce(r); }
  Montgomery.prototype.convert = montConvert;
  Montgomery.prototype.revert = montRevert;
  Montgomery.prototype.reduce = montReduce;
  Montgomery.prototype.mulTo = montMulTo;
  Montgomery.prototype.sqrTo = montSqrTo;
  // (protected) true iff this is even
  function bnpIsEven() { return ((this.t>0)?(this[0]&1):this.s) == 0; }
  // (protected) this^e, e < 2^32, doing sqr and mul with "r" (HAC 14.79)
  function bnpExp(e,z) {
    if(e > 0xffffffff || e < 1) return BigInteger.ONE;
    var r = nbi(), r2 = nbi(), g = z.convert(this), i = nbits(e)-1;
    g.copyTo(r);
    while(--i >= 0) {
      z.sqrTo(r,r2);
      if((e&(1<<i)) > 0) z.mulTo(r2,g,r);
      else { var t = r; r = r2; r2 = t; }
    }
    return z.revert(r);
  }
  // (public) this^e % m, 0 <= e < 2^32
  function bnModPowInt(e,m) {
    var z;
    if(e < 256 || m.isEven()) z = new Classic(m); else z = new Montgomery(m);
    return this.exp(e,z);
  }
  // protected
  BigInteger.prototype.copyTo = bnpCopyTo;
  BigInteger.prototype.fromInt = bnpFromInt;
  BigInteger.prototype.fromString = bnpFromString;
  BigInteger.prototype.clamp = bnpClamp;
  BigInteger.prototype.dlShiftTo = bnpDLShiftTo;
  BigInteger.prototype.drShiftTo = bnpDRShiftTo;
  BigInteger.prototype.lShiftTo = bnpLShiftTo;
  BigInteger.prototype.rShiftTo = bnpRShiftTo;
  BigInteger.prototype.subTo = bnpSubTo;
  BigInteger.prototype.multiplyTo = bnpMultiplyTo;
  BigInteger.prototype.squareTo = bnpSquareTo;
  BigInteger.prototype.divRemTo = bnpDivRemTo;
  BigInteger.prototype.invDigit = bnpInvDigit;
  BigInteger.prototype.isEven = bnpIsEven;
  BigInteger.prototype.exp = bnpExp;
  // public
  BigInteger.prototype.toString = bnToString;
  BigInteger.prototype.negate = bnNegate;
  BigInteger.prototype.abs = bnAbs;
  BigInteger.prototype.compareTo = bnCompareTo;
  BigInteger.prototype.bitLength = bnBitLength;
  BigInteger.prototype.mod = bnMod;
  BigInteger.prototype.modPowInt = bnModPowInt;
  // "constants"
  BigInteger.ZERO = nbv(0);
  BigInteger.ONE = nbv(1);
  // jsbn2 stuff
  // (protected) convert from radix string
  function bnpFromRadix(s,b) {
    this.fromInt(0);
    if(b == null) b = 10;
    var cs = this.chunkSize(b);
    var d = Math.pow(b,cs), mi = false, j = 0, w = 0;
    for(var i = 0; i < s.length; ++i) {
      var x = intAt(s,i);
      if(x < 0) {
        if(s.charAt(i) == "-" && this.signum() == 0) mi = true;
        continue;
      }
      w = b*w+x;
      if(++j >= cs) {
        this.dMultiply(d);
        this.dAddOffset(w,0);
        j = 0;
        w = 0;
      }
    }
    if(j > 0) {
      this.dMultiply(Math.pow(b,j));
      this.dAddOffset(w,0);
    }
    if(mi) BigInteger.ZERO.subTo(this,this);
  }
  // (protected) return x s.t. r^x < DV
  function bnpChunkSize(r) { return Math.floor(Math.LN2*this.DB/Math.log(r)); }
  // (public) 0 if this == 0, 1 if this > 0
  function bnSigNum() {
    if(this.s < 0) return -1;
    else if(this.t <= 0 || (this.t == 1 && this[0] <= 0)) return 0;
    else return 1;
  }
  // (protected) this *= n, this >= 0, 1 < n < DV
  function bnpDMultiply(n) {
    this[this.t] = this.am(0,n-1,this,0,0,this.t);
    ++this.t;
    this.clamp();
  }
  // (protected) this += n << w words, this >= 0
  function bnpDAddOffset(n,w) {
    if(n == 0) return;
    while(this.t <= w) this[this.t++] = 0;
    this[w] += n;
    while(this[w] >= this.DV) {
      this[w] -= this.DV;
      if(++w >= this.t) this[this.t++] = 0;
      ++this[w];
    }
  }
  // (protected) convert to radix string
  function bnpToRadix(b) {
    if(b == null) b = 10;
    if(this.signum() == 0 || b < 2 || b > 36) return "0";
    var cs = this.chunkSize(b);
    var a = Math.pow(b,cs);
    var d = nbv(a), y = nbi(), z = nbi(), r = "";
    this.divRemTo(d,y,z);
    while(y.signum() > 0) {
      r = (a+z.intValue()).toString(b).substr(1) + r;
      y.divRemTo(d,y,z);
    }
    return z.intValue().toString(b) + r;
  }
  // (public) return value as integer
  function bnIntValue() {
    if(this.s < 0) {
      if(this.t == 1) return this[0]-this.DV;
      else if(this.t == 0) return -1;
    }
    else if(this.t == 1) return this[0];
    else if(this.t == 0) return 0;
    // assumes 16 < DB < 32
    return ((this[1]&((1<<(32-this.DB))-1))<<this.DB)|this[0];
  }
  // (protected) r = this + a
  function bnpAddTo(a,r) {
    var i = 0, c = 0, m = Math.min(a.t,this.t);
    while(i < m) {
      c += this[i]+a[i];
      r[i++] = c&this.DM;
      c >>= this.DB;
    }
    if(a.t < this.t) {
      c += a.s;
      while(i < this.t) {
        c += this[i];
        r[i++] = c&this.DM;
        c >>= this.DB;
      }
      c += this.s;
    }
    else {
      c += this.s;
      while(i < a.t) {
        c += a[i];
        r[i++] = c&this.DM;
        c >>= this.DB;
      }
      c += a.s;
    }
    r.s = (c<0)?-1:0;
    if(c > 0) r[i++] = c;
    else if(c < -1) r[i++] = this.DV+c;
    r.t = i;
    r.clamp();
  }
  BigInteger.prototype.fromRadix = bnpFromRadix;
  BigInteger.prototype.chunkSize = bnpChunkSize;
  BigInteger.prototype.signum = bnSigNum;
  BigInteger.prototype.dMultiply = bnpDMultiply;
  BigInteger.prototype.dAddOffset = bnpDAddOffset;
  BigInteger.prototype.toRadix = bnpToRadix;
  BigInteger.prototype.intValue = bnIntValue;
  BigInteger.prototype.addTo = bnpAddTo;
  //======= end jsbn =======
  // Emscripten wrapper
  var Wrapper = {
    subtract: function(xl, xh, yl, yh) {
      var x = new goog.math.Long(xl, xh);
      var y = new goog.math.Long(yl, yh);
      var ret = x.subtract(y);
      HEAP32[tempDoublePtr>>2] = ret.low_;
      HEAP32[tempDoublePtr+4>>2] = ret.high_;
    },
    multiply: function(xl, xh, yl, yh) {
      var x = new goog.math.Long(xl, xh);
      var y = new goog.math.Long(yl, yh);
      var ret = x.multiply(y);
      HEAP32[tempDoublePtr>>2] = ret.low_;
      HEAP32[tempDoublePtr+4>>2] = ret.high_;
    },
    abs: function(l, h) {
      var x = new goog.math.Long(l, h);
      var ret;
      if (x.isNegative()) {
        ret = x.negate();
      } else {
        ret = x;
      }
      HEAP32[tempDoublePtr>>2] = ret.low_;
      HEAP32[tempDoublePtr+4>>2] = ret.high_;
    },
    ensureTemps: function() {
      if (Wrapper.ensuredTemps) return;
      Wrapper.ensuredTemps = true;
      Wrapper.two32 = new BigInteger();
      Wrapper.two32.fromString('4294967296', 10);
      Wrapper.two64 = new BigInteger();
      Wrapper.two64.fromString('18446744073709551616', 10);
      Wrapper.temp1 = new BigInteger();
      Wrapper.temp2 = new BigInteger();
    },
    lh2bignum: function(l, h) {
      var a = new BigInteger();
      a.fromString(h.toString(), 10);
      var b = new BigInteger();
      a.multiplyTo(Wrapper.two32, b);
      var c = new BigInteger();
      c.fromString(l.toString(), 10);
      var d = new BigInteger();
      c.addTo(b, d);
      return d;
    },
    divide: function(xl, xh, yl, yh, unsigned) {
      Wrapper.ensureTemps();
      if (!unsigned) {
        var x = new goog.math.Long(xl, xh);
        var y = new goog.math.Long(yl, yh);
        var ret = x.div(y);
        HEAP32[tempDoublePtr>>2] = ret.low_;
        HEAP32[tempDoublePtr+4>>2] = ret.high_;
      } else {
        // slow precise bignum division
        var x = Wrapper.lh2bignum(xl >>> 0, xh >>> 0);
        var y = Wrapper.lh2bignum(yl >>> 0, yh >>> 0);
        var z = new BigInteger();
        x.divRemTo(y, z, null);
        var l = new BigInteger();
        var h = new BigInteger();
        z.divRemTo(Wrapper.two32, h, l);
        HEAP32[tempDoublePtr>>2] = parseInt(l.toString()) | 0;
        HEAP32[tempDoublePtr+4>>2] = parseInt(h.toString()) | 0;
      }
    },
    modulo: function(xl, xh, yl, yh, unsigned) {
      Wrapper.ensureTemps();
      if (!unsigned) {
        var x = new goog.math.Long(xl, xh);
        var y = new goog.math.Long(yl, yh);
        var ret = x.modulo(y);
        HEAP32[tempDoublePtr>>2] = ret.low_;
        HEAP32[tempDoublePtr+4>>2] = ret.high_;
      } else {
        // slow precise bignum division
        var x = Wrapper.lh2bignum(xl >>> 0, xh >>> 0);
        var y = Wrapper.lh2bignum(yl >>> 0, yh >>> 0);
        var z = new BigInteger();
        x.divRemTo(y, null, z);
        var l = new BigInteger();
        var h = new BigInteger();
        z.divRemTo(Wrapper.two32, h, l);
        HEAP32[tempDoublePtr>>2] = parseInt(l.toString()) | 0;
        HEAP32[tempDoublePtr+4>>2] = parseInt(h.toString()) | 0;
      }
    },
    stringify: function(l, h, unsigned) {
      var ret = new goog.math.Long(l, h).toString();
      if (unsigned && ret[0] == '-') {
        // unsign slowly using jsbn bignums
        Wrapper.ensureTemps();
        var bignum = new BigInteger();
        bignum.fromString(ret, 10);
        ret = new BigInteger();
        Wrapper.two64.addTo(bignum, ret);
        ret = ret.toString(10);
      }
      return ret;
    },
    fromString: function(str, base, min, max, unsigned) {
      Wrapper.ensureTemps();
      var bignum = new BigInteger();
      bignum.fromString(str, base);
      var bigmin = new BigInteger();
      bigmin.fromString(min, 10);
      var bigmax = new BigInteger();
      bigmax.fromString(max, 10);
      if (unsigned && bignum.compareTo(BigInteger.ZERO) < 0) {
        var temp = new BigInteger();
        bignum.addTo(Wrapper.two64, temp);
        bignum = temp;
      }
      var error = false;
      if (bignum.compareTo(bigmin) < 0) {
        bignum = bigmin;
        error = true;
      } else if (bignum.compareTo(bigmax) > 0) {
        bignum = bigmax;
        error = true;
      }
      var ret = goog.math.Long.fromString(bignum.toString()); // min-max checks should have clamped this to a range goog.math.Long can handle well
      HEAP32[tempDoublePtr>>2] = ret.low_;
      HEAP32[tempDoublePtr+4>>2] = ret.high_;
      if (error) throw 'range error';
    }
  };
  return Wrapper;
})();
//======= end closure i64 code =======
// === Auto-generated postamble setup entry stuff ===
Module.callMain = function callMain(args) {
  var argc = args.length+1;
  function pad() {
    for (var i = 0; i < 4-1; i++) {
      argv.push(0);
    }
  }
  var argv = [allocate(intArrayFromString("/bin/this.program"), 'i8', ALLOC_STATIC) ];
  pad();
  for (var i = 0; i < argc-1; i = i + 1) {
    argv.push(allocate(intArrayFromString(args[i]), 'i8', ALLOC_STATIC));
    pad();
  }
  argv.push(0);
  argv = allocate(argv, 'i32', ALLOC_STATIC);
  var ret;
  var initialStackTop = STACKTOP;
  try {
    ret = Module['_main'](argc, argv, 0);
  }
  catch(e) {
    if (e.name == 'ExitStatus') {
      return e.status;
    } else if (e == 'SimulateInfiniteLoop') {
      Module['noExitRuntime'] = true;
    } else {
      throw e;
    }
  } finally {
    STACKTOP = initialStackTop;
  }
  return ret;
}
function run(args) {
  args = args || Module['arguments'];
  if (runDependencies > 0) {
    Module.printErr('run() called, but dependencies remain, so not running');
    return 0;
  }
  if (Module['preRun']) {
    if (typeof Module['preRun'] == 'function') Module['preRun'] = [Module['preRun']];
    var toRun = Module['preRun'];
    Module['preRun'] = [];
    for (var i = toRun.length-1; i >= 0; i--) {
      toRun[i]();
    }
    if (runDependencies > 0) {
      // a preRun added a dependency, run will be called later
      return 0;
    }
  }
  function doRun() {
    var ret = 0;
    calledRun = true;
    if (Module['_main']) {
      preMain();
      ret = Module.callMain(args);
      if (!Module['noExitRuntime']) {
        exitRuntime();
      }
    }
    if (Module['postRun']) {
      if (typeof Module['postRun'] == 'function') Module['postRun'] = [Module['postRun']];
      while (Module['postRun'].length > 0) {
        Module['postRun'].pop()();
      }
    }
    return ret;
  }
  if (Module['setStatus']) {
    Module['setStatus']('Running...');
    setTimeout(function() {
      setTimeout(function() {
        Module['setStatus']('');
      }, 1);
      doRun();
    }, 1);
    return 0;
  } else {
    return doRun();
  }
}
Module['run'] = Module.run = run;
// {{PRE_RUN_ADDITIONS}}
if (Module['preInit']) {
  if (typeof Module['preInit'] == 'function') Module['preInit'] = [Module['preInit']];
  while (Module['preInit'].length > 0) {
    Module['preInit'].pop()();
  }
}
initRuntime();
var shouldRunNow = true;
if (Module['noInitialRun']) {
  shouldRunNow = false;
}
if (shouldRunNow) {
  run();
}
// {{POST_RUN_ADDITIONS}}
  // {{MODULE_ADDITIONS}}
