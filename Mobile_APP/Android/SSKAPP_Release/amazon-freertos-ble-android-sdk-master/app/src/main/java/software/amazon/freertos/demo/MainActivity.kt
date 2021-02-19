package software.amazon.freertos.demo

import android.app.Activity
import android.content.pm.PackageManager
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.Toast
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer
import software.amazon.freertos.amazonfreertossdk.CalculateCRC16
import software.amazon.freertos.demo.utils.AES
import software.amazon.freertos.demo.utils.BleDataUtils.Companion.bytesToHexString
import software.amazon.freertos.demo.utils.BleDataUtils.Companion.bytesToHexStringForLog
import software.amazon.freertos.demo.utils.BleDataUtils.Companion.getDataBytes
import software.amazon.freertos.demo.utils.BleDataUtils.Companion.getFullPublicKey
import software.amazon.freertos.demo.utils.BleDataUtils.Companion.getFullSignature
import software.amazon.freertos.demo.utils.BleDataUtils.Companion.hexToByteArray
import software.amazon.freertos.demo.utils.CommonFileUtils.getFileBytes
import java.security.*
import java.security.spec.ECGenParameterSpec
import java.security.spec.InvalidKeySpecException
import java.security.spec.X509EncodedKeySpec
import java.util.*
import javax.crypto.KeyAgreement
import kotlin.collections.ArrayList


