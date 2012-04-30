/*************************************************************************************************
 * Test cases for Curia for Java
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
 * Test case for Curia.
 */
class CuriaTester extends Thread {
  //----------------------------------------------------------------
  // constants
  //----------------------------------------------------------------
  private static final int LOOPNUM = 10000;
  private static final int LOBLOOPNUM = 1000;
  private static final int BUCKETNUM = 1000;
  private static final int LOBBUCKETNUM = 100;
  private static final int DBDIVNUM = 10;
  private static final int THREADNUM = 10;
  private static final java.util.Random RND = new java.util.Random();
  //----------------------------------------------------------------
  // class variables
  //----------------------------------------------------------------
  private static int loopnum;
  private static int bucketnum;
  //----------------------------------------------------------------
  // instance fields
  //----------------------------------------------------------------
  private Curia mycuria;
  private boolean mylob;
  private int myid;
  private boolean myprinter;
  private CuriaException myce;
  //----------------------------------------------------------------
  // public or protected methods
  //----------------------------------------------------------------
  /**
   * Main routine.
   * @param args specifies a name of a database file with the first element.
   */
  public static void main(String[] args){
    if(args.length != 1){
      System.err.println("usege: java qdbm.CuriaTester name");
      return;
    }
    loopnum = LOOPNUM;
    bucketnum = BUCKETNUM;
    if(!dowrite(args[0], false)) System.exit(1);
    if(!doread(args[0], false)) System.exit(1);
    if(!domulti(args[0], false)) System.exit(1);
    loopnum = LOBLOOPNUM;
    bucketnum = LOBBUCKETNUM;
    if(!dowrite(args[0], true)) System.exit(1);
    if(!doread(args[0], true)) System.exit(1);
    if(!domulti(args[0], true)) System.exit(1);
    System.out.println("all ok");
    System.out.println("");
    System.exit(0);
  }
  /**
   * Perform writing test.
   * @param name specifies a name of a database.
   * @param lob specifies whether test large objects or not.
   * @return whether the test is success or not.
   */
  public static boolean dowrite(String name, boolean lob){
    System.out.println("<Writing Test>");
    System.out.println("  name=" + name + "  lob=" + lob);
    System.out.println("");
    Curia curia = null;
    boolean err = false;
    try {
      // open a database
      System.out.print("Creating ... ");
      curia = new Curia(name, Curia.OWRITER | Curia.OCREAT | Curia.OTRUNC, bucketnum, DBDIVNUM);
      System.out.println("ok");
      // loop for storing
      System.out.println("Writing");
      for(int i = 1; i <= loopnum; i++){
        byte[] buf = Util.numstr(i, 8, '0').getBytes();
        // store a record into the database
        if(lob){
          curia.putlob(buf, buf, Curia.DOVER);
        } else {
          curia.put(buf, buf, Curia.DOVER);
        }
        // print progression
        if(i % (loopnum / 250) == 0){
          System.out.print('.');
          System.out.flush();
          if(i == loopnum || i % (loopnum / 10) == 0){
            System.out.println(" (" + new String(buf) + ")");
            System.out.flush();
          }
        }
      }
      System.out.println("ok");
    } catch(CuriaException e){
      err = true;
      // report an error
      System.err.println(e);
    } finally {
      try {
        // close the database
        System.out.print("Closing ... ");
        if(curia != null) curia.close();
        System.out.println("ok");
      } catch(CuriaException e){}
    }
    System.out.println("");
    return err ? false : true;
  }
  /**
   * Perform reading test.
   * @param name specifies a name of a database.
   * @param lob specifies whether test large objects or not.
   * @return whether the test is success or not.
   */
  public static boolean doread(String name, boolean lob){
    System.out.println("<Reading Test>");
    System.out.println("  name=" + name + "  lob=" + lob);
    System.out.println("");
    Curia curia = null;
    boolean err = false;
    try {
      // open a database
      System.out.print("Opening ... ");
      curia = new Curia(name);
      System.out.println("ok");
      System.out.println("Reading");
      // loop for retrieving
      for(int i = 1; i <= loopnum; i++){
        byte[] buf = Util.numstr(i, 8, '0').getBytes();
        // retrieve a record from the database
        if(lob){
          curia.getlob(buf, 0, -1);
        } else {
          curia.get(buf, 0, -1);
        }
        // print progression
        if(i % (loopnum / 250) == 0){
          System.out.print('.');
          System.out.flush();
          if(i == loopnum || i % (loopnum / 10) == 0){
            System.out.println(" (" + new String(buf) + ")");
            System.out.flush();
          }
        }
      }
      System.out.println("ok");
    } catch(CuriaException e){
      err = true;
      // report an error
      System.err.println(e);
    } finally {
      try {
        // close the database
        System.out.print("Closing ... ");
        if(curia != null) curia.close();
        System.out.println("ok");
      } catch(CuriaException e){}
    }
    System.out.println("");
    return err ? false : true;
  }
  /**
   * Perform multi thread test.
   * @param name specifies a name of a database.
   * @param lob specifies whether test large objects or not.
   * @return whether the test is success or not.
   */
  public static boolean domulti(String name, boolean lob){
    System.out.println("<Multi-thread Test>");
    System.out.println("  name=" + name + "  lob=" + lob);
    System.out.println("");
    Curia curia = null;
    boolean err = false;
    try {
      // open a database
      System.out.print("Opening ... ");
      curia = new Curia(name, Curia.OWRITER | Curia.OCREAT | Curia.OTRUNC, bucketnum, DBDIVNUM);
      System.out.println("ok");
      // prepare threads
      CuriaTester[] dts = new CuriaTester[THREADNUM];
      // roop for each threads
      System.out.println("Writing");
      for(int i = 0; i < dts.length; i++){
        dts[i] = new CuriaTester(curia, lob, i, i == dts.length / 2);
        dts[i].start();
      }
      // join every threads
      for(int i = 0; i < dts.length; i++){
        try {
          dts[i].join();
          if(dts[i].myce != null){
            err = true;
            System.err.println(dts[i].myce);
          }
        } catch(InterruptedException e){
          i--;
        }
      }
      if(!err) System.out.println("ok");
      System.out.flush();
      // check every record
      System.out.print("Validation checking ... ");
      for(int i = 1; i <= loopnum; i++){
        byte[] buf = Util.numstr(i, 8, '0').getBytes();
        byte[] val;
        if(lob){
          val = curia.getlob(buf, 0, -1);
        } else {
          val = curia.get(buf, 0, -1);
        }
        if(val.length != THREADNUM){
          err = true;
          System.err.println("size error: " + val.length);
        }
      }
      if(!err) System.out.println("ok");
    } catch(CuriaException e){
      err = true;
      System.err.println(e);
    } finally {
      try {
        // close the database
        System.out.print("Closing ... ");
        if(curia != null) curia.close();
        System.out.println("ok");
      } catch(CuriaException e){}
    }
    System.out.println("");
    return err ? false : true;
  }
  /**
   * Constructor for multi thread test.
   */
  private CuriaTester(Curia curia, boolean lob, int id, boolean printer){
    mycuria = curia;
    mylob = lob;
    myid = id;
    myprinter = printer;
    myce = null;
  }
  /**
   * Method for multi thread test.
   */
  public void run(){
    // roop for storing
    for(int i = 1; i <= loopnum; i++){
      // store a record into the database
      byte[] buf = Util.numstr(i, 8, '0').getBytes();
      try {
        if(RND.nextInt(LOOPNUM / 20) == 0) yield();
        if(mylob){
          mycuria.putlob(buf, (myid % 2 == 0 ? "=" : "*").getBytes(), Curia.DCAT);
        } else {
          mycuria.put(buf, (myid % 2 == 0 ? "=" : "*").getBytes(), Curia.DCAT);
        }
      } catch(CuriaException e){
        myce = e;
        return;
      }
      // print progression
      if(myprinter && i % (loopnum / 250) == 0){
        System.out.print('.');
        System.out.flush();
        if(i == loopnum || i % (loopnum / 10) == 0){
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
