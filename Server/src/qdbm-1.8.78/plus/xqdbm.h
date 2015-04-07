/*************************************************************************************************
 * Common setting for QDBM
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


#ifndef _XQDBM_H                         /* duplication check */
#define _XQDBM_H

extern "C" {
#include <pthread.h>
}



/**
 * Namespace of QDBM.
 */
namespace qdbm {
  class DBM_error;
  class Datum;
  Datum operator +(const Datum& former, const Datum& latter);
  Datum operator +(const Datum& datum, const char* str);
  Datum operator +(const char* str, const Datum& datum);
  class ADBM;
  class Depot_error;
  class Depot;
  class Curia_error;
  class Curia;
  class Villa_error;
  class Villa;
  extern pthread_mutex_t ourmutex;
}


/**
 * Common Mutex of QDBM.
 */
extern pthread_mutex_t qdbm::ourmutex;



#endif                                   /* duplication check */


/* END OF FILE */
