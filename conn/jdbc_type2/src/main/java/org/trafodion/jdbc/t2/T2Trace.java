package org.trafodion.jdbc.t2;

import java.io.*;
import java.util.*;

public class T2Trace {
	private static PrintStream out; 

	public T2Trace() {
	}

	private static void initLog() {
		if(out == null) {
			try {
				out	= new PrintStream(new File("trace.log"));
			} catch(IOException e) {
				e.printStackTrace();
				System.out.println(e.getMessage());
			}
		}
	}

	public static void LOG_ENTER(String method, String msg) {
		initLog();
		out.println("ENTER: " + method + ": " + msg);
	}

	public static void LOG_OUT(String method, String msg) {
		initLog();
		out.println("RETURN: " + method + ": " + msg);
	}
}
