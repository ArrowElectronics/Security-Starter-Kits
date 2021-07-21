/*
package com.eic.sskwb55;

import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;

import com.starkbank.ellipticcurve.Ecdsa;
import com.starkbank.ellipticcurve.PrivateKey;
import com.starkbank.ellipticcurve.PublicKey;
import com.starkbank.ellipticcurve.Signature;
import com.starkbank.ellipticcurve.utils.ByteString;

import org.spongycastle.jcajce.provider.asymmetric.util.EC5Util;
import org.spongycastle.jce.ECNamedCurveTable;
import org.spongycastle.jce.ECPointUtil;
import org.spongycastle.jce.interfaces.ECPublicKey;
import org.spongycastle.jce.provider.BouncyCastleProvider;
import org.spongycastle.jce.spec.ECNamedCurveParameterSpec;
import org.spongycastle.jce.spec.ECNamedCurveSpec;
import org.spongycastle.jce.spec.ECParameterSpec;
import org.spongycastle.math.ec.ECCurve;

import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.NoSuchAlgorithmException;
import java.security.Provider;
import java.security.SecureRandom;
import java.security.Security;
import java.security.spec.ECPoint;
import java.security.spec.ECPublicKeySpec;
import java.security.spec.EllipticCurve;
import java.security.spec.InvalidKeySpecException;
import java.util.Base64;

import javax.crypto.KeyAgreement;

import com.eic.sskwb55.utils.BleDataUtils;
import com.eic.sskwb55.utils.CommonFileUtils;

import static com.eic.sskwb55.utils.BleDataUtils.bytesToHexStringForLog;

public class MainActivity extends Activity {

    private static String TAG = MainActivity.class.getSimpleName();

    static {
        Security.insertProviderAt(new BouncyCastleProvider(), 1);
    }


    @Override
    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        */
/*findViewById(R.id.btnClick).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                //genrateSecureRandom();
                //genrateSign();
                //generateSigning();
                //verifyUsingPemWorkingDontTouch();
                //verifyRecivedFromDevice();
                test();
                //createOwnCerti();
                //verifyPublicKeyInParts();


            }
        });*//*


        findViewById(R.id.btnGenrateSecreat).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                test();

            }
        });


    }


    private void createOwnCerti() {
        try {
            byte[] message = genrateSecureRandom();
            Log.d("Tag ", "Message size " + message.length + "\n" + bytesToHexStringForLog(message));
            // Generate Keys
            PrivateKey privateKey = new PrivateKey();
            PublicKey publicKey = privateKey.publicKey();
            Signature signature = Ecdsa.sign(message, privateKey);
            Log.d("Tag ", "publicKey Size " + publicKey.toByteString().getBytes().length + "\n" + bytesToHexStringForLog(publicKey.toByteString().getBytes()));

            // Verify if signature is valid
            boolean verified = Ecdsa.verify(message, signature, publicKey);
            Log.d("Tag ", "signature " + signature.toBase64() + "Size" + convertBase64ToHex(signature.toBase64()).length + "  bytes \n" + bytesToHexStringForLog(convertBase64ToHex(signature.toBase64())));
            Log.d("Tag", "Verified \n" + verified);

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    byte[] convertBase64ToHex(String text) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            return Base64.getDecoder().decode(text.getBytes(StandardCharsets.UTF_8));
        }
        return new byte[0];
    }

    private int[] convertToSignedBytes(byte[] signed) {
        int[] unsigned = new int[signed.length];
        for (int i = 0; i < signed.length; i++) {
            unsigned[i] = signed[i] & 0xFF;
        }
        return unsigned;
    }

    private void verifyRecivedFromDevice() {
        try {
            byte[] message = CommonFileUtils.getFileBytes("message.txt");
            byte[] signatureBin = CommonFileUtils.getFileBytes("signatureBinary.txt");
            byte[] publicKeyPem = CommonFileUtils.getFileBytes("publicKey.der");


            ByteString byteString = new ByteString(signatureBin);
            ByteString byteStringDerFile = new ByteString(publicKeyPem);

            PublicKey publicKey = PublicKey.fromDer(byteStringDerFile);
            Signature signature = Signature.fromDer(byteString);

            // Get verification status:
            boolean verified = Ecdsa.verify(message, signature, publicKey);
            System.out.println("Verification status: " + verified);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    static {
        Security.insertProviderAt(new org.spongycastle.jce.provider.BouncyCastleProvider(), 1);
    }

    */
