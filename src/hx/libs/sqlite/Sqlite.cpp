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
#include "sqlite3.h"
#include <stdlib.h>


// Put in anon-namespace to avoid conflicts if static-linked
namespace {



struct result : public hx::Object
{
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdSqlite };

   sqlite3 *db;
   sqlite3_stmt *r;
   int ncols;
   int count;
   String *names;
   int *bools;
   int done;
   int first;

   void create(sqlite3 *inDb, sqlite3_stmt *inR, String sql)
   {
      _hx_set_finalizer(this, finalize);

      db = inDb;
      r = inR;

      ncols = sqlite3_column_count(r);
      names = (String *)malloc(sizeof(String)*ncols);
      bools = (int*)malloc(sizeof(int)*ncols);
      first = 1;
      done = 0;
      for(int i=0;i<ncols;i++)
      {
         names[i] = String::createPermanent(sqlite3_column_name(r,i),-1);
         for(int j=0;j<i;j++)
            if( names[j] == names[i] )
               hx::Throw(HX_CSTRING("Error, same field is two times in the request ") + sql);

         const char *dtype = sqlite3_column_decltype(r,i);
         bools[i] = dtype?(strcmp(dtype,"BOOL") == 0):0;
      }
   }

   static void finalize(Dynamic obj) { ((result *)(obj.mPtr))->destroy(false); }
   void destroy(bool inThrowError)
   {
      if (bools)
      {
         free(bools);
         bools = 0;
      }
      if (names)
      {
         free(names);
         names = 0;
      }
      if (r)
      {
         first = 0;
         done = 1;
         if( ncols == 0 )
            count = sqlite3_changes(db);

         bool err = sqlite3_finalize(r) != SQLITE_OK;
         db = 0;
         r = 0;

         if( err && inThrowError)
            hx::Throw(HX_CSTRING("Could not finalize request"));
      }
   }

   String toString() { return HX_CSTRING("Sqlite Result"); }

 //static void finalize_result( result *r, int exc, bool throwError = true )
};



/**
   <doc>
   <h1>SQLite</h1>
   <p>
   Sqlite is a small embeddable SQL database that store all its data into
   a single file. See http://sqlite.org for more details.
   </p>
   </doc>
**/

struct database : public hx::Object
{
   sqlite3 *db;
   hx::ObjectPtr<result> last;

   void create(sqlite3 *inDb)
   {
      db = inDb;
      _hx_set_finalizer(this, finalize);
   }
   static void finalize(Dynamic obj) { ((database *)(obj.mPtr))->destroy(false); }
   void destroy(bool inThrowError)
   {
      if (db)
      {
         if (last.mPtr)
         {
            last->destroy(inThrowError);
            last = null();
         }

         if( sqlite3_close(db) != SQLITE_OK )
         {
            if (inThrowError)
               hx::Throw(HX_CSTRING("Sqlite: could not close"));
         }
         db = 0;
      }
   }


   void setResult(result *inResult)
   {
      if (last.mPtr)
         last->destroy(true);

      last = inResult;
      HX_OBJ_WB_GET(this, last.mPtr);
   }

   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER(last); }
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(last); }
   #endif

   String toString() { return HX_CSTRING("Sqlite Databse"); }
};

static void sqlite_error( sqlite3 *db ) {
   hx::Throw( HX_CSTRING("Sqlite error : ") + String(sqlite3_errmsg(db)) );
}

database *getDatabase(Dynamic handle)
{
   database *db = dynamic_cast<database *>(handle.mPtr);
   if (!db || !db->db)
      hx::Throw( HX_CSTRING("Invalid sqlite database") );
   return db;
}


result *getResult(Dynamic handle, bool inRequireStatement)
{
   result *r = dynamic_cast<result *>(handle.mPtr);
   if (!r || (inRequireStatement && !r->r))
      hx::Throw( HX_CSTRING("Invalid sqlite result") );
   return r;
}


} // End anon-namespace




/**
   connect : filename:string -> 'db
   <doc>Open or create the database stored in the specified file.</doc>
**/
Dynamic _hx_sqlite_connect(String filename)
{
   int err;
   sqlite3 *sqlDb = 0;
   if( (err = sqlite3_open(filename.utf8_str(),&sqlDb)) != SQLITE_OK )
      sqlite_error(sqlDb);

   database *db = new database();
   db->create(sqlDb);
   return db;
}


/**
   close : 'db -> void
   <doc>Closes the database.</doc>
**/
void _hx_sqlite_close(Dynamic handle)
{
   database *db = getDatabase(handle);
   db->destroy(true);
}

/**
   last_insert_id : 'db -> int
   <doc>Returns the last inserted auto_increment id.</doc>
**/
int     _hx_sqlite_last_insert_id(Dynamic handle)
{
   database *db = getDatabase(handle);
   return sqlite3_last_insert_rowid(db->db);
}

