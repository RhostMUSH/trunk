/*************************************************************************************************
 * Test cases for Depot for Java
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
 * Test case for Depot.
 */
class DepotTester extends Thread {
  //----------------------------------------------------------------
  // constants
  //----------------------------------------------------------------
  private static final int LOOPNUM = 10000;
  private static final int BUCKETNUM = 1000;
  private static final int THREADNUM = 10;
  private static final java.util.Random RND = new java.util.Random();
  //----------------------------------------------------------------
  // instance fields
  //----------------------------------------------------------------
  private Depot mydepot;
  private int myid;
  private boolean myprinter;
  private DepotException myde;
  //----------------------------------------------------------------
  // public or protected methods
  //----------------------------------------------------------------
  /**
   * Main routine.
   * @param args specifies a name of a database file with the first element.
   */
  public static void main(String[] args){
    if(args.length != 1){
      System.err.println("usege: java qdbm.DepotTester name");
      return;
    }
    if(!dowrite(args[0])) System.exit(1);
    if(!doread(args[0])) System.exit(1);
    if(!domulti(args[0])) System.exit(1);
    System.out.println("all ok");
    System.out.println("");
    System.exit(0);
  }
  /**
   * Perform writing test.
   * @param name specifies a name of a database.
   * @return whether the test is success or not.
   */
  public static boolean dowrite(String name){
    System.out.println("<Writing Test>");
    System.out.println("  name=" + name);
    System.out.println("");
    Depot depot = null;
    boolean err = false;
    try {
      // open a database
      System.out.print("Creating ... ");
      depot = new Depot(name, Depot.OWRITER | Depot.OCREAT | Depot.OTRUNC, BUCKETNUM);
      System.out.println("ok");
      // loop for storing
      System.out.println("Writing");
      for(int i = 1; i <= LOOPNUM; i++){
        byte[] buf = Util.numstr(i, 8, '0').getBytes();
        // store a record into the database
        depot.put(buf, buf, Depot.DOVER);
        // print progression
        if(i % (LOOPNUM / 250) == 0){
          System.out.print('.');
          System.out.flush();
          if(i == LOOPNUM || i % (LOOPNUM / 10) == 0){
            System.out.println(" (" + new String(buf) + ")");
            System.out.flush();
          }
        }
      }
      System.out.println("ok");
    } catch(DepotException e){
      err = true;
      // report an error
      System.err.println(e);
    } finally {
      try {
        // close the database
        System.out.print("Closing ... ");
        if(depot != null) depot.close();
        System.out.println("ok");
      } catch(DepotException e){}
    }
    System.out.println("");
    return err ? false : true;
  }
  /**
   * Perform reading test.
   * @param name specifies a name of a database.
   * @return whether the test is success or not.
   */
  public static boolean doread(String name){
    System.out.println("<Reading Test>");
    System.out.println("  name=" + name);
    System.out.println("");
    Depot depot = null;
    boolean err = false;
    try {
      // open a database
      System.out.print("Opening ... ");
      depot = new Depot(name);
      System.out.println("ok");
      System.out.println("Reading");
      // loop for retrieving
      for(int i = 1; i <= LOOPNUM; i++){
        byte[] buf = Util.numstr(i, 8, '0').getBytes();
        // retrieve a record from the database
        depot.get(buf, 0, -1);
        // print progression
        if(i % (LOOPNUM / 250) == 0){
          System.out.print('.');
          System.out.flush();
          if(i == LOOPNUM || i % (LOOPNUM / 10) == 0){
            System.out.println(" (" + new String(buf) + ")");
            System.out.flush();
          }
        }
      }
      System.out.println("ok");
    } catch(DepotException e){
      err = true;
      // report an error
      System.err.println(e);
    } finally {
      try {
        // close the database
        System.out.print("Closing ... ");
        if(depot != null) depot.close();
        System.out.println("ok");
      } catch(DepotException e){}
    }
    System.out.println("");
    return err ? false : true;
  }
  /**
   * Perform multi thread test.
   * @param name specifies a name of a database.
   * @return whether the test is success or not.
   */
  public static boolean domulti(String name){
    System.out.println("<Multi-thread Test>");
    System.out.println("  name=" + name);
    System.out.println("");
    Depot depot = null;
    boolean err = false;
    try {
      // open a database
      System.out.print("Opening ... ");
      depot = new Depot(name, Depot.OWRITER | Depot.OCREAT | Depot.OTRUNC, BUCKETNUM);
      System.out.println("ok");
      // prepare threads
      DepotTester[] dts = new DepotTester[THREADNUM];
      // roop for each threads
      System.out.println("Writing");
      for(int i = 0; i < dts.length; i++){
        dts[i] = new DepotTester(depot, i, i == dts.length / 2);
        dts[i].start();
      }
      // join every threads
      for(int i = 0; i < dts.length; i++){
        try {
          dts[i].join();
          if(dts[i].myde != null){
            err = true;
            System.err.println(dts[i].myde);
          }
        } catch(InterruptedException e){
          i--;
        }
      }
      if(!err) System.out.println("ok");
      System.out.flush();
      // check every record
      System.out.print("Validation checking ... ");
      for(int i = 1; i <= LOOPNUM; i++){
        byte[] buf = Util.numstr(i, 8, '0').getBytes();
        byte[] val = depot.get(buf, 0, -1);
        if(val.length != THREADNUM){
          err = true;
          System.err.println("size error: " + val.length);
        }
      }
      if(!err) System.out.println("ok");
    } catch(DepotException e){
      err = true;
      System.err.println(e);
    } finally {
      try {
        // close the database
        System.out.print("Closing ... ");
        if(depot != null) depot.close();
        System.out.println("ok");
      } catch(DepotException e){}
    }
    System.out.println("");
    return err ? false : true;
  }
  /**
   * Constructor for multi thread test.
   */
  private DepotTester(Depot depot, int id, boolean printer){
    mydepot = depot;
    myid = id;
    myprinter = printer;
    myde = null;
  }
  /**
   * Method for multi thread test.
   */
  public void run(){
    // roop for storing
    for(int i = 1; i <= LOOPNUM; i++){
      // store a record into the database
      byte[] buf = Util.numstr(i, 8, '0').getBytes();
      try {
        if(RND.nextInt(LOOPNUM / 20) == 0) yield();
        mydepot.put(buf, (myid % 2 == 0 ? "=" : "*").getBytes(), Depot.DCAT);
      } catch(DepotException e){
        myde = e;
        return;
      }
      // print progression
      if(myprinter && i % (LOOPNUM / 250) == 0){
        System.out.print('.');
        System.out.flush();
        if(i == LOOPNUM || i % (LOOPNUM / 10) == 0){
          System.out.println(" (" + new String(buf) + ")");
          System.out.flush();
          // garbage collection test
          System.gc();
        }
      }
    }
  }
}



/* END OF FILE */
