/*************************************************************************************************
 * Implementation of common setting for QDBM
 *                                                      Copyright (C) 2000-2005 Mikio Hirabayashi
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


#include "xqdbm.h"



/*************************************************************************************************
 * mutex
 *************************************************************************************************/


pthread_mutex_t qdbm::ourmutex;


namespace {
  class Init {
  public:
    Init(){
      pthread_mutex_init(&qdbm::ourmutex, NULL);
    }
    ~Init(){
      pthread_mutex_destroy(&qdbm::ourmutex);
    }
  };
  Init init;
}



/* END OF FILE */
