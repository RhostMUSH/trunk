/*************************************************************************************************
 * Abstraction for database managers compatible with DBM
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


#ifndef _XADBM_H                         /* duplication check */
#define _XADBM_H


#include <xqdbm.h>



/**
 * Error container for ADBM.
 */
class qdbm::DBM_error {
  //----------------------------------------------------------------
  // public member functions
  //----------------------------------------------------------------
public:
  /**
   * Create an instance.
   */
  DBM_error() throw();
  /**
   * Create an instance.
   * @param message the string of a error message.
   */
  DBM_error(const char* message) throw();
  /**
   * Release resources of the instance.
   */
  virtual ~DBM_error() throw();
  /**
   * Cast operator into pointer to char.
   * @return the pointer to the string.
   */
  virtual operator const char*() const throw();
  /**
   * Get the message.
   * @return the pointer to the string.
   */
  virtual const char* message() const throw();
  //----------------------------------------------------------------
  // private member variables
  //----------------------------------------------------------------
private:
  /** The pointer to the region of the error message. */
  char* errmsg;
};



//----------------------------------------------------------------
// Outer operators for Datum
//----------------------------------------------------------------
/**
 * Temporary concatenation operator for Datum.
 * @param former the former datum.
 * @param latter the latter datum.
 * @return reference to a temporary instance.
 */
qdbm::Datum qdbm::operator +(const qdbm::Datum& former, const qdbm::Datum& latter);
/**
 * Temporary concatenation operator for Datum.
 * @param datum the former datum.
 * @param str the latter string.
 * @return reference to a temporary instance.
 */
qdbm::Datum qdbm::operator +(const qdbm::Datum& datum, const char* str);
/**
 * Temporary concatenation operator for Datum.
 * @param str the former string.
 * @param datum the latter datum.
 * @return reference to a temporary instance.
 */
qdbm::Datum qdbm::operator +(const char* str, const qdbm::Datum& datum);



/**
 * Datum of records for ADBM.
 */
class qdbm::Datum {
  //----------------------------------------------------------------
  // public member functions
  //----------------------------------------------------------------
public:
  /**
   * Create an instance.
   * @param dptr the pointer to the region of data.
   * @param dsize the size of the region.  If it is negative, the size is assigned
   * with `std::strlen(dptr)'.
   */
  Datum(const char* dptr = "", int dsize = -1);
  /**
   * Create an instance.
   * @param num an integer number.
   */
  Datum(int num);
  /**
   * Copy constructor.
   * @param datum a source instance.
   */
  Datum(const Datum& datum);
  /**
   * Release resources of the instance.
   */
  virtual ~Datum() throw();
  /**
   * Assignment operator.
   * @param datum a source instance.
   * @return reference to itself.
   */
  Datum& operator =(const Datum& datum);
  /**
   * Assignment operator.
   * @param str a source string.
   * @return reference to itself.
   */
  Datum& operator =(const char* str);
  /**
   * Concatenation operator.
   * @param datum a latter instance.
   * @return reference to itself.
   */
  virtual Datum& operator <<(const Datum& datum);
  /**
   * Concatenation operator.
   * @param str a latter string.
   * @return reference to itself.
   */
  virtual Datum& operator <<(const char* str);
  /**
   * Equality operator.
   * @param datum a comparing instance.
   * @return true if both equal, else, false.
   */
  virtual bool operator ==(const Datum& datum) const;
  /**
   * Inequality operator.
   * @param datum a comparing instance.
   * @return true if both do not equal, else, false.
   */
  virtual bool operator !=(const Datum& datum) const;
  /**
   * Equality operator.
   * @param str a comparing string.
   * @return true if both equal, else, false.
   */
  virtual bool operator ==(const char* str) const;
  /**
   * Inequality operator.
   * @param str a comparing string.
   * @return true if both do not equal, else, false.
   */
  virtual bool operator !=(const char* str) const;
  /**
   * Subscripting operator.
   * @param idx the index of a character.
   * @return reference of the character.
   */
  virtual char& operator [](int idx) const;
  /**
   * Cast operator into pointer to char.
   * @return the pointer of the region of the datum.
   * @note Because an additional zero code is appended at the end of the region of the return
   * value, the return value can be treated as a character string.
   */
  virtual operator const char*() const;
  /**
   * Get the pointer of the region of the datum.
   * @return the pointer of the region of the datum.
   * @note Because an additional zero code is appended at the end of the region of the return
   * value, the return value can be treated as a character string.
   */
  virtual const char* ptr() const;
  /**
   * Get the size of the region of the datum.
   * @return the size of the region of the datum.
   */
  virtual int size() const;
  //----------------------------------------------------------------
  // private member variables
  //----------------------------------------------------------------
private:
  /** The pointer to the region. */
  char* dptr;
  /** The size of the region. */
  int dsize;
  //----------------------------------------------------------------
  // private member functions
  //----------------------------------------------------------------
private:
  /**
   * Create an instance.
   * @param dptr the region allocated with `std::malloc' or `std::realloc'.
   * @param dsize the size of the region.
   * @param dummy ignored.  It is for difference of signature.
   * @note the specified region is released with `std::free' at the destruction of the instance.
   */
  Datum(char* dptr, int dsize, bool dummy);
  //----------------------------------------------------------------
  // friend classes and functions
  //----------------------------------------------------------------
  friend class qdbm::Depot;
  friend class qdbm::Curia;
  friend class qdbm::Villa;
  friend Datum qdbm::operator +(const Datum& former, const Datum& latter);
  friend Datum qdbm::operator +(const Datum& datum, const char* str);
  friend Datum qdbm::operator +(const char* str, const Datum& datum);
};



/**
 * Aabstraction for database managers compatible with DBM.
 */
class qdbm::ADBM {
  //----------------------------------------------------------------
  // public member functions
  //----------------------------------------------------------------
public:
  /**
   * Release resources of the instance.
   */
  virtual ~ADBM(){}
  /**
   * Close the database connection.
   * @throw DBM_error if an error occurs.
   */
  virtual void close() throw(DBM_error) = 0;
  /**
   * Store a record.
   * @param key reference to a key object.
   * @param val reference to a value object.
   * @param replace whether the existing value is to be overwritten or not.
   * @throw DBM_error if an error occurs or replace is cancelled.
   */
  virtual void storerec(const Datum& key, const Datum& val, bool replace = true)
    throw(DBM_error) = 0;
  /**
   * Delete a record.
   * @param key reference to a key object.
   * @throw DBM_error if an error occurs or no record corresponds.
   */
  virtual void deleterec(const Datum& key) throw(DBM_error) = 0;
  /**
   * Fetch a record.
   * @param key reference to a key object.
   * @return a temporary instance of the value of the corresponding record.
   * @throw DBM_error if an error occurs or no record corresponds.
   */
  virtual Datum fetchrec(const Datum& key) throw(DBM_error) = 0;
  /**
   * Get the first key.
   * @return a temporary instance of the key of the first record.
   * @throw DBM_error if an error occurs or no record corresponds.
   */
  virtual Datum firstkey() throw(DBM_error) = 0;
  /**
   * Get the next key.
   * @return a temporary instance of the key of the next record.
   * @throw DBM_error if an error occurs or no record corresponds.
   */
  virtual Datum nextkey() throw(DBM_error) = 0;
  /**
   * Check whether a fatal error occured or not.
   * @return true if the database has a fatal error, false if not.
   * @throw DBM_error if an error occurs.
   */
  virtual bool error() throw(DBM_error) = 0;
};



#endif                                   /* duplication check */


/* END OF FILE */
