/*************************************************************************************************
 * C++ API of Villa, the advanced API of QDBM
 *                                                      Copyright (C) 2000-2006 Mikio Hirabayashi
 * This file is part of QDBM, Quick Database Manager.
 * QDBM is free software; you can redistribute it and/or modify it under the terms of the GNU
 * Lesser General Public License as published by the Free Software Foundation; either version
 * 2.1 of the License or any later version.  QDBM is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 * You should have received a copy of the GNU Lesser General Public License along with QDBM; if
 * not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 USA.
 *************************************************************************************************/


#ifndef _XVILLA_H                        /* duplication check */
#define _XVILLA_H


#include <xqdbm.h>
#include <xadbm.h>

extern "C" {
#include <depot.h>
#include <cabin.h>
#include <villa.h>
#include <stdlib.h>
#include <time.h>
}



/**
 * Error container for Villa.
 */
class qdbm::Villa_error : public virtual DBM_error {
  //----------------------------------------------------------------
  // public member functions
  //----------------------------------------------------------------
public:
  /**
   * Create an instance.
   */
  Villa_error() throw();
  /**
   * Create an instance.
   * @param ecode the error code.
   */
  Villa_error(int ecode) throw();
  /**
   * Copy constructor.
   * @param ve a source instance.
   */
  Villa_error(const Villa_error& ve) throw();
  /**
   * Release resources of the instance.
   */
  virtual ~Villa_error() throw();
  /**
   * Assignment operator.
   * @param ve a source instance.
   * @return reference to itself.
   */
  Villa_error& operator =(const Villa_error& ve) throw();
  /**
   * Assignment operator.
   * @param ecode the error code.
   * @return reference to itself.
   */
  Villa_error& operator =(int ecode) throw();
  /**
   * Equality operator.
   * @param ve a comparing instance.
   * @return true if both equal, else, false.
   */
  virtual bool operator ==(const Villa_error& ve) const throw();
  /**
   * Inequality operator.
   * @param ve a comparing instance.
   * @return true if both do not equal, else, false.
   */
  virtual bool operator !=(const Villa_error& ve) const throw();
  /**
   * Equality operator.
   * @param ecode a comparing error code.
   * @return true if both equal, else, false.
   */
  virtual bool operator ==(int ecode) const throw();
  /**
   * Inequality operator.
   * @param ecode a comparing error code.
   * @return true if both do not equal, else, false.
   */
  virtual bool operator !=(int ecode) const throw();
  /**
   * Cast operator into pointer to char.
   * @return the pointer to the string.
   */
  virtual operator const char*() const throw();
  /**
   * Get the error code.
   * @return the error code.
   */
  virtual int code() const throw();
  /**
   * Get the error message.
   * @return the pointer to the string.
   */
  virtual const char* message() const throw();
  //----------------------------------------------------------------
  // private member variables
  //----------------------------------------------------------------
private:
  /** The error code. */
  int ecode;
};



/**
 * C++ API of Villa, the basic API of QDBM.
 */