class MainActivity : Activity() {
    var a = 0
    var publicKeyPass = 0
    var devicePrivateKey = ArrayList<String>()
    var totalDataSize = 0
    var receivedData = 2
    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.main)

        var array = ArrayList<String>()
        array.add("004504530d7a4d69a3e80fdcdb96e7eb23e2f9d2")
        array.add("391e7cded79480307663e2ac6aa3eaff55a8b8bd")
        array.add("cb5ddcbd97f2314b0520cebf4c99000ab71e80c0")
        array.add("e1af9ffa9632b930b8cebf4c99000ab71e80c0CO")
        array.add("########################################")
        findViewById<View>(R.id.btnGenrateSecreat).setOnClickListener {

            /*if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                try {
                    ActivityCompat.requestPermissions(this@MainActivity, arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE), 200)
                } catch (e: Exception) {
                    e.printStackTrace()
                    throw e
                }
            }*/


            //test();`
        }
        findViewById<View>(R.id.btnClick).setOnClickListener {
            ///###############Working
            /*  sendDataToBle();
                        retriveCertificateFromBle();
                        genrateECDH(BleDataUtils.hexToByteArray("3059301306072a8648ce3d020106082a8648ce3d030107034200042424022fd6d389509c56a5100f29f5dc0faa8b7d9fdfdc2a9662407221b386ea68095189d7cc24d3b9b6539c183b089b8aa6ff399f583a0eecd5783ab68c06e7"));*/
            //GenerateMyClientCertificate();
            //Log.d("tag", "calculateCrc16 " + abc(BleDataUtils.demoCode(), BleDataUtils.demoCode().length));
            //retriveCertificateFromBle()
            //genrateECDH(BleDataUtils.hexToByteArray("3059301306072a8648ce3d020106082a8648ce3d030107034200042424022fd6d389509c56a5100f29f5dc0faa8b7d9fdfdc2a9662407221b386ea68095189d7cc24d3b9b6539c183b089b8aa6ff399f583a0eecd5783ab68c06e7"));
            //genrateECDH1()
            //sendSecretInFrame("0a29ad4cb98ae47d809bfcd91a1c27dc1a06fcf7f1db7c497b79d7dc19d7fdb4")
            AES().performAES()

          /*  val input = "0102"
            var byteData = hexToByteArray(input)

            Log.d(TAG, "Hex " + integerToHex(CalculateCRC16.CRC16CCITT(byteData), 4))*/
        }
    }

    private fun sendSecretInFrame(convertSecretToSHA256: String?) {
        var secretFrame = BleCommandTransfer.generateDataFrameInHEX(BleCommandTransfer.hexToByteArray(convertSecretToSHA256!!))
        var secretDataChunks = BleCommandTransfer.convertHexToChunk(secretFrame, 2)
        var firstPassData = secretDataChunks.subList(0, 20)
        var secondPassData = secretDataChunks.subList(20, secretDataChunks.size)
        Log.d(TAG, "First pass data ${removeArrayData(firstPassData)} \n" + "Second pass data ${removeArrayData(secondPassData)}")
    }

    private fun emmitData(publicKeyData: String) {
        val hexData = BleCommandTransfer.convertHexToChunk(publicKeyData, 2)
        if (publicKeyPass != -1) {
            if (publicKeyPass == 0) { //First time
                publicKeyPass++
                receivedData += hexData.size
                val length = BleCommandTransfer.hexToInteger(hexData[0] + hexData[1]) //100
                totalDataSize = length - 2   //100-2 = 98
                if (hexData.size >= 20) {
                    devicePrivateKey.addAll(hexData.subList(2, hexData.size))
                    Log.d(TAG, "Device public key $devicePrivateKey")
                } else { //Data complete
                    publicKeyPass = -1
                }
            } else { //Repeat until data arrives
                receivedData += hexData.size
                devicePrivateKey.addAll(hexData)
                if (receivedData > totalDataSize) {
                    publicKeyPass = -1
                    var abc = devicePrivateKey.subList(0, totalDataSize)
                    Log.d(TAG, "Device public key final $abc")

                }
                Log.d(TAG, "Device public key $devicePrivateKey")
            }

        }

    }

    private fun removeArrayData(publicKeyData: List<String>): String {
        var removeChars = publicKeyData.toString().replace("[", "").replace("]", "").replace(",", "").replace(" ", "")
        return removeChars
    }

    var startLimit = 0
    var endLimit = 19
    var remainingLength = 0

    fun calculateData() {
        /* startLimit = 0
         endLimit = 20
         val mainFrame = BleDataUtils.demoCode()
         val completeFrame = BleDataUtils.generateDataFrame(mainFrame)
         remainingLength = completeFrame.size
         if (completeFrame.size == 20) {
             Log.d(TAG, "calculateData Process all $remainingLength")
         } else {
             while (remainingLength > 0) {
                 Log.d(TAG, "calculateData start $remainingLength")
                 getChunk(completeFrame)
             }
         }*/

        //    -23713


    }

    fun getChunk(completeData: ByteArray) {
        if (remainingLength >= 20) {
            Log.d(TAG, "calculateData start $startLimit end $endLimit")
            val data = Arrays.copyOfRange(completeData, startLimit, endLimit)
            Log.d(TAG, "calculateData start $startLimit end $endLimit  data ${bytesToHexString(data)}")
            remainingLength = completeData.size - (endLimit)
            startLimit = endLimit
            endLimit = endLimit + 20
        } else {
            startLimit = completeData.size - remainingLength
            val data = Arrays.copyOfRange(completeData, startLimit, completeData.size)
            Log.d(TAG, "calculateData start $startLimit end $endLimit  data ${bytesToHexString(data)}")
            remainingLength = 0
        }


    }


    fun crc16_2(data_p: ByteArray): Short {
        var i: Char
        var data: Int
        var crc = 0xffff
        var length = data_p.size
        if (data_p.size == 0) do {
            i = 0.toChar()
            data = 0xff and data_p[i.toInt()]++.toInt()
            while (i.toInt() < 8) {
                crc = if (crc and 0x0001 xor (data and 0x0001) == 1) crc shr 1 xor 0x8408 else crc shr 1
                i++
                data = data shr 1
            }
            length--
        } while (length != 0)
        crc = crc.inv()
        data = crc
        crc = crc shl 8 or (data shr 8 and 0xff)
        return crc.toShort()
    }

    fun integerToHex(data: Int, capacity: Int): String {
        val sb = StringBuilder()
        sb.append("%0")
        sb.append(capacity)
        sb.append("X")
        val objArr = arrayOf<Any>(Integer.valueOf(data))
        return String.format(sb.toString(), *Arrays.copyOf(objArr, objArr.size))
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == 200) {
            if (grantResults.size > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                // Permission granted.
                dataFromBleInFileFormat
            } else {
                // User refused to grant permission.
                Toast.makeText(this, "Grant permission", Toast.LENGTH_SHORT).show()
            }
        }
    }

    //########################################################################
    //########## WORKING CODE
    //########################################################################
    fun rertivePublicKey(publicKey: ByteArray?): PublicKey? {
        try {
            return KeyFactory.getInstance("EC").generatePublic(X509EncodedKeySpec(publicKey))
        } catch (e: InvalidKeySpecException) {
            e.printStackTrace()
        } catch (e: Exception) {
            e.printStackTrace()
        }
        return null
    }

    private fun retriveCertificateFromBle() { //Ble
        try {
            val message = getDataBytes()
            val signatureBin = getFullSignature() //Set for true
            val publicKeyPem = getFullPublicKey()
            val s = Signature.getInstance("SHA256withECDSA")
            s.initVerify(rertivePublicKey(publicKeyPem))
            s.update(message)
            val valid = s.verify(signatureBin)
            Log.d(TAG, "is valid $valid")
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun sendDataToBle() {
        try {
            val random = SecureRandom()
            val bytes = ByteArray(16)
            random.nextBytes(bytes)
            val generator = KeyPairGenerator.getInstance("EC")
            generator.initialize(ECGenParameterSpec("prime256v1"))
            val keyPair = generator.generateKeyPair()
            val privateKey1 = keyPair.private
            val publicKey = keyPair.public
            val signature = Signature.getInstance("SHA256withECDSA")
            signature.initSign(privateKey1)
            signature.update(bytes)
            val signatureValue = signature.sign()
            signature.initVerify(publicKey)
            signature.update(bytes) // <-- Here
            //Signature remove 2 bytes 30 45
            //Public key 0-25 fix
            val isVerified = signature.verify(signatureValue)
            Log.d(TAG, "Data Bytes " + bytesToHexStringForLog(bytes))
            Log.d(TAG, "Data Public key  " + bytesToHexStringForLog(publicKey.encoded))
            Log.d(TAG, "Data signatureValue  " + bytesToHexStringForLog(signatureValue))
            Log.d(TAG, "Data Verification $isVerified")
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun genrateECDH(bleECDHPublicKey: ByteArray) { //Ble ECDH public key
        //https://neilmadden.blog/2016/05/20/ephemeral-elliptic-curve-diffie-hellman-key-agreement-in-java/
        try {
            val kpg = KeyPairGenerator.getInstance("EC")
            kpg.initialize(256)
            val kp = kpg.generateKeyPair()
            val ourPk = kp.public.encoded //Send to ble
            Log.d(TAG, "Hex public key " + bytesToHexStringForLog(ourPk))
            // Read other's public key:
            val kf = KeyFactory.getInstance("EC")
            val pkSpec = X509EncodedKeySpec(bleECDHPublicKey)
            val otherPublicKey = kf.generatePublic(pkSpec)
            val ka = KeyAgreement.getInstance("ECDH") //Sign
            ka.init(kp.private)
            ka.doPhase(otherPublicKey, true)
            val sharedSecret = ka.generateSecret()
            Log.d(TAG, "Secret " + bytesToHexString(sharedSecret))
        } catch (e: Exception) {
            e.printStackTrace()
            Log.e(TAG, "Exception caught in ECDHInit function")
        }
    }//Set for true

    private fun genrateECDH1() { //Ble ECDH public key
        try {
            val kpg = KeyPairGenerator.getInstance("EC")
            kpg.initialize(256)
            val kp = kpg.generateKeyPair()
            val ka = KeyAgreement.getInstance("ECDH") //Sign
            ka.init(kp.private)
            kp.public
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }//Set for true

    //Ble
    private val dataFromBleInFileFormat: Unit
        private get() { //Ble
            try {
                val message = getFileBytes("message_new.txt")
                val signatureBin = getFileBytes("signatureBinary_new.txt") //Set for true
                val publicKeyPem = getFileBytes("ecpubkey.der")
                val s = Signature.getInstance("SHA256withECDSA")
                s.initVerify(rertivePublicKey(publicKeyPem))
                s.update(message)
                val valid = s.verify(signatureBin)
                Log.d(TAG, "is valid $valid")
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }

    companion object {
        private val TAG = MainActivity::class.java.simpleName


        const val POLY = 0x8408
        fun abc(dataP: ByteArray, length_U: Int): Short {
            var length_U = length_U
            var dataPIndex = 0
            var i_U: Byte
            var data_U: Int
            var crc_U = 0xffff
            if (length_U == 0) {
                return crc_U.inv().toShort()
            }
            do {
                i_U = 0
                data_U = 0xff and dataP[dataPIndex++].toInt()
                while (i_U < 8) {
                    crc_U = if (crc_U and 0x0001 xor data_U and 0x0001 != 0) {
                        crc_U ushr 1 xor POLY
                    } else {
                        crc_U ushr 1
                    }
                    i_U++
                    data_U = data_U ushr 1
                }
            } while (--length_U != 0)
            crc_U = crc_U.inv()
            data_U = crc_U
            crc_U = crc_U shl 8 or data_U ushr 8 and 0xff
            return crc_U.toShort()
        }


    }
}