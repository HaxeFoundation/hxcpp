/*
 * Copyright (C)2005-2012 Haxe Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <hxcpp.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "mysql.h"
#include <string.h>

#ifdef HX_ANDROID
#define atof(x) strtod((x),0)
#endif

/**
   <doc>
   <h1>MySQL</h1>
   <p>
   API to connect and use MySQL database
   </p>
   </doc>
**/


#define HXTHROW(x) hx::Throw(HX_CSTRING(x))




namespace
{

struct Connection : public hx::Object
{
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdMysql };


   MYSQL *m;


   void create(MYSQL *inM)
   {
      m = inM;
      _hx_set_finalizer(this, finalize);
   }

   void destroy()
   {
      if (m)
      {
         mysql_close(m);
         m = 0;
      }
   }

   static void finalize(Dynamic obj)
   {
      ((Connection *)(obj.mPtr))->destroy();
   }
};

Connection *getConnection(Dynamic o)
{
   Connection *connection = dynamic_cast<Connection *>(o.mPtr);
   if (!connection || !connection->m)
      hx::Throw( HX_CSTRING("Invalid Connection") );
   return connection;
}


static void error( MYSQL *m, const char *msg )
{
   hx::Throw( String(msg) + HX_CSTRING(" ") + String(mysql_error(m)) );
}

// ---------------------------------------------------------------
// Result

/**
   <doc><h2>Result</h2></doc>
**/

#undef CONV_FLOAT
typedef enum {
   CONV_INT,
   CONV_STRING,
   CONV_FLOAT,
   CONV_BINARY,
   CONV_DATE,
   CONV_DATETIME,
   CONV_BOOL
} CONV;

struct Result : public hx::Object
{
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdMysqlResult };

   MYSQL_RES *r;
   int nfields;
   CONV *fields_convs;
   String *field_names;
   MYSQL_ROW current;

   void create(MYSQL_RES *inR)
   {
      r = inR;
      fields_convs = 0;
      field_names = 0;
      nfields = 0;
      _hx_set_finalizer(this, finalize);
   }

   void destroy()
   {
      if (r)
      {
         if (fields_convs)
           free(fields_convs);
         if (field_names)
           free(field_names);
         fields_convs = 0;
         field_names = 0;
         mysql_free_result(r);
         r = 0;
      }
   }

   int numRows() { return mysql_num_rows(r); }

   static void finalize(Dynamic obj)
   {
      ((Result *)(obj.mPtr))->destroy();
   }

};

Result *getResult(Dynamic o)
{
   Result *result = dynamic_cast<Result *>(o.mPtr);
   if (!result)
      HXTHROW("Invalid result");
   return result;
}

cpp::Function< Dynamic(Dynamic) > gDataToBytes;
cpp::Function< Dynamic(Float) > gDateFromSeconds;

}

void _hx_mysql_set_conversion(
      cpp::Function< Dynamic(Dynamic) > inDataToBytes,
      cpp::Function< Dynamic(Float) > inDateFromSeconds )
{
   gDataToBytes = inDataToBytes;
   gDateFromSeconds = inDateFromSeconds;
}



/**
   result_get_length : 'result -> int
   <doc>Return the number of rows returned or affected</doc>
**/
int  _hx_mysql_result_get_length(Dynamic handle)
{
   if( handle->__GetType() == vtInt )
      return handle;

   return getResult(handle)->numRows();
}

/**
   result_get_nfields : 'result -> int
   <doc>Return the number of fields in a result row</doc>
**/
int  _hx_mysql_result_get_nfields(Dynamic handle)
{
   if( handle->__GetType() == vtInt )
     return 0;

   return getResult(handle)->nfields;
}

/**
   result_get_fields_names : 'result -> string array
   <doc>Return the fields names corresponding results columns</doc>
**/
Array<String> _hx_mysql_result_get_fields_names(Dynamic handle)
{
   Result *r = getResult(handle);

   MYSQL_FIELD *fields = mysql_fetch_fields(r->r);
   int count = r->nfields;
   Array<String> output = Array_obj<String>::__new(count);

   for(int k=0;k<count;k++)
      output[k] = String(fields[k].name);

   return output;
}