/**
   request : 'db -> sql:string -> 'result
   <doc>Executes the SQL request and returns its result</doc>
**/
Dynamic _hx_sqlite_request(Dynamic handle,String sql)
{
   database *db = getDatabase(handle);


   sqlite3_stmt *statement = 0;
   const char *tl = 0;
   if( sqlite3_prepare(db->db,sql.utf8_str(),sql.length,&statement,&tl) != SQLITE_OK )
   {
      hx::Throw( HX_CSTRING("Sqlite error in ") + sql + HX_CSTRING(" : ") +
                  String(sqlite3_errmsg(db->db) ) );
   }
   if( *tl )
   {
      sqlite3_finalize(statement);
      hx::Throw(HX_CSTRING("Cannot execute several SQL requests at the same time"));
   }

   int i,j;

   result *r = new result();
   r->create(db->db, statement,sql);

   db->setResult(r);

   return r;
}







/**
   result_get_length : 'result -> int
   <doc>Returns the number of rows in the result or the number of rows changed by the request.</doc>
**/
int  _hx_sqlite_result_get_length(Dynamic handle)
{
   result *r = getResult(handle,false);
   if( r->ncols != 0 )
      hx::Throw(HX_CSTRING("Getting change count from non-change request")); // ???
   return r->count;
}

/**
   result_get_nfields : 'result -> int
   <doc>Returns the number of fields in the result.</doc>
**/
int     _hx_sqlite_result_get_nfields(Dynamic handle)
{
   return getResult(handle,false)->ncols;
}

/**
   result_next : 'result -> object?
   <doc>Returns the next row in the result or [null] if no more result.</doc>
**/

Dynamic _hx_sqlite_result_next(Dynamic handle)
{
   result *r = getResult(handle,false);
   if( r->done )
      return null();

   switch( sqlite3_step(r->r) )
   {
      case SQLITE_ROW:
      {
         hx::Anon v = hx::Anon_obj::Create();
         r->first = 0;
         for(int i=0;i<r->ncols;i++)
         {
            Dynamic f;
            switch( sqlite3_column_type(r->r,i) )
            {
            case SQLITE_NULL:
               break;
            case SQLITE_INTEGER:
               if( r->bools[i] )
                  f = bool(sqlite3_column_int(r->r,i));
               else
                  f = int(sqlite3_column_int(r->r,i));
               break;
            case SQLITE_FLOAT:
               f = Float(sqlite3_column_double(r->r,i));
               break;
            case SQLITE_TEXT:
               f = String((char*)sqlite3_column_text(r->r,i));
               break;
            case SQLITE_BLOB:
               {
                  int size = sqlite3_column_bytes(r->r,i);
                  f = Array_obj<unsigned char>::fromData((const unsigned char *)sqlite3_column_blob(r->r,i),size);
                  break;
               }
            default:
               {
                  hx::Throw( HX_CSTRING("Unknown Sqlite type #") +
                               String((int)sqlite3_column_type(r->r,i)));
               }
            }
            v->__SetField(r->names[i],f,hx::paccDynamic);
         }
         return v;
      }
      case SQLITE_DONE:
         r->destroy(true);
         return null();
      case SQLITE_BUSY:
         hx::Throw(HX_CSTRING("Database is busy"));
      case SQLITE_ERROR:
         sqlite_error(r->db);
      default:
         hx::Throw(HX_CSTRING("Unkown sqlite result"));
   }

   return null();
}


static sqlite3_stmt *prepStatement(Dynamic handle,int n)
{
   result *r = getResult(handle,true);
   if( n < 0 || n >= r->ncols )
      hx::Throw( HX_CSTRING("Sqlite: Invalid index") );

   if( r->first )
      _hx_sqlite_result_next(handle);

   if( r->done )
      hx::Throw( HX_CSTRING("Sqlite: no more results") );

   return r->r;
}

/**
   result_get : 'result -> n:int -> string
   <doc>Return the [n]th field of the current result row.</doc>
**/


String  _hx_sqlite_result_get(Dynamic handle,int n)
{
   sqlite3_stmt *r = prepStatement(handle,n);
   return String((char*)sqlite3_column_text(r,n));
}

/**
   result_get_int : 'result -> n:int -> int
   <doc>Return the [n]th field of the current result row as an integer.</doc>
**/
int     _hx_sqlite_result_get_int(Dynamic handle,int n)
{
   sqlite3_stmt *r = prepStatement(handle,n);
   return sqlite3_column_int(r,n);
}

/**
   result_get_float : 'result -> n:int -> float
   <doc>Return the [n]th field of the current result row as a float.</doc>
**/
Float   _hx_sqlite_result_get_float(Dynamic handle,int n)
{
   sqlite3_stmt *r = prepStatement(handle,n);
   return sqlite3_column_double(r,n);
}




