package software.amazon.freertos.amazonfreertossdk;

import android.os.Environment;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

public class FileUtil {

    public static void appendLog(String text) {
        File appDirectory = new File(Environment.getExternalStorageDirectory() + "/SSKProject");
        File logDirectory = new File(appDirectory + "/log");
        File logFile = new File(logDirectory, "1.txt");

        // create app folder
        if (!appDirectory.exists()) {
            appDirectory.mkdir();
        }

        // create log folderṢre
        if (!logDirectory.exists()) {
            logDirectory.mkdir();
        }
        try {
            // BufferedWriter for performance, true to set append to file flag
            BufferedWriter buf = new BufferedWriter(new FileWriter(logFile, true));
            buf.append(text);
            buf.newLine();
            buf.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    void writeDataToFile() {
        File appDirectory = new File(Environment.getExternalStorageDirectory() + "/SSKProject");
        File logDirectory = new File(appDirectory + "/log");
        File logFile = new File(logDirectory, "logcat" + System.currentTimeMillis() + ".txt");

        // create app folder
        if (!appDirectory.exists()) {
            appDirectory.mkdir();
        }

        // create log folderṢre
        if (!logDirectory.exists()) {
            logDirectory.mkdir();
        }

        // clear the previous logcat and then write the new one to the file
        try {
            Process process = Runtime.getRuntime().exec("logcat -c");
            process = Runtime.getRuntime().exec("logcat -f " + logFile);
        } catch (IOException e) {
            e.printStackTrace();
        }

    }
}