/**
   result_next : 'result -> object?
   <doc>
   Return the next row if available. A row is represented
   as an object, which fields have been converted to the
   corresponding Neko value (int, float or string). For
   Date and DateTime you can specify your own conversion
   function using [result_set_conv_date]. By default they're
   returned as plain strings. Additionally, the TINYINT(1) will
   be converted to either true or false if equal to 0.
   </doc>
**/
Dynamic _hx_mysql_result_next(Dynamic handle)
{
   Result *r = getResult(handle);
   MYSQL_ROW row = mysql_fetch_row(r->r);
   if( !row )
      return null();

   int count = r->nfields;
   hx::Anon cur = hx::Anon_obj::Create(0);

   r->current = row;
   unsigned long *lengths = 0;
   for(int i=0;i<r->nfields;i++)
   {
      if( row[i] )
      {
         Dynamic v;
         switch( r->fields_convs[i] )
         {
            case CONV_INT:
               v = atoi(row[i]);
               break;
            case CONV_STRING:
               v = String(row[i]);
               break;
            case CONV_BOOL:
               v = *row[i] != '0';
               break;
            case CONV_FLOAT:
               v = atof(row[i]);
               break;
            case CONV_BINARY:
               {
               if( lengths == NULL )
               {
                  lengths = mysql_fetch_lengths(r->r);
                  if( lengths == NULL )
                     HXTHROW("mysql_fetch_lengths");
               }
               Array<unsigned char> buf = Array_obj<unsigned char>::__new(lengths[i],lengths[i]);
               memcpy(&buf[0],row[i],lengths[i]);
               v = gDataToBytes.call(buf);
               }
               break;

            case CONV_DATE:
               {
                  struct tm t;
                  sscanf(row[i],"%4d-%2d-%2d",&t.tm_year,&t.tm_mon,&t.tm_mday);
                  t.tm_hour = 0;
                  t.tm_min = 0;
                  t.tm_sec = 0;
                  t.tm_isdst = -1;
                  t.tm_year -= 1900;
                  t.tm_mon--;
                  v = gDateFromSeconds.call((int)mktime(&t));
               }
               break;
            case CONV_DATETIME:
               {
                  struct tm t;
                  sscanf(row[i],"%4d-%2d-%2d %2d:%2d:%2d",&t.tm_year,&t.tm_mon,&t.tm_mday,&t.tm_hour,&t.tm_min,&t.tm_sec);
                  t.tm_isdst = -1;
                  t.tm_year -= 1900;
                  t.tm_mon--;
                  v = gDateFromSeconds.call(mktime(&t));
               }
               break;
            default:
               break;
         }
         cur->__SetField(r->field_names[i],v, hx::paccDynamic );
      }
   }
   return cur;
}


/**
   result_get : 'result -> n:int -> string
   <doc>Return the [n]th field of the current row</doc>
**/
String  _hx_mysql_result_get(Dynamic handle,int n)
{
   Result *r = getResult(handle);
   if( n < 0 || n >= r->nfields )
      HXTHROW("Invalid index");

   if( !r->current )
   {
      _hx_mysql_result_next(handle);
      if( !r->current )
         HXTHROW("No more results");
   }

   return String(r->current[n]);
}

/**
   result_get_int : 'result -> n:int -> int
   <doc>Return the [n]th field of the current row as an integer (or 0)</doc>
**/
int _hx_mysql_result_get_int(Dynamic handle,int n)
{
   Result *r = getResult(handle);
   if( n < 0 || n >= r->nfields )
      HXTHROW("Invalid index");

   if( !r->current )
   {
      _hx_mysql_result_next(handle);
      if( !r->current )
         HXTHROW("No more results");
   }

   const char *s = r->current[n];
   return  s?atoi(s):0;
}

/**
   result_get_float : 'result -> n:int -> float
   <doc>Return the [n]th field of the current row as a float (or 0)</doc>
**/
Float   _hx_mysql_result_get_float(Dynamic handle,int n)
{
   Result *r = getResult(handle);
   if( n < 0 || n >= r->nfields )
      HXTHROW("Invalid index");

   if( !r->current )
   {
      _hx_mysql_result_next(handle);
      if( !r->current )
         HXTHROW("No more results");
   }

   const char *s = r->current[n];
   return s?atof(s):0;
}

static CONV convert_type( enum enum_field_types t, int flags, unsigned int length ) {
   // FIELD_TYPE_TIME
   // FIELD_TYPE_YEAR
   // FIELD_TYPE_NEWDATE
   // FIELD_TYPE_NEWDATE + 2: // 5.0 MYSQL_TYPE_BIT
   switch( t ) {
   case FIELD_TYPE_TINY:
      if( length == 1 )
         return CONV_BOOL;
   case FIELD_TYPE_SHORT:
   case FIELD_TYPE_LONG:
   case FIELD_TYPE_INT24:
      return CONV_INT;
   case FIELD_TYPE_LONGLONG:
   case FIELD_TYPE_DECIMAL:
   case FIELD_TYPE_FLOAT:
   case FIELD_TYPE_DOUBLE:
   case 246: // 5.0 MYSQL_NEW_DECIMAL
      return CONV_FLOAT;
   case FIELD_TYPE_BLOB:
   case FIELD_TYPE_TINY_BLOB:
   case FIELD_TYPE_MEDIUM_BLOB:
   case FIELD_TYPE_LONG_BLOB:
      if( (flags & BINARY_FLAG) != 0 )
         return CONV_BINARY;
      return CONV_STRING;
   case FIELD_TYPE_DATETIME:
   case FIELD_TYPE_TIMESTAMP:
      return CONV_DATETIME;
   case FIELD_TYPE_DATE:
      return CONV_DATE;
   case FIELD_TYPE_NULL:
   case FIELD_TYPE_ENUM:
   case FIELD_TYPE_SET:
   //case FIELD_TYPE_VAR_STRING:
   //case FIELD_TYPE_GEOMETRY:
   // 5.0 MYSQL_TYPE_VARCHAR
   default:
      if( (flags & BINARY_FLAG) != 0 )
         return CONV_BINARY;
      return CONV_STRING;
   }
}



