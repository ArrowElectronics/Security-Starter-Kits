package software.amazon.freertos.demo.utils

import org.apache.commons.io.FileUtils
import java.io.File
import java.io.IOException

object CommonFileUtils {
    @JvmStatic
    fun getFileBytes(filename: String?): ByteArray? {
        val path = "/storage/emulated/0/Crypto/2/"
        val file = File(path, filename)
        //File file = new File("/storage/emulated/0/Crypto/privateKey.pem");
        try {
            return FileUtils.readFileToByteArray(file)
        } catch (e: IOException) {
            e.printStackTrace()
        }
        return null
    }
    @JvmStatic
    fun getFileBytes1(filename: String?): ByteArray? {
        val path = "/storage/emulated/0/Crypto/1"
        val file = File(path, filename)
        //File file = new File("/storage/emulated/0/Crypto/privateKey.pem");
        try {
            return FileUtils.readFileToByteArray(file)
        } catch (e: IOException) {
            e.printStackTrace()
        }
        return null
    }
}