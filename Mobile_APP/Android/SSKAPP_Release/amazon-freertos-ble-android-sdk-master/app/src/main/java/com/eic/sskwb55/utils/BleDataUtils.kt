package com.eic.sskwb55.utils

import android.util.Log
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer
import java.security.KeyPair
import java.security.KeyPairGenerator
import java.security.spec.ECGenParameterSpec
import java.util.*

open class BleDataUtils {
    companion object {
        @JvmStatic
        fun hexToByteArray(hex: String): ByteArray {
            var hex = hex
            hex = if (hex.length % 2 != 0) "0$hex" else hex
            val b = ByteArray(hex.length / 2)
            for (i in b.indices) {
                val index = i * 2
                val v = hex.substring(index, index + 2).toInt(16)
                b[i] = v.toByte()
            }
            return b
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

        fun generateDataFrame(data: ByteArray): ByteArray {
            val actualDataInHex = BleCommandTransfer.byteArrayToHex(data)
            val totalDataLength = data.size + 4
            val MSBLSBLength = BleCommandTransfer.integerToHex(totalDataLength, 4)
            val crc16 = BleCommandTransfer.integerToHex(BleCommandTransfer.calculateCRC16(data, data.size).toInt(), 4)
            val dataFrame = MSBLSBLength + actualDataInHex + crc16
            Log.d(BleCommandTransfer.TAG, "All data frame $dataFrame")
            return BleCommandTransfer.hexToByteArray(dataFrame)
        }

        @JvmStatic
        fun bytesToHexString(bytes: ByteArray): String? {
            val sb = StringBuilder(bytes.size * 2)
            val formatter = Formatter(sb)
            for (i in bytes.indices) {
                formatter.format("%02x", bytes[i])
                /*if (!VDBG && i > 10) {
                    break;
                }*/
            }
            return sb.toString()
        }

        @JvmStatic
        fun bytesToHexStringForLog(bytes: ByteArray): String? {
            val sb = StringBuilder(bytes.size * 2)
            val formatter = Formatter(sb)
            for (i in bytes.indices) {
                formatter.format("$i           %02x \n", bytes[i])
                /*if (!VDBG && i > 10) {
                    break;
                }*/
            }
            return sb.toString()
        }


        @JvmStatic
        fun getDataBytes(): ByteArray {
            return hexToByteArray("01020304050607080901020304050607")
        }

        @JvmStatic
        fun getPublicKey(): ByteArray {
            return hexToByteArray("04a1d3f9c1abc6e816866cb16027d9d03040c853d582b76a42fa369b10389977571e23babe5bf050fe0649a8820e7abc53c481861a517ac08e9124b23569ce5ba0")
        }

        @JvmStatic
        fun getPublicKeyHeader(): ByteArray {
            return hexToByteArray("3059301306072a8648ce3d020106082a8648ce3d030107034200")
        }

        @JvmStatic
        fun getFullPublicKey(): ByteArray {
            //return hexToByteArray("3059301306072a8648ce3d020106082a8648ce3d03010703420004a1d3f9c1abc6e816866cb16027d9d03040c853d582b76a42fa369b10389977571e23babe5bf050fe0649a8820e7abc53c481861a517ac08e9124b23569ce5ba0")
          return hexToByteArray("3059301306072a8648ce3d020106082a8648ce3d0301070342000460ce6a8eaabb25a0035a5e2ee0929a843e7b424e51e334b0b5148363722d319e7ac69baffbf864c44bf42e4a1a4c75927f3e6e5268020cb59347dfbebf6aa8c6")
        }


        @JvmStatic
        fun getSignature(): ByteArray {
            return hexToByteArray("022100b2998ca8c66bc2683eb83012f925b934313423e57e5342377623306dc2d163cc022061bca12c387b1264e8b260e176a6cdf0c9b854507ac0507866f1294f55c9f312")
        }

        @JvmStatic
        fun getFullSignature(): ByteArray {
            return hexToByteArray("3046304402205b545edc6996f62fd7404a4794dd93e0e028fc8da3e8ad3c593ba46f936da96f02204d0d188356f1e0fdaf7e7ee136f3a5cb2cbf47ac042dddb784eee42d3847f5fc")
        }

        @JvmStatic
        fun demoCode1(): ByteArray {
            return byteArrayOf(0x01.toByte(), 0x02.toByte(), 0x03.toByte(), 0x04.toByte(), 0x05.toByte(), 0x06.toByte(), 0x07.toByte(), 0x08.toByte(), 0x09.toByte(), 0x01.toByte(), 0x02.toByte(), 0x03.toByte(), 0x04.toByte(), 0x05.toByte(), 0x06.toByte(), 0x07.toByte(), 0x08.toByte(), 0x09.toByte())

        }

        @JvmStatic
        fun demoCode(): ByteArray {
            return byteArrayOf(0x01.toByte(), 0x02.toByte(), 0x03.toByte(), 0x04.toByte(), 0x01.toByte(), 0x02.toByte(), 0x03.toByte(), 0x04.toByte(), 0x05.toByte(), 0x06.toByte(), 0x07.toByte(), 0x08.toByte(), 0x09.toByte(), 0x01.toByte(), 0x02.toByte(), 0x03.toByte(), 0x04.toByte(), 0x05.toByte(), 0x06.toByte(), 0x07.toByte(), 0x08.toByte(), 0x09.toByte(), 0x01.toByte(), 0x02.toByte(), 0x03.toByte(), 0x04.toByte(), 0x05.toByte(), 0x06.toByte(), 0x07.toByte(), 0x08.toByte(), 0x09.toByte(), 0x01.toByte(), 0x02.toByte(), 0x03.toByte(), 0x04.toByte(), 0x05.toByte(), 0x06.toByte(), 0x07.toByte(), 0x08.toByte(), 0x09.toByte())

        }


        open fun getKeyPair(): KeyPair? {
            val generator = KeyPairGenerator.getInstance("EC")
            generator.initialize(ECGenParameterSpec("prime256v1"))
            val keyPair = generator.generateKeyPair()
            return keyPair
        }

    }

}



