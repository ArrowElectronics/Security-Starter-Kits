/*
 * Copyright 2010-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
package software.amazon.freertos.amazonfreertossdk

import android.bluetooth.*
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Handler
import android.os.HandlerThread
import android.os.Looper
import android.os.Looper.*
import android.util.Log
import android.widget.Toast
import com.amazonaws.auth.AWSCredentialsProvider
import com.amazonaws.mobileconnectors.iot.AWSIotMqttClientStatusCallback
import com.amazonaws.mobileconnectors.iot.AWSIotMqttClientStatusCallback.AWSIotMqttClientStatus
import com.amazonaws.mobileconnectors.iot.AWSIotMqttManager
import com.amazonaws.mobileconnectors.iot.AWSIotMqttMessageDeliveryCallback
import com.amazonaws.mobileconnectors.iot.AWSIotMqttMessageDeliveryCallback.MessageDeliveryStatus
import com.amazonaws.mobileconnectors.iot.AWSIotMqttQos
import lombok.Getter
import org.bouncycastle.util.encoders.DecoderException
import software.amazon.freertos.amazonfreertossdk.AmazonFreeRTOSConstants.*
import software.amazon.freertos.amazonfreertossdk.BleCommand.CommandType.READ_CHARACTERISTIC
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.byteArrayToHex
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.convertHexToChunk
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.convertSecretToSHA256
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.ecdhSessionKeyGenrated
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.ecdhSignatureVerificationFail
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.ecdhSignatureVerificationPass
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.gatewaySecreMatch
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.gatewaySecretFail
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.generateDataFrame
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.generateDataFrameInHEX
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.generateSecureRandom
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.getPublicKeyHeaderInHEX
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.hexToByteArray
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.hexToInteger
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.indicateCertificate
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.indicateECDHPublicKey
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.indicateECDHSignature
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.indicatePublicKey
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.indicateSignatureRandom
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.integerToHex
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.signatureVerificationFail
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.signatureVerificationPass
import software.amazon.freertos.amazonfreertossdk.BleCommandTransfer.Companion.writeRandomNumber
import software.amazon.freertos.amazonfreertossdk.deviceinfo.BrokerEndpoint
import software.amazon.freertos.amazonfreertossdk.deviceinfo.Mtu
import software.amazon.freertos.amazonfreertossdk.deviceinfo.Version
import software.amazon.freertos.amazonfreertossdk.mqttproxy.*
import software.amazon.freertos.amazonfreertossdk.networkconfig.*
import java.io.*
import java.nio.ByteBuffer
import java.nio.charset.StandardCharsets
import java.security.*
import java.security.cert.Certificate
import java.security.cert.CertificateFactory
import java.security.spec.ECGenParameterSpec
import java.security.spec.InvalidKeySpecException
import java.security.spec.PKCS8EncodedKeySpec
import java.security.spec.X509EncodedKeySpec
import java.util.*
import java.util.concurrent.Semaphore
import javax.crypto.Cipher
import javax.crypto.KeyAgreement
import javax.crypto.spec.IvParameterSpec
import javax.crypto.spec.SecretKeySpec
import javax.security.cert.CertificateException

class AmazonFreeRTOSDevice private constructor(@field:Getter private val mBluetoothDevice: BluetoothDevice, private val mContext: Context, private val mAWSCredential: AWSCredentialsProvider?, private val mKeystore: KeyStore?) {

    private var mBluetoothGatt: BluetoothGatt? = null
    private var mBleConnectionStatusCallback: BleConnectionStatusCallback? = null
    private var mNetworkConfigCallback: NetworkConfigCallback? = null
    private var mDeviceInfoCallback: DeviceInfoCallback? = null
    private var mBleConnectionState = BleConnectionState.BLE_DISCONNECTED
    private var mAmazonFreeRTOSLibVersion = "NA"
    private var mAmazonFreeRTOSDeviceType = "NA"
    private var mAmazonFreeRTOSDeviceId = "NA"
    private var mGattAutoReconnect = false
    private var mBondStateCallback: BroadcastReceiver? = null
    private var mMtu = 0
    private var rr = false
    private val mMqttQueue: Queue<BleCommand?> = LinkedList()
    private val mNetworkQueue: Queue<BleCommand?> = LinkedList()
    private val mIncomingQueue: Queue<BleCommand> = LinkedList()
    private var mBleOperationInProgress = false
    private var mRWinProgress = false
    private var mHandler: Handler? = null
    private var mHandlerThread: HandlerThread? = null
    private var mValueWritten: ByteArray? = null
    private val mutex = Semaphore(1)
    private val mIncomingMutex = Semaphore(1)

    //Buffer for receiving messages from device
    private val mTxLargeObject = ByteArrayOutputStream()
    private val mTxLargeNw = ByteArrayOutputStream()


    private var mIotMqttManager: AWSIotMqttManager? = null
    private var mMqttConnectionState = MqttConnectionState.MQTT_Disconnected
    private var deviceCertificate = ""
    private var devicePublicKey = ""
    private var bleECDHPublicKey = ""
    private var deviceRandomNumber = ""
    private var gateWayStatus = ""
    private var signatureFromBle = ""
    private var ecdhSignatureFromBle = ""
    var gatewayECDHSecreat = ""
    var bleECDHSecreat = ""

    var devicePrivateKey = ArrayList<String>()
    var deviceECDHPrivateKey = ArrayList<String>()
    var dataToSendInChunks = ArrayList<String>()

    private var currentParam = -1
    private var publicKeyPass = -1
    private var totalDataSize = 0
    private var receivedData = 2
    private var startLimit = 0
    private var endLimit = 19
    private var remainingLength = 0
    private var secreatReadCounter = -1

    //Buffer for sending messages to device.
    private var mTotalPackets = 0
    private var mPacketCount = 1
    private var mMessageId = 0
    private var mMaxPayloadLen = 0

    var privateKeyObj: PrivateKey? = null
    var publicKeyObj: PublicKey? = null
    var gateWayECDHPrivateKey: PrivateKey? = null

    var gatewayRandom: ByteArray? = null //Random number genrated by device without frame
    var gatewaySignature: ByteArray? = null
    var gatewayECDHSignature: ByteArray? = null
    var gateWayECDHPublicKey: ByteArray? = null
    var devicePublicKeyByteArray: ByteArray? = null
    var bleECDHPublicKeyByteArray: ByteArray? = null
    var gatewayECDHSecreatWithoutSHA: ByteArray? = null


    /****
     * Current param value - Current param variable shows current process ie current param is 1 > Certificate operations
     * in progress
     *  1-> Certificate indication
     *  2 -> Public key
     *  3 -> Random number
     *  4 -> Signature
     *  5 ->
     */

    /**
     * Construct an AmazonFreeRTOSDevice instance.
     *
     * @param context The app context. Should be passed in by the app that creates a new instance
     * of AmazonFreeRTOSDevice.
     * @param device  BluetoothDevice returned from BLE scan result.
     * @param cp      AWS credential for connection to AWS IoT. If null is passed in,
     * then it will not be able to do MQTT proxy over BLE as it cannot
     * connect to AWS IoT.
     */
    internal constructor(device: BluetoothDevice, context: Context, cp: AWSCredentialsProvider?) : this(device, context, cp, null)

    /**
     *
     * Construct an AmazonFreeRTOSDevice instance.
     *
     * @param context The app context. Should be passed in by the app that creates a new instance
     * of AmazonFreeRTOSDevice.
     * @param device  BluetoothDevice returned from BLE scan result.
     * @param ks      the KeyStore which contains the certificate used to connect to AWS IoT.
     */
    internal constructor(device: BluetoothDevice, context: Context, ks: KeyStore?) : this(device, context, null, ks)

    fun connect(connectionStatusCallback: BleConnectionStatusCallback, autoReconnect: Boolean) {
        mBleConnectionStatusCallback = connectionStatusCallback
        mHandlerThread = HandlerThread("BleCommandHandler")
        mHandlerThread!!.start()
        mHandler = Handler(mHandlerThread!!.looper)
        mGattAutoReconnect = autoReconnect
        mBluetoothGatt = mBluetoothDevice.connectGatt(mContext, false, mGattCallback, BluetoothDevice.TRANSPORT_LE)
    }

    /**
     * User initiated disconnect
     */
    fun disconnect() {
        if (mBluetoothGatt != null) {
            mGattAutoReconnect = false;
            mBluetoothGatt!!.disconnect();
        }
    }

    /**
     * Sends a ListNetworkReq command to the connected BLE device. The available WiFi networks found
     * by the connected BLE device will be returned in the callback as a ListNetworkResp. Each found
     * WiFi network should trigger the callback once. For example, if there are 10 available networks
     * found by the BLE device, this callback will be triggered 10 times, each containing one
     * ListNetworkResp that represents that WiFi network. In addition, the order of the callbacks will
     * be triggered as follows: the saved networks will be returned first, in decreasing order of their
     * preference, as denoted by their index. (The smallest non-negative index denotes the highest
     * preference, and is therefore returned first.) For example, the saved network with index 0 will
     * be returned first, then the saved network with index 1, then index 2, etc. After all saved
     * networks have been returned, the non-saved networks will be returned, in the decreasing order
     * of their RSSI value, a network with higher RSSI value will be returned before one with lower
     * RSSI value.
     *
     * @param listNetworkReq The ListNetwork request
     * @param callback       The callback which will be triggered once the BLE device sends a ListNetwork
     * response.
     */
    fun listNetworks(listNetworkReq: ListNetworkReq, callback: NetworkConfigCallback?) {
        mNetworkConfigCallback = callback
        val listNetworkReqBytes = listNetworkReq.encode()
        sendDataToDevice(UUID_NETWORK_SERVICE, UUID_NETWORK_RX, UUID_NETWORK_RXLARGE, listNetworkReqBytes)
    }

    /**
     * Sends a SaveNetworkReq command to the connected BLE device. The SaveNetworkReq contains the
     * network credential. A SaveNetworkResp will be sent by the BLE device and triggers the callback.
     * To get the updated order of all networks, call listNetworks again.
     *
     * @param saveNetworkReq The SaveNetwork request.
     * @param callback       The callback that is triggered once the BLE device sends a SaveNetwork response.
     */
    fun saveNetwork(saveNetworkReq: SaveNetworkReq, callback: NetworkConfigCallback?) {
        mNetworkConfigCallback = callback
        val saveNetworkReqBytes = saveNetworkReq.encode()
        sendDataToDevice(UUID_NETWORK_SERVICE, UUID_NETWORK_RX, UUID_NETWORK_RXLARGE, saveNetworkReqBytes)
    }

    /**
     * Sends an EditNetworkReq command to the connected BLE device. The EditNetwork request is used
     * to update the preference of a saved network. It contains the current index of the saved network
     * to be updated, and the desired new index of the save network to be updated to. Both the current
     * index and the new index must be one of those saved networks. Behavior is undefined if an index
     * of an unsaved network is provided in the EditNetworkReq.
     * To get the updated order of all networks, call listNetworks again.
     *
     * @param editNetworkReq The EditNetwork request.
     * @param callback       The callback that is triggered once the BLE device sends an EditNetwork response.
     */
    fun editNetwork(editNetworkReq: EditNetworkReq, callback: NetworkConfigCallback?) {
        mNetworkConfigCallback = callback
        val editNetworkReqBytes = editNetworkReq.encode()
        sendDataToDevice(UUID_NETWORK_SERVICE, UUID_NETWORK_RX, UUID_NETWORK_RXLARGE, editNetworkReqBytes)
    }

    /**
     * Sends a DeleteNetworkReq command to the connected BLE device. The saved network with the index
     * specified in the delete network request will be deleted, making it a non-saved network again.
     * To get the updated order of all networks, call listNetworks again.
     *
     * @param deleteNetworkReq The DeleteNetwork request.
     * @param callback         The callback that is triggered once the BLE device sends a DeleteNetwork response.
     */
    fun deleteNetwork(deleteNetworkReq: DeleteNetworkReq, callback: NetworkConfigCallback?) {
        mNetworkConfigCallback = callback
        val deleteNetworkReqBytes = deleteNetworkReq.encode()
        sendDataToDevice(UUID_NETWORK_SERVICE, UUID_NETWORK_RX, UUID_NETWORK_RXLARGE, deleteNetworkReqBytes)
    }

    /**
     * Get the current mtu value between device and Android phone. This method returns immediately.
     * The request to get mtu value is asynchronous through BLE command. The response will be delivered
     * through DeviceInfoCallback.
     *
     * @param callback The callback to notify app of current mtu value.
     */
    fun getMtu(callback: DeviceInfoCallback?) {
        mDeviceInfoCallback = callback
        if (!getMtu() && mDeviceInfoCallback != null) {
            mDeviceInfoCallback!!.onError(AmazonFreeRTOSError.BLE_DISCONNECTED_ERROR)
        }
    }

    /**
     * Get the current broker endpoint on the device. This broker endpoint is used to connect to AWS
     * IoT, hence, this is also the AWS IoT endpoint. This method returns immediately.
     * The request is sent asynchronously through BLE command. The response will be delivered
     * through DeviceInfoCallback.
     *
     * @param callback The callback to notify app of current broker endpoint on device.
     */
    fun getBrokerEndpoint(callback: DeviceInfoCallback?) {
        mDeviceInfoCallback = callback
        if (!brokerEndpoint && mDeviceInfoCallback != null) {
            mDeviceInfoCallback!!.onError(AmazonFreeRTOSError.BLE_DISCONNECTED_ERROR)
        }
    }

    /**
     * Get the AmazonFreeRTOS library software version running on the device. This method returns
     * immediately. The request is sent asynchronously through BLE command. The response will be
     * delivered through DeviceInfoCallback.
     *
     * @param callback The callback to notify app of current software version.
     */
    fun getDeviceVersion(callback: DeviceInfoCallback?) {
        mDeviceInfoCallback = callback
        if (!getDeviceVersion() && mDeviceInfoCallback != null) {
            mDeviceInfoCallback!!.onError(AmazonFreeRTOSError.BLE_DISCONNECTED_ERROR)
        }
    }

    /**
     * Try to read a characteristic from the Gatt service. If pairing is enabled, it will be triggered
     * by this action.
     */
    private fun probe() {
        getDeviceVersion()
    }

    /**
     * Initialize the Gatt services
     */
    private fun initialize() {
        //startMQTTProcess()
        resetVariables()
        //createECDSAKeyPairs()
        GetPrivatePublicKey("AndroidAppCert.der","AndroidApp_private.ecc.pem")
        generateECDHPublicKey()
        startMutualAuthenticationProcess()

    }

    var isStart = true
    private fun startMutualAuthenticationProcess() {
        if (isStart) {
            Handler(getMainLooper()).post {
                Toast.makeText(mContext, "Mutual authentication start", Toast.LENGTH_LONG).show()
            }

            sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_DESCRIPTOR, UUID_CHAR_GATE_WAY_STATUS, MUTUAL_AUTH_SERVICE))
            currentParam = 1
            if (indicateParamValue(1)) { //Indication success for device certificate
                if (createAndSendParamValue(1)) {  //Write success for certificate
                    currentParam = 1
                    //readParamValue(1)
                }
            }
            /*currentParam = 2
            if (indicateParamValue(2)) { //Indication success for device certificate
                if (createAndSendParamValue(2)) {  //Write success for certificate
                    currentParam = 2
                    readParamValue(2)
                }
            }*/
        }
    }


    private fun createAndSendParamValue(number: Int): Boolean {
        return when (number) {
            1 -> {
                if (isBLEConnected && mBluetoothGatt != null) {
                    val certFrame = CertificateDataFromFile("AndroidAppCert.der")
                    if (certFrame != null) {

                        startLimit = 0
                        endLimit = 20
                        remainingLength = certFrame.size

                        val certificateFrame = generateDataFrame(certFrame)

                        //Log.e(TAG, "Send data full frame public key ${bytesToHexString(publicKeyFrame!!)} " + "Frame bytes ${getFrameLengthForLog(publicKeyFrame)}")
                        if (certFrame.size == 20) {
                            //Log.e(TAG, "calculateData Process all $remainingLength")
                            sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, certificateFrame))
                        } else {
                            while (remainingLength > 0) {
                                sendPublicKeyInChunks(certificateFrame)
                            }
                            if (remainingLength <= 0) { //Loop end
                                currentParam = 1
                                publicKeyPass = 0
                                receivedData = 2
                                totalDataSize = 0
                                readParamValue(1)
                            }
                        }
                        Log.e(TAG, "[Mutual Auth] : Send Certificate")
                    }
                    // sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, generateCertificate()))
                    true
                } else {
                    Log.e(TAG, "Bluetooth is not connected.")
                    false
                }
            }
            2 -> {
                if (isBLEConnected && mBluetoothGatt != null) {

                    val publicKeyHeader = bytesToHexString(publicKeyObj!!.encoded!!)
                    val publicKeyChunks = convertHexToChunk(publicKeyHeader, 2)
                    val removeData = publicKeyChunks.subList(26, publicKeyChunks.size)
                    val publicKeyFrame = BleCommandTransfer.generateDataFrame(hexToByteArray(removeArrayData(removeData)))
                    startLimit = 0
                    endLimit = 20
                    remainingLength = publicKeyFrame.size
                    //Log.e(TAG, "Send data full frame public key ${bytesToHexString(publicKeyFrame!!)} " + "Frame bytes ${getFrameLengthForLog(publicKeyFrame)}")
                    if (publicKeyFrame.size == 20) {
                        //Log.e(TAG, "calculateData Process all $remainingLength")
                        sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, publicKeyFrame))
                    } else {
                        while (remainingLength > 0) {
                            sendPublicKeyInChunks(publicKeyFrame)
                        }
                    }
                    Log.e(TAG, "[Mutual Auth] : Send public key")
                    true
                } else {
                    Log.e(TAG, "Bluetooth is not connected.")
                    false
                }
            }
            3 -> {
                if (isBLEConnected && mBluetoothGatt != null) {
                    gatewayRandom = generateSecureRandom()
                    val randomNumberFrame = BleCommandTransfer.generateDataFrame(gatewayRandom!!)
                    //val randomNumberFrame = hexToByteArray("0014010203040506070809010203040506077ea7")
                    //Log.e(TAG, "Send data full frame message " + "0014010203040506070809010203040506077ea7")
                    //Log.e(TAG, "Send data full frame message " + BleCommandTransfer.generateDataFrameInHEX(messageBytes!!))
                    sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, randomNumberFrame))
                    Log.e(TAG, "[Mutual Auth] : Send random number")

                    true
                } else {
                    Log.e(TAG, "Bluetooth is not connected.")
                    false
                }
            }
            4 -> {
                if (isBLEConnected && mBluetoothGatt != null) {
                    //Send signature
                    val signatureData = BleCommandTransfer.byteArrayToHex(gatewaySignature!!)
                    //Log.e(TAG, "Send signature value  $signatureData ")
                    var signatureInChunk = BleCommandTransfer.convertHexToChunk(signatureData, 2)
                    signatureInChunk = signatureInChunk.subList(2, signatureInChunk.size) //Remove signature header
                    val signatureValueInHex = removeArrayData(signatureInChunk)
                    val signatureFrame = BleCommandTransfer.generateDataFrameInHEX(hexToByteArray(signatureValueInHex))
                    //Log.e(TAG, "Send data full frame signature $signatureFrame Frame bytes ${getFrameLengthForLogFromHex(signatureFrame)}")
                    startLimit = 0
                    endLimit = 20
                    remainingLength = signatureInChunk.size
                    if (signatureInChunk.size == 20) {
                        //Log.e(TAG, "Send signature Process all $remainingLength")
                        sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, hexToByteArray(removeArrayData(signatureInChunk))))
                    } else {
                        while (remainingLength > 0) {
                            sendSignatureInChunk(BleCommandTransfer.convertHexToChunk(signatureFrame, 2))
                        }

                        if (remainingLength <= 0) { //Loop end
                            currentParam = 4
                            publicKeyPass = 0
                            receivedData = 2
                            totalDataSize = 0
                            readParamValue(4)
                        }
                    }
                    Log.e(TAG, "[Mutual Auth] : Send signature")

                    true
                } else {
                    Log.e(TAG, "Bluetooth is not connected.")
                    false
                }
            }
            5 -> {
                if (isBLEConnected && mBluetoothGatt != null) {
                    var ecdhPublicKeyWithHeader = gateWayECDHPublicKey
                    val chunks = convertHexToChunk(bytesToHexString(ecdhPublicKeyWithHeader!!), 2)
                    val data = chunks.subList(26, chunks.size)
                    var ecdhFrame = generateDataFrameInHEX(hexToByteArray(removeArrayData(data)))
                    var ecdhFrameChunks = convertHexToChunk(ecdhFrame, 2)
                    var ecdhByteArray = hexToByteArray(ecdhFrame)
                    //Log.e(TAG, "ECDH Public key full data ${removeArrayData(ecdhFrameChunks)}")
                    startLimit = 0
                    endLimit = 20
                    remainingLength = ecdhFrameChunks.size
                    if (ecdhFrameChunks.size == 20) {
                        sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, ecdhByteArray))
                    } else {
                        while (remainingLength > 0) {
                            sendECDHPublicKeyInChunks(ecdhByteArray)
                        }
                        if (remainingLength <= 0) { //Loop end
                            currentParam = 5
                            publicKeyPass = 0
                            receivedData = 2
                            totalDataSize = 0
                            readParamValue(5)
                        }
                    }
                    Log.e(TAG, "[Mutual Auth] : Send ECDH public key")

                    true
                } else {
                    Log.e(TAG, "Bluetooth is not connected.")
                    false
                }
            }
            6 -> {
                if (isBLEConnected && mBluetoothGatt != null) {
                    //Send signature
                    gatewayECDHSignature = generateECDHSignature()
                    val ecdhSignatureData = BleCommandTransfer.byteArrayToHex(gatewayECDHSignature!!)
                    //Log.e(TAG, "Send ecdh signature value  $ecdhSignatureData ")
                    var ecdhSignatureInChunk = BleCommandTransfer.convertHexToChunk(ecdhSignatureData, 2)
                    ecdhSignatureInChunk = ecdhSignatureInChunk.subList(2, ecdhSignatureInChunk.size) //Remove signature header
                    val signatureValueInHex = removeArrayData(ecdhSignatureInChunk)
                    val signatureFrame = BleCommandTransfer.generateDataFrameInHEX(hexToByteArray(signatureValueInHex))
                    //Log.e(TAG, "Send ecdh signature data full frame ecdh signature $signatureFrame Frame bytes ${getFrameLengthForLogFromHex(signatureFrame)}")
                    startLimit = 0
                    endLimit = 20
                    remainingLength = ecdhSignatureInChunk.size
                    if (ecdhSignatureInChunk.size == 20) {
                        //Log.e(TAG, "Send signature Process all $remainingLength")
                        sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, hexToByteArray(removeArrayData(ecdhSignatureInChunk))))
                    } else {
                        while (remainingLength > 0) {
                            sendECDHSignatureInChunk(BleCommandTransfer.convertHexToChunk(signatureFrame, 2))
                        }
                        if (remainingLength <= 0) { //Loop end
                            if (remainingLength <= 0) { //Loop end
                                currentParam = 6
                                publicKeyPass = 0
                                receivedData = 2
                                totalDataSize = 0
                                dataToSendInChunks.clear()
                                readParamValue(6)
                            }
                        }
                    }
                    Log.e(TAG, "[Mutual Auth] : Send ECDH signature")

                    true
                } else {
                    Log.e(TAG, "Bluetooth is not connected.")
                    false
                }
            }

            else -> false
        }
    }


    private fun sendPublicKeyInChunks(publicKeyFrame: ByteArray, startLength: Int, endLength: Int) {
        val copyArray = byteArrayOf(0x00)
        if (publicKeyFrame.size > 20) {
            for (i in startLength..endLength) {
                copyArray[i] = publicKeyFrame[i]
                if (publicKeyFrame.size > i) {
                    break
                }
            }
            sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, copyArray))
        }
    }

    private fun readParamValue(number: Int): Boolean { //Can be set in common don't remove parameter argument maybe usefull in future
        if (isBLEConnected && mBluetoothGatt != null) {
            publicKeyPass = 0
            sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_EDGE_PARAM_VALUE, MUTUAL_AUTH_SERVICE))
            return true
        } else {
            Log.e(TAG, "Bluetooth is not connected.")
            return false
        }
        /*return when (number) {
            1 -> {
                if (isBLEConnected && mBluetoothGatt != null) {
                    sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_EDGE_PARAM_VALUE, MUTUAL_AUTH_SERVICE))
                    true
                } else {
                    Log.e(TAG, "Bluetooth is not connected.")
                    false
                }
            }
            2 -> {
                if (isBLEConnected && mBluetoothGatt != null) {
                    sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_EDGE_PARAM_VALUE, MUTUAL_AUTH_SERVICE))
                    true
                } else {
                    Log.e(TAG, "Bluetooth is not connected.")
                    false
                }
            }
            3 -> {
                if (isBLEConnected && mBluetoothGatt != null) {
                    Log.e(TAG, "Getting device cert id...")
                    sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_EDGE_PARAM_VALUE, MUTUAL_AUTH_SERVICE))
                    true
                } else {
                    Log.e(TAG, "Bluetooth is not connected.")
                    false
                }
            }
            else -> false
        }*/
    }

    private fun indicateParamValue(number: Int): Boolean {
        //Read ble status before procced
        //sendBleCommand(BleCommand(BleCommand.CommandType.READ_CHARACTERISTIC, UUID_CHAR_GATE_WAY_STATUS, MUTUAL_AUTH_SERVICE, writeRandomNumber()))
        return when (number) {
            1 -> {
                if (isBLEConnected && mBluetoothGatt != null) {
                    Log.e(TAG, "Getting device cert id...")
                    sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_INDICATION_SERVICE, MUTUAL_AUTH_SERVICE, indicateCertificate()))
                    true
                } else {
                    Log.e(TAG, "Bluetooth is not connected.")
                    false
                }
            }
            2 -> { //Public key process
                if (isBLEConnected && mBluetoothGatt != null) {
                    sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_INDICATION_SERVICE, MUTUAL_AUTH_SERVICE, indicatePublicKey()))
                    true
                } else {
                    Log.e(TAG, "Bluetooth is not connected.")
                    false
                }
            }
            3 -> {
                //Read valur first
                if (isBLEConnected && mBluetoothGatt != null) {
                    sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_INDICATION_SERVICE, MUTUAL_AUTH_SERVICE, writeRandomNumber()))
                    true
                } else {
                    Log.e(TAG, "Bluetooth is not connected.")
                    false
                }
            }
            4 -> {
                if (isBLEConnected && mBluetoothGatt != null) {
                    sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_INDICATION_SERVICE, MUTUAL_AUTH_SERVICE, indicateSignatureRandom()))
                    true
                } else {
                    Log.e(TAG, "Bluetooth is not connected.")
                    false
                }
            }
            5 -> {
                if (isBLEConnected && mBluetoothGatt != null) {
                    sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_INDICATION_SERVICE, MUTUAL_AUTH_SERVICE, indicateECDHPublicKey()))
                    true
                } else {
                    Log.e(TAG, "Bluetooth is not connected.")
                    false
                }
            }
            6 -> {
                if (isBLEConnected && mBluetoothGatt != null) {
                    Log.e(TAG, "Getting device cert id...")
                    sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_INDICATION_SERVICE, MUTUAL_AUTH_SERVICE, indicateECDHSignature()))
                    true
                } else {
                    Log.e(TAG, "Bluetooth is not connected.")
                    false
                }
            }
            7 -> {
                if (isBLEConnected && mBluetoothGatt != null) {
                    Log.e(TAG, "Getting device cert id...")
                    sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_INDICATION_SERVICE, MUTUAL_AUTH_SERVICE, ecdhSessionKeyGenrated()))
                    true
                } else {
                    Log.e(TAG, "Bluetooth is not connected.")
                    false
                }
            }
            else -> false
        }
    }

    private fun enableService(serviceUuid: String, enable: Boolean) {
        val ready = ByteArray(1)
        if (enable) {
            ready[0] = 1
        } else {
            ready[0] = 0
        }
        when (serviceUuid) {
            UUID_NETWORK_SERVICE -> {
                Log.e(TAG, (if (enable) "Enabling" else "Disabling") + " Wifi provisioning")
                sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_NETWORK_CONTROL, UUID_NETWORK_SERVICE, ready))
            }
            UUID_MQTT_PROXY_SERVICE -> if (mKeystore != null || mAWSCredential != null) {
                Log.e(TAG, (if (enable) "Enabling" else "Disabling") + " MQTT Proxy")
                sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_MQTT_PROXY_CONTROL, UUID_MQTT_PROXY_SERVICE, ready))
            }
            else -> Log.e(TAG, "Unknown service. Ignoring.")
        }
    }

    private fun processIncomingQueue() {
        try {
            mIncomingMutex.acquire()
            while (mIncomingQueue.size != 0) {
                val bleCommand = mIncomingQueue.poll()
                //Log.e(TAG, "Processing incoming queue. size: " + mIncomingQueue.size)

                val responseBytes = bleCommand.data
                val cUuid = bleCommand.characteristicUuid
                //Log.e(TAG, "Processing incoming queue. Message size: " + bytesToHexString(responseBytes))
                when (cUuid) {
                    UUID_MQTT_PROXY_TX -> {
                        handleMqttTxMessage(responseBytes)
                    }
                    UUID_MQTT_PROXY_TXLARGE -> try {
                        mTxLargeObject.write(responseBytes)
                        sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_MQTT_PROXY_TXLARGE, UUID_MQTT_PROXY_SERVICE))
                    } catch (e: IOException) {
                        Log.e(TAG, "Failed to concatenate byte array.", e)
                    }
                    UUID_NETWORK_TX -> handleNwTxMessage(responseBytes)
                    UUID_NETWORK_TXLARGE -> try {
                        mTxLargeNw.write(responseBytes)
                        sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_NETWORK_TXLARGE, UUID_NETWORK_SERVICE))
                    } catch (e: IOException) {
                        Log.e(TAG, "Failed to concatenate byte array.", e)
                    }
                    else -> Log.e(TAG, "Unknown characteristic $cUuid")
                }
            }
            mIncomingMutex.release()
        } catch (e: InterruptedException) {
            Log.e(TAG, "Incoming mutex error, ", e)
        }
    }

    private fun registerBondStateCallback() {
        mBondStateCallback = object : BroadcastReceiver() {
            override fun onReceive(context: Context, intent: Intent) {
                val action = intent.action
                when (action) {
                    BluetoothDevice.ACTION_BOND_STATE_CHANGED -> {
                        val prev = intent.getIntExtra(BluetoothDevice.EXTRA_PREVIOUS_BOND_STATE, BluetoothDevice.ERROR)
                        val now = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.ERROR)
                        Log.e(TAG, "Bond state changed from $prev to $now")
                        /**
                         * If the state changed from bonding to bonded, then we have a valid bond created
                         * for the device.
                         * If discovery is not performed initiate discovery.
                         * If services are discovered start initialization by reading device version characteristic.
                         */
                        if (prev == BluetoothDevice.BOND_BONDING && now == BluetoothDevice.BOND_BONDED) {
                            val services = mBluetoothGatt!!.services
                            if (services == null || services.isEmpty()) {
                                discoverServices()
                            } else {
                                probe()
                            }
                        }
                    }
                    else -> {
                    }
                }
            }
        }
        val bondFilter = IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED)
        mContext.registerReceiver(mBondStateCallback, bondFilter)
    }

    private fun unRegisterBondStateCallback() {
        if (mBondStateCallback != null) {
            try {
                mContext.unregisterReceiver(mBondStateCallback)
                mBondStateCallback = null
            } catch (ex: IllegalArgumentException) {
                Log.e(TAG, "Caught exception while unregistering broadcast receiver")
            }
        }
    }

    /**
     * This is the callback for all BLE commands sent from SDK to device. The response of BLE
     * command is included in the callback, together with the status code.
     */
    private val mGattCallback: BluetoothGattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            Log.e(TAG, ":[BLE APP] : BLE connection state changed: " + status + "; new state: " + BleConnectionState.values()[newState])
            if (status == BluetoothGatt.GATT_SUCCESS) {
                if (newState == BluetoothProfile.STATE_CONNECTED) {
                    val bondState = mBluetoothDevice.bondState
                    mBleConnectionState = BleConnectionState.BLE_CONNECTED
                    Log.e(TAG, ":[BLE APP] : Connected to GATT server.")

                    // If the device is already bonded or will not bond we can call discoverServices() immediately
                    if (mBluetoothDevice.bondState != BluetoothDevice.BOND_BONDING) {
                        discoverServices()
                    } else {
                        registerBondStateCallback()
                    }
                } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                    mBleConnectionState = BleConnectionState.BLE_DISCONNECTED
                    Log.e(TAG, ":[BLE APP] : Disconnected from GATT server.")

                    // If ble connection is closed, there's no need to keep mqtt connection open.
                    if (mMqttConnectionState != MqttConnectionState.MQTT_Disconnected) {
                        disconnectFromIot()
                    }
                    unRegisterBondStateCallback()
                    resetVariables()
                    mBleConnectionStatusCallback!!.onBleConnectionStatusChanged(mBleConnectionState)
                    /**
                     * If auto reconnect is enabled, start a reconnect procedure in background
                     * using connect() method. Else close the GATT.
                     * Auto reconnect will be disabled when user initiates disconnect.
                     */
                    if (!mGattAutoReconnect) {
                        gatt.close()
                        mBluetoothGatt = null
                    } else {
                        mBluetoothGatt!!.connect()
                    }
                }
            } else {
                Log.e(TAG, ":[BLE APP] : Disconnected from GATT server due to error ot peripheral initiated disconnect.")
                mBleConnectionState = BleConnectionState.BLE_DISCONNECTED
                if (mMqttConnectionState != MqttConnectionState.MQTT_Disconnected) {
                    disconnectFromIot()
                }
                unRegisterBondStateCallback()
                resetVariables()
                mBleConnectionStatusCallback!!.onBleConnectionStatusChanged(mBleConnectionState)
                /**
                 * If auto reconnect is enabled, start a reconnect procedure in background
                 * using connect() method. Else close the GATT.
                 * Auto reconnect will be disabled when user initiates disconnect.
                 */
                if (!mGattAutoReconnect) {
                    gatt.close()
                    mBluetoothGatt = null
                } else {
                    mBluetoothGatt!!.connect()
                }
            }
        }

        // New services discovered
        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.e(TAG, ":[BLE APP] : Discovered Ble gatt services successfully. Bonding state: " + mBluetoothDevice.bondState)
                describeGattServices(mBluetoothGatt!!.services)
                /**
                 * Trigger bonding if needed, by reading device version characteristic, if bonding is not already
                 * in progress by the stack.
                 */
                if (gatt.getService(UUID.fromString(AmazonFreeRTOSConstants.MUTUAL_AUTH_SERVICE)) == null) {
                    isStart = false
                    mBleConnectionStatusCallback!!.onFail("Mututal Authentication service not found", ERROR_CODE_SERVICE_NOT_FOUND)
                }
                if (mBluetoothDevice.bondState != BluetoothDevice.BOND_BONDING) {
                    probe()
                }
            } else {
                Log.e(TAG, ":[BLE APP] : onServicesDiscovered received: $status")
                disconnect()
            }
            processNextBleCommand()
        }

        override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
            val responseBytes = characteristic.value
            //Log.e(TAG, "->->-> Characteristic changed for: " + uuidToName[characteristic.uuid.toString()] + " with data: " + bytesToHexString(responseBytes))
            val incomingCommand = BleCommand(BleCommand.CommandType.NOTIFICATION, characteristic.uuid.toString(), characteristic.service.uuid.toString(), responseBytes)
            mIncomingQueue.add(incomingCommand)
            if (!mRWinProgress) {
                processIncomingQueue()
            }
        }

        override fun onDescriptorWrite(gatt: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) {
            //Log.e(TAG, "onDescriptorWrite for characteristic: " + uuidToName[descriptor.characteristic.uuid.toString()] + "; Status: " + if (status == 0) "Success" else status)
            processNextBleCommand()
        }

        override fun onMtuChanged(gatt: BluetoothGatt, mtu: Int, status: Int) {
            Log.e(TAG, "onMTUChanged : " + mtu + " status: " + if (status == 0) "Success" else status)
            mMtu = mtu
            mMaxPayloadLen = Math.max(mMtu - 3, 0)
            // The BLE service should be initialized at this stage
            if (mBleConnectionState == BleConnectionState.BLE_INITIALIZING) {
                mBleConnectionState = BleConnectionState.BLE_INITIALIZED
                mBleConnectionStatusCallback!!.onBleConnectionStatusChanged(mBleConnectionState)
            }
            enableService(UUID_NETWORK_SERVICE, true)
            enableService(UUID_MQTT_PROXY_SERVICE, true)
            processNextBleCommand()
        }

        // Result of a characteristic read operation
        override fun onCharacteristicRead(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
            mRWinProgress = false
            //Log.e(TAG, "->->-> onCharacteristicRead status: ${uuidToName[characteristic.uuid.toString()]} " + if (status == 0) "Success. " else status)
            if (status == BluetoothGatt.GATT_SUCCESS) {
                // On the first successful read we enable the services
                if (mBleConnectionState == BleConnectionState.BLE_CONNECTED) {
                    Log.e(TAG, ":[BLE APP] : GATT services initializing...")
                    mBleConnectionState = BleConnectionState.BLE_INITIALIZING
                    mBleConnectionStatusCallback!!.onBleConnectionStatusChanged(mBleConnectionState)
                    initialize()
                }
                val responseBytes = characteristic.value
                //Log.e(TAG, "->->-> onCharacteristicRead: " + bytesToHexString(responseBytes))
                when (characteristic.uuid.toString()) {
                    UUID_MQTT_PROXY_TXLARGE -> try {
                        mTxLargeObject.write(responseBytes)
                        if (responseBytes.size < mMaxPayloadLen) {
                            val largeMessage = mTxLargeObject.toByteArray()
                            Log.e(TAG, "MQTT Large object received from device successfully: " + bytesToHexString(largeMessage))
                            handleMqttTxMessage(largeMessage)
                            mTxLargeObject.reset()
                        } else {
                            sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_MQTT_PROXY_TXLARGE, UUID_MQTT_PROXY_SERVICE))
                        }
                    } catch (e: IOException) {
                        Log.e(TAG, "Failed to concatenate byte array.", e)
                    }
                    UUID_NETWORK_TXLARGE -> try {
                        mTxLargeNw.write(responseBytes)
                        if (responseBytes.size < mMaxPayloadLen) {
                            val largeMessage = mTxLargeNw.toByteArray()
                            Log.e(TAG, "NW Large object received from device successfully: " + bytesToHexString(largeMessage))
                            handleNwTxMessage(largeMessage)
                            mTxLargeNw.reset()
                        } else {
                            sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_NETWORK_TXLARGE, UUID_NETWORK_SERVICE))
                        }
                    } catch (e: IOException) {
                        Log.e(TAG, "Failed to concatenate byte array.", e)
                    }
                    UUID_DEVICE_MTU -> {
                        val currentMtu = Mtu()
                        currentMtu.mtu = String(responseBytes)
                        Log.e(TAG, "Default MTU is set to: " + currentMtu.mtu)
                        try {
                            mMtu = currentMtu.mtu.toInt()
                            mMaxPayloadLen = Math.max(mMtu - 3, 0)
                            if (mDeviceInfoCallback != null) {
                                mDeviceInfoCallback!!.onObtainMtu(mMtu)
                            }
                            setMtu(mMtu)
                        } catch (e: NumberFormatException) {
                            Log.e(TAG, "Cannot parse default MTU value.")
                        }
                    }
                    UUID_IOT_ENDPOINT -> {
                        val currentEndpoint = BrokerEndpoint()
                        currentEndpoint.brokerEndpoint = String(responseBytes)
                        Log.e(TAG, "Current broker endpoint is set to: " + currentEndpoint.brokerEndpoint)
                        if (mDeviceInfoCallback != null) {
                            mDeviceInfoCallback!!.onObtainBrokerEndpoint(currentEndpoint.brokerEndpoint)
                        }
                    }
                    UUID_DEVICE_VERSION -> {
                        val currentVersion = Version()
                        currentVersion.version = String(responseBytes)
                        if (!currentVersion.version.isEmpty()) {
                            mAmazonFreeRTOSLibVersion = currentVersion.version
                        }
                        Log.e(TAG, "Ble software version on device is: " + currentVersion.version)
                        if (mDeviceInfoCallback != null) {
                            mDeviceInfoCallback!!.onObtainDeviceSoftwareVersion(currentVersion.version)
                        }
                    }
                    UUID_DEVICE_PLATFORM -> {
                        val platform = String(responseBytes)
                        if (!platform.isEmpty()) {
                            mAmazonFreeRTOSDeviceType = platform
                        }
                        Log.e(TAG, "Device type is: $mAmazonFreeRTOSDeviceType")
                    }
                    UUID_DEVICE_ID -> {
                        val devId = String(responseBytes)
                        if (!devId.isEmpty()) {
                            mAmazonFreeRTOSDeviceId = devId
                        }
                        Log.e(TAG, "Device id is: $mAmazonFreeRTOSDeviceId")
                    }
                    UUID_EDGE_PARAM_VALUE -> {
                        parseAuthenticationData(responseBytes)

                    }
                    UUID_CHAR_GATE_WAY_STATUS -> {
                        gateWayStatus = byteArrayToHex(responseBytes)
                        Log.e(TAG, "Param $currentParam gateWayStatus : $gateWayStatus ")
                        when (currentParam) {
                            4 -> {
                                if (hexToInteger(gateWayStatus) == ERROR_CODE_BLE_SIGNATURE_RANDOM_PASS) {
                                    if (indicateParamValue(5)) { //Indication success for device certificate
                                        currentParam = 5
                                        createAndSendParamValue(5)
                                    }
                                } else { //Failure cases
                                    mBleConnectionStatusCallback!!.onFail("ECDSA Signature verification fail from ble", ERROR_CODE_BLE_SIGNATURE_RANDOM_FAIL)
                                }
                            }
                            6 -> {
                                if (hexToInteger(gateWayStatus) == ERROR_CODE_ECDH_BLE_SIGNATURE_PASS) {
                                    sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_CHAR_EDGE_STATUS, MUTUAL_AUTH_SERVICE, ecdhSignatureVerificationPass()))
                                    generateECDHSecreat(bleECDHPublicKeyByteArray!!)
                                } else { //Failure cases
                                    mBleConnectionStatusCallback!!.onFail("ECDSA Signature verification fail from ble", ERROR_CODE_ECDH_BLE_SIGNATURE_FAIL)
                                }
                            }
                        }
                    }
                    else -> Log.e(TAG, "Unknown characteristic read. ")
                }
            }
            processIncomingQueue()
            processNextBleCommand()
        }

        override fun onCharacteristicWrite(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
            mRWinProgress = false
            val value = characteristic.value
            //Log.e(TAG, "onCharacteristicWrite for: " + uuidToName[characteristic.uuid.toString()] + "; status: " + (if (status == 0) "Success" else status) + "; value: " + bytesToHexString(value))
            if (Arrays.equals(mValueWritten, value)) {
                processIncomingQueue()
                processNextBleCommand()
            } else {
                Log.e(TAG, "values don't match!")
            }
        }
    }

    private fun parseAuthenticationData(bytes: ByteArray) {
        //Log.d(TAG, "Received array  : ${byteArrayToHex(bytes)} Param value $currentParam")
        val receivedDataFromChar = byteArrayToHex(bytes)
        when (currentParam) {
            1 -> {
                deviceCertificate = receivedDataFromChar
                /*if (indicateParamValue(2)) { //Indication success for device certificate
                    if (createAndSendParamValue(2)) {  //Write success for certificate
                        currentParam = 2
                        readParamValue(2)
                    }
                }*/

                if (publicKeyPass >= 0) {
                    appendDeviceCertificate(deviceCertificate)
                }

            }
            2 -> {

                devicePublicKey = receivedDataFromChar
                if (publicKeyPass >= 0) {
                    appendPublicKey(devicePublicKey)
                }

            }
            3 -> {
                //Log.e(TAG, "Reading random number when received $receivedDataFromChar")
                deviceRandomNumber = receivedDataFromChar.substring(4, receivedDataFromChar.length)
                val isCheck = isCRCValid(hexToByteArray(deviceRandomNumber), currentParam)  //Remove length bytes
                if (isCheck) {
                    deviceRandomNumber = deviceRandomNumber.substring(0, deviceRandomNumber.length - 4)
                    //Log.e(TAG, "Reading random number isCheck after crc $isCheck $deviceRandomNumber")
                    Log.e(TAG, "[Mutual Auth] : Received random number")

                    gatewaySignature = signRandomNumber(hexToByteArray(deviceRandomNumber))
                } else {
                    mBleConnectionStatusCallback!!.onFail("Random number crc fail", ERROR_CODE_CRC_FAIL)
                }
                if (indicateParamValue(4)) { //Indication success for device certificate
                    createAndSendParamValue(4)
                }
            }
            4 -> {
                signatureFromBle = receivedDataFromChar
                ///Log.e(TAG, "Reading Signature $signatureFromBle")

                if (publicKeyPass >= 0) {
                    readSignature(signatureFromBle, 4)
                }
            }
            5 -> {
                //Log.e(TAG, "Reading ecdh public key ")
                bleECDHPublicKey = receivedDataFromChar
                if (publicKeyPass >= 0) {
                    appendECDHPublicKey(bleECDHPublicKey)
                }
            }
            6 -> {
                ecdhSignatureFromBle = receivedDataFromChar
                //Log.e(TAG, "Reading ECDH Signature $ecdhSignatureFromBle")

                if (publicKeyPass >= 0) {
                    readECDHSignature(ecdhSignatureFromBle, 6)
                }
            }
            7 -> {

                //Log.e(TAG, "Reading ECDH shared secret $receivedDataFromChar")
                if (secreatReadCounter >= 0) {  //If counter = -1 don't read
                    readDeviceSHA(receivedDataFromChar)
                }
            }
        }
    }


    var receivedCertificateData = ArrayList<String>()

    private fun appendDeviceCertificate(CertData: String) {
        val hexData = BleCommandTransfer.convertHexToChunk(CertData, 2)
        if (publicKeyPass != -1) {
            if (publicKeyPass == 0) { //First time
                publicKeyPass++
                receivedData += hexData.size
                val length = BleCommandTransfer.hexToInteger(hexData[0] + hexData[1]) //100
                totalDataSize = length - 2   //100-2 = 98
                Log.e(TAG, "Total data append certificate  $totalDataSize")
                if (hexData.size >= 20) {
                    receivedCertificateData.addAll(hexData.subList(2, hexData.size))
                    //Log.e(TAG, "Device public key $devicePrivateKey")
                    sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_EDGE_PARAM_VALUE, MUTUAL_AUTH_SERVICE))
                } else { //Data complete
                    publicKeyPass = -1
                    Log.e(TAG, "[Mutual Auth] : Received Device Certificate")
                    receivedCertificateData.addAll(hexData.subList(2, hexData.size))
                    calculateCRC16EndProcessForCertificate()
                }
            } else { //Repeat until data arrives
                receivedData += hexData.size
                receivedCertificateData.addAll(hexData)
                if (receivedData >= totalDataSize) {
                    publicKeyPass = -1
                    Log.e(TAG, "[Mutual Auth] : Received Device Certificate")
                    calculateCRC16EndProcessForCertificate()
                } else {
                    //Request for data
                    sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_EDGE_PARAM_VALUE, MUTUAL_AUTH_SERVICE))
                }
                //Log.e(TAG, "Device public key ${removeArrayData(devicePrivateKey)}")
            }
        }

    }

    var deviceCertificeByteArray: ByteArray? = null

    private fun calculateCRC16EndProcessForCertificate() {
        val certWithCRC = receivedCertificateData.subList(0, totalDataSize)
        val crc16Data = certWithCRC.takeLast(2).toString().replace("[", "").replace("]", "").replace(",", "").replace(" ", "")
        val privateKeyWithoutCRC = certWithCRC.take(certWithCRC.size - 2)
        deviceCertificeByteArray = hexToByteArray(removeArrayData(privateKeyWithoutCRC))
        val crc16Check = integerToHex(CalculateCRC16.crc16_U(deviceCertificeByteArray), 4)
        if (crc16Data.equals(crc16Check, true)) {
            Log.e(TAG, "[Mutual Auth] : Certificete CRC matched")
            //Edited by alpesh : certificate verify here
            val isVerifyCertificate = CertificateVerifyUsingByteArray(deviceCertificeByteArray,"optiga_RootCa.der")
            if(isVerifyCertificate!!){
                Log.e(TAG, "[Mutual Auth] : Certificate verification successful")
                resetVariables()
                if (indicateParamValue(2)) { //Indication success for device certificate
                    if (createAndSendParamValue(2)) {  //Write success for certificate
                        currentParam = 2
                        readParamValue(2)
                    }
                }
            }else{
                Log.e(TAG, "[Mutual Auth] : Certificate verification fail")
                mBleConnectionStatusCallback!!.onFail("Certificate verification fail", ERROR_CODE_CRC_FAIL)
            }

        } else {
            Log.e(TAG, "[Mutual Auth] : Certificate CRC not matched")
            mBleConnectionStatusCallback!!.onFail("Certificate CRC not matched", ERROR_CODE_CRC_FAIL)

        }
        //deviceCertificeByteArray = hexToByteArray(removeArrayData(privateKeyWithoutCRC))

        //Log.e(TAG, "Reading public key  ${getPublicKeyHeaderInHEX()}${removeArrayData(privateKeyWithoutCRC)}")
    }

    var bleSecreat = ""
    private fun readDeviceSHA(receivedDataFromChar: String) {
        secreatReadCounter++
        val hexData = BleCommandTransfer.convertHexToChunk(receivedDataFromChar, 2)
        if (secreatReadCounter == 1) { //First time
            val dataWithoutLength = hexData.subList(2, hexData.size)
            bleECDHSecreat = removeArrayData(dataWithoutLength)
            readParamValue(7)
        }
        if (secreatReadCounter == 2) {
            val dataWithoutCRC = hexData.subList(0, hexData.size - 2)
            bleECDHSecreat += removeArrayData(dataWithoutCRC)
            Log.e(TAG, "[Mutual Auth] : Received Hash of secret key")

            if (bleECDHSecreat.equals(gatewayECDHSecreat, true)) {
                Log.e(TAG, "[Mutual Auth] : ECDH Hashed verified successfully")
                mBleConnectionStatusCallback!!.onSecreteGenerated(bleECDHSecreat)
                sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_CHAR_EDGE_STATUS, MUTUAL_AUTH_SERVICE, gatewaySecreMatch()))
                startMQTTProcess()
            } else {
                Log.e(TAG, "[Mutual Auth] : ECDH Hashed verification fail")

                //Log.e(TAG, "Auth fail $bleECDHSecreat $gatewayECDHSecreat")
                mBleConnectionStatusCallback!!.onFail("ECDSA HashedECDH signature verified successfully verification fail", ERROR_CODE_ECDSA_SIGN_FAIL)
                sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_CHAR_EDGE_STATUS, MUTUAL_AUTH_SERVICE, gatewaySecretFail()))
            }
            secreatReadCounter = -1
        }
    }

    private fun readECDHSignature(dataFromChar: String, paramValue: Int) {
        val hexData = BleCommandTransfer.convertHexToChunk(dataFromChar, 2)
        if (publicKeyPass != -1) {
            if (publicKeyPass == 0) { //First time
                publicKeyPass++
                receivedData += hexData.size
                val length = BleCommandTransfer.hexToInteger(hexData[0] + hexData[1]) //100
                totalDataSize = length - 2   //100-2 = 98
                if (hexData.size >= 20) {
                    dataToSendInChunks.addAll(hexData.subList(2, hexData.size))
                    //Log.e(TAG, "readDataInLoop $dataToSendInChunks")
                    sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_EDGE_PARAM_VALUE, MUTUAL_AUTH_SERVICE))
                } else { //Data complete
                    publicKeyPass = -1
                    Log.e(TAG, "[Mutual Auth] : Received ECDH signature")
                    dataToSendInChunks.addAll(hexData.subList(2, hexData.size))
                    val dataInBytes = hexToByteArray(removeArrayData(dataToSendInChunks))
                    var dataWithoutCRC = removeCRCBytes(bytesToHexString(dataInBytes))
                    if (isCRCValid(dataInBytes, paramValue)) {
                        // Log.e(TAG, "readDataInLoop finish with crc ${bytesToHexString(dataInBytes)} without crc ${removeCRCBytes(bytesToHexString(dataInBytes))}")
                        appendECDHSignatureHeaderAndVerify(dataWithoutCRC)
                    } else {
                        mBleConnectionStatusCallback!!.onFail("ECDH crc fail", ERROR_CODE_CRC_FAIL)
                    }
                }
            } else { //Repeat until data arrives
                receivedData += hexData.size
                dataToSendInChunks.addAll(hexData)
                if (receivedData >= totalDataSize) {
                    publicKeyPass = -1
                    Log.e(TAG, "[Mutual Auth] : Received ECDH signature")
                    val dataInBytes = hexToByteArray(removeArrayData(dataToSendInChunks))
                    if (isCRCValid(dataInBytes, paramValue)) {

                        var dataWithoutCRC = removeCRCBytes(bytesToHexString(dataInBytes))
                        //Log.e(TAG, "readDataInLoop finish with crc ${bytesToHexString(dataInBytes)} without crc ${removeCRCBytes(bytesToHexString(dataInBytes))}")
                        appendECDHSignatureHeaderAndVerify(dataWithoutCRC)
                    } else {
                        mBleConnectionStatusCallback!!.onFail("ECDH number crc fail", ERROR_CODE_CRC_FAIL)
                    }
                } else {
                    //Request for data
                    sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_EDGE_PARAM_VALUE, MUTUAL_AUTH_SERVICE))
                }
                //Log.e(TAG, "readDataInLoop $dataToSendInChunks")
            }

        }
    }

    private fun readSignature(dataFromChar: String, paramValue: Int) {
        val hexData = BleCommandTransfer.convertHexToChunk(dataFromChar, 2)
        if (publicKeyPass != -1) {
            if (publicKeyPass == 0) { //First time
                publicKeyPass++
                receivedData += hexData.size
                val length = BleCommandTransfer.hexToInteger(hexData[0] + hexData[1]) //100
                totalDataSize = length - 2   //100-2 = 98
                if (hexData.size >= 20) {
                    dataToSendInChunks.addAll(hexData.subList(2, hexData.size))
                    //Log.e(TAG, "readDataInLoop $dataToSendInChunks")
                    sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_EDGE_PARAM_VALUE, MUTUAL_AUTH_SERVICE))
                } else { //Data complete
                    publicKeyPass = -1
                    Log.e(TAG, "[Mutual Auth] : Received signature")
                    dataToSendInChunks.addAll(hexData.subList(2, hexData.size))
                    val dataInBytes = hexToByteArray(removeArrayData(dataToSendInChunks))
                    var dataWithoutCRC = removeCRCBytes(bytesToHexString(dataInBytes))
                    if (isCRCValid(dataInBytes, paramValue)) {
                        //Log.e(TAG, "readDataInLoop finish with crc ${bytesToHexString(dataInBytes)} without crc ${removeCRCBytes(bytesToHexString(dataInBytes))}")
                        appendSignatureHeaderAndVerify(dataWithoutCRC)
                    }
                }
            } else { //Repeat until data arrives
                receivedData += hexData.size
                dataToSendInChunks.addAll(hexData)
                if (receivedData >= totalDataSize) {
                    publicKeyPass = -1
                    val dataInBytes = hexToByteArray(removeArrayData(dataToSendInChunks))
                    if (isCRCValid(dataInBytes, paramValue)) {
                        Log.e(TAG, "[Mutual Auth] : Received signature")
                        var dataWithoutCRC = removeCRCBytes(bytesToHexString(dataInBytes))
                        //Log.e(TAG, "readDataInLoop finish with crc ${bytesToHexString(dataInBytes)} without crc ${removeCRCBytes(bytesToHexString(dataInBytes))}")
                        appendSignatureHeaderAndVerify(dataWithoutCRC)
                    }
                } else {
                    //Request for data
                    sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_EDGE_PARAM_VALUE, MUTUAL_AUTH_SERVICE))
                }
                //Log.e(TAG, "readDataInLoop $dataToSendInChunks")
            }

        }
    }

    private fun appendECDHSignatureHeaderAndVerify(dataWithoutCRC: String) {
        val signatureWithoutHeader = dataWithoutCRC
        val dataSize = BleCommandTransfer.integerToHex((signatureWithoutHeader.length) / 2, 2)
        val signatureWithHeader = "30$dataSize$dataWithoutCRC"
        //Log.e(TAG, "ECDH Signature data $signatureWithoutHeader")
        var ecdhPublicKeyWithoutHeader = bytesToHexString(bleECDHPublicKeyByteArray!!) //Remove header
        var ecdhKeyWithChunks = convertHexToChunk(ecdhPublicKeyWithoutHeader, 2)
        var ecdhKeyWithoutArrayData = removeArrayData(ecdhKeyWithChunks.subList(26, ecdhKeyWithChunks.size))
        val isVerified = verifyECDH(hexToByteArray(ecdhKeyWithoutArrayData), hexToByteArray(signatureWithHeader), devicePublicKeyByteArray!!)
    }

    private fun appendSignatureHeaderAndVerify(dataWithoutCRC: String) {
        val signatureWithoutHeader = dataWithoutCRC
        val dataSize = BleCommandTransfer.integerToHex((signatureWithoutHeader.length) / 2, 2)
        val signatureWithHeader = "30$dataSize$dataWithoutCRC"
        //Log.e(TAG, "Signature data $signatureWithoutHeader")
        val isVerified = verifyData(gatewayRandom!!, hexToByteArray(signatureWithHeader), devicePublicKeyByteArray!!)
        if (isVerified) {
            sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_CHAR_EDGE_STATUS, MUTUAL_AUTH_SERVICE, signatureVerificationPass()))
            //Check status from gateway
            sendBleCommand(BleCommand(BleCommand.CommandType.READ_CHARACTERISTIC, UUID_CHAR_GATE_WAY_STATUS, MUTUAL_AUTH_SERVICE))
            /* if (indicateParamValue(5)) { //Indication success for device certificate
                currentParam = 5
                createAndSendParamValue(5)
            }*/
            Log.e(TAG, "[Mutual Auth] : ECDSA Signature verified successfully");

        } else {
            Log.e(TAG, "[Mutual Auth] : ECDSA Signature verification fail");
            sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_CHAR_EDGE_STATUS, MUTUAL_AUTH_SERVICE, signatureVerificationFail()))
            mBleConnectionStatusCallback!!.onFail("ECDSA Signature verification fail", ERROR_CODE_ECDSA_SIGN_FAIL)

        }
        //mBleConnectionStatusCallback!!.onFail("ECDSA Signature verification fail")
    }

    private fun appendPublicKey(publicKeyData: String) {
        val hexData = BleCommandTransfer.convertHexToChunk(publicKeyData, 2)
        if (publicKeyPass != -1) {
            if (publicKeyPass == 0) { //First time
                publicKeyPass++
                receivedData += hexData.size
                val length = BleCommandTransfer.hexToInteger(hexData[0] + hexData[1]) //100
                totalDataSize = length - 2   //100-2 = 98
                if (hexData.size >= 20) {
                    devicePrivateKey.addAll(hexData.subList(2, hexData.size))
                    //Log.e(TAG, "Device public key $devicePrivateKey")
                    sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_EDGE_PARAM_VALUE, MUTUAL_AUTH_SERVICE))
                } else { //Data complete
                    publicKeyPass = -1
                    Log.e(TAG, "[Mutual Auth] : Received public key")
                    devicePrivateKey.addAll(hexData.subList(2, hexData.size))
                    calculateCRC16EndProcess()
                }
            } else { //Repeat until data arrives
                receivedData += hexData.size
                devicePrivateKey.addAll(hexData)
                if (receivedData >= totalDataSize) {
                    publicKeyPass = -1
                    Log.e(TAG, "[Mutual Auth] : Received public key")
                    calculateCRC16EndProcess()
                } else {
                    //Request for data
                    sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_EDGE_PARAM_VALUE, MUTUAL_AUTH_SERVICE))
                }
                //Log.e(TAG, "Device public key ${removeArrayData(devicePrivateKey)}")
            }
        }

    }

    private fun appendECDHPublicKey(publicKeyData: String) {
        val hexData = BleCommandTransfer.convertHexToChunk(publicKeyData, 2)
        if (publicKeyPass != -1) {
            if (publicKeyPass == 0) { //First time
                publicKeyPass++
                receivedData += hexData.size
                val length = BleCommandTransfer.hexToInteger(hexData[0] + hexData[1]) //100
                totalDataSize = length - 2   //100-2 = 98
                if (hexData.size >= 20) {
                    deviceECDHPrivateKey.addAll(hexData.subList(2, hexData.size))
                    //Log.e(TAG, "Device ECDH public key $deviceECDHPrivateKey")
                    sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_EDGE_PARAM_VALUE, MUTUAL_AUTH_SERVICE))
                } else { //Data complete
                    publicKeyPass = -1
                    Log.e(TAG, "[Mutual Auth] : Received ECDH public key")
                    deviceECDHPrivateKey.addAll(hexData.subList(2, hexData.size))
                    calculateECDHCRC16EndProcess()
                }
            } else { //Repeat until data arrives
                receivedData += hexData.size
                deviceECDHPrivateKey.addAll(hexData)
                if (receivedData >= totalDataSize) {
                    publicKeyPass = -1
                    Log.e(TAG, "[Mutual Auth] : Received ECDH public key")
                    calculateECDHCRC16EndProcess()
                } else {
                    //Request for data
                    sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_EDGE_PARAM_VALUE, MUTUAL_AUTH_SERVICE))
                }
                //Log.e(TAG, "Device ECDH public key ${removeArrayData(deviceECDHPrivateKey)}")
            }
        }

    }

    private fun calculateECDHCRC16EndProcess() {
        val privateKeyWithCRC = deviceECDHPrivateKey.subList(0, totalDataSize)
        val crc16Data = privateKeyWithCRC.takeLast(2).toString().replace("[", "").replace("]", "").replace(",", "").replace(" ", "")
        val privateKeyWithoutCRC = privateKeyWithCRC.take(privateKeyWithCRC.size - 2)
        bleECDHPublicKeyByteArray = hexToByteArray(removeArrayData(privateKeyWithoutCRC))
        val crc16Check = integerToHex(CalculateCRC16.crc16_U(bleECDHPublicKeyByteArray), 4)

        if (crc16Data.equals(crc16Check, true)) {
            Log.e(TAG, "[Mutual Auth] : ECDH public key CRC matched")
            if (indicateParamValue(6)) { //Indication success for device certificate
                createAndSendParamValue(6)
            }
        } else {
            Log.e(TAG, "[Mutual Auth] : ECDH public key CRC not matched")

            // Log.e(TAG, "CRC ECDHṢCRC Fail")
        }
        bleECDHPublicKeyByteArray = hexToByteArray(getPublicKeyHeaderInHEX() + removeArrayData(privateKeyWithoutCRC))
        //Log.e(TAG, "Reading ECDH public key with header  ${getPublicKeyHeaderInHEX()}${removeArrayData(privateKeyWithoutCRC)}")
    }

    private fun calculateCRC16EndProcess() {
        val privateKeyWithCRC = devicePrivateKey.subList(0, totalDataSize)
        val crc16Data = privateKeyWithCRC.takeLast(2).toString().replace("[", "").replace("]", "").replace(",", "").replace(" ", "")
        val privateKeyWithoutCRC = privateKeyWithCRC.take(privateKeyWithCRC.size - 2)
        devicePublicKeyByteArray = hexToByteArray(removeArrayData(privateKeyWithoutCRC))
        val crc16Check = integerToHex(CalculateCRC16.crc16_U(devicePublicKeyByteArray), 4)
        if (crc16Data.equals(crc16Check, true)) {
            Log.e(TAG, "[Mutual Auth] : Public key CRC matched")
            if (indicateParamValue(3)) { //Indication success for device certificate
                if (createAndSendParamValue(3)) {  //Write success for certificate
                    currentParam = 3
                    readParamValue(3)
                }
            }
        } else {
            Log.e(TAG, "[Mutual Auth] : Public key CRC not matched")
        }
        devicePublicKeyByteArray = hexToByteArray(getPublicKeyHeaderInHEX() + removeArrayData(privateKeyWithoutCRC))
        //Log.e(TAG, "Reading public key  ${getPublicKeyHeaderInHEX()}${removeArrayData(privateKeyWithoutCRC)}")
    }

    private fun isCRCValid(byteArray: ByteArray, paramValue: Int): Boolean {
        val dataWithHex = BleCommandTransfer.byteArrayToHex(byteArray)
        val dataWithCRC = BleCommandTransfer.convertHexToChunk(dataWithHex, 2)
        val crc16Data = dataWithCRC.takeLast(2).toString().replace("[", "").replace("]", "").replace(",", "").replace(" ", "")
        val privateKey = dataWithCRC.take(byteArray.size - 2)
        val crcToCheck = hexToByteArray(removeArrayData(privateKey))
        val crc16Check = integerToHex(CalculateCRC16.crc16_U(crcToCheck), 4)
        if (crc16Data.equals(crc16Check, true)) {
            //Log.e(TAG, "CRC Match $paramValue")
            return true
        } else {
            //Log.e(TAG, "CRC Fail $paramValue")
            return false
        }
    }

    private fun removeCRCBytes(byteArray: String): String {
        val dataWithCRC = BleCommandTransfer.convertHexToChunk(byteArray, 2)
        return removeArrayData(dataWithCRC.take(dataWithCRC.size - 2))
    }


    private fun removeArrayData(publicKeyData: List<String>): String {
        var removeChars = publicKeyData.toString().replace("[", "").replace("]", "").replace(",", "").replace(" ", "")
        return removeChars
    }

    /**
     * Handle mqtt messages received from device.
     *
     * @param message message received from device.
     */

    // private fun handleMqttTxMessage(message: ByteArray) {
    private fun handleMqttTxMessage(encrypetedData: ByteArray) {
        //Log.e(TAG, "Handling Mqtt Message type before decrypt: " + bytesToHexString(encrypetedData!!))
        /* var hexArray = bytesToHexString(encrypetedData!!)
         var orignalChunks = convertHexToChunk(hexArray, 2)
         var chunks = convertHexToChunk(hexArray, 2)
         chunks = chunks.subList(17, chunks.size)
         Log.e(TAG, "Handling Mqtt +16 : " + removeArrayData(chunks))
         var dataToDecrypt = hexToByteArray(removeArrayData(chunks))

         //Appended data
         var appendedData = orignalChunks.subList(0, 16)
         var hexArrayOrignalMessage = removeArrayData(appendedData!!)
         var message = hexToByteArray(hexArrayOrignalMessage + bytesToHexString(dataDecrypt!!))
         Log.e(TAG, "Handling Mqtt Message type : " + bytesToHexStringForLog(message!!) +
                 " String " + String(message) + " Size " + message.size)*/
        Log.e(TAG, "[SECURITY] : [RECEIVED] : CIPHER TEXT : " + String(encrypetedData!!))
        val message = decrypt(encrypetedData)
        Log.e(TAG, "[SECURITY] : [RECEIVED] : PLAIN TEXT : " + String(message!!))
        val messageType = MessageType()
        if (!messageType.decode(message)) {
            return
        }
        //Log.e(TAG, "Handling Mqtt Message type : " + messageType.type + " message " + String(message!!))
        when (messageType.type) {
            MQTT_MSG_CONNECT -> { //�awadistm32wb55aax/a3vwfgdm06d0xb-ats.iot.ap-south-1.amazonaws.comac�
                val connect = Connect()
                if (connect.decode(message)) {
                    connectToIoT(connect)
                }
            }

            MQTT_MSG_SUBSCRIBE -> {  //�aav�oiotdemo/topic/1oiotdemo/topic/2oiotdemo/topic/3oiotdemo/topic/4ao�ai
                val subscribe = Subscribe()
                if (subscribe.decode(message)) {
                    Log.e(TAG, subscribe.toString())
                    subscribeToIoT(subscribe)
                    /*
                  Currently, because the IoT part of aws mobile sdk for Android
                  does not provide suback callback when subscribe is successful,
                  we create a fake suback message and send to device as a workaround.
                  Wait for 0.5 sec so that the subscribe is complete. Potential bug:
                  Message is received from the subscribed topic before suback
                  is sent to device.
                 */mHandler!!.postDelayed({ sendSubAck(subscribe) }, 500)
                }
            }
            MQTT_MSG_UNSUBSCRIBE -> {
                val unsubscribe = Unsubscribe()
                if (unsubscribe.decode(message)) {
                    unsubscribeToIoT(unsubscribe)
                    /*
                  TODO: add unsuback support in Aws Mobile sdk
                 */sendUnsubAck(unsubscribe)
                }
            }
            MQTT_MSG_PUBLISH -> {  //�awauoiotdemo/topic/1anakNHello world 0!ai
                val publish = Publish()
                if (publish.decode(message)) {
                    mMessageId = publish.msgID
                    publishToIoT(publish)
                }
            }
            MQTT_MSG_DISCONNECT -> disconnectFromIot()
            MQTT_MSG_PUBACK -> {
                /*
                 AWS Iot SDK currently sends pub ack back to cloud without waiting
                 for pub ack from device.
                 */
                val puback = Puback()
                if (puback.decode(message)) {
                    //Log.e(TAG, "Received mqtt pub ack from device. MsgID: " + puback.msgID)
                }
            }
            MQTT_MSG_PINGREQ -> {
                val pingResp = PingResp()
                val pingRespBytes = pingResp.encode()
                //val pingRespBytes = encryptMessage(pingResp.encode())
                sendDataToDevice(UUID_MQTT_PROXY_SERVICE, UUID_MQTT_PROXY_RX, UUID_MQTT_PROXY_RXLARGE, pingRespBytes)
            }
            else -> Log.e(TAG, ":[MQTT] Unknown mqtt message type: " + messageType.type)
        }
    }

    private fun sendSignatureInChunk(completeData: List<String>) {
        if (remainingLength >= 20) {
            //Log.e(TAG, "Send signature start $startLimit end $endLimit")
            val data = completeData.subList(startLimit, endLimit)
            //Log.e(TAG, "Send signature start $startLimit end $endLimit  data ${removeArrayData(data)}")
            sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, hexToByteArray(removeArrayData(data))))
            remainingLength = completeData.size - (endLimit)
            startLimit = endLimit
            endLimit += 20
        } else {
            startLimit = completeData.size - remainingLength
            val data = completeData.subList(startLimit, completeData.size)
            //Log.e(TAG, "Send signature start $startLimit end $endLimit  data ${removeArrayData(data)}")
            sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, hexToByteArray(removeArrayData(data))))
            remainingLength = 0
        }
    }

    private fun sendECDHSignatureInChunk(completeData: List<String>) {
        if (remainingLength >= 20) {
            //Log.e(TAG, "Send ecdh signature start $startLimit end $endLimit")
            val data = completeData.subList(startLimit, endLimit)
            //Log.e(TAG, "Send ecdh signature start $startLimit end $endLimit  data ${removeArrayData(data)}")
            sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, hexToByteArray(removeArrayData(data))))
            remainingLength = completeData.size - (endLimit)
            startLimit = endLimit
            endLimit += 20
        } else {
            startLimit = completeData.size - remainingLength
            val data = completeData.subList(startLimit, completeData.size)
            //Log.e(TAG, "Send ecdh signature start $startLimit end $endLimit  data ${removeArrayData(data)}")
            sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, hexToByteArray(removeArrayData(data))))
            remainingLength = 0
        }
    }

    private fun sendECDHPublicKeyInChunks(completeData: ByteArray) {
        if (remainingLength >= 20) {
            //Log.e(TAG, "ECDH Public key calculateData start $startLimit end $endLimit")
            val data = Arrays.copyOfRange(completeData, startLimit, endLimit)
            //Log.e(TAG, "ECDH Public key  calculateData start $startLimit end $endLimit  data ${bytesToHexString(data)}")
            sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, data))
            remainingLength = completeData.size - (endLimit)
            startLimit = endLimit
            endLimit += 20
        } else {
            startLimit = completeData.size - remainingLength
            val data = Arrays.copyOfRange(completeData, startLimit, completeData.size)
            //Log.e(TAG, "ECDH Public key calculateData start $startLimit end $endLimit  data ${bytesToHexString(data)}")
            sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, data))
            remainingLength = 0
        }
    }

    private fun sendPublicKeyInChunks(completeData: ByteArray) {
        if (remainingLength >= 20) {
            //Log.e(TAG, "calculateData start $startLimit end $endLimit")
            val data = Arrays.copyOfRange(completeData, startLimit, endLimit)
            //Log.e(TAG, "calculateData start $startLimit end $endLimit  data ${bytesToHexString(data)}")
            sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, data))
            remainingLength = completeData.size - (endLimit)
            startLimit = endLimit
            endLimit += 20
        } else {
            startLimit = completeData.size - remainingLength
            val data = Arrays.copyOfRange(completeData, startLimit, completeData.size)
            //Log.e(TAG, "calculateData start $startLimit end $endLimit  data ${bytesToHexString(data)}")
            sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, data))
            remainingLength = 0
        }
    }

    private fun handleNwTxMessage(message: ByteArray) {
        val messageType = MessageType()
        if (!messageType.decode(message)) {
            return
        }
        Log.e(TAG, "Handling Network Message type : " + messageType.type)
        when (messageType.type) {
            LIST_NETWORK_RESP -> {
                val listNetworkResp = ListNetworkResp()
                if (listNetworkResp.decode(message) && mNetworkConfigCallback != null) {
                    Log.e(TAG, listNetworkResp.toString())
                    mNetworkConfigCallback!!.onListNetworkResponse(listNetworkResp)
                }
            }
            SAVE_NETWORK_RESP -> {
                val saveNetworkResp = SaveNetworkResp()
                if (saveNetworkResp.decode(message) && mNetworkConfigCallback != null) {
                    mNetworkConfigCallback!!.onSaveNetworkResponse(saveNetworkResp)
                }
            }
            EDIT_NETWORK_RESP -> {
                val editNetworkResp = EditNetworkResp()
                if (editNetworkResp.decode(message) && mNetworkConfigCallback != null) {
                    mNetworkConfigCallback!!.onEditNetworkResponse(editNetworkResp)
                }
            }
            DELETE_NETWORK_RESP -> {
                val deleteNetworkResp = DeleteNetworkResp()
                if (deleteNetworkResp.decode(message) && mNetworkConfigCallback != null) {
                    mNetworkConfigCallback!!.onDeleteNetworkResponse(deleteNetworkResp)
                }
            }
            else -> Log.e(TAG, "Unknown network message type: " + messageType.type)
        }
    }

    private fun connectToIoT(connect: Connect) {
        if (mMqttConnectionState == MqttConnectionState.MQTT_Connected) {
            Log.e(TAG, "Already connected to IOT, sending connack to device again.")
            sendConnAck()
            return
        }
        if (mMqttConnectionState != MqttConnectionState.MQTT_Disconnected) {
            Log.e(TAG, "Previous connection is active, please retry or disconnect mqtt first.")
            return
        }
        mIotMqttManager = AWSIotMqttManager(connect.clientID, connect.brokerEndpoint)
        val userMetaData: MutableMap<String, String> = HashMap()
        userMetaData["AFRSDK"] = "Android"
        //userMetaData.put("AFRSDKVersion", AMAZONFREERTOS_SDK_VERSION);
        userMetaData["AFRLibVersion"] = mAmazonFreeRTOSLibVersion
        userMetaData["Platform"] = mAmazonFreeRTOSDeviceType
        userMetaData["AFRDevID"] = mAmazonFreeRTOSDeviceId
        mIotMqttManager!!.updateUserMetaData(userMetaData)
        val mqttClientStatusCallback = AWSIotMqttClientStatusCallback { status, throwable ->
            Log.e(TAG, ":[MQTT] mqtt connection status changed to: $status")
            when (status) {
                AWSIotMqttClientStatus.Connected -> {
                    mMqttConnectionState = MqttConnectionState.MQTT_Connected
                    //sending connack
                    if (isBLEConnected && mBluetoothGatt != null) {
                        sendConnAck()
                    } else {
                        Log.e(TAG, ":[MQTT] Cannot send CONNACK because BLE connection is: $mBleConnectionState")
                    }
                }
                AWSIotMqttClientStatus.Connecting, AWSIotMqttClientStatus.Reconnecting -> mMqttConnectionState = MqttConnectionState.MQTT_Connecting
                AWSIotMqttClientStatus.ConnectionLost -> mMqttConnectionState = MqttConnectionState.MQTT_Disconnected
                else -> Log.e(TAG, ":[MQTT] Unknown mqtt connection state: $status")
            }
        }
        if (mKeystore != null) {
            Log.e(TAG, "Connecting to IoT using KeyStore: " + connect.brokerEndpoint)
            mIotMqttManager!!.connect(mKeystore, mqttClientStatusCallback)
        } else {
            Log.e(TAG, "Connecting to IoT using AWS credential: " + connect.brokerEndpoint)
            mIotMqttManager!!.connect(mAWSCredential, mqttClientStatusCallback)
        }
    }

    private fun disconnectFromIot() {
        if (mIotMqttManager != null) {
            try {
                mIotMqttManager!!.disconnect()
                mMqttConnectionState = MqttConnectionState.MQTT_Disconnected
            } catch (e: Exception) {
                Log.e(TAG, ":[MQTT] Mqtt disconnect error: ", e)
            }
        }
    }

    private fun subscribeToIoT(subscribe: Subscribe) {
        if (mMqttConnectionState != MqttConnectionState.MQTT_Connected) {
            Log.e(TAG, ":[MQTT] Cannot subscribe because mqtt state is not connected.")
            return
        }
        for (i in subscribe.topics.indices) {
            try {
                val topic = subscribe.topics[i]
                Log.e(TAG, ":[MQTT] Subscribing to IoT on topic : $topic")
                val QoS = subscribe.qoSs[i]
                val qos = if (QoS == 0) AWSIotMqttQos.QOS0 else AWSIotMqttQos.QOS1
                mIotMqttManager!!.subscribeToTopic(topic, qos) { topic, data ->
                    val message = String(data, StandardCharsets.UTF_8)
                    //Log.e(TAG, " Message arrived on topic: $topic")
                    val publish = Publish(MQTT_MSG_PUBLISH, topic, mMessageId, QoS, data)
                    publishToDevice(publish)
                }

            } catch (e: Exception) {
                Log.e(TAG, ":[MQTT] Subscription error.", e)
            }
        }
    }

    private fun unsubscribeToIoT(unsubscribe: Unsubscribe) {
        if (mMqttConnectionState != MqttConnectionState.MQTT_Connected) {
            Log.e(TAG, ":[MQTT] Cannot unsubscribe because mqtt state is not connected.")
            return
        }
        for (i in unsubscribe.topics.indices) {
            try {
                val topic = unsubscribe.topics[i]
                Log.e(TAG, ":[MQTT] UnSubscribing to IoT on topic : $topic")
                mIotMqttManager!!.unsubscribeTopic(topic)
            } catch (e: Exception) {
                Log.e(TAG, ":[MQTT] Unsubscribe error.", e)
            }
        }
    }

    private fun publishToIoT(publish: Publish) {
        if (mMqttConnectionState != MqttConnectionState.MQTT_Connected) {
            Log.e(TAG, ":[MQTT] Cannot publish message to IoT because mqtt connection state is not connected.")
            return
        }
        val deliveryCallback = AWSIotMqttMessageDeliveryCallback { messageDeliveryStatus, o ->
            //Log.e(TAG, ":[MQTT] Publish msg delivery status: $messageDeliveryStatus")
            if (messageDeliveryStatus == MessageDeliveryStatus.Success && publish.qos == 1) {
                sendPubAck(publish)
            }
        }
        try {
            val topic = publish.topic
            val data = publish.payload

            //Sending mqtt message to IoT on topic: iotdemo/topic/1 message: Hello world 0! MsgID: 3
            //Perform description here
            if (data.size > 0) {
                decrypt(data)
                Log.e(TAG, ":[MQTT] Sending mqtt message to IoT on topic: " + topic + " message: " + String(decrypt(data)!!) + " MsgID: " + publish.msgID)
                mIotMqttManager!!.publishData(data, topic, AWSIotMqttQos.values()[publish.qos], deliveryCallback, null)
            } else {
                Log.e(TAG, ":[MQTT] Publish error payload missing")
            }
        } catch (e: Exception) {
            Log.e(TAG, ":[MQTT] Publish error.", e)
        }
    }

    private fun sendConnAck() {
        val connack = Connack()
        connack.type = MQTT_MSG_CONNACK
        connack.status = MqttConnectionState.MQTT_Connected.ordinal
        val connackBytes = connack.encode()
        //val connackBytes = encryptMessage(connack.encode())
        sendDataToDevice(UUID_MQTT_PROXY_SERVICE, UUID_MQTT_PROXY_RX, UUID_MQTT_PROXY_RXLARGE, connackBytes)
    }

    private val isBLEConnected: Boolean
        private get() = mBleConnectionState == BleConnectionState.BLE_CONNECTED || mBleConnectionState == BleConnectionState.BLE_INITIALIZED || mBleConnectionState == BleConnectionState.BLE_INITIALIZING

    private fun sendSubAck(subscribe: Subscribe) {
        if (!isBLEConnected && mBluetoothGatt != null) {
            Log.e(TAG, "Cannot send SUB ACK to BLE device because BLE connection state" + " is not connected")
            return
        }
        Log.e(TAG, "Sending SUB ACK back to device.")
        val suback = Suback()
        suback.type = MQTT_MSG_SUBACK
        suback.msgID = subscribe.msgID
        suback.status = subscribe.qoSs[0]
        val subackBytes = suback.encode()
        //val subackBytes = encryptMessage(suback.encode())
        sendDataToDevice(UUID_MQTT_PROXY_SERVICE, UUID_MQTT_PROXY_RX, UUID_MQTT_PROXY_RXLARGE, subackBytes)
    }

    private fun sendUnsubAck(unsubscribe: Unsubscribe) {
        if (!isBLEConnected && mBluetoothGatt != null) {
            Log.e(TAG, "Cannot send Unsub ACK to BLE device because BLE connection state" + " is not connected")
            return
        }
        Log.e(TAG, "Sending Unsub ACK back to device.")
        val unsuback = Unsuback()
        unsuback.type = MQTT_MSG_UNSUBACK
        unsuback.msgID = unsubscribe.msgID
        val unsubackBytes = unsuback.encode()
        //val unsubackBytes = encryptMessage(unsuback.encode())
        sendDataToDevice(UUID_MQTT_PROXY_SERVICE, UUID_MQTT_PROXY_RX, UUID_MQTT_PROXY_RXLARGE, unsubackBytes)
    }

    private fun sendPubAck(publish: Publish) {
        if (!isBLEConnected && mBluetoothGatt != null) {
            Log.e(TAG, "Cannot send PUB ACK to BLE device because BLE connection state" + " is not connected")
            return
        }
        Log.e(TAG, "Sending PUB ACK back to device. MsgID: " + publish.msgID)
        val puback = Puback()
        puback.type = MQTT_MSG_PUBACK
        puback.msgID = publish.msgID
        val pubackBytes = puback.encode()
        //val pubackBytes = encryptMessage(puback.encode())
        sendDataToDevice(UUID_MQTT_PROXY_SERVICE, UUID_MQTT_PROXY_RX, UUID_MQTT_PROXY_RXLARGE, pubackBytes)
    }

    private fun publishToDevice(publish: Publish) {
        if (!isBLEConnected && mBluetoothGatt != null) {
            Log.e(TAG, "Cannot deliver mqtt message to BLE device because BLE connection state" + " is not connected")
            return
        }
        Log.e(TAG, "Sending received mqtt message back to device, topic: " + publish.topic + " payload bytes: " + bytesToHexString(publish.payload) + " MsgID: " + publish.msgID)
        //val publishBytes = encryptMessage(publish.encode())
        val publishBytes = publish.encode()
        Log.e(TAG, "Sending received mqtt message back to device, topic: " + bytesToHexString(publishBytes))
        sendDataToDevice(UUID_MQTT_PROXY_SERVICE, UUID_MQTT_PROXY_RX, UUID_MQTT_PROXY_RXLARGE, publishBytes)
    }

    private fun discoverServices() {
        if (isBLEConnected && mBluetoothGatt != null) {
            sendBleCommand(BleCommand(BleCommand.CommandType.DISCOVER_SERVICES))
        } else {
            Log.e(TAG, "Bluetooth connection state is not connected.")
        }
    }

    private fun setMtu(mtu: Int) {
        if (isBLEConnected && mBluetoothGatt != null) {
            Log.e(TAG, "Setting mtu to: $mtu")
            sendBleCommand(BleCommand(BleCommand.CommandType.REQUEST_MTU, mtu))
        } else {
            Log.e(TAG, "Bluetooth connection state is not connected.")
        }
    }

    private fun getMtu(): Boolean {
        return if (isBLEConnected && mBluetoothGatt != null) {
            Log.e(TAG, "Getting current MTU.")
            sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_DEVICE_MTU, UUID_DEVICE_INFORMATION_SERVICE))
            true
        } else {
            Log.e(TAG, "Bluetooth is not connected.")
            false
        }
    }

    private val brokerEndpoint: Boolean
        private get() = if (isBLEConnected && mBluetoothGatt != null) {
            Log.e(TAG, "Getting broker endpoint.")
            sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_IOT_ENDPOINT, UUID_DEVICE_INFORMATION_SERVICE))
            true
        } else {
            Log.e(TAG, "Bluetooth is not connected.")
            false
        }

    private fun getDeviceVersion(): Boolean {
        return if (isBLEConnected && mBluetoothGatt != null) {
            Log.e(TAG, "Getting ble software version on device.")
            sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_DEVICE_VERSION, UUID_DEVICE_INFORMATION_SERVICE))
            true
        } else {
            Log.e(TAG, "Bluetooth is not connected.")
            false
        }
    }

    private fun getDeviceType(): Boolean {
        return if (isBLEConnected && mBluetoothGatt != null) {
            Log.e(TAG, "Getting device type...")
            sendBleCommand(BleCommand(READ_CHARACTERISTIC, UUID_DEVICE_PLATFORM, UUID_DEVICE_INFORMATION_SERVICE))
            true
        } else {
            Log.e(TAG, "Bluetooth is not connected.")
            false
        }
    }

    private fun getDeviceId(): Boolean {
        return if (isBLEConnected && mBluetoothGatt != null) {
            Log.e(TAG, "Getting device cert id...")
            sendBleCommand(BleCommand(BleCommand.CommandType.READ_CHARACTERISTIC, UUID_DEVICE_ID, UUID_DEVICE_INFORMATION_SERVICE))
            true
        } else {
            Log.e(TAG, "Bluetooth is not connected.")
            false
        }
    }

    private fun sendDataToDevice(service: String, rx: String, rxlarge: String, data: ByteArray?) {
        if (data != null) {
            if (data.size < mMaxPayloadLen) {
                //Edited by alpesh for encryted data send to BLE
                Log.e(TAG, "[SECURITY] : [SEND] : PLAIN TEXT : " + String(data))

                val encryData = encryptMessage(data)
                Log.e(TAG, "[SECURITY] : [SEND] : CIPHER TEXT : " + String(encryData))

                sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, rx, service, encryData))
            } else {
                mTotalPackets = data.size / mMaxPayloadLen + 1
                // Log.e(TAG, "This message is larger than max payload size: $mMaxPayloadLen. Breaking down to $mTotalPackets packets.")
                mPacketCount = 0 //reset packet count
                while (mMaxPayloadLen * mPacketCount <= data.size) {
                    val packet = Arrays.copyOfRange(data, mMaxPayloadLen * mPacketCount, Math.min(data.size, mMaxPayloadLen * mPacketCount + mMaxPayloadLen))
                    mPacketCount++
                    //Log.e(TAG, "Packet #" + mPacketCount + ": " + bytesToHexString(packet))
                    sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, rxlarge, service, packet))
                }
            }
        }
    }

    private fun sendBleCommand(command: BleCommand) {
        if (UUID_MQTT_PROXY_SERVICE == command.serviceUuid) {
            mMqttQueue.add(command)
        } else {
            mNetworkQueue.add(command)
        }
        processBleCommandQueue()
    }

    private fun processBleCommandQueue() {
        try {
            mutex.acquire()
            if (mBleOperationInProgress) {
                //Log.e(TAG, "Ble operation is in progress. mqtt queue: " + mMqttQueue.size + " network queue: " + mNetworkQueue.size)
            } else {
                if (mMqttQueue.peek() == null && mNetworkQueue.peek() == null) {
                    Log.e(TAG, "There's no ble command in the queue.")
                    mBleOperationInProgress = false
                } else {
                    mBleOperationInProgress = true
                    val bleCommand: BleCommand?
                    if (mNetworkQueue.peek() != null && mMqttQueue.peek() != null) {
                        bleCommand = if (rr) {
                            mMqttQueue.poll()
                        } else {
                            mNetworkQueue.poll()
                        }
                        rr = !rr
                    } else if (mNetworkQueue.peek() != null) {
                        bleCommand = mNetworkQueue.poll()
                    } else {
                        bleCommand = mMqttQueue.poll()
                    }
                    //Log.e(TAG, "Processing BLE command: " + bleCommand!!.type + " remaining mqtt queue " + mMqttQueue.size + ", network queue " + mNetworkQueue.size)
                    var commandSent = false
                    when (bleCommand!!.type) {
                        BleCommand.CommandType.WRITE_DESCRIPTOR -> if (writeDescriptor(bleCommand.serviceUuid, bleCommand.characteristicUuid)) {
                            commandSent = true
                        }
                        BleCommand.CommandType.WRITE_CHARACTERISTIC -> if (writeCharacteristic(bleCommand.serviceUuid, bleCommand.characteristicUuid, bleCommand.data)) {
                            commandSent = true
                        }
                        READ_CHARACTERISTIC -> if (readCharacteristic(bleCommand.serviceUuid, bleCommand.characteristicUuid)) {
                            commandSent = true
                        }
                        BleCommand.CommandType.DISCOVER_SERVICES -> if (mBluetoothGatt!!.discoverServices()) {
                            commandSent = true
                        } else {
                            Log.e(TAG, "Failed to discover services!")
                        }
                        BleCommand.CommandType.REQUEST_MTU -> if (mBluetoothGatt!!.requestMtu(ByteBuffer.wrap(bleCommand.data).int)) {
                            commandSent = true
                        } else {
                            Log.e(TAG, "Failed to set MTU.")
                        }
                        else -> Log.e(TAG, "Unknown Ble command, cannot process.")
                    }
                    if (commandSent) {
                        mHandler!!.postDelayed(resetOperationInProgress, BLE_COMMAND_TIMEOUT.toLong())
                    } else {
                        mHandler!!.post(resetOperationInProgress)
                    }
                }
            }
            mutex.release()
        } catch (e: InterruptedException) {
            Log.e(TAG, "Mutex error", e)
        }
    }

    private val resetOperationInProgress = Runnable {
        Log.e(TAG, "Ble command failed to be sent OR timeout after " + BLE_COMMAND_TIMEOUT + "ms")
        // If current ble command timed out, process the next ble command.
        if (mBluetoothDevice.bondState != BluetoothDevice.BOND_BONDING) {
            processNextBleCommand()
        }
    }

    private fun processNextBleCommand() {
        mHandler!!.removeCallbacks(resetOperationInProgress)
        mBleOperationInProgress = false
        processBleCommandQueue()
    }

    private fun writeDescriptor(serviceUuid: String, characteristicUuid: String): Boolean {
        val characteristic = getCharacteristic(serviceUuid, characteristicUuid)
        if (characteristic != null) {
            mBluetoothGatt!!.setCharacteristicNotification(characteristic, true)
            val descriptor = characteristic.getDescriptor(convertFromInteger(0x2902))
            if (descriptor != null) {
                descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                mBluetoothGatt!!.writeDescriptor(descriptor)
                return true
            } else {
                Log.e(TAG, "There's no such descriptor on characteristic: $characteristicUuid")
            }
        }
        return false
    }

    private fun writeCharacteristic(serviceUuid: String, characteristicUuid: String, value: ByteArray): Boolean {
        val characteristic = getCharacteristic(serviceUuid, characteristicUuid)
        if (characteristic != null) {
            characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
            // Log.e(TAG, "<-<-<- Writing to characteristic: " + uuidToName[characteristicUuid] + "  with data: " + bytesToHexString(value))
            mValueWritten = value
            characteristic.value = value
            if (!mBluetoothGatt!!.writeCharacteristic(characteristic)) {
                mRWinProgress = false
                Log.e(TAG, "Failed to write characteristic.")
            } else {
                mRWinProgress = true
                return true
            }
        }
        return false
    }

    private fun getCharacteristic(serviceUuid: String, characteristicUuid: String): BluetoothGattCharacteristic? {
        val service = mBluetoothGatt!!.getService(UUID.fromString(serviceUuid))
        if (service == null) {
            Log.e(TAG, "There's no such service found with uuid: $serviceUuid")
            return null
        }
        val characteristic = service.getCharacteristic(UUID.fromString(characteristicUuid))
        if (characteristic == null) {
            Log.e(TAG, "There's no such characteristic with uuid: $characteristicUuid")
            return null
        }
        return characteristic
    }

    private fun readCharacteristic(serviceUuid: String, characteristicUuid: String): Boolean {
        val characteristic = getCharacteristic(serviceUuid, characteristicUuid)
        if (characteristic != null) {
            //Log.e(TAG, "<-<-<- Reading from characteristic: " + uuidToName[characteristicUuid])
            if (!mBluetoothGatt!!.readCharacteristic(characteristic)) {
                mRWinProgress = false
                Log.e(TAG, "Failed to read characteristic.")
            } else {
                mRWinProgress = true
                return true
            }
        }
        return false
    }

    override fun equals(obj: Any?): Boolean {
        if (this === obj) return true
        if (obj == null || javaClass != obj.javaClass) return false
        val aDevice = obj as AmazonFreeRTOSDevice
        return aDevice.mBluetoothDevice.address == mBluetoothDevice.address
    }


    private fun createECDSAKeyPairs() {
        try {
            val generator = KeyPairGenerator.getInstance("EC")
            generator.initialize(ECGenParameterSpec("prime256v1"))
            val keyPair = generator.generateKeyPair()
            privateKeyObj = keyPair.private
            publicKeyObj = keyPair.public
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
/*
    private fun createKeyPairs() {
        try {
            messageBytes = hexToByteArray("01020304050607080901020304050607")
            Log.e(TAG, "Hex data " + byteArrayToHex(messageBytes!!))
            val generator = KeyPairGenerator.getInstance("EC")
            generator.initialize(ECGenParameterSpec("prime256v1"))
            val keyPair = generator.generateKeyPair()
            privateKeyObj = keyPair.private
            publicKeyObj = keyPair.public
            val signature = Signature.getInstance("SHA256withECDSA")
            signature.initSign(privateKeyObj)
            signature.update(messageBytes)
            gatewaySignature = signature.sign()
            signature.initVerify(publicKeyObj)
            signature.update(messageBytes) // <-- Here
            publicKeyByte = publicKeyObj!!.encoded
            privateKey = privateKeyObj!!.encoded
            publicKeyByte = Arrays.copyOfRange(publicKeyByte, 26, publicKeyByte!!.size);
            val isVerified = signature.verify(gatewaySignature)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
*/

    private fun verifyData(message: ByteArray, signatureData: ByteArray, publicKey: ByteArray): Boolean {
        try {
            val s = Signature.getInstance("SHA256withECDSA")
            s.initVerify(rertivePublicKey(publicKey))
            s.update(message)
            val isVerified = s.verify(signatureData)
            //Log.e(TAG, "Sign data streams verifyData \nMessage  ${bytesToHexString(message)} \n " + "signatureData ${bytesToHexString(signatureData)} " + "public key  ${bytesToHexString(publicKey)} " + "\nIs verified $isVerified")
            return isVerified
        } catch (e: Exception) {
            e.printStackTrace()
            return false
        }
    }

    private fun generateECDHPublicKey() {
        try {
            val kpg = KeyPairGenerator.getInstance("EC")
            kpg.initialize(256)
            val kp = kpg.generateKeyPair()
            val ka = KeyAgreement.getInstance("ECDH") //Sign
            ka.init(kp.private)
            gateWayECDHPublicKey = kp.public.encoded
            gateWayECDHPrivateKey = kp.private
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun signRandomNumber(message: ByteArray): ByteArray? {
        try {
            val signature = Signature.getInstance("SHA256withECDSA")
            signature.initSign(privateKeyObj)
            signature.update(message)
            gatewaySignature = signature.sign()
            signature.initVerify(publicKeyObj)
            signature.update(message) // <-- Here
            //Signature remove 2 bytes 30 45
            //Public key 0-25 fix
            val isVerified = signature.verify(gatewaySignature)
            //Log.e(TAG, "Sign data streams \nPrivate key  ${bytesToHexString(privateKeyObj!!.encoded)} \n " + "Message ${bytesToHexString(message)} \n" + "Signature ${bytesToHexString(gatewaySignature!!)} \n" + "Is verified $isVerified")

            return gatewaySignature
        } catch (e: Exception) {
            e.printStackTrace()
            return null
        }
    }

    private fun generateECDHSignature(): ByteArray? {
        try {
            var keyToSign = bytesToHexString(gateWayECDHPublicKey!!)
            var keyInChunks = convertHexToChunk(keyToSign, 2)
            var keyWithoutHeader = keyInChunks.subList(26, keyInChunks.size)
            //Log.e(TAG, "Generated ECDH key without header ${removeArrayData(keyWithoutHeader)}")
            val signature = Signature.getInstance("SHA256withECDSA")
            signature.initSign(privateKeyObj)
            signature.update(hexToByteArray(removeArrayData(keyWithoutHeader))) //Message = public key ECDH Key pair public key
            return signature.sign()
        } catch (e: Exception) {
            e.printStackTrace()
            return null
        }
    }


    /***
     * Messabge : ble at the time of ecdh 0x05 read   public key
     * signature : ble ecdh signature
     * publickey : ble simple public key (ECC Algo public key)
     */
    private fun verifyECDH(message: ByteArray, signatureData: ByteArray, publicKey: ByteArray): Boolean {
        try {
            val s = Signature.getInstance("SHA256withECDSA")
            s.initVerify(rertivePublicKey(publicKey))
            s.update(message)
            val isVerified = s.verify(signatureData)
            //Log.e(TAG, "verifyECDH  \nMessage ${bytesToHexString(message)} " + "\nsignatureData ${bytesToHexString(signatureData)} " + "\npublic key  ${bytesToHexString(publicKey)} " + "\nIs verified $isVerified")
            if (isVerified) {
                Log.e(TAG, "[Mutual Auth] : ECDH Signature verified successfully")

                sendBleCommand(BleCommand(BleCommand.CommandType.READ_CHARACTERISTIC, UUID_CHAR_GATE_WAY_STATUS, MUTUAL_AUTH_SERVICE))
                /*sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_CHAR_EDGE_STATUS, MUTUAL_AUTH_SERVICE, ecdhSignatureVerificationPass()))
                generateECDHSecreat(bleECDHPublicKeyByteArray!!)*/
            } else {
                Log.e(TAG, "[Mutual Auth] : ECDH Signature verification fail")
                mBleConnectionStatusCallback!!.onFail("ECDH Signature verification fail", ERROR_CODE_ECDH_FAIL)
                sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_CHAR_EDGE_STATUS, MUTUAL_AUTH_SERVICE, ecdhSignatureVerificationFail()))
            }
            return isVerified
        } catch (e: Exception) {
            e.printStackTrace()
            return false
        }
    }

    private fun generateECDHSecreat(bleECDHPublicKey: ByteArray) { //Ble ECDH public key with header
        //https://neilmadden.blog/2016/05/20/ephemeral-elliptic-curve-diffie-hellman-key-agreement-in-java/
        try {
            val kf = KeyFactory.getInstance("EC")
            val pkSpec = X509EncodedKeySpec(bleECDHPublicKey)
            val otherPublicKey = kf.generatePublic(pkSpec)
            val ka = KeyAgreement.getInstance("ECDH") //Sign
            ka.init(gateWayECDHPrivateKey)
            ka.doPhase(otherPublicKey, true)
            val sharedSecret = ka.generateSecret()
            gatewayECDHSecreatWithoutSHA = sharedSecret
            sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_CHAR_EDGE_STATUS, MUTUAL_AUTH_SERVICE, ecdhSessionKeyGenrated()))
            resetVariables()
            currentParam = 7
            indicateParamValue(7)
            gatewayECDHSecreat = convertSecretToSHA256(sharedSecret)!!
            sendSecretInFrame(gatewayECDHSecreat)
            //startMQTTProcess()
            //Log.e(TAG, "ECDH Secret ${bytesToHexString(sharedSecret)} SHA256 $" + convertSecretToSHA256(sharedSecret))
        } catch (e: Exception) {
            e.printStackTrace()
            Log.e(TAG, "Exception caught in ECDHInit function")
        }
    }//Set for true

    private fun sendSecretInFrame(convertSecretToSHA256: String?) {
        secreatReadCounter = 0
        val secretFrame = BleCommandTransfer.generateDataFrameInHEX(BleCommandTransfer.hexToByteArray(convertSecretToSHA256!!))
        val secretDataChunks = BleCommandTransfer.convertHexToChunk(secretFrame, 2)
        val firstPassData = removeArrayData(secretDataChunks.subList(0, 20))
        val secondPassData = removeArrayData(secretDataChunks.subList(20, secretDataChunks.size))
        //Log.e(TAG, "First pass data $firstPassData} \nSecond pass data $secondPassData")
        sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, hexToByteArray(firstPassData)))
        sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_CHARACTERISTIC, UUID_WRITE_DATA_PARAM_VALUE_SERVICE, MUTUAL_AUTH_SERVICE, hexToByteArray(secondPassData)))
        readParamValue(7)
    }

    private fun startMQTTProcess() {
        getDeviceType()
        getDeviceId()
        getMtu()
        sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_DESCRIPTOR, UUID_MQTT_PROXY_TX, UUID_MQTT_PROXY_SERVICE))
        sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_DESCRIPTOR, UUID_MQTT_PROXY_TXLARGE, UUID_MQTT_PROXY_SERVICE))
        sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_DESCRIPTOR, UUID_NETWORK_TX, UUID_NETWORK_SERVICE))
        sendBleCommand(BleCommand(BleCommand.CommandType.WRITE_DESCRIPTOR, UUID_NETWORK_TXLARGE, UUID_NETWORK_SERVICE))
    }

    private fun resetVariables() {
        publicKeyPass = -1
        devicePrivateKey.clear()
        deviceECDHPrivateKey.clear()
        dataToSendInChunks.clear()
        totalDataSize = 0
        receivedData = 2
        startLimit = 0
        endLimit = 19
        remainingLength = 0

    }


    private fun rertivePublicKey(publicKey: ByteArray?): PublicKey? {
        try {
            return KeyFactory.getInstance("EC").generatePublic(X509EncodedKeySpec(publicKey))
        } catch (e: InvalidKeySpecException) {
            e.printStackTrace()
        } catch (e: Exception) {
            e.printStackTrace()
        }
        return null
    }

    val iv: ByteArray = byteArrayOf(0x65.toByte(), 0x69.toByte(), 0x6e.toByte(), 0x66.toByte(), 0x6f.toByte(), 0x63.toByte(), 0x68.toByte(), 0x69.toByte(), 0x70.toByte(), 0x73.toByte(), 0x5f.toByte(), 0x61.toByte(), 0x72.toByte(), 0x72.toByte(), 0x6f.toByte(), 0x77.toByte())
    fun encryptMessage(message: ByteArray): ByteArray {
        val iv = IvParameterSpec(iv)
        val AESCipher = Cipher.getInstance("AES/CFB8/NoPadding")
        AESCipher.init(Cipher.ENCRYPT_MODE, getSecretKey(gatewayECDHSecreatWithoutSHA!!), iv)
        return AESCipher.doFinal(message)
    }

    val secreat = "000102030405060708090a0b0c0d0e0f000102030405060708090a0b0c0d0e0f" //set this for static
    fun encryptMessageForTest(message: ByteArray): ByteArray {
        val iv = IvParameterSpec(iv)
        val AESCipher = Cipher.getInstance("AES/CFB8/NoPadding")
        AESCipher.init(Cipher.ENCRYPT_MODE, getSecretKey(hexToByteArray(secreat)!!), iv)
        return AESCipher.doFinal(message)
    }

    private fun decrypt(ciphertext: ByteArray?): ByteArray? {
        val iv = IvParameterSpec(iv)
        val AESCipher = Cipher.getInstance("AES/CFB8/NoPadding")
        AESCipher.init(Cipher.DECRYPT_MODE, getSecretKey(gatewayECDHSecreatWithoutSHA!!), iv)
        return AESCipher.doFinal(ciphertext)
    }


    private fun getSecretKey(byteData: ByteArray?): SecretKeySpec? {
        return SecretKeySpec(byteData, "AES")
    }

    fun generateSecretStatic(): SecretKeySpec? {
        return getSecreatKey(byteArrayOf(0x0.toByte(), 0x1.toByte(), 0x2.toByte(), 0x3.toByte(), 0x4.toByte(), 0x5.toByte(), 0x6.toByte(), 0x7.toByte(), 0x8.toByte(), 0x9.toByte(), 0xa.toByte(), 0xb.toByte(), 0xc.toByte(), 0xd.toByte(), 0xe.toByte(), 0xf.toByte(), 0x0.toByte(), 0x1.toByte(), 0x2.toByte(), 0x3.toByte(), 0x4.toByte(), 0x5.toByte(), 0x6.toByte(), 0x7.toByte(), 0x8.toByte(), 0x9.toByte(), 0xa.toByte(), 0xb.toByte(), 0xc.toByte(), 0xd.toByte(), 0xe.toByte(), 0xf.toByte()))
    }

    private fun decryptStatic(ciphertext: ByteArray?): ByteArray? {
        val iv = IvParameterSpec(getIVStatic())
        val AESCipher = Cipher.getInstance("AES/CFB8/NoPadding")
        AESCipher.init(Cipher.DECRYPT_MODE, generateSecretStatic(), iv)
        return AESCipher.doFinal(ciphertext)
    }


    fun getSecreatKey(hex: ByteArray?): SecretKeySpec? {
        return SecretKeySpec(hex, "AES")
    }

    fun getIVStatic(): ByteArray? {
        return hexToByteArray("000102030405060708090a0b0c0d0e0f") //random number first time gateway without crc and length
    }

    companion object {
        private const val TAG = "FRD"
        private const val VDBG = false
        private fun describeGattServices(gattServices: List<BluetoothGattService>) {
            for (service in gattServices) {
                Log.e(TAG, "GattService: " + service.uuid)
                val characteristics = service.characteristics
                for (characteristic in characteristics) {
                    Log.e(TAG, " |-characteristics: " + if (uuidToName.containsKey(characteristic.uuid.toString())) uuidToName[characteristic.uuid.toString()] else characteristic.uuid)
                }
            }
        }

        private fun convertFromInteger(i: Int): UUID {
            val MSB = 0x0000000000001000L
            val LSB = -0x7fffff7fa064cb05L
            val value = (i and (-0x1.toLong()).toInt()).toLong()
            return UUID(MSB or (value shl 32), LSB)
        }

        private fun bytesToHexString(bytes: ByteArray): String {
            val sb = StringBuilder(bytes.size * 2)
            val formatter = Formatter(sb)
            for (i in bytes.indices) {
                formatter.format("%02x", bytes[i])
                /*if (!VDBG && i > 10) {
                    break
                }*/
            }
            return sb.toString()
        }

        private fun bytesToHexStringForLog(bytes: ByteArray): String {
            val sb = StringBuilder(bytes.size * 2)
            val formatter = Formatter(sb)
            for (i in bytes.indices) {
                formatter.format("\n $i   %02x", bytes[i])

                /*if (!VDBG && i > 10) {
                    break
                }*/
            }
            return sb.toString()
        }


    }
    @Throws(IOException::class)
    fun CertificateDataFromFile(devCertStr: String?): ByteArray? {
        // Load CAs from an InputStream
        var cf: CertificateFactory? = null
        cf = try {
            CertificateFactory.getInstance("X.509")
        } catch (e: CertificateException) {
            e.printStackTrace()
            return null
        }
        //get device certificate
        val devCertInput: InputStream = mContext.getAssets().open(devCertStr)
        var devCert: Certificate? = null
        try {
            devCert = try {
                cf.generateCertificate(devCertInput)
            } catch (e: CertificateException) {
                e.printStackTrace()
                return null
            }
            //println("Device cert=" + (devCert as X509Certificate?)!!.subjectDN)
        } finally {
            devCertInput.close()
        }
        var byteArr = devCert?.encoded
        return byteArr
    }
    @Throws(IOException::class)
    fun CertificateVerifyUsingByteArray(array: ByteArray?,rootCAStr:String?): Boolean? {
        // Load CAs from an InputStream
        var cf: CertificateFactory? = null
        cf = try {
            CertificateFactory.getInstance("X.509")
        } catch (e: CertificateException) {
            e.printStackTrace()
            return false
        }
        //get root CA certificate
        val caCertInput: InputStream = mContext.getAssets().open(rootCAStr)
        var caCert: Certificate? = null
        try {
            caCert = try {
                cf.generateCertificate(caCertInput)
            } catch (e: CertificateException) {
                e.printStackTrace()
                return false
            }
            //println("Device cert=" + (devCert as X509Certificate?)!!.subjectDN)
        } finally {
            caCertInput.close()
        }

        //bytes array using get device certificate
        val receInStream: InputStream = ByteArrayInputStream(array)
        var receivedCert: Certificate? = null
        try {
            receivedCert = try {
                cf.generateCertificate(receInStream)
            } catch (e: CertificateException) {
                e.printStackTrace()
                return false
            }
            //println("Device cert=" + (devCert as X509Certificate?)!!.subjectDN)
        } finally {
            receInStream.close()
        }

        if (caCert != null) {
            if (receivedCert != null) {
                try{
                    receivedCert.verify(caCert.publicKey)
                    return true
                }catch (e:Exception){
                    e.printStackTrace()
                    return false
                }

            }
        }

        //var byteArr = devCert?.encoded
        return false
    }
    @Throws(IOException::class)
    fun GetPrivatePublicKey(devCertStr: String?,privateKeyStr:String?) {
        // Load CAs from an InputStream
        var cf: CertificateFactory? = null
        cf = try {
            CertificateFactory.getInstance("X.509")
        } catch (e: CertificateException) {
            e.printStackTrace()
            return
        }
        //get device certificate
        val devCertInput: InputStream = mContext.getAssets().open(devCertStr)
        var devCert: Certificate? = null
        try {
            devCert = try {
                cf.generateCertificate(devCertInput)
            } catch (e: CertificateException) {
                e.printStackTrace()
                return
            }
            //println("Device cert=" + (devCert as X509Certificate?)!!.subjectDN)
        } finally {
            devCertInput.close()
        }
        //get public key from device certificate
        publicKeyObj = devCert?.publicKey

        //get private key from private key certificate
        val priKeyInput: InputStream = mContext.getAssets().open(privateKeyStr)
        var privKeyByteArray=priKeyInput.readBytes()
        val strKey = String(privKeyByteArray)
        val priKey=getPublicKeyFromArray(strKey)
        privateKeyObj=priKey
    }

    @Throws(NoSuchAlgorithmException::class, DecoderException::class)
    fun getPublicKeyFromArray(praviteKeyStr: String): PrivateKey? {
        val pkcs8Lines = java.lang.StringBuilder()
        val rdr = BufferedReader(StringReader(praviteKeyStr))
        var line: String?=""
        while (rdr.readLine().also({ line = it }) != null) {
            pkcs8Lines.append(line)
        }
        // Remove the "BEGIN" and "END" lines, as well as any whitespace
        var pkcs8Pem = pkcs8Lines.toString()
        pkcs8Pem = pkcs8Pem.replace("-----BEGIN PRIVATE KEY-----", "")
        pkcs8Pem = pkcs8Pem.replace("-----END PRIVATE KEY-----", "")
        pkcs8Pem = pkcs8Pem.replace("\\s+".toRegex(), "")
        pkcs8Pem = pkcs8Pem.replace("\\t+".toRegex(), "")
        pkcs8Pem = pkcs8Pem.replace("\\n+".toRegex(), "")

        // Base64 decode the result
        val pkcs8EncodedBytes: ByteArray = android.util.Base64.decode(pkcs8Pem, android.util.Base64.DEFAULT)
        var privKey:PrivateKey?=null
        try{
        // extract the private key
            val keySpec = PKCS8EncodedKeySpec(pkcs8EncodedBytes)
            val kf = KeyFactory.getInstance("EC")
            privKey = kf.generatePrivate(keySpec)
        }catch (e:java.lang.Exception){
            Log.e("ERROR",e.message.toString())
        }
        return privKey
    }



}


