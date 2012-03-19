/*************************************************************************************************
 * Test cases for Villa for Java
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
 * Test case for Villa.
 */
class VillaTester extends Thread {
  //----------------------------------------------------------------
  // constants
  //----------------------------------------------------------------
  private static final int LOOPNUM = 10000;
  private static final int THREADNUM = 10;
  private static final java.util.Random RND = new java.util.Random();
  //----------------------------------------------------------------
  // instance fields
  //----------------------------------------------------------------
  private Villa myvilla;
  private int myid;
  private boolean myprinter;
  private VillaException myve;
  //----------------------------------------------------------------
  // public or protected methods
  //----------------------------------------------------------------
  /**
   * Main routine.
   * @param args specifies a name of a database file with the first element.
   */
  public static void main(String[] args) throws Exception {
    if(args.length != 1){
      System.err.println("usege: java qdbm.VillaTester name");
      return;
    }
    if(!dowrite(args[0], false)) System.exit(1);
    if(!doread(args[0])) System.exit(1);
    if(!domulti(args[0], false)) System.exit(1);
    if(!dowrite(args[0], true)) System.exit(1);
    if(!doread(args[0])) System.exit(1);
    if(!domulti(args[0], true)) System.exit(1);
    System.out.println("all ok");
    System.out.println("");
    System.exit(0);
  }
  /**
   * Perform writing test.
   * @param name specifies a name of a database.
   * @param zc specifies whether to compress leaves in the database.
   * @return whether the test is success or not.
   */
  public static boolean dowrite(String name, boolean zc){
    System.out.println("<Writing Test>");
    System.out.println("  name=" + name);
    System.out.println("");
    Villa villa = null;
    boolean err = false;
    try {
      // open a database
      System.out.print("Creating ... ");
      int omode = Villa.OWRITER | Villa.OCREAT | Villa.OTRUNC;
      if(zc) omode |= Villa.OZCOMP;
      villa = new Villa(name, omode, Villa.CMPLEX);
      System.out.println("ok");
      // loop for storing
      System.out.println("Writing");
      for(int i = 1; i <= LOOPNUM; i++){
        byte[] buf = Util.numstr(i, 8, '0').getBytes();
        // store a record into the database
        villa.put(buf, buf, Villa.DOVER);
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
    } catch(VillaException e){
      err = true;
      // report an error
      System.err.println(e);
    } finally {
      try {
        // close the database
        System.out.print("Closing ... ");
        if(villa != null) villa.close();
        System.out.println("ok");
      } catch(VillaException e){}
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
    Villa villa = null;
    boolean err = false;
    try {
      // open a database
      System.out.print("Opening ... ");
      villa = new Villa(name);
      System.out.println("ok");
      System.out.println("Reading");
      // loop for retrieving
      for(int i = 1; i <= LOOPNUM; i++){
        byte[] buf = Util.numstr(i, 8, '0').getBytes();
        // retrieve a record from the database
        villa.get(buf);
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
    } catch(VillaException e){
      err = true;
      // report an error
      System.err.println(e);
    } finally {
      try {
        // close the database
        System.out.print("Closing ... ");
        if(villa != null) villa.close();
        System.out.println("ok");
      } catch(VillaException e){}
    }
    System.out.println("");
    return err ? false : true;
  }
  /**
   * Perform multi thread test.
   * @param name specifies a name of a database.
   * @param zc specifies whether to compress leaves in the database.
   * @return whether the test is success or not.
   */
  public static boolean domulti(String name, boolean zc){
    System.out.println("<Multi-thread Test>");
    System.out.println("  name=" + name);
    System.out.println("");
    Villa villa = null;
    boolean err = false;
    try {
      // open a database
      System.out.print("Opening ... ");
      int omode = Villa.OWRITER | Villa.OCREAT | Villa.OTRUNC;
      if(zc) omode |= Villa.OZCOMP;
      villa = new Villa(name, omode, Villa.CMPLEX);
      System.out.println("ok");
      // prepare threads
      VillaTester[] dts = new VillaTester[THREADNUM];
      // roop for each threads
      System.out.println("Writing");
      for(int i = 0; i < dts.length; i++){
        dts[i] = new VillaTester(villa, i, i == dts.length / 2);
        dts[i].start();
      }
      // join every threads
      for(int i = 0; i < dts.length; i++){
        try {
          dts[i].join();
          if(dts[i].myve != null){
            err = true;
            System.err.println(dts[i].myve);
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
        byte[] val = villa.get(buf);
        if(val.length != 1 || villa.vsiz(buf) != 1){
          err = true;
          System.err.println("size error: " + val.length);
        }
        int vnum = villa.vnum(buf);
        if(vnum != THREADNUM){
          err = true;
          System.err.println("vnum error: " + vnum);
        }
      }
      if(!err) System.out.println("ok");
    } catch(VillaException e){
      err = true;
      System.err.println(e);
    } finally {
      try {
        // close the database
        System.out.print("Closing ... ");
        if(villa != null) villa.close();
        System.out.println("ok");
      } catch(VillaException e){}
    }
    System.out.println("");
    return err ? false : true;
  }
  /**
   * Constructor for multi thread test.
   */
  private VillaTester(Villa villa, int id, boolean printer){
    myvilla = villa;
    myid = id;
    myprinter = printer;
    myve = null;
  }
  /**
   * Method for multi thread test.
   */
  public void run(){
    // roop for storing
    for(int i = 1; i <= LOOPNUM; i++){
      // store a record into the database
      byte[] buf = Util.numstr(i, 8, '0').getBytes();
      boolean tran = false;
      try {
        if(RND.nextInt(LOOPNUM / 7 + 1) == 0){
          myvilla.tranbegin();
          tran = true;
        }
        if(RND.nextInt(LOOPNUM / 20) == 0) yield();
        myvilla.put(buf, (myid % 2 == 0 ? "=" : "*").getBytes(),
                    RND.nextInt(3) == 0 ? Villa.DDUPR : Villa.DDUP);
      } catch(VillaException e){
        try {
          if(tran) myvilla.tranabort();
        } catch(VillaException ee){}
        myve = e;
        return;
      }
      try {
        if(tran) myvilla.trancommit();
      } catch(VillaException e){
        myve = e;
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
