/*************************************************************************************************
 * Exception container of Curia
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
 * Exception container of Curia.
 */
public class CuriaException extends DBMException {
  //----------------------------------------------------------------
  // instance fields
  //----------------------------------------------------------------
  /** error code */
  public final int ecode;
  //----------------------------------------------------------------
  // public or protected methods
  //----------------------------------------------------------------
  /**
   * Set the error code with `Curia.EMISC'.
   */
  public CuriaException(){
    this(Curia.EMISC);
  }
  /**
   * Set the error code.
   * @param ecode an error code.
   */
  public CuriaException(int ecode){
    super(Curia.errmsg(ecode));
    this.ecode = ecode;
  }
}



/* END OF FILE */