static Result *alloc_result( Connection *c, MYSQL_RES *r )
{
   Result *res = new Result();
   res->create(r);

   int num_fields = mysql_num_fields(r);
   int i,j;
   MYSQL_FIELD *fields = mysql_fetch_fields(r);
   res->current = 0;
   res->nfields = num_fields;
   res->field_names = (String *)malloc(sizeof(String)*num_fields);
   res->fields_convs = (CONV*)malloc(sizeof(CONV)*num_fields);   

   for(i=0;i<num_fields;i++)
   {
      String name;
      if( strchr(fields[i].name,'(') )
         name = String::createPermanent("???",3); // looks like an inner request : prevent hashing + cashing it
      else
         name = String::createPermanent(fields[i].name, -1);

      res->field_names[i] = name;
      res->fields_convs[i] = convert_type(fields[i].type,fields[i].flags,fields[i].length);
   }

   return res;
}

// ---------------------------------------------------------------
// Connection

/** <doc><h2>Connection</h2></doc> **/

/**
   close : 'connection -> void
   <doc>Close the connection. Any subsequent operation will fail on it</doc>
**/
Dynamic _hx_mysql_close(Dynamic handle)
{
   Connection *connection = getConnection(handle);
   connection->destroy();
   return true;
}

/**
   select_db : 'connection -> string -> void
   <doc>Select the database</doc>
**/
void _hx_mysql_select_db(Dynamic handle,String db)
{
   Connection *connection = getConnection(handle);

   if( mysql_select_db(connection->m,db.utf8_str()) != 0 )
      error(connection->m,"Failed to select database :");
}

/**
   request : 'connection -> string -> 'result
   <doc>Execute an SQL request. Exception on error</doc>
**/
Dynamic _hx_mysql_request(Dynamic handle,String req)
{
   Connection *connection = getConnection(handle);

   if( mysql_real_query(connection->m,req.utf8_str(),req.length) != 0 )
      error(connection->m,req);

   MYSQL_RES *res = mysql_store_result(connection->m);
   if( !res )
   {
      if( mysql_field_count(connection->m) == 0 )
         return mysql_affected_rows(connection->m);
      else
         error(connection->m,req);
   }

   return alloc_result(connection,res);
}


/**
   escape : 'connection -> string -> string
   <doc>Escape the string for inserting into a SQL request</doc>
**/
struct AutoBuf
{
   AutoBuf(int inLen) { buffer = new char[inLen]; }
   ~AutoBuf() { delete [] buffer; }
   char *buffer;
};


String  _hx_mysql_escape(Dynamic handle,String str)
{
   Connection *connection = getConnection(handle);
   int len = str.length * 2 + 1;
   AutoBuf sout(len);

   int finalLen = mysql_real_escape_string(connection->m,sout.buffer,str.utf8_str(),str.length);
   if( finalLen < 0 )
      hx::Throw( HX_CSTRING("Unsupported charset : ") + String(mysql_character_set_name(connection->m)) );

   return String::create(sout.buffer,finalLen);
}

// ---------------------------------------------------------------
// Sql


/**
   connect : { host => string, port => int, user => string, pass => string, socket => string? } -> 'connection
   <doc>Connect to a database using the connection informations</doc>
**/
Dynamic _hx_mysql_connect(Dynamic params)
{
   String host = params->__Field(HX_CSTRING("host"), hx::paccDynamic );
   int    port = params->__Field(HX_CSTRING("port"), hx::paccDynamic);
   String user = params->__Field(HX_CSTRING("user"), hx::paccDynamic);
   String pass = params->__Field(HX_CSTRING("pass"), hx::paccDynamic);
   String socket = params->__Field(HX_CSTRING("socket"), hx::paccDynamic );

   MYSQL *cnx = mysql_init(NULL);
   if( mysql_real_connect(cnx,host.utf8_str(),user.utf8_str(),pass.utf8_str(),NULL,port,socket.utf8_str(),0) == NULL )
   {
      String error = HX_CSTRING("Failed to connect to mysql server : ") + String(mysql_error(cnx));
      mysql_close(cnx);
      hx::Throw(error);
   }

   Connection *connection = new Connection();
   connection->create(cnx);
   return connection;
}


