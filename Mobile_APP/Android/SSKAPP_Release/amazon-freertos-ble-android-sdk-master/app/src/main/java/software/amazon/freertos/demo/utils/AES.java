package software.amazon.freertos.demo.utils;

import android.util.Log;

import java.nio.charset.Charset;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;

import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

import software.amazon.freertos.amazonfreertossdk.BleCommandTransferDemo;

public class AES {
    String TAG = "AES";
    public static byte[] genratedIv;


    public static SecretKey getECDHSecreat(int bits) throws NoSuchAlgorithmException {
        //This method is provided as to securely generate a AES key of the given length.

        //In practice you can specify your own SecureRandom instance.
        /*KeyGenerator kgen = KeyGenerator.getInstance("AES");
        kgen.init(bits);
        return kgen.generateKey();*/
        //return getSecreatKey("0123456789abcdef0123456789abcdef");
        return getSecreatKey(new byte[]{
                (byte) 0x0,
                (byte) 0x1,
                (byte) 0x2,
                (byte) 0x3,
                (byte) 0x4,
                (byte) 0x5,
                (byte) 0x6,
                (byte) 0x7,
                (byte) 0x8,
                (byte) 0x9,
                (byte) 0xa,
                (byte) 0xb,
                (byte) 0xc,
                (byte) 0xd,
                (byte) 0xe,
                (byte) 0xf,
                (byte) 0x0,
                (byte) 0x1,
                (byte) 0x2,
                (byte) 0x3,
                (byte) 0x4,
                (byte) 0x5,
                (byte) 0x6,
                (byte) 0x7,
                (byte) 0x8,
                (byte) 0x9,
                (byte) 0xa,
                (byte) 0xb,
                (byte) 0xc,
                (byte) 0xd,
                (byte) 0xe,
                (byte) 0xf
        });
    }

    public static SecretKeySpec getSecreatKey(byte[] hex) {
        SecretKeySpec key = new SecretKeySpec(hex, "AES");
        return key;
    }

    public static byte[] getIV() {
      /*  SecureRandom rnd = new SecureRandom();
        byte[] IV = new byte[128 / 8];
        rnd.nextBytes(IV);
        return IV;*/


       /* return new byte[]{
                (byte) 0x0,
                (byte) 0x1,
                (byte) 0x2,
                (byte) 0x3,
                (byte) 0x4,
                (byte) 0x5,
                (byte) 0x6,
                (byte) 0x7,
                (byte) 0x8,
                (byte) 0x9,
                (byte) 0xa,
                (byte) 0xb,
                (byte) 0xc,
                (byte) 0xd,
                (byte) 0xe,
                (byte) 0xf};*/
        return new byte[]{(byte) 0x65, (byte) 0x69, (byte) 0x6e,
                (byte) 0x66, (byte) 0x6f, (byte) 0x63, (byte) 0x68,
                (byte) 0x69, (byte) 0x70, (byte) 0x73, (byte) 0x5f,
                (byte) 0x61, (byte) 0x72, (byte) 0x72, (byte) 0x6f, (byte) 0x77};


        //return BleCommandTransfer.Companion.hexToByteArray("000102030405060708090a0b0c0d0e0f");//random number first time gateway without crc and length

    }

    public static byte[] encrypt(SecretKey key, byte[] plaintext) throws Exception {
        IvParameterSpec IVSpec = new IvParameterSpec(genratedIv);

        //Create the cipher object to perform AES operations.
        //Specify Advanced Encryption Standard - Cipher Feedback Mode - No Padding
        Cipher AESCipher = Cipher.getInstance("AES/CFB8/NoPadding");

        //Initialize the Cipher with the key and initialization vector.
        AESCipher.init(Cipher.ENCRYPT_MODE, key, IVSpec);

        //Encrypts the plaintext data
        byte[] ciphertext = AESCipher.doFinal(plaintext);

        return ciphertext;
    }

    public static byte[] decrypt(SecretKey key, byte[] IV, byte[] ciphertext) throws Exception {
        //Create the cipher object to perform AES operations.
        //Specify Advanced Encryption Standard - Cipher Feedback Mode - No Padding
        //Cipher AESCipher = Cipher.getInstance("AES/CFB/NoPadding");
        Cipher AESCipher = Cipher.getInstance("AES/CFB8/NoPadding");

        //Create the IvParameterSpec object from the raw IV
        IvParameterSpec IVSpec = new IvParameterSpec(genratedIv);

        //Initialize the Cipher with the key and initialization vector.
        AESCipher.init(Cipher.DECRYPT_MODE, key, IVSpec);

        //Decrypts the ciphertext data

     /*   byte[] cipher1 = new byte[]{(byte) 0x01,
                (byte) 0x39,
                (byte) 0xd2,
                (byte) 0xdf,
                (byte) 0xbb,
                (byte) 0xbd,
                (byte) 0x01,
                (byte) 0xbc};*/
        byte[] plaintext = AESCipher.doFinal(BleCommandTransferDemo.Companion.hexToByteArray("0139d2dfbbbd01bc"));

        return plaintext;
    }

    public void performAES() throws Exception {
        //Demo the program

        genratedIv = getIV();
        String sPlaintext = "Test"; //String plaintext
        byte[] rPlaintext = sPlaintext.getBytes(Charset.forName("UTF-8")); //Raw byte array plaintext

        //We first need to generate a key of 128-bit
        SecretKey key = getECDHSecreat(256); //Add ecdh secreat here

        //Encrypt the plaintext
        byte[] output = encrypt(key, rPlaintext);

        //Decrypt the ciphertext
        byte[] dPlaintext = decrypt(key, genratedIv, output);

        String decryptedMessage = new String(dPlaintext, Charset.forName("UTF-8"));
        //Print stuff out

        Log.d(TAG, "Original message: " + sPlaintext);
        Log.d(TAG, "Original message bytes: " + Arrays.toString(rPlaintext));
        Log.d(TAG, "Encryption Output bytes: " + Arrays.toString(output));
        Log.d(TAG, "Decrypted message bytes: " + Arrays.toString(dPlaintext));
        Log.d(TAG, "Decrypted message: " + decryptedMessage);

        Log.d(TAG, "Encryption message hex: " + BleCommandTransferDemo.Companion.byteArrayToHex(rPlaintext));
        Log.d(TAG, "Encryption Output hex: " + BleCommandTransferDemo.Companion.byteArrayToHex(output));
        Log.d(TAG, "Encryption Decrypted hex: " + BleCommandTransferDemo.Companion.byteArrayToHex(dPlaintext));
    }


}