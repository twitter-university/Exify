package com.twitter.university.exify;

import java.io.IOException;
import java.util.Map;

public class JHead {

    public static synchronized native Map<String, String> getImageInfo(
            String filename) throws IOException;

    public static synchronized native byte[] getThumbnail(String filename)
            throws IOException;

    static {
        System.loadLibrary("jhead_jni");
    }
}