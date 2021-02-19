package software.amazon.freertos.amazonfreertossdk

import android.util.Log
import java.security.SecureRandom
import java.util.*

open class BleCommandTransferDemo {

    companion object {
        var TAG = "BleCommandTransfer"
        fun writeRandomNumber(): ByteArray? {
            return byteArrayOf(0x03)
        }

        fun generateSecureRandom(): ByteArray? {
            val random = SecureRandom()
            val bytes = ByteArray(16)
            random.nextBytes(bytes)
            var randomNumber = random.generateSeed(16)
            Log.d("BleCommandTransfer", "Hex data " + byteArrayToHex(randomNumber))
            return randomNumber
        }

        fun indicateCertificate(): ByteArray? {
            return byteArrayOf(0x01)
        }

        fun generateCertificate(): ByteArray? {
            return byteArrayOf(0x03)
        }

        fun indicatePublicKey(): ByteArray? {
            return byteArrayOf(0x02)
        }

        fun generatePublicKey(): ByteArray? {
            return byteArrayOf(0x02)
        }


        fun indicateSignatureRandom(): ByteArray? {
            return byteArrayOf(0x04)
        }

        fun generateSignatureRandomNumber(): ByteArray? {
            return byteArrayOf(0x04)
        }

        fun indicateECDHPublicKey(): ByteArray? {
            return byteArrayOf(0x05)
        }

        fun generateECDHPublicKey(): ByteArray? {
            return byteArrayOf(0x05)
        }

        fun indicateECDHSignature(): ByteArray? {
            return byteArrayOf(0x06)
        }

        fun generateECDHSignature(): ByteArray? {
            return byteArrayOf(0x06)
        }

        fun generateDataFrame(data: ByteArray): ByteArray {
            val actualDataInHex = byteArrayToHex(data)
            Log.d(TAG, "All data frame $actualDataInHex")
            val totalDataLength = data.size + 4
            val MSBLSBLength = integerToHex(totalDataLength, 4)
            val crc16 = CalculateCRC16.crc16CCITTFalse(data).toInt()
            val crc16ToInt = integerToHex(crc16, 4)
            Log.d(TAG, "All data frame $crc16ToInt")
            val dataFrame = MSBLSBLength + actualDataInHex + crc16ToInt
            Log.d(TAG, "All data frame $dataFrame")
            return hexToByteArray(dataFrame)
        }

        fun calculateCRC16(dataP: ByteArray, length_U1: Int): Short {
            var dataLength = length_U1
            var dataPIndex = 0
            var i_U: Byte
            var data_U: Int
            var crc_U = 0xffff
            if (dataLength == 0) {
                return crc_U.inv().toShort()
            }
            do {
                i_U = 0
                data_U = 0xff and dataP[dataPIndex++].toInt()
                while (i_U < 8) {
                    crc_U = if (crc_U and 0x0001 xor data_U and 0x0001 != 0) {
                        crc_U ushr 1 xor 0x8408
                    } else {
                        crc_U ushr 1
                    }
                    i_U++
                    data_U = data_U ushr 1
                }
            } while (--dataLength != 0)
            crc_U = crc_U.inv()
            data_U = crc_U
            crc_U = crc_U shl 8 or data_U ushr 8 and 0xff
            return crc_U.toShort()
        }

        fun integerToHex(data: Int, capacity: Int): String {
            val sb = java.lang.StringBuilder()
            sb.append("%0")
            sb.append(capacity)
            sb.append("X")
            val objArr = arrayOf<Any>(Integer.valueOf(data))
            return String.format(sb.toString(), *Arrays.copyOf(objArr, objArr.size))
        }

        fun hexToInteger(data: String): Int {
            return Integer.parseInt(data, 16)

        }

        fun byteArrayToHex(bytes: ByteArray): String {
            val sb = StringBuilder(bytes.size * 2)
            val formatter = Formatter(sb)
            for (i in bytes.indices) {
                formatter.format("%02x", bytes.get(i))
            }
            return sb.toString()
        }

        fun convertHexToChunk(hex: String, chunk: Int): List<String> {
            return hex.chunked(chunk)
        }

        fun hexToByteArray(hex: String): ByteArray {
            var hexData = hex
            hexData = if (hexData.length % 2 != 0) "0$hexData" else hexData
            val b = ByteArray(hexData.length / 2)
            for (i in b.indices) {
                val index = i * 2
                val v = hexData.substring(index, index + 2).toInt(16)
                b[i] = v.toByte()
            }
            return b
        }

    }
}

//Frame stracture
/*
3059301306072a8648ce3d020106082a8648ce3d030107034200042424022fd6d389509c56a5100f29f5dc0faa8b7d9fdfdc2a9662407221b386ea68095189d7cc24d3b9b6539c183b089b8aa6ff399f583a0eecd5783ab68c06e7

var a = data.size + 4 = Frame ni length = convert data 20 + 4 = convert 24 to hex 2 byte  00 24

var b = data ni crc16 2 byte [] []

2 byte length
{lebg}{dat}+crc16 [] [] 2 byte '


frame = a + data + crc16

Receive
frame = a + data + crc16 > 20 */