class qdbm::Villa : public virtual ADBM {
  //----------------------------------------------------------------
  // public static member constants
  //----------------------------------------------------------------
public:
  static const int ENOERR;            ///< error code: no error
  static const int EFATAL;            ///< error code: with fatal error
  static const int EMODE;             ///< error code: invalid mode
  static const int EBROKEN;           ///< error code: broken database file
  static const int EKEEP;             ///< error code: existing record
  static const int ENOITEM;           ///< error code: no item found
  static const int EALLOC;            ///< error code: memory allocation error
  static const int EMAP;              ///< error code: memory mapping error
  static const int EOPEN;             ///< error code: open error
  static const int ECLOSE;            ///< error code: close error
  static const int ETRUNC;            ///< error code: trunc error
  static const int ESYNC;             ///< error code: sync error
  static const int ESTAT;             ///< error code: stat error
  static const int ESEEK;             ///< error code: seek error
  static const int EREAD;             ///< error code: read error
  static const int EWRITE;            ///< error code: write error
  static const int ELOCK;             ///< error code: lock error
  static const int EUNLINK;           ///< error code: unlink error
  static const int EMKDIR;            ///< error code: mkdir error
  static const int ERMDIR;            ///< error code: rmdir error
  static const int EMISC;             ///< error code: miscellaneous error
  static const int OREADER;           ///< open mode: open as a reader
  static const int OWRITER;           ///< open mode: open as a writer
  static const int OCREAT;            ///< open mode: writer creating
  static const int OTRUNC;            ///< open mode: writer truncating
  static const int ONOLCK;            ///< open mode: open without locking
  static const int OLCKNB;            ///< open mode: lock without blocking
  static const int OZCOMP;            ///< open mode: compress leaves with ZLIB
  static const int OYCOMP;            ///< open mode: compress leaves with LZO
  static const int OXCOMP;            ///< open mode: compress leaves with BZIP2
  static const int DOVER;             ///< write mode: overwrite the existing value
  static const int DKEEP;             ///< write mode: keep the existing value
  static const int DCAT;              ///< write mode: concatenate values
  static const int DDUP;              ///< write mode: allow duplication of keys
  static const int DDUPR;             ///< write mode: allow duplication with reverse order
  static const int JFORWARD;          ///< jump mode: step forward
  static const int JBACKWARD;         ///< jump mode: step backward
  static const int CPCURRENT;         ///< insertion mode: overwrite the current record
  static const int CPBEFORE;          ///< insertion mode: insert before the current record
  static const int CPAFTER;           ///< insertion mode: insert after the current record
  //----------------------------------------------------------------
  // public static member functions
  //----------------------------------------------------------------
public:
  /**
   * Get the version information.
   * @return the string of the version information.
   */
  static const char* version() throw();
  /**
   * Remove a database directory.
   * @param name the name of a database directory.
   * @throw Villa_error if an error occurs.
   */
  static void remove(const char* name) throw(Villa_error);
  /**
   * Compare keys of two records by lexical order.
   * @param aptr the pointer to the region of one key.
   * @param asiz the size of the region of one key.
   * @param bptr the pointer to the region of the other key.
   * @param bsiz the size of the region of the other key.
   * @return positive if the former is big, negative if the latter is big, 0 if both are
   * equivalent.
   */
  static int cmplex(const char *aptr, int asiz, const char *bptr, int bsiz) throw();
  /**
   * Compare keys of two records as integers.
   * @param aptr the pointer to the region of one key.
   * @param asiz the size of the region of one key.
   * @param bptr the pointer to the region of the other key.
   * @param bsiz the size of the region of the other key.
   * @return positive if the former is big, negative if the latter is big, 0 if both are
   * equivalent.
   */
  static int cmpint(const char *aptr, int asiz, const char *bptr, int bsiz) throw();
  /**
   * Compare keys of two records as numbers of big endian.
   * @param aptr the pointer to the region of one key.
   * @param asiz the size of the region of one key.
   * @param bptr the pointer to the region of the other key.
   * @param bsiz the size of the region of the other key.
   * @return positive if the former is big, negative if the latter is big, 0 if both are
   * equivalent.
   */
  static int cmpnum(const char *aptr, int asiz, const char *bptr, int bsiz) throw();
  /**
   * Compare keys of two records as decimal strings.
   * @param aptr the pointer to the region of one key.
   * @param asiz the size of the region of one key.
   * @param bptr the pointer to the region of the other key.
   * @param bsiz the size of the region of the other key.
   * @return positive if the former is big, negative if the latter is big, 0 if both are
   * equivalent.
   */
  static int cmpdec(const char *aptr, int asiz, const char *bptr, int bsiz) throw();
  //----------------------------------------------------------------
  // public member functions
  //----------------------------------------------------------------
public:
  /**
   * Get the database handle.
   * @param name the name of a database file.
   * @param omode the connection mode: `Villa::OWRITER' as a writer, `Villa::OREADER' as
   * a reader.  If the mode is `Villa::OWRITER', the following may be added by bitwise or:
   * `Villa::OCREAT', which means it creates a new database if not exist, `Villa::OTRUNC',
   * which means it creates a new database regardless if one exists, `Villa::OZCOMP', which means
   * leaves in the database are compressed with ZLIB, `Villa::OYCOMP', which means leaves in the
   * database are compressed with LZO, `Villa::OXCOMP', which means leaves in the database are
   * compressed with BZIP2.  Both of `Villa::OREADER' and `Villa::OWRITER' can be added to by
   * bitwise or: `Villa::ONOLCK', which means it opens a database file without file locking, or
   * `Villa::OLCKNB', which means locking is performed without blocking.
   * @param cmp the comparing function: `Villa::cmplex' comparing keys in lexical order,
   * `Villa::cmpint' comparing keys as objects of `int' in native byte order, `Villa::cmpnum'
   * comparing keys as numbers of big endian, `Villa::cmpdec' comparing keys as decimal strings.
   * Any function compatible with them can be assigned to the comparing function.  The comparing
   * function should be kept same in the life of a database.
   * @throw Villa_error if an error occurs.
   * @note While connecting as a writer, an exclusive lock is invoked to the database file.
   * While connecting as a reader, a shared lock is invoked to the database file.  The thread
   * blocks until the lock is achieved.  `Villa::OZCOMP', `Villa::OYCOMP', and `Villa::OXCOMP'
   * are available only if QDBM was built each with ZLIB, LZO, and BZIP2 enabled.  If
   * `Villa::ONOLCK' is used, the application is responsible for exclusion control.
   */
  Villa(const char* name, int omode = Villa::OREADER, VLCFUNC cmp = Villa::cmplex)
    throw(Villa_error);
  /**
   * Release the resources.
   * @note If the database handle is not closed yet, it is closed.
   */
  virtual ~Villa() throw();
  /**
   * Close the database handle.
   * @throw Villa_error if an error occurs.
   * @note Updating a database is assured to be written when the handle is closed.  If a writer
   * opens a database but does not close it appropriately, the database will be broken.  If the
   * transaction is activated and not committed, it is aborted.
   */
  virtual void close() throw(Villa_error);
  /**
   * Store a record.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @param vbuf the pointer to the region of a value.
   * @param vsiz the size of the region of the value.  If it is negative, the size is assigned
   * with `std::strlen(vbuf)'.
   * @param dmode behavior when the key overlaps, by the following values: `Villa::DOVER',
   * which means the specified value overwrites the existing one, `Villa::DKEEP', which means
   * the existing value is kept, `Villa::DCAT', which means the specified value is concatenated
   * at the end of the existing value, `Villa::DDUP', which means duplication of keys is allowed
   * and the specified value is added as the last one, `Villa::DDUPR', which means duplication of
   * keys is allowed and the specified value is added as the first one.
   * @return always true.  However, if the silent flag is true and replace is cancelled, false is
   * returned instead of exception.
   * @throw Villa_error if an error occurs or replace is cancelled.
   * @note The cursor becomes unavailable due to updating database.
   */
  virtual bool put(const char* kbuf, int ksiz, const char* vbuf, int vsiz,
                   int dmode = Villa::DOVER) throw(Villa_error);
  /**
   * Delete a record.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throw Villa_error if an error occurs or no record corresponds.
   * @note When the key of duplicated records is specified, the first record of the same key
   * is deleted.  The cursor becomes unavailable due to updating database.
   */
  virtual bool out(const char* kbuf, int ksiz) throw(Villa_error);
  /**
   * Retrieve a record.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @param sp the pointer to a variable to which the size of the region of the return value
   * is assigned.  If it is 0, it is not used.
   * @return the pointer to the region of the value of the corresponding record.  If the silent
   * flag is true and no record corresponds, 0 is returned instead of exception.
   * @throw Villa_error if an error occurs or no record corresponds.
   * @note When the key of duplicated records is specified, the value of the first record of
   * the same key is selected.  Because an additional zero code is appended at the end of the
   * region of the return value, the return value can be treated as a character string.
   * Because the region of the return value is allocated with the `std::malloc' call, it
   * should be released with the `std::free' call if it is no longer in use.
   */
  virtual char* get(const char* kbuf, int ksiz, int* sp = 0) throw(Villa_error);
  /**
   * Get the size of the value of a record.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @return the size of the value of the corresponding record.  If the silent flag is true and
   * no record corresponds, -1 is returned instead of exception.  If multiple records correspond,
   * the size of the first is returned.
   * @throw Villa_error if an error occurs or no record corresponds.
   */
  virtual int vsiz(const char* kbuf, int ksiz) throw(Villa_error);
  /**
   * Get the number of records corresponding a key.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @return the number of corresponding records.  If no record corresponds, 0 is returned.
   * @throw Villa_error if an error occurs.
   */
  virtual int vnum(const char* kbuf, int ksiz) throw(Villa_error);
  /**
   * Move the cursor to the first record.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throw Villa_error if an error occurs or there is no record in the database.
   */
  virtual bool curfirst() throw(Villa_error);
  /**
   * Move the cursor to the last record.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throw Villa_error if an error occurs or there is no record in the database.
   */
  virtual bool curlast() throw(Villa_error);
  /**
   * Move the cursor to the previous record.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throw Villa_error if an error occurs or there is no previous record.
   */
  virtual bool curprev() throw(Villa_error);
  /**
   * Move the cursor to the next record.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throw Villa_error if an error occurs or there is no next record.
   */
  virtual bool curnext() throw(Villa_error);
  /**
   * Move the cursor to a position around a record.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @param jmode detail adjustment: `Villa::JFORWARD', which means that the cursor is set to
   * the first record of the same key and that the cursor is set to the next substitute if
   * completely matching record does not exist, `Villa::JBACKWARD', which means that the cursor
   * is set to the last record of the same key and that the cursor is set to the previous
   * substitute if completely matching record does not exist.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throw Villa_error if an error occurs or there is no record corresponding the condition.
   */
  virtual bool curjump(const char* kbuf, int ksiz, int jmode = Villa::JFORWARD)
    throw(Villa_error);
  /**
   * Get the key of the record where the cursor is.
   * @param sp the pointer to a variable to which the size of the region of the return value
   * is assigned.  If it is 0, it is not used.
   * @return the pointer to the region of the key of the corresponding record.  If the silent
   * flag is true and no record corresponds, 0 is returned instead of exception.
   * @throw Villa_error if an error occurs or no record corresponds to the cursor.
   * @note Because an additional zero code is appended at the end of the region of the return
   * value, the return value can be treated as a character string.  Because the region of the
   * return value is allocated with the `std::malloc' call, it should be released with the
   * `std::free' call if it is no longer in use.
   */
  virtual char* curkey(int* sp = 0) throw(Villa_error);
  /**
   * Get the value of the record where the cursor is.
   * @param sp the pointer to a variable to which the size of the region of the return value
   * is assigned.  If it is 0, it is not used.
   * @return the pointer to the region of the value of the corresponding record.  If the silent
   * flag is true and no record corresponds, 0 is returned instead of exception.
   * @throw Villa_error if an error occurs or no record corresponds to the cursor.
   * @note Because an additional zero code is appended at the end of the region of the return
   * value, the return value can be treated as a character string.  Because the region of the
   * return value is allocated with the `std::malloc' call, it should be released with the
   * `std::free' call if it is no longer in use.
   */
  virtual char* curval(int* sp = 0) throw(Villa_error);
  /**
   * Insert a record around the cursor.
   * @param vbuf the pointer to the region of a value.
   * @param vsiz the size of the region of the value.  If it is negative, the size is assigned
   * with `std::strlen(vbuf)'.
   * @param cpmode detail adjustment: `Villa::CPCURRENT', which means that the value of the
   * current record is overwritten, `Villa::CPBEFORE', which means that a new record is inserted
   * before the current record, `Villa::CPAFTER', which means that a new record is inserted after
   * the current record.
   * @return always true.  However, if the silent flag is true and no record corresponds to the
   * cursor, false is returned instead of exception.
   * @throw Villa_error if an error occurs or no record corresponds to the cursor.
   * @note After insertion, the cursor is moved to the inserted record.
   */
  virtual bool curput(const char* vbuf, int vsiz, int cpmode = Villa::CPCURRENT)
    throw(Villa_error);
  /**
   * Delete the record where the cursor is.
   * @return always true.  However, if the silent flag is true and no record corresponds to the
   * cursor, false is returned instead of exception.
   * @throw Villa_error if an error occurs or no record corresponds to the cursor.
   * @note After deletion, the cursor is moved to the next record if possible.
   */
  virtual bool curout() throw(Villa_error);
  /**
   * Set the tuning parameters for performance.
   * @param lrecmax the max number of records in a leaf node of B+ tree.  If it is not more
   * than 0, the default value is specified.
   * @param nidxmax the max number of indexes in a non-leaf node of B+ tree.  If it is not more
   * than 0, the default value is specified.
   * @param lcnum the max number of caching leaf nodes.  If it is not more than 0, the default
   * value is specified.
   * @param ncnum the max number of caching non-leaf nodes.  If it is not more than 0, the
   * default value is specified.
   * @throw Villa_error if an error occurs.
   * @note The default setting is equivalent to `settuning(49, 192, 1024, 512)'.  Because tuning
   * paremeters are not saved in a database, you should specify them every opening a database.
   */
  virtual void settuning(int lrecmax, int nidxmax, int lcnum, int ncnum) throw(Villa_error);
  /**
   * Synchronize updating contents with the file and the device.
   * @throw Villa_error if an error occurs.
   * @note This function is useful when another process uses the connected database file.  This
   * function shuold not be used while the transaction is activated.
   */
  virtual void sync() throw(Villa_error);
  /**
   * Optimize a database.
   * @throw Villa_error if an error occurs.
   * @note In an alternating succession of deleting and storing with overwrite or concatenate,
   * dispensable regions accumulate.  This function is useful to do away with them.  This
   * function shuold not be used while the transaction is activated.
   */
  virtual void optimize() throw(Villa_error);
  /**
   * Get the name of the database.
   * @return the pointer to the region of the name of the database.
   * @throw Villa_error if an error occurs.
   * @note Because the region of the return value is allocated with the `std::malloc' call,
   * it should be released with the `std::free' call if it is no longer in use.
   */
  virtual char* name() throw(Villa_error);
  /**
   * Get the size of the database file.
   * @return the size of the database file.
   * @throw Villa_error if an error occurs.
   * @note Because of the I/O buffer, the return value may be less than the real size.
   */
  virtual int fsiz() throw(Villa_error);
  /**
   * Get the number of the leaf nodes of B+ tree.
   * @return the number of the leaf nodes.
   * @throw Villa_error if an error occurs.
   */
  virtual int lnum() throw(Villa_error);
  /**
   * Get the number of the non-leaf nodes of B+ tree.
   * @return the number of the non-leaf nodes.
   * @throw Villa_error if an error occurs.
   */
  virtual int nnum() throw(Villa_error);
  /**
   * Get the number of the records stored in a database.
   * @return the number of the records stored in the database.
   * @throw Villa_error if an error occurs.
   */
  virtual int rnum() throw(Villa_error);
  /**
   * Check whether the database handle is a writer or not.
   * @return true if the handle is a writer, false if not.
   * @throw Villa_error if an error occurs.
   */
  virtual bool writable() throw(Villa_error);
  /**
   * Check whether the database has a fatal error or not.
   * @return true if the database has a fatal error, false if not.
   * @throw Villa_error if an error occurs.
   */
  virtual bool fatalerror() throw(Villa_error);
  /**
   * Get the inode number of the database file.
   * @return the inode number of the database file.
   * @throw Villa_error if an error occurs.
   */
  virtual int inode() throw(Villa_error);
  /**
   * Get the last modified time of the database.
   * @return the last modified time the database.
   * @throw Villa_error if an error occurs.
   */
  virtual time_t mtime() throw(Villa_error);
  /**
   * Begin the transaction.
   * @throw Villa_error if an error occurs.
   * @note If a thread is already in the transaction, the other threads block until the prius
   * is out of the transaction.  Only one transaction can be activated with a database handle
   * at the same time.
   */
  virtual void tranbegin() throw(Villa_error);
  /**
   * Commit the transaction.
   * @throw Villa_error if an error occurs.
   * @note Updating a database in the transaction is fixed when it is committed successfully.
   * Any other thread except for the one which began the transaction should not call this
   * function.
   */
  virtual void trancommit() throw(Villa_error);
  /**
   * Abort the transaction.
   * @throw Villa_error if an error occurs.
   * @note Updating a database in the transaction is discarded when it is aborted.  The state
   * of the database is rollbacked to before transaction.  Any other thread except for the one
   * which began the transaction should not call this function.
   */
  virtual void tranabort() throw(Villa_error);
  /**
   * Store a record.
   * @param key reference to a key object.
   * @param val reference to a value object.
   * @param replace whether the existing value is to be overwritten or not.
   * @throw Villa_error if an error occurs or replace is cancelled.
   */
  virtual void storerec(const Datum& key, const Datum& val, bool replace = true)
    throw(Villa_error);
  /**
   * Delete a record.
   * @param key reference to a key object.
   * @throw Villa_error if an error occurs or no record corresponds.
   */
  virtual void deleterec(const Datum& key) throw(Villa_error);
  /**
   * Fetch a record.
   * @param key reference to a key object.
   * @return a temporary instance of the value of the corresponding record.
   * @throw Villa_error if an error occurs or no record corresponds.
   */
  virtual Datum fetchrec(const Datum& key) throw(Villa_error);
  /**
   * Get the first key.
   * @return a temporary instance of the key of the first record.
   * @throw Villa_error if an error occurs or no record corresponds.
   */
  virtual Datum firstkey() throw(Villa_error);
  /**
   * Get the next key.
   * @return a temporary instance of the key of the next record.
   * @throw Villa_error if an error occurs or no record corresponds.
   */
  virtual Datum nextkey() throw(Villa_error);
  /**
   * Check whether a fatal error occured or not.
   * @return true if the database has a fatal error, false if not.
   * @throw Villa_error if an error occurs.
   */
  virtual bool error() throw(Villa_error);
  //----------------------------------------------------------------
  // public member variables
  //----------------------------------------------------------------
public:
  bool silent;                        ///< whether to repress frequent exceptions
  //----------------------------------------------------------------
  // private member variables
  //----------------------------------------------------------------
private:
  VILLA* villa;                       ///< internal database handle
  pthread_mutex_t mymutex;            ///< internal mutex
  pthread_mutex_t tranmutex;          ///< mutex for the transaction
  //----------------------------------------------------------------
  // private member functions
  //----------------------------------------------------------------
private:
  /** copy constructor: This should not be used. */
  Villa(const Villa& villa) throw(Villa_error);
  /** assignment operator: This should not be used. */
  Villa& operator =(const Villa& villa) throw(Villa_error);
  /** lock the database. */
  bool lock();
  /** unlock the database. */
  void unlock();
};



#endif                                   /* duplication check */


/* END OF FILE */