/*public static KeyPair generate() {
        try {
            ECParameterSpec ecSpec = ECNamedCurveTable.getParameterSpec("prime256v1");
            KeyPairGenerator generator = KeyPairGenerator.getInstance("ECDSA", "SC");
            generator.initialize(ecSpec, new SecureRandom());
            KeyPair keyPair = generator.generateKeyPair();
            Log.i(TAG, "EC Pub Key generated: " + utils.bytesToHex(keyPair.getPublic().getEncoded()));
            Log.i(TAG, "EC Private Key generated: " + utils.bytesToHex(keyPair.getPrivate().getEncoded()));
            return generator.generateKeyPair();
        } catch (Exception e) {
            e.printStackTrace();
        }

    }*//*


    private void test() {
        try {
            byte[] message = CommonFileUtils.getFileBytes("message_new.txt");
            byte[] signatureBin = CommonFileUtils.getFileBytes("signatureBinary_new.txt");
            byte[] publicKeyPem = CommonFileUtils.getFileBytes("ecpubkey.der");

            ByteString byteString = new ByteString(signatureBin);
            ByteString byteStringDerFile = new ByteString(getPublicKeyFromHex(publicKeyPem));

            PublicKey publicKey = PublicKey.fromDer(byteStringDerFile);
            Signature signature = Signature.fromDer(byteString);

            // Get verification status:
            boolean verified = Ecdsa.verify(message, signature, publicKey);
            //System.out.println("Verification status: " + verified);
            Log.d("TAG", "Verification status: " + verified);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }


    private ECPublicKey getPublicKeyFromBytes(byte[] pubKey) throws NoSuchAlgorithmException, InvalidKeySpecException {

        ECNamedCurveParameterSpec spec = ECNamedCurveTable.getParameterSpec("prime256v1");
        KeyFactory kf = KeyFactory.getInstance("ECDSA", new BouncyCastleProvider());
        ECNamedCurveSpec params = new ECNamedCurveSpec("prime256v1", spec.getCurve(), spec.getG(), spec.getN());
        ECPoint point = ECPointUtil.decodePoint(params.getCurve(), pubKey);
        ECPublicKeySpec pubKeySpec = new ECPublicKeySpec(point, params);
        ECPublicKey pk = (ECPublicKey) kf.generatePublic(pubKeySpec);
        return pk;
    }


    public byte[] getPublicKeyFromHex(byte[] rawPublicKey) {
        try {

            ECPublicKey ecPublicKey = null;
            KeyFactory kf = null;
            ECNamedCurveParameterSpec ecNamedCurveParameterSpec = ECNamedCurveTable.getParameterSpec("prime256v1");
            ECCurve curve = ecNamedCurveParameterSpec.getCurve();
            EllipticCurve ellipticCurve = EC5Util.convertCurve(curve, ecNamedCurveParameterSpec.getSeed());
            java.security.spec.ECPoint ecPoint = ECPointUtil.decodePoint(ellipticCurve, rawPublicKey);
            java.security.spec.ECParameterSpec ecParameterSpec = EC5Util.convertSpec(ellipticCurve,
                    ecNamedCurveParameterSpec);
            java.security.spec.ECPublicKeySpec publicKeySpec = new java.security.spec.ECPublicKeySpec(ecPoint,
                    ecParameterSpec);

            kf = KeyFactory.getInstance("ECDSA", new BouncyCastleProvider());

            ecPublicKey = (ECPublicKey) kf.generatePublic(publicKeySpec);
            return ecPublicKey.toString().getBytes();

        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;


    }

    private void verifyPublicKeyInParts() {
        try {
            byte[] message = CommonFileUtils.getFileBytes("message.txt");
            byte[] signatureBin = CommonFileUtils.getFileBytes("signatureBinary.txt");
            ByteBuffer bb = ByteBuffer.allocate(BleDataUtils.publicKerHeader().length + BleDataUtils.publicKery().length);
            bb.put(BleDataUtils.publicKerHeader());
            bb.put(BleDataUtils.publicKery());
            byte[] result = bb.array();
            ByteString byteString = new ByteString(signatureBin);
            ByteString byteStringDerFile = new ByteString(result);

            PublicKey publicKey = PublicKey.fromDer(byteStringDerFile);
            Signature signature = Signature.fromDer(byteString);

            // Get verification status:
            boolean verified = Ecdsa.verify(message, signature, publicKey);
            System.out.println("Verification status: " + verified);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void verifyUsingPemWorkingDontTouch() {
        try {
            byte[] message = CommonFileUtils.getFileBytes("message.txt");
            byte[] signatureBin = CommonFileUtils.getFileBytes("signatureBinary.txt");
            byte[] publicKeyPem = CommonFileUtils.getFileBytes("publicKey.pem");

            ByteString byteString = new ByteString(signatureBin);

            PublicKey publicKey = PublicKey.fromPem(new String(publicKeyPem));
            Signature signature = Signature.fromDer(byteString);

            // Get verification status:
            boolean verified = Ecdsa.verify(message, signature, publicKey);
            System.out.println("Verification status: " + verified);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }


    private void generateSigning() {
        //https://github.com/starkbank/ecdsa-java
        com.starkbank.ellipticcurve.PrivateKey privateKey = new com.starkbank.ellipticcurve.PrivateKey();
        PublicKey publicKey = privateKey.publicKey();

        String message = "Testing message";
        // Generate Signature
        com.starkbank.ellipticcurve.Signature signature = Ecdsa.sign(message, privateKey);

        // Verify if signature is valid
        boolean verified = Ecdsa.verify(message, signature, publicKey);

        // Return the signature verification status
        Log.d("TAG", "Valid " + verified);
    }


    private byte[] genrateSecureRandom() {
        SecureRandom random = new SecureRandom();
        byte[] bytes = new byte[16];
        random.nextBytes(bytes);
        return random.generateSeed(16);
    }

    private static final boolean VDBG = false;


}


//Refered links
//https://github.com/google/wycheproof - Current*/
