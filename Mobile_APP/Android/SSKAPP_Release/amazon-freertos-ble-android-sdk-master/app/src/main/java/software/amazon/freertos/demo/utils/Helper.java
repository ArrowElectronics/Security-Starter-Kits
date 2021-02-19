package software.amazon.freertos.demo.utils;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.security.SecureRandom;

public class Helper {
    public static String toHex(byte[] data, int length) {
        String out = new String();
        int loop;
        for (loop = 0; loop < length; loop++)
            out = out + String.format("%02X", data[loop]);
        return out;
    }

    public static String toHex(byte[] data) {
        return toHex(data, data.length);
    }

    public static String toHexAndTrim00AtFirst(byte[] data) {
        String result = toHex(data, data.length);
        while (result.startsWith("00")) {
            result = result.substring(2);
        }
        return result;
    }

    public static String toHex(byte data) {
        byte dataArray[] = {data};
        return toHex(dataArray);
    }

    public static byte[] rendomByte(int length) {
        SecureRandom random = new SecureRandom();
        byte bytes[] = new byte[length];
        random.nextBytes(bytes);
        return bytes;
    }

    public static byte[] toByte(String hex) {
        if (hex == null)
            return null;
        hex = hex.replaceAll("\\s", "");
        byte[] buffer = null;
        if (hex.length() % 2 != 0) {
            hex = "0" + hex;
        }
        int len = hex.length() / 2;
        buffer = new byte[len];
        for (int i = 0; i < len; i++) {
            buffer[i] = (byte) Integer.parseInt(
                    hex.substring(i * 2, i * 2 + 2), 16);
        }
        return buffer;
    }

    public static byte[] intTo2ByteArray(int value) {
        return new byte[]{(byte) (value >>> 8), (byte) value};
    }

    public static short byteArrayToInt(byte[] bArray) {
        ByteBuffer buffer = ByteBuffer.wrap(bArray);
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        return buffer.getShort();
    }

    private static final char[] HEX_CHARS = "0123456789abcdef".toCharArray();

    public static String stringToHex(String input)
            throws UnsupportedEncodingException {
        if (input == null)
            throw new NullPointerException();
        return asHex(input.getBytes());
    }

    private static String asHex(byte[] buf) {
        char[] chars = new char[2 * buf.length];
        for (int i = 0; i < buf.length; ++i) {
            chars[2 * i] = HEX_CHARS[(buf[i] & 0xF0) >>> 4];
            chars[2 * i + 1] = HEX_CHARS[buf[i] & 0x0F];
        }
        return new String(chars);
    }

    public static String hexToString(String hex) {

        StringBuilder sb = new StringBuilder();
        char[] hexData = hex.toCharArray();
        for (int count = 0; count < hexData.length - 1; count += 2) {
            int firstDigit = Character.digit(hexData[count], 16);
            int lastDigit = Character.digit(hexData[count + 1], 16);
            int decimal = firstDigit * 16 + lastDigit;
            sb.append((char) decimal);
        }
        return sb.toString();
    }
}
