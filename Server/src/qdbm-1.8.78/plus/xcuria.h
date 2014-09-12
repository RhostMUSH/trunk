/*************************************************************************************************
 * C++ API of Curia, the extended API of QDBM
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


#ifndef _XCURIA_H                        /* duplication check */
#define _XCURIA_H


#include <xqdbm.h>
#include <xadbm.h>

extern "C" {
#include <depot.h>
#include <curia.h>
#include <stdlib.h>
#include <time.h>
}



/**
 * Error container for Curia.
 */
class qdbm::Curia_error : public virtual DBM_error {
  //----------------------------------------------------------------
  // public member functions
  //----------------------------------------------------------------
public:
  /**
   * Create an instance.
   */
  Curia_error() throw();
  /**
   * Create an instance.
   * @param ecode the error code.
   */
  Curia_error(int ecode) throw();
  /**
   * Copy constructor.
   * @param ce a source instance.
   */
  Curia_error(const Curia_error& ce) throw();
  /**
   * Release resources of the instance.
   */
  virtual ~Curia_error() throw();
  /**
   * Assignment operator.
   * @param ce a source instance.
   * @return reference to itself.
   */
  Curia_error& operator =(const Curia_error& ce) throw();
  /**
   * Assignment operator.
   * @param ecode the error code.
   * @return reference to itself.
   */
  Curia_error& operator =(int ecode) throw();
  /**
   * Equality operator.
   * @param ce a comparing instance.
   * @return true if both equal, else, false.
   */
  virtual bool operator ==(const Curia_error& ce) const throw();
  /**
   * Inequality operator.
   * @param ce a comparing instance.
   * @return true if both do not equal, else, false.
   */
  virtual bool operator !=(const Curia_error& ce) const throw();
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
 * C++ API of Curia, the extended API of QDBM
 */
class qdbm::Curia : public virtual ADBM {
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
  static const int OSPARSE;           ///< open mode: create as sparse files
  static const int DOVER;             ///< write mode: overwrite the existing value
  static const int DKEEP;             ///< write mode: keep the existing value
  static const int DCAT;              ///< write mode: concatenate values
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
   * @throw Curia_error if an error occurs.
   */
  static void remove(const char* name) throw(Curia_error);
  /**
   * Retrieve a record directly from a database directory.
   * @param name the name of a database directory.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @param sp the pointer to a variable to which the size of the region of the return value
   * is assigned.  If it is 0, it is not used.
   * @return the pointer to the region of the value of the corresponding record.
   * @throw Curia_error if an error occurs or no record corresponds.
   * @note Because an additional zero code is appended at the end of the region of the return
   * value, the return value can be treated as a character string.  Because the region of the
   * return value is allocated with the `std::malloc' call, it should be released with the
   * `std::free' call if it is no longer in use.  Although this method can be used even while
   * the database directory is locked by another process, it is not assured that recent updated
   * is reflected.
   */
  static char* snaffle(const char* name, const char* kbuf, int ksiz, int* sp) throw(Curia_error);
  //----------------------------------------------------------------
  // public member functions
  //----------------------------------------------------------------
public:
  /**
   * Get the database handle.
   * @param name the name of a database directory.
   * @param omode the connection mode: `Curia::OWRITER' as a writer, `Curia::OREADER' as
   * a reader.  If the mode is `Curia::OWRITER', the following may be added by bitwise or:
   * `Curia::OCREAT', which means it creates a new database if not exist, `Curia::OTRUNC',
   * which means it creates a new database regardless if one exists.  Both of `Curia::OREADER'
   * and `Curia::OWRITER' can be added to by bitwise or: `Curia::ONOLCK', which means it opens a
   * database directory without file locking, or `Curia::OLCKNB', which means locking is
   * performed without blocking.  `Curia::OCREAT' can be added to by bitwise or: `Curia::OSPARSE',
   * which means it creates database files as sparse files.
   * @param bnum the number of elements of each bucket array.  If it is not more than 0,
   * the default value is specified.  The size of each bucket array is determined on creating,
   * and can not be changed except for by optimization of the database.  Suggested size of each
   * bucket array is about from 0.5 to 4 times of the number of all records to store.
   * @param dnum the number of division of the database.  If it is not more than 0, the default
   * value is specified.  The number of division can not be changed from the initial value.  The
   * max number of division is 512.
   * @note While connecting as a writer, an exclusive lock is invoked to the database directory.
   * While connecting as a reader, a shared lock is invoked to the database directory.
   * The thread blocks until the lock is achieved.  If `Curia::ONOLCK' is used, the application
   * is responsible for exclusion control.
   */
  Curia(const char* name, int omode = Curia::OREADER, int bnum = -1, int dnum = -1)
    throw(Curia_error);
  /**
   * Release the resources.
   * @note If the database handle is not closed yet, it is closed.
   */
  virtual ~Curia() throw();
  /**
   * Close the database handle.
   * @throw Curia_error if an error occurs.
   * @note Updating a database is assured to be written when the handle is closed.  If a writer
   * opens a database but does not close it appropriately, the database will be broken.
   */
  virtual void close() throw(Curia_error);
  /**
   * Store a record.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @param vbuf the pointer to the region of a value.
   * @param vsiz the size of the region of the value.  If it is negative, the size is assigned
   * with `std::strlen(vbuf)'.
   * @param dmode behavior when the key overlaps, by the following values: `Curia::DOVER',
   * which means the specified value overwrites the existing one, `Curia::DKEEP', which means the
   * existing value is kept, `Curia::DCAT', which means the specified value is concatenated at
   * the end of the existing value.
   * @return always true.  However, if the silent flag is true and replace is cancelled, false is
   * returned instead of exception.
   * @throw Curia_error if an error occurs or replace is cancelled.
   */
  virtual bool put(const char* kbuf, int ksiz, const char* vbuf, int vsiz,
                   int dmode = Curia::DOVER) throw(Curia_error);
  /**
   * Delete a record.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throw Curia_error if an error occurs or no record corresponds.
   */
  virtual bool out(const char* kbuf, int ksiz) throw(Curia_error);
  /**
   * Retrieve a record.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @param start the offset address of the beginning of the region of the value to be read.
   * @param max the max size to read with.  If it is negative, the size is unlimited.
   * @param sp the pointer to a variable to which the size of the region of the return value
   * is assigned.  If it is 0, it is not used.
   * @return the pointer to the region of the value of the corresponding record.  If the silent
   * flag is true and no record corresponds, 0 is returned instead of exception.
   * @throw Curia_error if an error occurs, no record corresponds, or the size of the value
   * of the corresponding record is less than `start'.
   * @note Because an additional zero code is appended at the end of the region of the return
   * value, the return value can be treated as a character string.  Because the region of the
   * return value is allocated with the `std::malloc' call, it should be released with the
   * `std::free' call if it is no longer in use.
   */
  virtual char* get(const char* kbuf, int ksiz, int start = 0, int max = -1, int* sp = 0)
    throw(Curia_error);
  /**
   * Retrieve a record and write the value into a buffer.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @param start the offset address of the beginning of the region of the value to be read.
   * @param max the max size to be read.  It shuld be equal to or less than the size of the
   * writing buffer.
   * @param vbuf the pointer to a buffer into which the value of the corresponding record is
   * written.
   * @return the return value is the size of the written data.  If the silent flag is true and
   * no record corresponds, -1 is returned instead of exception.
   * @throw Curia_error if an error occurs, no record corresponds, or the size of the value
   * of the corresponding record is less than `start'.
   * @note No additional zero code is appended at the end of the region of the writing buffer.
   */
  virtual int getwb(const char *kbuf, int ksiz, int start, int max, char *vbuf)
    throw(Curia_error);
  /**
   * Get the size of the value of a record.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @return the size of the value of the corresponding record.  If the silent flag is true and
   * no record corresponds, -1 is returned instead of exception.
   * @throw Curia_error if an error occurs or no record corresponds.
   * @note Because this function does not read the entity of a record, it is faster than `get'.
   */
  virtual int vsiz(const char* kbuf, int ksiz) throw(Curia_error);
  /**
   * Initialize the iterator of the database handle.
   * @throw Curia_error if an error occurs.
   * @note The iterator is used in order to access the key of every record stored in a database.
   */
  virtual void iterinit() throw(Curia_error);
  /**
   * Get the next key of the iterator.
   * @param sp the pointer to a variable to which the size of the region of the return value is
   * assigned.  If it is 0, it is not used.
   * @return the pointer to the region of the next key.  If the silent flag is true and no record
   * corresponds, 0 is returned instead of exception.
   * @throw Curia_error if an error occurs or no record is to be get out of the iterator.
   * @note Because an additional zero code is appended at the end of the region of the return
   * value, the return value can be treated as a character string.  Because the region of the
   * return value is allocated with the `std::malloc' call, it should be released with the
   * `std::free' call if it is no longer in use.  It is possible to access every record by
   * iteration of calling this function.  However, it is not assured if updating the database
   * is occurred while the iteration.  Besides, the order of this traversal access method is
   * arbitrary, so it is not assured that the order of storing matches the one of the traversal
   * access.
   */
  virtual char* iternext(int* sp = 0) throw(Curia_error);
  /**
   * Set alignment of the database handle.
   * @param align the basic size of alignment.
   * @throw Curia_error if an error occurs.
   * @note If alignment is set to a database, the efficiency of overwriting values is improved.
   * The size of alignment is suggested to be average size of the values of the records to be
   * stored.  If alignment is positive, padding whose size is multiple number of the alignment
   * is placed.  If alignment is negative, as `vsiz' is the size of a value, the size of padding
   * is calculated with `(vsiz / pow(2, abs(align) - 1))'.  Because alignment setting is not
   * saved in a database, you should specify alignment every opening a database.
   */
  virtual void setalign(int align) throw(Curia_error);
  /**
   * Set the size of the free block pool.
   * @param size the size of the free block pool of a database.
   * @throw Curia_error if an error occurs.
   * @note The default size of the free block pool is 16.  If the size is greater, the space
   * efficiency of overwriting values is improved with the time efficiency sacrificed.
   */
  virtual void setfbpsiz(int size) throw(Curia_error);
  /**
   * Synchronize updating contents with the files and the devices.
   * @note Curia_error if an error occurs.
   * This function is useful when another process uses the connected database directory.
   */
  virtual void sync() throw(Curia_error);
  /**
   * Optimize the database.
   * @param bnum the number of the elements of each bucket array.  If it is not more than 0,
   * the default value is specified.
   * @throw Curia_error if an error occurs.
   * @note In an alternating succession of deleting and storing with overwrite or concatenate,
   * dispensable regions accumulate.  This function is useful to do away with them.
   */
  virtual void optimize(int bnum = -1) throw(Curia_error);
  /**
   * Get the name of the database.
   * @return the pointer to the region of the name of the database.
   * @throw Curia_error if an error occurs.
   * @note Because the region of the return value is allocated with the `std::malloc' call,
   * it should be released with the `std::free' call if it is no longer in use.
   */
  virtual char* name() throw(Curia_error);
  /**
   * Get the total size of database files.
   * @return the total size of the database files.
   * @throw Curia_error if an error occurs.
   * @note If the total size is more than 2GB, the return value overflows.
   */
  virtual int fsiz() throw(Curia_error);
  /**
   * Get the total size of database files as double-precision floating-point number.
   * @return the total size of the database files.
   * @throw Curia_error if an error occurs.
   */
  virtual double fsizd() throw(Curia_error);
  /**
   * Get the total number of the elements of each bucket array.
   * @return the total number of the elements of each bucket array.
   * @throw Curia_error if an error occurs.
   */
  virtual int bnum() throw(Curia_error);
  /**
   * Get the total number of the used elements of each bucket array.
   * @return the total number of the used elements of each bucket array.
   * @throw Curia_error if an error occurs.
   * @note This function is inefficient because it accesses all elements of each bucket array.
   */
  virtual int busenum() throw(Curia_error);
  /**
   * Get the number of the records stored in the database.
   * @return the number of the records stored in the database.
   * @throw Curia_error if an error occurs.
   */
  virtual int rnum() throw(Curia_error);
  /**
   * Check whether the database handle is a writer or not.
   * @return true if the handle is a writer, false if not.
   * @throw Curia_error if an error occurs.
   */
  virtual bool writable() throw(Curia_error);
  /**
   * Check whether the database has a fatal error or not.
   * @return true if the database has a fatal error, false if not.
   * @throw Curia_error if an error occurs.
   */
  virtual bool fatalerror() throw(Curia_error);
  /**
   * Get the inode number of the database directory.
   * @return the inode number of the database directory.
   * @throw Curia_error if an error occurs.
   */
  virtual int inode() throw(Curia_error);
  /**
   * Get the last modified time of the database.
   * @return the last modified time the database.
   * @throw Curia_error if an error occurs.
   */
  virtual time_t mtime() throw(Curia_error);
  /**
   * Store a large object.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @param vbuf the pointer to the region of a value.
   * @param vsiz the size of the region of the value.  If it is negative, the size is assigned
   * with `std::strlen(vbuf)'.
   * @param dmode behavior when the key overlaps, by the following values: `Curia::DOVER',
   * which means the specified value overwrites the existing one, `Curia::DKEEP', which means the
   * existing value is kept, `Curia::DCAT', which means the specified value is concatenated at
   * the end of the existing value.
   * @return always true.  However, if the silent flag is true and replace is cancelled, false is
   * returned instead of exception.
   * @throw Curia_error if an error occurs or replace is cancelled.
   */
  virtual bool putlob(const char* kbuf, int ksiz, const char* vbuf, int vsiz,
                      int dmode = Curia::DOVER) throw(Curia_error);
  /**
   * Delete a large object.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throw Curia_error if an error occurs or no large object corresponds.
   */
  virtual bool outlob(const char* kbuf, int ksiz) throw(Curia_error);
  /**
   * Retrieve a large object.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @param start the offset address of the beginning of the region of the value to be read.
   * @param max the max size to be read.  If it is negative, the size to read is unlimited.
   * @param sp the pointer to a variable to which the size of the region of the return value
   * is assigned.  If it is 0, it is not used.
   * @return the pointer to the region of the value of the corresponding large object.  If the
   * silent flag is true and no record corresponds, 0 is returned instead of exception.
   * @throw Curia_error if an error occurs, no record corresponds, or the size of the value
   * of the corresponding record is less than `start'.
   * @note Because an additional zero code is appended at the end of the region of the return
   * value, the return value can be treated as a character string.  Because the region of the
   * return value is allocated with the `std::malloc' call, it should be released with the
   * `std::free' call if it is no longer in use.
   */
  virtual char* getlob(const char* kbuf, int ksiz, int start = 0, int max = -1, int* sp = 0)
    throw(Curia_error);
  /**
   * Get the file descriptor of a large object.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @return the file descriptor of the corresponding large object.  If the silent flag is true
   * and no record corresponds, -1 is returned instead of exception.
   * @throw Curia_error if an error occurs or no record corresponds.
   * @note The returned file descriptor is opened with the `open' call.  If the database handle
   * was opened as a writer, the descriptor is writable (O_RDWR), else, it is not writable
   * (O_RDONLY).  The descriptor should be closed with the `close' call if it is no longer in use.
   */
  virtual int getlobfd(const char *kbuf, int ksiz);
  /**
   * Get the size of the value of a large object.
   * @param kbuf the pointer to the region of a key.
   * @param ksiz the size of the region of the key.  If it is negative, the size is assigned
   * with `std::strlen(kbuf)'.
   * @return the size of the value of the corresponding large object.  If the silent flag is true
   * and no record corresponds, -1 is returned instead of exception.
   * @throw Curia_error if an error occurs or no large object corresponds.
   * @note Because this function does not read the entity of a large object, it is faster than
   * `getlob'.
   */
  virtual int vsizlob(const char* kbuf, int ksiz) throw(Curia_error);
  /**
   * Get the number of the large objects stored in the database.
   * @return the number of the large objects stored in the database.
   * @throw Curia_error if an error occurs.
   */
  virtual int rnumlob() throw(Curia_error);
  /**
   * Store a record.
   * @param key reference to a key object.
   * @param val reference to a value object.
   * @param replace whether the existing value is to be overwritten or not.
   * @throw Curia_error if an error occurs or replace is cancelled.
   */
  virtual void storerec(const Datum& key, const Datum& val, bool replace = true)
    throw(Curia_error);
  /**
   * Delete a record.
   * @param key reference to a key object.
   * @throw Curia_error if an error occurs or no record corresponds.
   */
  virtual void deleterec(const Datum& key) throw(Curia_error);
  /**
   * Fetch a record.
   * @param key reference to a key object.
   * @return a temporary instance of the value of the corresponding record.
   * @throw Curia_error if an error occurs or no record corresponds.
   */
  virtual Datum fetchrec(const Datum& key) throw(Curia_error);
  /**
   * Get the first key.
   * @return a temporary instance of the key of the first record.
   * @throw Curia_error if an error occurs or no record corresponds.
   */
  virtual Datum firstkey() throw(Curia_error);
  /**
   * Get the next key.
   * @return a temporary instance of the key of the next record.
   * @throw Curia_error if an error occurs or no record corresponds.
   */
  virtual Datum nextkey() throw(Curia_error);
  /**
   * Check whether a fatal error occured or not.
   * @return true if the database has a fatal error, false if not.
   * @throw Curia_error if an error occurs.
   */
  virtual bool error() throw(Curia_error);
  //----------------------------------------------------------------
  // public member variables
  //----------------------------------------------------------------
public:
  bool silent;                        ///< whether to repress frequent exceptions
  //----------------------------------------------------------------
  // private member variables
  //----------------------------------------------------------------
private:
  CURIA *curia;                       ///< internal database handle
  pthread_mutex_t mymutex;            ///< internal mutex
  //----------------------------------------------------------------
  // private member functions
  //----------------------------------------------------------------
private:
  /** copy constructor: This should not be used. */
  Curia(const Curia& curia) throw(Curia_error);
  /** assignment operator: This should not be used. */
  Curia& operator =(const Curia& curia) throw(Curia_error);
  /** lock the database. */
  bool lock();
  /** unlock the database. */
  void unlock();
};



#endif                                   /* duplication check */


/* END OF FILE */
