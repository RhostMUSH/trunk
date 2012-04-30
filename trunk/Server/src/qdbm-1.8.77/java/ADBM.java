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


package qdbm;



/**
 * Abstraction for database managers compatible with DBM.
 */
public interface ADBM {
  //----------------------------------------------------------------
  // methods
  //----------------------------------------------------------------
  /**
   * Close the database connection.
   * @throws DBMException if an error occurs.
   */
  void close() throws DBMException;
  /**
   * Store a record.
   * @param key a byte array of a key.
   * @param val a byte array of a value.
   * @param replace whether the existing value is to be overwritten or not.
   * @throws DBMException if an error occurs or replace is cancelled.
   */
  void store(byte[] key, byte[] val, boolean replace) throws DBMException;
  /**
   * Delete a record.
   * @param key a byte array of a key.
   * @throws DBMException if an error occurs or no record corresponds.
   */
  void delete(byte[] key) throws DBMException;
  /**
   * Fetch a record.
   * @param key a byte array of a key.
   * @return a byte array of the value of the corresponding record.
   * @throws DBMException if an error occurs or no record corresponds.
   */
  byte[] fetch(byte[] key) throws DBMException;
  /**
   * Get the first key.
   * @return a byte array of the key of the first record.
   * @throws DBMException if an error occurs or no record corresponds.
   */
  byte[] firstkey() throws DBMException;
  /**
   * Get the next key.
   * @return a byte array of the key of the next record.
   * @throws DBMException if an error occurs or no record corresponds.
   */
  byte[] nextkey() throws DBMException;
  /**
   * Check whether a fatal error occured or not.
   * @return true if the database has a fatal error, false if not.
   * @throws DBMException if an error occurs.
   */
  boolean error() throws DBMException;
}



/* END OF FILE */
