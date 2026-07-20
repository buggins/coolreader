package org.coolreader.crengine;

public class L {
  public static class LoggerImpl implements Logger {
    public void i(String msg) {
      System.out.println(msg);
    }
    public void i(String msg, Exception e) {
      System.out.println(msg);
    }
    public void w(String msg) {
      System.out.println(msg);
    }
    public void w(String msg, Exception e) {
      System.out.println(msg);
    }
    public void e(String msg) {
      System.out.println(msg);
    }
    public void e(String msg, Exception e) {
      System.out.println(msg);
    }
    public void d(String msg) {
      System.out.println(msg);
    }
    public void d(String msg, Exception e) {
      System.out.println(msg);
    }
    public void v(String msg) {
      System.out.println(msg);
    }
    public void v(String msg, Exception e) {
      System.out.println(msg);
    }
    public void setLevel(int level) {
    }
  }

  public static void i(String msg) {
    System.out.println(msg);
  }
  public static void i(String msg, Exception e) {
    System.out.println(msg);
  }
  public static void w(String msg) {
    System.out.println(msg);
  }
  public static void w(String msg, Exception e) {
    System.out.println(msg);
  }
  public static void e(String msg) {
    System.out.println(msg);
  }
  public static void e(String msg, Exception e) {
    System.out.println(msg);
  }
  public static void d(String msg) {
    System.out.println(msg);
  }
  public static void d(String msg, Exception e) {
    System.out.println(msg);
  }
  public static void v(String msg) {
    System.out.println(msg);
  }
  public static void v(String msg, Exception e) {
    System.out.println(msg);
  }
  public static Logger create(String name) {
    return new LoggerImpl();
  }
  public static Logger create(String name, int level) {
    return new LoggerImpl();
  }
}
