import AWSIoT
import CoreBluetooth
import os.log
import Foundation
#if canImport(CryptoKit)
import CryptoKit
#endif
import CommonCrypto
//import AWSMobileClient


protocol LogoutDelegate {
    // Classes that adopt this protocol MUST define
    // this method -- and hopefully do something in
    // that definition.
    func logoutMethod()
}

/// FreeRTOS manager.
public class AmazonFreeRTOSManager: NSObject {

    var logoutDelegate : LogoutDelegate!

    //weak var delegate : LogoutDelegate

    /// Shared instence of FreeRTOS manager.
    public static let shared = AmazonFreeRTOSManager()

    /// Enable debug messages.
    public var isDebug: Bool = false
    /// Debug messages.
    public var debugMessages = String()

    /// Service UUIDs in the advertising packets.
    public var advertisingServiceUUIDs: [CBUUID] = [AmazonFreeRTOSGattService.DeviceInfo]
    /// The FreeRTOS devices using peripheral identifier as key.
    public var devices: [UUID: AmazonFreeRTOSDevice] = [:]

    /// BLE Central Manager of the FreeRTOS manager.
    public var central: CBCentralManager?
    
    public var serviceMain:CBService?
    public var peripheralMain:CBPeripheral?
    public var stateNum:Int?
    public var readPublicKeyDataInStr:String?=""
    public var readRandomNumDataInStr:String?=""
    public var totalPublicKeySize:Int?
    public var totalRandomNumberSize:Int?
    public var publicKeyDataApp:Data?
    public var privateKeyDataApp:Data?
    public var privateKey:P256.Signing.PrivateKey?
    public var publicKey:P256.Signing.PublicKey?
    public var readSignatureDataInStr:String?=""
    public var totalSignatureSize:Int?
    public var signatureMain:P256.Signing.ECDSASignature?
    public var strRandom:String?=""
    
    public var privateKeyECDH:P256.KeyAgreement.PrivateKey?
    public var publicKeyECDH:P256.KeyAgreement.PublicKey?
    public var readECDHPublicKeyInStr:String?=""
    public var totalECDHPubKeySize:Int?
    public var readECDHSignatureInStr:String?=""
    public var totalECDHSignatureSize:Int?
    public var dataPubKeyECDH:Data?
    public var signatureECDH:P256.Signing.ECDSASignature?
    public var readECDHPublicKeyVerifyInStr:String?=""
    public var sharedSecret:SharedSecret?
    public var readHashedInStr:String?=""
    public var totalHashedSize:Int?
    public var hashedDataofSecretKey:Data?
    public var dataSecretKey:Data?
    public var dataIV:Data?
    public let ivStr:String? = "65696e666f63686970735f6172726f77"
    
    public var readDevCertDataInStr:String?=""
    public var totalDevCertSize:Int?
    
    /// Initializes a new FreeRTOS manager.
    ///
    /// - Returns: A new FreeRTOS manager.
    public override init() {
        super.init()
        central = CBCentralManager(delegate: self, queue: nil, options: [CBCentralManagerOptionShowPowerAlertKey: true])
    }
    
    public func PopUpMessage(title:String,message:String){

        let alertView = UIAlertController(title: title, message: message, preferredStyle: .alert)
        let ok = UIAlertAction(title: "OK", style: .default, handler: { action in
            if title == "Error"{
                //logout from app
                self.debugPrint("Exit from app")
                exit(0)
            }else{
                //mqtt service start and check
                self.peripheralMain!.discoverServices([AmazonFreeRTOSGattService.DeviceInfo, AmazonFreeRTOSGattService.MqttProxy, AmazonFreeRTOSGattService.NetworkConfig])
            }
        })
        alertView.addAction(ok)

        UIApplication.shared.keyWindow?.rootViewController?.present(alertView, animated: true, completion: nil)
    }
    public func clearData(){
        stateNum = 1
        readDevCertDataInStr = ""
        totalDevCertSize = 0
        readPublicKeyDataInStr=""
        readRandomNumDataInStr=""
        totalPublicKeySize=0
        totalRandomNumberSize=0
        publicKeyDataApp=nil
        privateKeyDataApp=nil
        privateKey=nil
        publicKey=nil
        readSignatureDataInStr=""
        totalSignatureSize=0
        signatureMain=nil
        privateKeyECDH=nil
        publicKeyECDH=nil
        readECDHPublicKeyInStr=""
        totalECDHPubKeySize=0
        readECDHSignatureInStr=""
        totalECDHSignatureSize=0
        dataPubKeyECDH=nil
        signatureECDH=nil
        readECDHPublicKeyVerifyInStr=""
        sharedSecret=nil
        readHashedInStr=""
        totalHashedSize=0
        hashedDataofSecretKey=nil
    }
}

extension AmazonFreeRTOSManager {

    /// Start scan for FreeRTOS devices.
    ///
    /// - Precondition: `central` is ready and not scanning.
    public func startScanForDevices() {
        debugPrint("[BLE APP] : Start Scan for devices")

        clearData()
        if let central = central, !central.isScanning {
            central.scanForPeripherals(withServices: advertisingServiceUUIDs, options: nil)
        }
    }

    /// Stop scan for FreeRTOS devices.
    ///
    /// - Precondition: `central` is ready and is scanning.
    public func stopScanForDevices() {
        debugPrint("[BLE APP] : Stop Scan for devices")

        if let central = central, central.isScanning {
            central.stopScan()
        }
    }

    /// Disconnect. Clear all contexts. Scan for FreeRTOS devices.
    public func rescanForDevices() {
        stopScanForDevices()

        for device in devices.values {
            device.disconnect()
        }
        devices.removeAll()
        startScanForDevices()
    }

}
extension Data {
    init?(hexString: String) {
        let len = hexString.count / 2
        var data = Data(capacity: len)
        for i in 0..<len {
            let j = hexString.index(hexString.startIndex, offsetBy: i*2)
            let k = hexString.index(j, offsetBy: 2)
            let bytes = hexString[j..<k]
            if var num = UInt8(bytes, radix: 16) {
                data.append(&num, count: 1)
            } else {
                return nil
            }
        }
        self = data
    }
}

extension AmazonFreeRTOSManager: CBCentralManagerDelegate {

    /// CBCentralManagerDelegate
    public func centralManagerDidUpdateState(_ central: CBCentralManager) {
        NotificationCenter.default.post(name: .afrCentralManagerDidUpdateState, object: nil)
        debugPrint("[Central] afrCentralManagerDidUpdateState: \(central.state.rawValue)")
    }

    /// CBCentralManagerDelegate
    public func centralManager(_: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String: Any], rssi RSSI: NSNumber) {
        debugPrint("[BLE APP] : Discovered BLE devices")

        if !devices.keys.contains(peripheral.identifier) {
            devices[peripheral.identifier] = AmazonFreeRTOSDevice(peripheral: peripheral)
        }
        devices[peripheral.identifier]?.advertisementData = advertisementData
        devices[peripheral.identifier]?.RSSI = RSSI
        NotificationCenter.default.post(name: .afrCentralManagerDidDiscoverDevice, object: nil, userInfo: ["identifier": peripheral.identifier])
        debugPrint("[Central] afrCentralManagerDidDiscoverPeripheral: \(peripheral.identifier.uuidString)")
    }

    /// CBCentralManagerDelegate
    public func centralManager(_: CBCentralManager, didConnect peripheral: CBPeripheral) {
        devices[peripheral.identifier]?.reset()
        peripheral.delegate = self
        //peripheral.discoverServices([AmazonFreeRTOSGattService.DeviceInfo, AmazonFreeRTOSGattService.MqttProxy, AmazonFreeRTOSGattService.NetworkConfig])
        peripheral.discoverServices([AmazonFreeRTOSGattService.MutualAuthService])
        NotificationCenter.default.post(name: .afrCentralManagerDidConnectDevice, object: nil, userInfo: ["identifier": peripheral.identifier])
        //debugPrint("[\(peripheral.identifier.uuidString)] afrCentralManagerDidConnectPeripheral")
        debugPrint("[BLE APP] : Connected BLE device")
    }

    /// CBCentralManagerDelegate
    public func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        devices[peripheral.identifier]?.reset()
        if devices[peripheral.identifier]?.reconnect ?? false {
            central.connect(peripheral, options: nil)
        }
        NotificationCenter.default.post(name: .afrCentralManagerDidDisconnectDevice, object: nil, userInfo: ["identifier": peripheral.identifier])
        if let error = error {
            debugPrint("[\(peripheral.identifier.uuidString)][ERROR] afrCentralManagerDidDisconnectPeripheral: \(error.localizedDescription)")
            return
        }
        debugPrint("[BLE APP] : Disconnect BLE device")

        //debugPrint("[\(peripheral.identifier.uuidString)] afrCentralManagerDidDisconnectPeripheral")
    }

    /// CBCentralManagerDelegate
    public func centralManager(_: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        debugPrint("[BLE APP] : Fail to connect BLE device")

        NotificationCenter.default.post(name: .afrCentralManagerDidFailToConnectDevice, object: nil, userInfo: ["identifier": peripheral.identifier])
        if let error = error {
            debugPrint("[\(peripheral.identifier.uuidString)][ERROR] afrCentralManagerDidFailToConnectPeripheral: \(error.localizedDescription)")
            return
        }
        //debugPrint("[\(peripheral.identifier.uuidString)] afrCentralManagerDidFailToConnectPeripheral")
    }
}


func crc16CCITTFalse(data: Data) -> UInt16 {
    let xor: UInt16 = 0x0000
    var crc: UInt16 = 0xFFFF
    let polynomial: UInt16 = 0x1021

    
    for byte in data {
        crc = (UInt16(byte) << 8) ^ crc;
        for _ in 0 ..< 8 {
            let check = crc & 0x8000
            
            if check != 0 {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc = crc << 1;
            }
        }
    }
    crc = crc & 0xffff
    return (crc ^ xor);
}

extension AmazonFreeRTOSManager: CBPeripheralDelegate {

    /// CBPeripheralDelegate
    public func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        //debugPrint("[\(peripheral.identifier.uuidString)] : didDiscoverServices")
        debugPrint("[BLE APP] : Discovered BLE Sevices")

        if let error = error {
            debugPrint("[\(peripheral.identifier.uuidString)][ERROR] afrPeripheralDidDiscoverServices: \(error.localizedDescription)")
            return
        }
        peripheralMain=peripheral
        for service in peripheral.services ?? [] {
            if service.uuid == AmazonFreeRTOSGattService.MutualAuthService{
                serviceMain=service
            }
            peripheral.discoverCharacteristics(nil, for: service)
        }
        NotificationCenter.default.post(name: .afrPeripheralDidDiscoverServices, object: nil, userInfo: ["peripheral": peripheral])
    }
    
    public func WriteToBle(peripheral:CBPeripheral,service:CBService, state:Int){
       
        //WriteDataIndication characteristic using send data to ble
               let characteristicWriteDataIndication = service.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.MutualAuthChar_WriteDataIndication);
               
               if characteristicWriteDataIndication == nil{
                   debugPrint("[\(peripheral.identifier.uuidString)][ERROR] WriteDataIndication characteristic doesn't exist")
                   return
               }else {
                    //debugPrint(" Write to Data indication")
                    var value = "01"
                    if state == 1 {
                        value = "01"
                    }else if state == 2 {
                        value = "02"
                    }else if state == 3 {
                        value = "03"
                    }else if state == 4 {
                        value = "04"
                    }else if state == 5 {
                        value = "05"
                    }else if state == 6 {
                        value = "06"
                    }else if state == 7 {
                        value = "07"
                    }
                    let data = Data(hexString: value)
                    peripheral.writeValue(data!, for: characteristicWriteDataIndication!, type: .withResponse)
               }
               
               var dataMain:Data?
               //WriteDataParamValue characteristic using send data to ble
               let characteristicWriteDataParams = service.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.MutualAuthChar_WriteDataParamValue);
               
               if characteristicWriteDataParams == nil{
                   debugPrint("[\(peripheral.identifier.uuidString)][ERROR] characteristicWriteDataParams characteristic doesn't exist")
                   return
               }else {
                   //debugPrint("Write to Data param value")
                    switch state {
                    case 1:
                        //public certificate send to BLE
                        let dataPub =  DeviceCertificateCertData(devCertStr: "IOSAppCert")
                        dataMain =  dataPub // Publish the device certificate
                        break
                    case 2:
                        //public key send to BLE
                        privateKey = P256.Signing.PrivateKey()// Keep the private key safe
                        privateKeyDataApp = privateKey?.rawRepresentation
                        publicKey = privateKey?.publicKey
                        publicKeyDataApp = publicKey?.rawRepresentation
                        let strMainData = publicKeyDataApp!.hexEncodedString()
                        let dataPub =  Data(hexString: String("04"+strMainData))!
                        dataMain =  dataPub // Publish the public key
                        break
                    case 3:
                        // random number generate
                        strRandom = random(digits: 32)
                        dataMain = Data(hexString: strRandom!)!
                        break
                    case 4:
                        // signature of random number
                        if let data = Data(hexString: readRandomNumDataInStr!),
                            let signature:P256.Signing.ECDSASignature? = try? privateKey?.signature(for: data) {
                            let dataMainTemp = signature?.derRepresentation
                            let strMainData = dataMainTemp!.hexEncodedString()
                            let start = strMainData.index(strMainData.startIndex, offsetBy: 4)
                            let end = strMainData.index(strMainData.endIndex, offsetBy: 0)
                            let range = start..<end
                            let dataStr = strMainData[range]
                            dataMain =  Data(hexString: String(dataStr))
                            signatureMain=signature
                         }
                        break
                    case 5:
                        privateKeyECDH = P256.KeyAgreement.PrivateKey()// Keep the private key safe
                        publicKeyECDH = privateKeyECDH?.publicKey
                        let strMainData = publicKeyECDH?.rawRepresentation.hexEncodedString()
                        dataPubKeyECDH =  Data(hexString: String("04"+strMainData!))!
                        dataMain =  dataPubKeyECDH // Publish the public key
                        break
                    case 6:
                        if let data = dataPubKeyECDH,
                         let signature:P256.Signing.ECDSASignature? = try? privateKey?.signature(for: data) {
                           let dataMainTemp = signature?.derRepresentation
                           let strMainData = dataMainTemp!.hexEncodedString()
                           let start = strMainData.index(strMainData.startIndex, offsetBy: 4)
                           let end = strMainData.index(strMainData.endIndex, offsetBy: 0)
                           let range = start..<end
                           let dataStr = strMainData[range]
                           dataMain =  Data(hexString: String(dataStr))
                           signatureECDH=signature
                        }
                        break
                    case 7:
                        let strSecret=sharedSecret!.description
                            let start = strSecret.index(strSecret.startIndex, offsetBy: 14)
                            let end = strSecret.index(strSecret.endIndex, offsetBy: 0)
                            let range = start..<end
                            let strSecretForData = strSecret[range]
                            dataSecretKey =  Data(hexString: String(strSecretForData))
                            dataIV = Data(hexString: String(ivStr!))
                            //let shareSecretData =  Data(hexString: String(strSecret))

                            let hashed = SHA256.hash(data: dataSecretKey!)
                            let strHashed=hashed.description
                            let start1 = strHashed.index(strHashed.startIndex, offsetBy: 15)
                            let end1 = strHashed.index(strHashed.endIndex, offsetBy: 0)
                            let range1 = start1..<end1
                            let strHashedForData = strHashed[range1]
                            hashedDataofSecretKey =  Data(hexString: String(strHashedForData))
                            dataMain =  hashedDataofSecretKey// Publish the public key
                        break
                    default:
                        return
                    }
                  
                   //debugPrint("[\(dataMain!.count)] : data payload size")
                   
                   let dataLen = dataMain!.count+4;
                   
                   let crc = crc16CCITTFalse(data:dataMain!)
                   let crcHexStr=String(format: "%04X", crc)
                   //debugPrint("[\(crcHexStr)] : CRC Data str")

                   let crcData = Data(hexString: crcHexStr)
                   //debugPrint("[\(crcData)] : CRC Data")
                   
                   let len = String(format:"%04X", dataLen)
                   let lenData = Data(hexString: len)
                   
                   let strLen = lenData!.hexEncodedString()
                   let strMainData = dataMain!.hexEncodedString()
                   let strCRC = crcData!.hexEncodedString()
                   let finalStr=strLen+strMainData+strCRC;
                   let finData = Data(hexString: finalStr)
                   //debugPrint("[\(finalStr)] : final str")

                   //debugPrint("[\(finData?.count)]: total frame size")
                   
                   let length=finData?.count
                   let chunkSize = 20      // 20 bytes chunk sizes
                   var offset = 0

                   repeat {
                       // get the length of the chunk
                       let thisChunkSize = ((length! - offset) > chunkSize) ? chunkSize : (length! - offset)

                       // get the chunk
                       let chunk = finData!.subdata(in: offset..<offset + thisChunkSize )
                       let chunkStr = chunk.hexEncodedString()
                       //debugPrint("[\(chunkStr)]  chunk data :[\(offset)] : current offset")
                    
                       peripheral.writeValue(chunk, for: characteristicWriteDataParams!, type: .withResponse)

                       // update the offset
                       offset += thisChunkSize;

                   } while (offset < length!);
                
                   switch state {
                   case 1:
                       //device certificate send to BLE
                       debugPrint("[Mutual Auth] : Send device certificate")
                       break
                   case 2:
                       //public key send to BLE
                       debugPrint("[Mutual Auth] : Send public key")
                       break
                   case 3:
                       // random number generate
                       debugPrint("[Mutual Auth] : Send random numer")
                       break
                   case 4:
                       // signature of random number
                       debugPrint("[Mutual Auth] : Send signature")
                       break
                   case 5:
                        debugPrint("[Mutual Auth] : Send ECDH public key")
                       break
                   case 6:
                        debugPrint("[Mutual Auth] : Send ECDH signature")
                       break
                   case 7:
                        debugPrint("[Mutual Auth] : Send Hash of secret key")
                       break
                   default:
                       return
                   }
                   //peripheral.writeValue(finData!, for: characteristic, type: .withResponse)
               }
               
               //Read edge params value characteristic using read data from ble
               let characteristicReadEdgeValue = service.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.MutualAuthChar_EdgeParamValue);
               
               if characteristicReadEdgeValue == nil{
                   debugPrint("[\(peripheral.identifier.uuidString)][ERROR] characteristicReadEdgeValue characteristic doesn't exist")
                   return
               }else {
                   //debugPrint("[\(service.uuid)] : for read  MutualAuthChar_WriteDataParamValue")
                   peripheral.readValue(for: characteristicReadEdgeValue!)
                   //peripheral.setNotifyValue(true, for: characteristicReadEdgeValue!)
               }
    }
    public func peripheral(_ peripheral: CBPeripheral, didUpdateNotificationStateFor characteristic: CBCharacteristic, error: Error?) {
        
        if (characteristic.uuid == AmazonFreeRTOSGattCharacteristic.MutualAuthChar_CharGatewayStatus){
            let edgeValue = characteristic.value!.hexEncodedString()
            let msg = DisplayErrorMessage(value:edgeValue)
            if(msg != ""){
                //debugPrint("[\(characteristic.uuid)] :[\(edgeValue)] Notification event of didUpdateNotificationStateFor")
                PopUpMessage(title:"Error",message:"Mutual Authentication Fail : "+msg)
            }
        }
        
    }
    /// CBPeripheralDelegate
    public func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        //debugPrint("[\(service.uuid)] : didDiscoverCharacteristicsFor")
        peripheralMain=peripheral
        
        if service.uuid == AmazonFreeRTOSGattService.MutualAuthService{
             serviceMain=service
                if(stateNum != 8){
                    // for pubblic key send to ble device
                    stateNum = 1

                    NotifyFromBle(peripheral:peripheral,service:service)
                    
                    WriteToBle(peripheral:peripheral,service:service, state:stateNum!)
                }
        }
        

       for characteristic in service.characteristics ?? [] {
           peripheral.setNotifyValue(true, for: characteristic)
       }
        
        if let error = error {
            debugPrint("[\(peripheral.identifier.uuidString)][ERROR] afrPeripheralDidDiscoverCharacteristics: \(error.localizedDescription)")
            return
        }
        
    
        switch service.uuid {

        case AmazonFreeRTOSGattService.MutualAuthService: break
            //debugPrint("[\(service.uuid)] : didDiscoverCharacteristicsFor MutualAuthService")

        case AmazonFreeRTOSGattService.DeviceInfo:
            devices[peripheral.identifier]?.getAfrVersion()
            devices[peripheral.identifier]?.getBrokerEndpoint()
            devices[peripheral.identifier]?.getMtu()
            devices[peripheral.identifier]?.getAfrPlatform()
            devices[peripheral.identifier]?.getAfrDevId()

        case AmazonFreeRTOSGattService.MqttProxy:
            guard let characteristic = service.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.MqttProxyControl) else {
                debugPrint("[\(peripheral.identifier.uuidString)][ERROR] MqttServiceState characteristic doesn't exist")
                return
            }
            devices[peripheral.identifier]?.peripheral.writeValue(Data([1]), for: characteristic, type: .withResponse)

        case AmazonFreeRTOSGattService.NetworkConfig:
            guard let characteristic = service.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.NetworkConfigControl) else {
                debugPrint("[\(peripheral.identifier.uuidString)][ERROR] NetworkServiceState characteristic doesn't exist")
                return
            }
            devices[peripheral.identifier]?.peripheral.writeValue(Data([1]), for: characteristic, type: .withResponse)
        
   
        default:
            return
        }

        NotificationCenter.default.post(name: .afrPeripheralDidDiscoverCharacteristics, object: nil, userInfo: ["peripheral": peripheral, "service": service])
    }

    /// CBPeripheralDelegate
    public func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        //debugPrint("[\(characteristic.uuid)] : didUpdateValueFor")

        if let error = error {
            debugPrint("[\(peripheral.identifier.uuidString)][ERROR] afrPeripheralDidUpdateValueForCharacteristic: \(error.localizedDescription)")
            return
        }

        
        switch characteristic.uuid {
            
        case AmazonFreeRTOSGattCharacteristic.MutualAuthChar_WriteDataIndication:
            return;
            
        case AmazonFreeRTOSGattCharacteristic.MutualAuthChar_WriteDataParamValue:
            return;

        case AmazonFreeRTOSGattCharacteristic.MutualAuthChar_EdgeParamValue:
            //debugPrint("[\(characteristic.uuid)] : didUpdateValueFor MutualAuthChar_EdgeParamValue")
            didUpdateValueForEdgeValue(peripheral: peripheral, characteristic: characteristic)
            return;

        case AmazonFreeRTOSGattCharacteristic.MutualAuthChar_CharGatewayStatus:
            return;

        case AmazonFreeRTOSGattCharacteristic.AfrVersion:
            didUpdateValueForAfrVersion(peripheral: peripheral, characteristic: characteristic)

        case AmazonFreeRTOSGattCharacteristic.BrokerEndpoint:
            didUpdateValueForBrokerEndpoint(peripheral: peripheral, characteristic: characteristic)

        case AmazonFreeRTOSGattCharacteristic.Mtu:
            didUpdateValueForMtu(peripheral: peripheral, characteristic: characteristic)

        case AmazonFreeRTOSGattCharacteristic.AfrPlatform:
            didUpdateValueForAfrPlatform(peripheral: peripheral, characteristic: characteristic)

        case AmazonFreeRTOSGattCharacteristic.AfrDevId:
            didUpdateValueForAfrDevId(peripheral: peripheral, characteristic: characteristic)

        case AmazonFreeRTOSGattCharacteristic.TXMqttMessage, AmazonFreeRTOSGattCharacteristic.TXNetworkMessage:
            didUpdateValueForTXMessage(peripheral: peripheral, characteristic: characteristic, data: nil)

        case AmazonFreeRTOSGattCharacteristic.TXLargeMqttMessage, AmazonFreeRTOSGattCharacteristic.TXLargeNetworkMessage:
            didUpdateValueForTXLargeMessage(peripheral: peripheral, characteristic: characteristic)

        default:
            return
        }
    }

    /// CBPeripheralDelegate
    public func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
        //debugPrint("[\(characteristic.uuid)] : didWriteValueFor")
        if let error = error {
            debugPrint("[\(peripheral.identifier.uuidString)][ERROR] afrPeripheralDidWriteValueForCharacteristic: \(error.localizedDescription)")
            return
        }
        
        switch characteristic.uuid {

        case AmazonFreeRTOSGattCharacteristic.MqttProxyControl:
            //debugPrint("[\(characteristic.uuid)] : MqttProxyControl ")
            return
           //devices[peripheral.identifier]?.peripheral.writeValue(Data([1]), for: characteristic, type: .withResponse)
             
        case AmazonFreeRTOSGattCharacteristic.RXLargeMqttMessage, AmazonFreeRTOSGattCharacteristic.RXLargeNetworkMessage:
            writeValueToRXLargeMessage(peripheral: peripheral, characteristic: characteristic)

        case AmazonFreeRTOSGattCharacteristic.MutualAuthChar_WriteDataIndication:
            return

        case AmazonFreeRTOSGattCharacteristic.MutualAuthChar_WriteDataParamValue:
            return

        case AmazonFreeRTOSGattCharacteristic.MutualAuthChar_EdgeParamValue:
            return
            //let data = Data("010203040506070809".utf8)
            //peripheral.writeValue(data, for: characteristic, type: .withResponse)
            //debugPrint("Write Edge param value")


        case AmazonFreeRTOSGattCharacteristic.MutualAuthChar_CharGatewayStatus:
            return
            //let data = Data("Char gateway status".ascii_bytes)
            //peripheral.writeValue(data, for: characteristic, type: .withResponse)
            //debugPrint("Write Char gateway status")


        default:
            return
        }
    }
}

extension AmazonFreeRTOSManager {

    internal func encode<T: Encborable>(_ object: T) -> Data? {
        if let encoded = CBOR.encode(object.toDictionary()) {
            return Data(encoded)
        }
        return nil
    }

    internal func decode<T: Decborable>(_: T.Type, from data: Data) -> T? {
        if let decoded = CBOR.decode(Array([UInt8](data))) as? NSDictionary {
            return T.toSelf(dictionary: decoded)
        }
        return nil
    }

    internal func debugPrint(_ debugMessage: String) {
        guard isDebug else {
            return
        }
        debugMessages += "\(Date())-\(debugMessage)\n"
        os_log("[AFR]%@", log: .default, type: .debug, debugMessage)
    }
}
extension Data{
    func hexEncodedString() -> String {
        return map { String(format: "%02hhx", $0) }.joined()
    }
}
public func EdgeStatusOnOperation(peripheral:CBPeripheral,service:CBService,value:String){
    let characteristicEdgeStatus = service.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.MutualAuthChar_EdgeStatus);
    
    if characteristicEdgeStatus == nil{
        debugPrint("[\(peripheral.identifier.uuidString)][ERROR] characteristicEdgeStatus characteristic doesn't exist")
        return
    }else {
        //debugPrint("[\(service.uuid)] : write to edge status MutualAuthChar_EdgeStatus")
        let data =  Data(hexString: String(value))!
        peripheral.writeValue(data, for: characteristicEdgeStatus!, type: .withResponse)
    }
}
public func DisplayErrorMessage(value:String)->String{
    var res:String?=""
    if value == "02"{
        res="Error:02 Gateway certificate verified fail"
    }else if value == "04"{
        res="Error:04 signature of random number fail"
    }else if value == "06"{
        res="Error:06 ECDSA signature fail"
    }else if value == "08"{
        res="Error:08 ECDSA secret not match"
    }
    return res!
}

public func ReadFromBle(peripheral:CBPeripheral,service:CBService, state:Int){
    //debugPrint("[\(service.uuid)] : [\(state)] :  for read  MutualAuthChar_WriteDataParamValue")

    let characteristicReadEdgeValue = service.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.MutualAuthChar_EdgeParamValue);
    if characteristicReadEdgeValue == nil{
        debugPrint("[\(peripheral.identifier.uuidString)][ERROR] characteristicReadEdgeValue characteristic doesn't exist")
        return
    }else {
        //debugPrint("[\(service.uuid)] : for read  MutualAuthChar_WriteDataParamValue")
        peripheral.readValue(for: characteristicReadEdgeValue!)
        

        //peripheral.setNotifyValue(true, for: characteristicReadEdgeValue!)
        
        //NotifyFromBle(peripheral:peripheral,service:service)

    }
    let characteristicReadGateWayStatus = service.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.MutualAuthChar_CharGatewayStatus);
    if characteristicReadGateWayStatus == nil{
        debugPrint("[\(peripheral.identifier.uuidString)][ERROR] characteristicReadGateWayStatus characteristic doesn't exist")
        return
    }else {
        peripheral.readValue(for: characteristicReadGateWayStatus!)
        peripheral.setNotifyValue(true, for: characteristicReadGateWayStatus!)

    }
}
/*public func convertFromInteger(i: Int) -> UUID {
          var MSB = 0x0000000000001000
          var LSB = -0x7fffff7fa064cb05
          //var value = (i & (-0x1.toLong()).toInt()).toLong()
         // return UUID(MSB ! (value << 32), LSB)
}*/

public func NotifyFromBle(peripheral:CBPeripheral,service:CBService){
    //debugPrint("[\(service.uuid)]  :  for read  MutualAuthChar_CharGatewayStatus")

    let characteristicReadGateWayStatus = service.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.MutualAuthChar_CharGatewayStatus);
    if characteristicReadGateWayStatus == nil{
        debugPrint("[\(peripheral.identifier.uuidString)][ERROR] characteristicReadGateWayStatus characteristic doesn't exist")
        return
    }else {
        //peripheral.setNotifyValue(true, for: characteristicReadGateWayStatus!)
        peripheral.discoverDescriptors(for: characteristicReadGateWayStatus!)
        
        let descriptors = characteristicReadGateWayStatus?.descriptors

        //var desId:CBUUID = CBUUID(string:"c6f2d9e3-49e7-4125-9014-bfc6d669ff03")
        //var myDescriptor = CBMutableDescriptor(type: desId, value: 0x2902)
        //peripheral.readValue(for: myDescriptor)
        
        //debugPrint("[\(descriptors)] : for read  characteristicReadGateWayStatus")

        //debugPrint("[\(service.uuid)] : for read  characteristicReadGateWayStatus")
        peripheral.readValue(for: characteristicReadGateWayStatus!)
        peripheral.setNotifyValue(true, for: characteristicReadGateWayStatus!)
    }
}

extension AmazonFreeRTOSManager {
    public func state1Process(peripheral: CBPeripheral){
        let crcStr = readDevCertDataInStr!.suffix(4)
        let start = readDevCertDataInStr!.index(readDevCertDataInStr!.startIndex, offsetBy: 4)
        let end = readDevCertDataInStr!.index(readDevCertDataInStr!.endIndex, offsetBy: -4)
        let range = start..<end

        let dataStr = readDevCertDataInStr![range]
        let mainData = Data(hexString: String(dataStr))

        let crc = crc16CCITTFalse(data:mainData!)
        let crcHexStr=String(format: "%04X", crc)
        if crcHexStr.caseInsensitiveCompare(crcStr) == ComparisonResult.orderedSame{
            debugPrint("[Mutual Auth] : Device Certificate CRC Matched")
            debugPrint("[Mutual Auth] : Received Device Certificate")

            let start = dataStr.index(dataStr.startIndex, offsetBy: 2)
            let end = dataStr.index(dataStr.endIndex, offsetBy: 0)
            let range = start..<end
            let dataStr1 = dataStr[range]
            readDevCertDataInStr = String(dataStr1)
            let isVerifyCertificate=CertVerifyWithRootCa(caCertStr: "rootCACert", devCertStr: "IOSAppCert")
            if isVerifyCertificate{
                debugPrint("[Mutual Auth] : Device Certificate Verified")
                stateNum =  2
                // for public key send to ble device
                WriteToBle(peripheral:peripheral,service:serviceMain!, state:stateNum!)
            }else{
                debugPrint("[Mutual Auth] : Device Certificate Verified Fail")
            }
            //stateNum =  2
            // for public key send to ble device
            //WriteToBle(peripheral:peripheral,service:serviceMain!, state:stateNum!)
        }else{
            debugPrint("[Mutual Auth] : Device Certificate CRC not Matched")
            PopUpMessage(title:"Error",message:"Mutual Authentication Fail : CRC not matched")
        }
    }
    public func state2Process(peripheral: CBPeripheral){
        let crcStr = readPublicKeyDataInStr!.suffix(4)
        let start = readPublicKeyDataInStr!.index(readPublicKeyDataInStr!.startIndex, offsetBy: 4)
        let end = readPublicKeyDataInStr!.index(readPublicKeyDataInStr!.endIndex, offsetBy: -4)
        let range = start..<end

        let dataStr = readPublicKeyDataInStr![range]
        let mainData = Data(hexString: String(dataStr))

        let crc = crc16CCITTFalse(data:mainData!)
        let crcHexStr=String(format: "%04X", crc)
        if crcHexStr.caseInsensitiveCompare(crcStr) == ComparisonResult.orderedSame{
            debugPrint("[Mutual Auth] : Public key CRC Matched")
            debugPrint("[Mutual Auth] : Received Public key")

            let start = dataStr.index(dataStr.startIndex, offsetBy: 2)
            let end = dataStr.index(dataStr.endIndex, offsetBy: 0)
            let range = start..<end
            let dataStr1 = dataStr[range]
            readPublicKeyDataInStr = String(dataStr1)
            //debugPrint("[\(characteristic.uuid)] → ready to write random number")
            stateNum =  3
            // for random number send to ble device
            WriteToBle(peripheral:peripheral,service:serviceMain!, state:stateNum!)
        }else{
            debugPrint("[Mutual Auth] : Public key CRC not Matched")
            PopUpMessage(title:"Error",message:"Mutual Authentication Fail : CRC not matched")

        }
    }
    public func state3Process(peripheral: CBPeripheral){
        let crcStr = readRandomNumDataInStr!.suffix(4)
        let start = readRandomNumDataInStr!.index(readRandomNumDataInStr!.startIndex, offsetBy: 4)
        let end = readRandomNumDataInStr!.index(readRandomNumDataInStr!.endIndex, offsetBy: -4)
        let range = start..<end

        let dataStr = readRandomNumDataInStr![range]
        let mainData = Data(hexString: String(dataStr))

        let crc = crc16CCITTFalse(data:mainData!)
        let crcHexStr=String(format: "%04X", crc)
        if crcHexStr.caseInsensitiveCompare(crcStr) == ComparisonResult.orderedSame{
            debugPrint("[Mutual Auth] : Random numer CRC Matched")
            debugPrint("[Mutual Auth] : Received Random Number")

            readRandomNumDataInStr = String(dataStr)
            //debugPrint("[\(characteristic.uuid)] → ready to write certificate")
            stateNum =  4
            // for signature send to ble device
            WriteToBle(peripheral:peripheral,service:serviceMain!, state:stateNum!)
        }else{
            debugPrint("[Mutual Auth] : Random number CRC not Matched")
            PopUpMessage(title:"Error",message:"Mutual Authentication Fail : CRC not matched")
        }
    }
    public func state4Process(peripheral: CBPeripheral){
        let crcStr = readSignatureDataInStr!.suffix(4)
        let start = readSignatureDataInStr!.index(readSignatureDataInStr!.startIndex, offsetBy: 4)
               let end = readSignatureDataInStr!.index(readSignatureDataInStr!.endIndex, offsetBy: -4)
               let range = start..<end

               let dataStr = readSignatureDataInStr![range]
               let mainData = Data(hexString: String(dataStr))

               let crc = crc16CCITTFalse(data:mainData!)
               let crcHexStr=String(format: "%04X", crc)
               if crcHexStr.caseInsensitiveCompare(crcStr) == ComparisonResult.orderedSame{
                   debugPrint("[Mutual Auth] : Signature CRC Matched")
                   debugPrint("[Mutual Auth] : Received Signature")

                   let dataLen = mainData!.count;
                   let lenStr=String(format: "%02X", dataLen)
                   let editedStr="30"+lenStr+dataStr
                
                   readSignatureDataInStr = String(editedStr)
                   //debugPrint("[\(characteristic.uuid)] → ready to check verify signature")
                   stateNum =  5
                   // for signature send to ble device
                    let signatureData = Data(hexString: String(readSignatureDataInStr!))

                    let randomData = Data(hexString: String(strRandom!))
                    let readPublicKeyData = Data(hexString: String(readPublicKeyDataInStr!))

                    let byteArrayPublicKey = readPublicKeyData!.withUnsafeBytes(Array.init)
          
                    let pubKey = try?P256.Signing.PublicKey(rawRepresentation: byteArrayPublicKey)
        
                    let byteArraySignature = signatureData!.withUnsafeBytes(Array.init)

                    let signatureKey = try?P256.Signing.ECDSASignature(derRepresentation: byteArraySignature)
                
                    if (pubKey?.isValidSignature(signatureKey!, for: randomData!))! {
                        //debugPrint("The signature is valid")
                        debugPrint("[Mutual Auth] : Signature Verified successfully")

                        stateNum =  5
                        EdgeStatusOnOperation(peripheral:peripheral,service:serviceMain!,value:"03")

                        // for ECDH public key send
                        WriteToBle(peripheral:peripheral,service:serviceMain!, state:stateNum!)
                        
                        

                    }else{
                        EdgeStatusOnOperation(peripheral:peripheral,service:serviceMain!,value:"04")

                        debugPrint("[Mutual Auth] : Signature Verification fail ")
                        PopUpMessage(title:"Error",message:"Mutual Authentication Fail : Signature Verification fail")

                    }
                
               }else{
                   debugPrint("[Mutual Auth] : Signature CRC not Matched")
                   PopUpMessage(title:"Error",message:"Mutual Authentication Fail : CRC not matched")

               }
    }
    public func state5Process(peripheral: CBPeripheral){
        let crcStr = readECDHPublicKeyInStr!.suffix(4)
        let start = readECDHPublicKeyInStr!.index(readECDHPublicKeyInStr!.startIndex, offsetBy: 4)
        let end = readECDHPublicKeyInStr!.index(readECDHPublicKeyInStr!.endIndex, offsetBy: -4)
        let range = start..<end

        let dataStr = readECDHPublicKeyInStr![range]
        let mainData = Data(hexString: String(dataStr))

        let crc = crc16CCITTFalse(data:mainData!)
        let crcHexStr=String(format: "%04X", crc)
        if crcHexStr.caseInsensitiveCompare(crcStr) == ComparisonResult.orderedSame{
            debugPrint("[Mutual Auth] : ECDH Public key CRC Matched")
            debugPrint("[Mutual Auth] : Received ECDH Public key")

            readECDHPublicKeyVerifyInStr=String(dataStr)

            let start = dataStr.index(dataStr.startIndex, offsetBy: 2)
            let end = dataStr.index(dataStr.endIndex, offsetBy: 0)
            let range = start..<end
            let dataStr1 = dataStr[range]
            readECDHPublicKeyInStr = String(dataStr1)
                                         
            //debugPrint("[\(characteristic.uuid)] → ready to write signature with public key")
            stateNum =  6
            
            // for sign pulic key send to ble device
            WriteToBle(peripheral:peripheral,service:serviceMain!, state:stateNum!)
        }else{
            debugPrint("[Mutual Auth] : ECDH Public key CRC not Matched")
            PopUpMessage(title:"Error",message:"Mutual Authentication Fail : CRC not matched")

        }
    }
    public func state6Process(peripheral: CBPeripheral){
        let crcStr = readECDHSignatureInStr!.suffix(4)
               let start = readECDHSignatureInStr!.index(readECDHSignatureInStr!.startIndex, offsetBy: 4)
               let end = readECDHSignatureInStr!.index(readECDHSignatureInStr!.endIndex, offsetBy: -4)
               let range = start..<end

               let dataStr = readECDHSignatureInStr![range]
               let mainData = Data(hexString: String(dataStr))

               let crc = crc16CCITTFalse(data:mainData!)
               let crcHexStr=String(format: "%04X", crc)
               if crcHexStr.caseInsensitiveCompare(crcStr) == ComparisonResult.orderedSame{
                   debugPrint("[Mutual Auth] : ECDH Signature CRC Matched")
                   debugPrint("[Mutual Auth] : Received ECDH Signature")

                   let dataLen = mainData!.count;
                   let lenStr=String(format: "%02X", dataLen)
                   let editedStr="30"+lenStr+dataStr
                
                   readECDHSignatureInStr = String(editedStr)
                   //debugPrint("[\(characteristic.uuid)] → ready to verify ECDH signature")
                   stateNum =  7
                   // for signature send to ble device
                   //WriteToBle(peripheral:peripheral,service:serviceMain!, state:5)
                    let signatureDataECDH = Data(hexString: String(readECDHSignatureInStr!))

                    let messageData = Data(hexString: String(readECDHPublicKeyVerifyInStr!))
                    let byteArrayMessage = messageData!.withUnsafeBytes(Array.init)
                  
                    let readPublicKeyData = Data(hexString: String(readPublicKeyDataInStr!))

                    let byteArrayPublicKey = readPublicKeyData!.withUnsafeBytes(Array.init)
          
                    let pubKey = try?P256.Signing.PublicKey(rawRepresentation: byteArrayPublicKey)
        
                    let byteArraySignature = signatureDataECDH!.withUnsafeBytes(Array.init)

                    let signatureKeyECDH = try?P256.Signing.ECDSASignature(derRepresentation: byteArraySignature)
                
                  if (pubKey?.isValidSignature(signatureKeyECDH!, for: byteArrayMessage))! {
                      debugPrint("[Mutual Auth] : ECDH signature verified successfully")
                      EdgeStatusOnOperation(peripheral:peripheral,service:serviceMain!,value:"05")

                      //generate secret key
                      let ecdhPublicKeyData = Data(hexString: String(readECDHPublicKeyInStr!))
                      let byteArrayECDHPubKey = ecdhPublicKeyData!.withUnsafeBytes(Array.init)
                       let pubKeyECDHRead = try?P256.KeyAgreement.PublicKey(rawRepresentation: byteArrayECDHPubKey)
                      sharedSecret = try? privateKeyECDH!.sharedSecretFromKeyAgreement(with: pubKeyECDHRead!)
                    
                      debugPrint("[Mutual Auth] : Generated ECDH Secret KEY")

                      WriteToBle(peripheral:peripheral,service:serviceMain!, state:stateNum!)

                    }else{
                        debugPrint("[Mutual Auth] : ECDH signature verification fail")

                        EdgeStatusOnOperation(peripheral:peripheral,service:serviceMain!,value:"06")
                        PopUpMessage(title:"Error",message:"Mutual Authentication Fail : ECDH Signature verification fail")

                    }
                
               }else{
                   debugPrint("[Mutual Auth] : ECDH Signature CRC not Matched")
                   PopUpMessage(title:"Error",message:"Mutual Authentication Fail : CRC not matched")
               }
    }
    public func state7Process(peripheral: CBPeripheral){
        let crcStr = readHashedInStr!.suffix(4)
        let start = readHashedInStr!.index(readHashedInStr!.startIndex, offsetBy: 4)
        let end = readHashedInStr!.index(readHashedInStr!.endIndex, offsetBy: -4)
        let range = start..<end

        let dataStr = readHashedInStr![range]
        let mainData = Data(hexString: String(dataStr))

        let crc = crc16CCITTFalse(data:mainData!)
        let crcHexStr=String(format: "%04X", crc)
        if crcHexStr.caseInsensitiveCompare(crcStr) == ComparisonResult.orderedSame{
            debugPrint("[Mutual Auth] : Hash of CRC Matched")
            readHashedInStr = String(dataStr)
            //debugPrint("[\(characteristic.uuid)] → ready to verify hashed")
            stateNum =  8
            // for verify hash
            let mainData = Data(hexString: String(readHashedInStr!))
           if(mainData == hashedDataofSecretKey){
                debugPrint("[Mutual Auth] : Hashed verify successfully")
                debugPrint("[Mutual Auth]  : Completed successfully")

                EdgeStatusOnOperation(peripheral:peripheral,service:serviceMain!,value:"07")
                
                PopUpMessage(title:"FreeRTOS",message:"Mutual Authentication Successful")

           }else{
               debugPrint("Hashed not verify successfully")
               EdgeStatusOnOperation(peripheral:peripheral,service:serviceMain!,value:"08")

               PopUpMessage(title:"Error",message:"Mutual Authentication Fail : Secret Key verification fail")
           }

        }else{
            debugPrint("[\(crcHexStr)] : [\(crcStr)] : Hashed CRC not Matched")
            PopUpMessage(title:"Error",message:"Mutual Authentication Fail : CRC not matched")
        }
    }
    
    public func readDataFromBLE(peripheral: CBPeripheral,characteristic:CBCharacteristic,edgeValue:String){
        switch stateNum {
        case 1:
            //read device certificate and write public key
            if readDevCertDataInStr != ""{
                readDevCertDataInStr! += edgeValue
                    //debugPrint("[\(readDevCertDataInStr)] ] :Received Device Certificate data")

                    if readDevCertDataInStr!.count < totalDevCertSize!{
                     ReadFromBle(peripheral:peripheralMain!,service:serviceMain!, state:stateNum!)
                    }else{
                       state1Process(peripheral: peripheral)
                    }
            }else{
                let dataReadLen = String(edgeValue.prefix(4))
                let dataIntLen = Int(dataReadLen, radix: 16)
                totalDevCertSize = dataIntLen!*2
                readDevCertDataInStr! += edgeValue
                //debugPrint("[\(readDevCertDataInStr)] ] :Received Device Certificate data")

                if readDevCertDataInStr!.count < totalDevCertSize!{
                    ReadFromBle(peripheral:peripheralMain!,service:serviceMain!, state:stateNum!)
                }else{
                     state1Process(peripheral: peripheral)
                }
            }
            break
        case 2:
            //read public key and write for random number
            if readPublicKeyDataInStr != ""{
                    readPublicKeyDataInStr! += edgeValue
                    //debugPrint("[\(readPublicKeyDataInStr)] ] :Received Public key data")

                    if readPublicKeyDataInStr!.count < totalPublicKeySize!{
                     ReadFromBle(peripheral:peripheralMain!,service:serviceMain!, state:stateNum!)
                    }else{
                       state2Process(peripheral: peripheral)
                    }
            }else{
                let dataReadLen = String(edgeValue.prefix(4))
                let dataIntLen = Int(dataReadLen, radix: 16)
                totalPublicKeySize = dataIntLen!*2
                readPublicKeyDataInStr! += edgeValue
                //debugPrint("[\(readPublicKeyDataInStr)] ] :Received Public key data")

                if readPublicKeyDataInStr!.count < totalPublicKeySize!{
                    ReadFromBle(peripheral:peripheralMain!,service:serviceMain!, state:stateNum!)
                }else{
                     state2Process(peripheral: peripheral)
                }
            }
            break
        case 3:
            //read random number && write for signature
            if readRandomNumDataInStr != ""{
                    readRandomNumDataInStr! += edgeValue
                    if readRandomNumDataInStr!.count < totalRandomNumberSize!{
                        //debugPrint("[\(readRandomNumDataInStr)] ] :Received Random number data")
                        ReadFromBle(peripheral:peripheralMain!,service:serviceMain!, state:stateNum!)
                    }else{
                        state3Process(peripheral: peripheral)
                    }
            }else{
                let dataReadLen = String(edgeValue.prefix(4))
                let dataIntLen = Int(dataReadLen, radix: 16)
                totalRandomNumberSize = dataIntLen!*2
                readRandomNumDataInStr! += edgeValue
                if readRandomNumDataInStr!.count < totalRandomNumberSize!{
                    //debugPrint("[\(readRandomNumDataInStr)] ] :Received random numer data")
                    ReadFromBle(peripheral:peripheralMain!,service:serviceMain!, state:stateNum!)
                }else{
                    state3Process(peripheral: peripheral)
                }
            }
            break
        case 4:
            //read signature and verify then  write for ECDH  public key
            if readSignatureDataInStr != ""{
                    readSignatureDataInStr! += edgeValue
                    if readSignatureDataInStr!.count < totalSignatureSize!{
                        //debugPrint("[\(readSignatureDataInStr)] ] :Received Signature data")
                        ReadFromBle(peripheral:peripheralMain!,service:serviceMain!, state:stateNum!)
                    }else{
                        state4Process(peripheral: peripheral)
                    }
            }else{
                let dataReadLen = String(edgeValue.prefix(4))
                let dataIntLen = Int(dataReadLen, radix: 16)
                totalSignatureSize = dataIntLen!*2
                readSignatureDataInStr! += edgeValue
                if readSignatureDataInStr!.count < totalSignatureSize!{
                    //debugPrint("[\(readSignatureDataInStr)] ] :Received Signature data")
                    ReadFromBle(peripheral:peripheralMain!,service:serviceMain!, state:stateNum!)
                }else{
                    state4Process(peripheral: peripheral)
                }
            }
            break
        case 5:
            //read ECDH public key and write for sign public key
                if readECDHPublicKeyInStr != ""{
                    readECDHPublicKeyInStr! += edgeValue
                    //debugPrint("[\(readECDHPublicKeyInStr)] ] :Received ECDH Public key data")

                    if readECDHPublicKeyInStr!.count < totalECDHPubKeySize!{
                        ReadFromBle(peripheral:peripheralMain!,service:serviceMain!, state:stateNum!)
                    }else{
                        state5Process(peripheral: peripheral)
                    }
            }else{
                let dataReadLen = String(edgeValue.prefix(4))
                let dataIntLen = Int(dataReadLen, radix: 16)
                totalECDHPubKeySize = dataIntLen!*2
                readECDHPublicKeyInStr! += edgeValue
                //debugPrint("[\(readECDHPublicKeyInStr)] ] :Received ECDH Public key data")

                if readECDHPublicKeyInStr!.count < totalECDHPubKeySize!{
                    ReadFromBle(peripheral:peripheralMain!,service:serviceMain!, state:stateNum!)
                }else{
                    state5Process(peripheral: peripheral)
                }
            }
            break
        case 6:
            //read ECDH signature and verify and then write for secret key using hash data
            if readECDHSignatureInStr != ""{
                    readECDHSignatureInStr! += edgeValue
                    if readECDHSignatureInStr!.count < totalECDHSignatureSize!{
                        //debugPrint("[\(readECDHSignatureInStr)] ] :Received ECDH Signature data")
                        ReadFromBle(peripheral:peripheralMain!,service:serviceMain!, state:stateNum!)
                    }else{
                        state6Process(peripheral: peripheral)
                    }
            }else{
                let dataReadLen = String(edgeValue.prefix(4))
                let dataIntLen = Int(dataReadLen, radix: 16)
                totalECDHSignatureSize = dataIntLen!*2
                readECDHSignatureInStr! += edgeValue
                if readECDHSignatureInStr!.count < totalECDHSignatureSize!{
                    //debugPrint("[\(readECDHSignatureInStr)] ] :Received ECDH Signature data")
                    ReadFromBle(peripheral:peripheralMain!,service:serviceMain!, state:stateNum!)
                }else{
                    state6Process(peripheral: peripheral)
                }
            }
            break
        case 7:
            //read Hashed data and verify and identify Mutual Authentication pass/fail
            if readHashedInStr != ""{
                    readHashedInStr! += edgeValue
                    if readHashedInStr!.count < totalHashedSize!{
                        //debugPrint("[\(readHashedInStr)] ] :Received Hashed data")
                        ReadFromBle(peripheral:peripheralMain!,service:serviceMain!, state:stateNum!)
                    }else{
                        state7Process(peripheral: peripheral)
                    }
            }else{
                let dataReadLen = String(edgeValue.prefix(4))
                let dataIntLen = Int(dataReadLen, radix: 16)
                totalHashedSize = dataIntLen!*2
                readHashedInStr! += edgeValue
                if readHashedInStr!.count < totalHashedSize!{
                    //debugPrint("[\(readHashedInStr)] ] :Received hashed data")
                    ReadFromBle(peripheral:peripheralMain!,service:serviceMain!, state:stateNum!)
                }else{
                    state7Process(peripheral: peripheral)
                }
            }
            break
        default:
            break
        }
    }
    /// Process data of EdgeValue characteristic from `peripheral`.
    ///
    /// - Parameters:
    ///    - peripheral: The FreeRTOS peripheral.
    ///    - characteristic: The EdgeValue characteristic.
    public func didUpdateValueForEdgeValue(peripheral: CBPeripheral, characteristic: CBCharacteristic) {
        //debugPrint("didUpdateValueForEdgeValue [\(characteristic.uuid)] ")
        let edgeValue = characteristic.value!.hexEncodedString()
        //debugPrint("[\(characteristic.uuid)] → afrDeviceInfoEdgeValue:[\(characteristic.value!)] \(edgeValue) : \(String(describing: stateNum))")
        
        if characteristic == AmazonFreeRTOSGattCharacteristic.MutualAuthChar_CharGatewayStatus{
            debugPrint("Notify from Gateway Status:[\(characteristic.uuid)] → [\(characteristic.value!)] \(edgeValue)")
        }else{
            //debugPrint("Read from :[\(characteristic.uuid)] → [\(characteristic.value!)] \(edgeValue)")
            readDataFromBLE(peripheral: peripheral,characteristic:characteristic,edgeValue:edgeValue)
        }
    }
  
    /// Process data of AfrVersion characteristic from `peripheral`.
    ///
    /// - Parameters:
    ///    - peripheral: The FreeRTOS peripheral.
    ///    - characteristic: The AfrVersion characteristic.
    public func didUpdateValueForAfrVersion(peripheral: CBPeripheral, characteristic: CBCharacteristic) {
        //debugPrint("[\(characteristic.uuid)] : didUpdateValueForAfrVersion")

        guard let value = characteristic.value, let afrVersion = String(data: value, encoding: .utf8) else {
            debugPrint("[\(peripheral.identifier.uuidString)][ERROR] afrDeviceInfoAfrVersion: Invalid AfrVersion")
            return
        }
        devices[peripheral.identifier]?.afrVersion = afrVersion
        devices[peripheral.identifier]?.updateIoTDataManager()
        NotificationCenter.default.post(name: .afrDeviceInfoAfrVersion, object: nil, userInfo: ["afrVersion": afrVersion])
        debugPrint("[\(peripheral.identifier.uuidString)] → afrDeviceInfoAfrVersion: \(afrVersion)")
    }

    /// Process data of BrokerEndpoint characteristic from `peripheral`.
    ///
    /// - Parameters:
    ///     - peripheral: The FreeRTOS peripheral.
    ///     - characteristic: The BrokerEndpoint characteristic.
    ///
    public func didUpdateValueForBrokerEndpoint(peripheral: CBPeripheral, characteristic: CBCharacteristic) {
        //debugPrint("[\(characteristic.uuid)] : didUpdateValueForBrokerEndpoint")

        guard let value = characteristic.value, let brokerEndpoint = String(data: value, encoding: .utf8) else {
            debugPrint("[\(peripheral.identifier.uuidString)][ERROR] afrDeviceInfoBrokerEndpoint: Invalid BrokerEndpoint")
            return
        }
        devices[peripheral.identifier]?.brokerEndpoint = brokerEndpoint
        NotificationCenter.default.post(name: .afrDeviceInfoBrokerEndpoint, object: nil, userInfo: ["brokerEndpoint": brokerEndpoint])
        debugPrint("[\(peripheral.identifier.uuidString)] → afrDeviceInfoBrokerEndpoint: \(brokerEndpoint)")
    }

    /// Process data of Mtu characteristic from `peripheral`. It will also triger on mtu value change.
    ///
    /// - Parameters:
    ///     - peripheral: The FreeRTOS peripheral.
    ///     - characteristic: The Mtu characteristic.
    public func didUpdateValueForMtu(peripheral: CBPeripheral, characteristic: CBCharacteristic) {
        //debugPrint("[\(characteristic.uuid)] : didUpdateValueForMtu")

        guard let value = characteristic.value, let mtuStr = String(data: value, encoding: .utf8), let mtu = Int(mtuStr), mtu > 3 else {
            debugPrint("[\(peripheral.identifier.uuidString)][ERROR] afrDeviceInfoMtu: Invalid Mtu")
            return
        }
        devices[peripheral.identifier]?.mtu = mtu
        NotificationCenter.default.post(name: .afrDeviceInfoMtu, object: nil, userInfo: ["mtu": mtu])
        debugPrint("[\(peripheral.identifier.uuidString)] → afrDeviceInfoMtu: \(mtu)")
    }

    /// Process data of AfrPlatform characteristic from `peripheral`.
    ///
    /// - Parameters:
    ///     - peripheral: The FreeRTOS peripheral.
    ///     - characteristic: The AfrPlatform characteristic.
    public func didUpdateValueForAfrPlatform(peripheral: CBPeripheral, characteristic: CBCharacteristic) {
        //debugPrint("[\(characteristic.uuid)] : didUpdateValueForAfrPlatform")

        guard let value = characteristic.value, let afrPlatform = String(data: value, encoding: .utf8) else {
            debugPrint("[\(peripheral.identifier.uuidString)][ERROR] afrDeviceInfoAfrPlatform: Invalid AfrPlatform")
            return
        }
        devices[peripheral.identifier]?.afrPlatform = afrPlatform
        devices[peripheral.identifier]?.updateIoTDataManager()
        NotificationCenter.default.post(name: .afrDeviceInfoAfrPlatform, object: nil, userInfo: ["afrPlatform": afrPlatform])
        debugPrint("[\(peripheral.identifier.uuidString)] → afrDeviceInfoAfrPlatform: \(afrPlatform)")
    }

    /// Process data of AfrDevId characteristic from `peripheral`.
    ///
    /// - Parameters:
    ///     - peripheral: The FreeRTOS peripheral.
    ///     - characteristic: The AfrDevId characteristic.
    public func didUpdateValueForAfrDevId(peripheral: CBPeripheral, characteristic: CBCharacteristic) {
        //debugPrint("[\(characteristic.uuid)] : didUpdateValueForAfrDevId")

        guard let value = characteristic.value, let afrDevId = String(data: value, encoding: .utf8) else {
            debugPrint("[\(peripheral.identifier.uuidString)][ERROR] afrDeviceInfoAfrDevId: Invalid AfrDevId")
            return
        }
        devices[peripheral.identifier]?.afrDevId = afrDevId
        devices[peripheral.identifier]?.updateIoTDataManager()
        NotificationCenter.default.post(name: .afrDeviceInfoAfrDevId, object: nil, userInfo: ["afrDevId": afrDevId])
        debugPrint("[\(peripheral.identifier.uuidString)] → afrDeviceInfoAfrDevId: \(afrDevId)")
    }
}

extension AmazonFreeRTOSManager {

    
    // Process data of TXMqttMessage or TXNetworkMessage characteristic from `peripheral`.
    internal func didUpdateValueForTXMessage(peripheral: CBPeripheral, characteristic: CBCharacteristic, data: Data?) {
        debugPrint("[\(characteristic.uuid)] : didUpdateValueForTXMessage")

       
        guard let value1 = data ?? characteristic.value else {
            debugPrint("[\(peripheral.identifier.uuidString)][ERROR] didUpdateValueForTXMessage: Invalid message")
            return
        }
        //here decrypt ble data and send to AWS
        //let encryptedData = try? encrypt(input:sourceData,key:dataSecretKey!,iv:dataIV!)
        
        print("[SECURITY] : [RECEIVED] CIPHER TEXT : \(String(describing:hex2ascii(example:value1.hexEncodedString())))")

        let value = try? decrypt(encrypted:value1,key:dataSecretKey!,iv:dataIV!)

        print("[SECURITY] : [RECEIVED] PLAIN TEXT :\(String(describing:hex2ascii(example:value!.hexEncodedString())))")

        switch characteristic.service.uuid {

        case AmazonFreeRTOSGattService.MqttProxy:

            guard let mqttMessage = decode(MqttMessage.self, from: value!) else {
                debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid message")
                return
            }
            
            switch mqttMessage.type {

            case .connect:

                guard let connect = decode(Connect.self, from: value!) else {
                    debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid connect")
                    return
                }

                debugPrint("[\(peripheral.identifier.uuidString)][MQTT] ↑ \(connect)")

                if !(devices[peripheral.identifier]?.registerIoTDataManager(connect: connect) ?? false) {
                    debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid connect: registerIoTDataManager failed")
                    return
                }

                if let certificateId = devices[peripheral.identifier]?.certificateId {
                    AWSIoTDataManager(forKey: peripheral.identifier.uuidString).connect(withClientId: connect.clientID, cleanSession: connect.cleanSession, certificateId: certificateId) { status in

                        switch status {

                        case .connected:
                            self.mqttConnack(peripheral: peripheral, characteristic: characteristic, status: status.rawValue)

                        default:
                            self.debugPrint("[\(peripheral.identifier.uuidString)][MQTT] connectUsingWebSocket status: \(status.rawValue)")
                            return
                        }
                    }
                } else if devices[peripheral.identifier]?.credentialsProvider != nil {
                    AWSIoTDataManager(forKey: peripheral.identifier.uuidString).connectUsingWebSocket(withClientId: connect.clientID, cleanSession: connect.cleanSession) { status in

                        switch status {

                        case .connected:
                            self.mqttConnack(peripheral: peripheral, characteristic: characteristic, status: status.rawValue)

                        default:
                            self.debugPrint("[\(peripheral.identifier.uuidString)][MQTT] connectUsingWebSocket status: \(status.rawValue)")
                            return
                        }
                    }
                } else {
                    debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid connect: No credential")
                }

            case .publish:

                guard let publish = decode(Publish.self, from: value!) else {
                    debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid publish")
                    return
                }

                debugPrint("[\(peripheral.identifier.uuidString)][MQTT] ↑ \(publish)")

                guard let qoS = AWSIoTMQTTQoS(rawValue: publish.qoS) else {
                    debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid publish: qoS")
                    return
                }

                if qoS == AWSIoTMQTTQoS.messageDeliveryAttemptedAtMostOnce {
                    AWSIoTDataManager(forKey: peripheral.identifier.uuidString).publishData(publish.payload, onTopic: publish.topic, qoS: qoS)
                    return
                }
                AWSIoTDataManager(forKey: peripheral.identifier.uuidString).publishData(publish.payload, onTopic: publish.topic, qoS: qoS) {
                    self.mqttPuback(peripheral: peripheral, characteristic: characteristic, msgID: publish.msgID)
                }

            case .puback:
                // need aws iot sdk change, current it auto send puback when recived at sdk side.
                return

            case .subscribe:

                guard let subscribe = decode(Subscribe.self, from: value!) else {
                    debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid subscribe")
                    return
                }

                debugPrint("[\(peripheral.identifier.uuidString)][MQTT] ↑ \(subscribe)")

                guard subscribe.topics.count == subscribe.qoSs.count else {
                    debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid subscribe: topics and qoSs not match")
                    return
                }

                for (index, topic) in subscribe.topics.enumerated() {

                    guard let qoS = AWSIoTMQTTQoS(rawValue: subscribe.qoSs[index]) else {
                        debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid subscribe: qoS")
                        return
                    }

                    AWSIoTDataManager(forKey: peripheral.identifier.uuidString).subscribe(toTopic: topic, qoS: qoS, messageCallback: { data in
                        self.mqttPublish(peripheral: peripheral, characteristic: characteristic, msgID: subscribe.msgID, topic: topic, qoS: subscribe.qoSs[index], data: data)
                    }, ackCallback: {
                        self.mqttSuback(peripheral: peripheral, characteristic: characteristic, msgID: subscribe.msgID, status: subscribe.qoSs[index])
                    })
                }

            case .unsubscribe:

                guard let unsubscribe = decode(Unsubscribe.self, from: value!) else {
                    debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid unsubscribe")
                    return
                }

                debugPrint("[\(peripheral.identifier.uuidString)][MQTT] ↑ \(unsubscribe)")

                for topic in unsubscribe.topics {

                    AWSIoTDataManager(forKey: peripheral.identifier.uuidString).unsubscribeTopic(topic)

                    mqttUnsubscribe(peripheral: peripheral, characteristic: characteristic, msgID: unsubscribe.msgID)
                }

            case .disconnnect:

                guard let disconnect = decode(Disconnect.self, from: value!) else {
                    debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid disconnect")
                    return
                }

                debugPrint("[\(peripheral.identifier.uuidString)][MQTT] ↑ \(disconnect)")

                devices[peripheral.identifier]?.removeIoTDataManager()

            case .pingreq:

                guard let pingreq = decode(Pingreq.self, from: value!) else {
                    debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid pingreq")
                    return
                }

                debugPrint("[\(peripheral.identifier.uuidString)][MQTT] ↑ \(pingreq)")

                guard AWSIoTDataManager(forKey: peripheral.identifier.uuidString).getConnectionStatus() == .connected else {
                    return
                }

                mqttPingresp(peripheral: peripheral, characteristic: characteristic)

            default:
                return
            }

        case AmazonFreeRTOSGattService.NetworkConfig:

            guard let networkMessage = decode(NetworkMessage.self, from: value!) else {
                debugPrint("[\(peripheral.identifier.uuidString)][NETWORK][ERROR] Invalid message")
                return
            }

            switch networkMessage.type {

            case .listNetworkResp:
                guard let listNetworkResp = decode(ListNetworkResp.self, from: value!) else {
                    debugPrint("[\(peripheral.identifier.uuidString)][NETWORK][ERROR] Invalid listNetworkResp")
                    return
                }
                debugPrint("[\(peripheral.identifier.uuidString)][NETWORK] → \(listNetworkResp)")

                if listNetworkResp.index < 0 {

                    // Scaned networks also include saved networks so we filter that out when ssid and security are the same, update the saved network with the scaned bssid, rssi and hidden prams.

                    if let indexSaved = devices[peripheral.identifier]?.savedNetworks.firstIndex(where: { network -> Bool in
                        network.ssid == listNetworkResp.ssid && network.security == listNetworkResp.security
                    }) {
                        if let rssi = devices[peripheral.identifier]?.savedNetworks[indexSaved].rssi, rssi < listNetworkResp.rssi {
                            devices[peripheral.identifier]?.savedNetworks[indexSaved].status = listNetworkResp.status
                            devices[peripheral.identifier]?.savedNetworks[indexSaved].bssid = listNetworkResp.bssid
                            devices[peripheral.identifier]?.savedNetworks[indexSaved].rssi = listNetworkResp.rssi
                            devices[peripheral.identifier]?.savedNetworks[indexSaved].hidden = listNetworkResp.hidden
                        }
                        return
                    }

                    // Scaned networks sorted by rssi, if ssid and security are same, choose the network with stronger rssi.

                    if let indexScaned = devices[peripheral.identifier]?.scanedNetworks.firstIndex(where: { network -> Bool in
                        network.ssid == listNetworkResp.ssid && network.security == listNetworkResp.security
                    }) {
                        if let rssi = devices[peripheral.identifier]?.scanedNetworks[indexScaned].rssi, rssi < listNetworkResp.rssi {
                            devices[peripheral.identifier]?.scanedNetworks[indexScaned] = listNetworkResp
                        }
                    } else {
                        devices[peripheral.identifier]?.scanedNetworks.append(listNetworkResp)
                    }
                    devices[peripheral.identifier]?.scanedNetworks.sort(by: { networkA, networkB -> Bool in
                        networkA.rssi > networkB.rssi
                    })
                } else {

                    // Saved networks sorted by index

                    devices[peripheral.identifier]?.savedNetworks.append(listNetworkResp)
                    devices[peripheral.identifier]?.savedNetworks.sort(by: { networkA, networkB -> Bool in
                        networkA.index < networkB.index
                    })
                }
                NotificationCenter.default.post(name: .afrDidListNetwork, object: nil, userInfo: ["peripheral": peripheral.identifier, "listNetworkResp": listNetworkResp])

            case .saveNetworkResp:
                guard let saveNetworkResp = decode(SaveNetworkResp.self, from: value!) else {
                    debugPrint("[\(peripheral.identifier.uuidString)][NETWORK][ERROR] Invalid saveNetworkResp")
                    return
                }
                debugPrint("[\(peripheral.identifier.uuidString)][NETWORK] → \(saveNetworkResp)")
                NotificationCenter.default.post(name: .afrDidSaveNetwork, object: nil, userInfo: ["peripheral": peripheral.identifier, "saveNetworkResp": saveNetworkResp])

            case .editNetworkResp:
                guard let editNetworkResp = decode(EditNetworkResp.self, from: value!) else {
                    debugPrint("[\(peripheral.identifier.uuidString)][NETWORK][ERROR] Invalid editNetworkResp")
                    return
                }
                debugPrint("[\(peripheral.identifier.uuidString)][NETWORK] → \(editNetworkResp)")
                NotificationCenter.default.post(name: .afrDidEditNetwork, object: nil, userInfo: ["peripheral": peripheral.identifier, "editNetworkResp": editNetworkResp])

            case .deleteNetworkResp:
                guard let deleteNetworkResp = decode(DeleteNetworkResp.self, from: value!) else {
                    debugPrint("[\(peripheral.identifier.uuidString)][NETWORK][ERROR] Invalid deleteNetworkResp")
                    return
                }
                debugPrint("[\(peripheral.identifier.uuidString)][NETWORK] → \(deleteNetworkResp)")
                NotificationCenter.default.post(name: .afrDidDeleteNetwork, object: nil, userInfo: ["peripheral": peripheral.identifier, "deleteNetworkResp": deleteNetworkResp])

            default:
                return
            }

        default:
            return
        }
    }

    // Process data of TXLargeMqttMessage or TXLargeNetworkMessage characteristic from `peripheral`. Used by large object transfer.
    internal func didUpdateValueForTXLargeMessage(peripheral: CBPeripheral, characteristic: CBCharacteristic) {
        debugPrint("[\(characteristic.uuid)] : didUpdateValueForTXLargeMessage")

        guard let mtu = devices[peripheral.identifier]?.mtu else {
            debugPrint("[\(peripheral.identifier.uuidString)][LOT][ERROR] Mtu unknown")
            return
        }

        if let txLotData = devices[peripheral.identifier]?.txLotDatas[characteristic.service.uuid.uuidString], let value = characteristic.value {
            let data = Data([UInt8](txLotData) + [UInt8](value))
            devices[peripheral.identifier]?.txLotDatas[characteristic.service.uuid.uuidString] = data
            debugPrint("[\(peripheral.identifier.uuidString)][LOT] ↑ \(data)")
        } else if let value = characteristic.value {
            devices[peripheral.identifier]?.txLotDatas[characteristic.service.uuid.uuidString] = value
            debugPrint("[\(peripheral.identifier.uuidString)][LOT] ↑ \(value)")
        }
        if characteristic.value?.count ?? 0 < mtu - 3 {
            if let txLotData = devices[peripheral.identifier]?.txLotDatas[characteristic.service.uuid.uuidString] {
                didUpdateValueForTXMessage(peripheral: peripheral, characteristic: characteristic, data: txLotData)
                debugPrint("[\(peripheral.identifier.uuidString)][LOT] LAST FULL ↑ \(txLotData)")
            }
            devices[peripheral.identifier]?.txLotDatas.removeValue(forKey: characteristic.service.uuid.uuidString)
        } else {
            peripheral.readValue(for: characteristic)
        }
    }

    // Write data to RXMqttMessage or RXNetworkMessage characteristic of `peripheral`. Used by large object transfer.
    internal func writeValueToRXMessage(peripheral: CBPeripheral, characteristic: CBCharacteristic, data: Data) {
        debugPrint("[\(characteristic.uuid)] : writeValueToRXMessage")

        DispatchQueue.main.async {
            guard let mtu = self.devices[peripheral.identifier]?.mtu else {
                self.debugPrint("[\(peripheral.identifier.uuidString)][ERROR] Mtu unknown")
                return
            }
            if data.count > mtu - 3 {
                guard let characteristic = characteristic.service.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.RXLargeMqttMessage) ?? characteristic.service.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.RXLargeNetworkMessage) else {
                    self.debugPrint("[\(peripheral.identifier.uuidString)][ERROR] RXLargeMqttMessage or RXLargeNetworkMessage characteristic doesn't exist")
                    return
                }
                if self.devices[peripheral.identifier]?.rxLotDataQueues[characteristic.service.uuid.uuidString] == nil {
                    self.devices[peripheral.identifier]?.rxLotDataQueues[characteristic.service.uuid.uuidString] = [data]
                } else {
                    self.devices[peripheral.identifier]?.rxLotDataQueues[characteristic.service.uuid.uuidString]?.append(data)
                }
                if self.devices[peripheral.identifier]?.rxLotDataQueues[characteristic.service.uuid.uuidString]?.count == 1 {
                    self.writeValueToRXLargeMessage(peripheral: peripheral, characteristic: characteristic)
                }
                return
            }
            guard let characteristic = characteristic.service.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.RXMqttMessage) ?? characteristic.service.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.RXNetworkMessage) else {
                self.debugPrint("[\(peripheral.identifier.uuidString)][ERROR] RXMqttMessage or RXNetworkMessage characteristic doesn't exist")
                return
            }
            //here encrypt data and send to BLE device
            print("[SECURITY] : [SEND] PLAIN TEXT : \(String(describing:hex2ascii(example:data.hexEncodedString())))")

            let encryptedData = try? encrypt(input:data,key:self.dataSecretKey!,iv:self.dataIV!)
            print("[SECURITY] : [SEND] Cipher TEXT : \(String(describing:hex2ascii(example:encryptedData!.hexEncodedString())))")
            peripheral.writeValue(encryptedData!, for: characteristic, type: .withResponse)
        }
    }

    // Write data to RXLargeMqttMessage or RXLargeNetworkMessage characteristic of `peripheral`. Used by large object transfer.
    internal func writeValueToRXLargeMessage(peripheral: CBPeripheral, characteristic: CBCharacteristic) {
        debugPrint("[\(characteristic.uuid)] : writeValueToRXLargeMessage")

        DispatchQueue.main.async {
            guard let mtu = self.devices[peripheral.identifier]?.mtu else {
                self.debugPrint("[\(peripheral.identifier.uuidString)][LOT][ERROR] Mtu unknown")
                return
            }
            guard let rxLotData = self.devices[peripheral.identifier]?.rxLotDataQueues[characteristic.service.uuid.uuidString]?.first else {
                return
            }
            let data = Data([UInt8](rxLotData).prefix(mtu - 3))
            if data.count < mtu - 3 {
                self.devices[peripheral.identifier]?.rxLotDataQueues[characteristic.service.uuid.uuidString]?.removeFirst()
                self.debugPrint("[\(peripheral.identifier.uuidString)][LOT] LAST PART ↓ \(data) - \(self.devices[peripheral.identifier]?.rxLotDataQueues[characteristic.service.uuid.uuidString]?.count ?? 0) in queue")
            } else {
                self.devices[peripheral.identifier]?.rxLotDataQueues[characteristic.service.uuid.uuidString]?[0] = Data([UInt8](rxLotData).dropFirst(mtu - 3))
                self.debugPrint("[\(peripheral.identifier.uuidString)][LOT] ↓ \(rxLotData) - \(self.devices[peripheral.identifier]?.rxLotDataQueues[characteristic.service.uuid.uuidString]?.count ?? 0) in queue")
            }
            peripheral.writeValue(data, for: characteristic, type: .withResponse)
        }
    }
}

extension AmazonFreeRTOSManager {

    internal func mqttConnack(peripheral: CBPeripheral, characteristic: CBCharacteristic, status: Int) {

        let connack = Connack(status: status)

        debugPrint("[\(peripheral.identifier.uuidString)][MQTT] ↓ \(connack)")

        guard let data = encode(connack) else {
            debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid connack")
            return
        }

        writeValueToRXMessage(peripheral: peripheral, characteristic: characteristic, data: data)
    }

    internal func mqttPuback(peripheral: CBPeripheral, characteristic: CBCharacteristic, msgID: Int) {

        let puback = Puback(msgID: msgID)

        debugPrint("[\(peripheral.identifier.uuidString)][MQTT] ↓ \(puback)")

        guard let data = encode(puback) else {
            debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid puback")
            return
        }

        writeValueToRXMessage(peripheral: peripheral, characteristic: characteristic, data: data)
    }

    internal func mqttPublish(peripheral: CBPeripheral, characteristic: CBCharacteristic, msgID: Int, topic: String, qoS: Int, data: Data) {

        let publish = Publish(topic: topic, msgID: msgID, qoS: qoS, payload: data)

        debugPrint("[\(peripheral.identifier.uuidString)][MQTT] ↓ \(publish)")

        guard let data = encode(publish) else {
            debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid publish")
            return
        }

        writeValueToRXMessage(peripheral: peripheral, characteristic: characteristic, data: data)
    }

    internal func mqttSuback(peripheral: CBPeripheral, characteristic: CBCharacteristic, msgID: Int, status: Int) {

        let suback = Suback(msgID: msgID, status: status)

        debugPrint("[\(peripheral.identifier.uuidString)][MQTT] ↓ \(suback)")

        guard let data = encode(suback) else {
            debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid suback")
            return
        }

        writeValueToRXMessage(peripheral: peripheral, characteristic: characteristic, data: data)
    }

    internal func mqttUnsubscribe(peripheral: CBPeripheral, characteristic: CBCharacteristic, msgID: Int) {

        let unsuback = Unsuback(msgID: msgID)

        debugPrint("[\(peripheral.identifier.uuidString)][MQTT] ↓ \(unsuback)")

        guard let data = encode(unsuback) else {
            debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid unsuback")
            return
        }

        writeValueToRXMessage(peripheral: peripheral, characteristic: characteristic, data: data)
    }

    internal func mqttPingresp(peripheral: CBPeripheral, characteristic: CBCharacteristic) {

        let pingresp = Pingresp()

        debugPrint("[\(peripheral.identifier.uuidString)][MQTT] ↓ \(pingresp)")

        guard let data = encode(pingresp) else {
            debugPrint("[\(peripheral.identifier.uuidString)][MQTT][ERROR] Invalid pingresp")
            return
        }

        writeValueToRXMessage(peripheral: peripheral, characteristic: characteristic, data: data)
    }
}

extension AmazonFreeRTOSManager {

    internal func listNetwork(_ peripheral: CBPeripheral, listNetworkReq: ListNetworkReq) {

        debugPrint("[\(peripheral.identifier.uuidString)][NETWORK] ↓ \(listNetworkReq)")

        guard let data = encode(listNetworkReq) else {
            debugPrint("[\(peripheral.identifier.uuidString)][NETWORK][ERROR] Invalid listNetworkReq")
            return
        }
        guard let characteristic = peripheral.serviceOf(uuid: AmazonFreeRTOSGattService.NetworkConfig)?.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.RXNetworkMessage) else {
            debugPrint("[\(peripheral.identifier.uuidString)][NETWORK][ERROR] NetworkConfig service or RXNetworkMessage characteristic doesn't exist")
            return
        }
        writeValueToRXMessage(peripheral: peripheral, characteristic: characteristic, data: data)
    }

    internal func saveNetwork(_ peripheral: CBPeripheral, saveNetworkReq: SaveNetworkReq) {

        debugPrint("[\(peripheral.identifier.uuidString)][NETWORK] ↓ \(saveNetworkReq)")

        guard let data = encode(saveNetworkReq) else {
            debugPrint("[\(peripheral.identifier.uuidString)][NETWORK][ERROR] Invalid saveNetworkReq")
            return
        }
        guard let characteristic = peripheral.serviceOf(uuid: AmazonFreeRTOSGattService.NetworkConfig)?.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.RXNetworkMessage) else {
            debugPrint("[\(peripheral.identifier.uuidString)][NETWORK][ERROR] NetworkConfig service or RXNetworkMessage characteristic doesn't exist")
            return
        }
        writeValueToRXMessage(peripheral: peripheral, characteristic: characteristic, data: data)
    }

    internal func editNetwork(_ peripheral: CBPeripheral, editNetworkReq: EditNetworkReq) {

        debugPrint("[\(peripheral.identifier.uuidString)][NETWORK] ↓ \(editNetworkReq)")

        guard let data = encode(editNetworkReq) else {
            debugPrint("[\(peripheral.identifier.uuidString)][NETWORK][ERROR] Invalid editNetworkReq")
            return
        }
        guard let characteristic = peripheral.serviceOf(uuid: AmazonFreeRTOSGattService.NetworkConfig)?.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.RXNetworkMessage) else {
            debugPrint("[\(peripheral.identifier.uuidString)][NETWORK][ERROR] NetworkConfig service or RXNetworkMessage characteristic doesn't exist")
            return
        }
        writeValueToRXMessage(peripheral: peripheral, characteristic: characteristic, data: data)
    }

    internal func deleteNetwork(_ peripheral: CBPeripheral, deleteNetworkReq: DeleteNetworkReq) {

        debugPrint("[\(peripheral.identifier.uuidString)][NETWORK] ↓ \(deleteNetworkReq)")

        guard let data = encode(deleteNetworkReq) else {
            debugPrint("[\(peripheral.identifier.uuidString)][NETWORK][ERROR] Invalid deleteNetworkReq")
            return
        }
        guard let characteristic = peripheral.serviceOf(uuid: AmazonFreeRTOSGattService.NetworkConfig)?.characteristicOf(uuid: AmazonFreeRTOSGattCharacteristic.RXNetworkMessage) else {
            debugPrint("[\(peripheral.identifier.uuidString)][NETWORK][ERROR] NetworkConfig service or RXNetworkMessage characteristic doesn't exist")
            return
        }
        writeValueToRXMessage(peripheral: peripheral, characteristic: characteristic, data: data)
    }
}
// Generate random number
public func random(digits: Int) -> String {
    var number = String()
    for _ in 1 ... digits {
        number += "\(Int.random(in: 1 ... 9))"
    }
    return number
}

/*extension Data {
    func aesEncrypt(key: String, iv: String) throws -> Data{
        let encypted = try AES(key: key.bytes, blockMode: CBC(iv: iv.bytes), padding: .pkcs7).encrypt(self.bytes)
        return Data(bytes: encypted)
    }
    func aesDecrypt(key: String, iv: String) throws -> Data {
        let decrypted = try AES(key: key.bytes, blockMode: CBC(iv: iv.bytes), padding: .pkcs7).decrypt(self.bytes)
        return Data(bytes: decrypted)
    }
}*/

extension Data {
    var hexStringAES: String {
        return map { String(format: "%02hhx", $0) }.joined()
    }
}
public func crypt(data: Data,key:Data,iv:Data,operation: CCOperation) throws -> Data {
   var cryptor: CCCryptorRef? = nil
    // 1. Create a cryptographic context.
    var status = CCCryptorCreateWithMode(operation, CCMode(kCCModeCFB8), CCAlgorithm(kCCAlgorithmAES), CCPadding(ccNoPadding), iv.bytes, key.bytes, (key.count), nil, 0, 0, CCModeOptions(kCCModeOptionCTR_BE), &cryptor)

    assert(status == kCCSuccess, "Failed to create a cryptographic context.")

    var retData = Data()

    // 2. Encrypt or decrypt data.
    let buffer = NSMutableData()
    buffer.length = CCCryptorGetOutputLength(cryptor, (data.bytes.count), true) // We'll reuse the buffer in -finish

    var dataOutMoved: size_t=0
    status = CCCryptorUpdate(cryptor, data.bytes, (data.bytes.count), buffer.mutableBytes, buffer.count, &dataOutMoved)
    assert(status == kCCSuccess, "Failed to encrypt or decrypt data")
    retData.append(buffer.subdata(with: NSRange(location: 0, length: dataOutMoved)))
    CCCryptorRelease(cryptor)

    return retData
}
public func encrypt(input: Data,key:Data,iv:Data) throws -> Data {
    return try crypt(data:input,key:key,iv:iv, operation: CCOperation(kCCEncrypt))
}

public func decrypt(encrypted: Data,key:Data,iv:Data) throws -> Data {
    return try crypt(data: encrypted,key:key,iv:iv, operation: CCOperation(kCCDecrypt))
}
func testEncryptDecrypt(key:Data,iv:Data){
    do {
        
        //let sourceData = "Test".data(using: .utf8)!
        let hexString = "einfochipsalpeshprajapati".data(using: .ascii)!.hexEncodedString()
        guard let sourceData = Data(hexString: hexString) else { return }

        let encryptedData = try encrypt(input:sourceData,key:key,iv:iv)
        let decryptedData = try decrypt(encrypted:encryptedData,key:key,iv:iv)
        
        print("Encrypted hex string: \(String(describing: encryptedData.hexEncodedString()))")
        print("Decrypted hex string: \(String(describing: decryptedData.hexEncodedString()))")
    } catch {
        print("Failed")
        print(error)
    }
}
public func hex2ascii (example: String) -> String {
    var chars = [Character]()
    for c in example{
        chars.append(c)
    }
    let numbers =  stride(from: 0, to: chars.count, by: 2).map{
        strtoul(String(chars[$0 ..< $0+2]), nil, 16)
    }
    var final = ""
    var i = 0
    while i < numbers.count {
        final.append(Character(UnicodeScalar(Int(numbers[i]))!))
        i += 1
    }
    return final
}

public func DeviceCertificateCertData(devCertStr:String)-> Data{
    //get device certificate
    let devFilePath = Bundle.main.path(forResource: devCertStr, ofType: "der")
    let devCertData = NSData(contentsOfFile: devFilePath ?? "") as Data?
    var devCert: SecCertificate?
    if let data = devCertData as? CFData? {
        devCert = SecCertificateCreateWithData(nil, data!)
    }
    assert(devCert != nil)
    let certData = SecCertificateCopyData(devCert!) as Data
    return certData
}

public func CertVerifyWithRootCa(caCertStr:String,devCertStr:String)->Bool{
    //get ca certificate
    let caFilePath = Bundle.main.path(forResource: caCertStr, ofType: "der")
    let caCertData = NSData(contentsOfFile: caFilePath ?? "") as Data?

    var caCert: SecCertificate?
    if let data = caCertData as? CFData? {
        caCert = SecCertificateCreateWithData(nil, data!)
    }
    assert(caCert != nil)
    
    //getting certificate from device data
    //let devCertData1 =  Data(hexString: String(devCertDataStr))
    let devFilePath = Bundle.main.path(forResource: devCertStr, ofType: "der")
    var devCertData = NSData(contentsOfFile: devFilePath ?? "") as Data?
    
    var devCert: SecCertificate?
    if let data = devCertData as? CFData? {
        devCert = SecCertificateCreateWithData(nil, data!)
    }
    assert(devCert != nil)

    var trust: SecTrust?
    
    let certArray:CFTypeRef? = [caCert, devCert] as CFTypeRef
    let secPolicy = SecPolicyCreateBasicX509()

    let statusTrust = SecTrustCreateWithCertificates(certArray!, secPolicy, &trust)
    if statusTrust == errSecSuccess{
        return true
    }
    return false;
}
public func VerifyRootCaWithDeviceCert(caCertStr:String,devCertStr:String)->Bool{
    //get ca certificate
    let caFilePath = Bundle.main.path(forResource: caCertStr, ofType: "der")
    let caCertData = NSData(contentsOfFile: caFilePath ?? "") as Data?

    var caCert: SecCertificate?
    if let data = caCertData as? CFData? {
        caCert = SecCertificateCreateWithData(nil, data!)
    }
    assert(caCert != nil)
    
    //get device certificate
    let devFilePath = Bundle.main.path(forResource: devCertStr, ofType: "der")
    var devCertData = NSData(contentsOfFile: devFilePath ?? "") as Data?
    //let mutdata = NSMutableData(data: devCertData as! Data)

    var devCert: SecCertificate?
    if let data = devCertData as? CFData? {
        devCert = SecCertificateCreateWithData(nil, data!)
    }
    assert(devCert != nil)

    //var caPublicKey: SecKey?
    var trust: SecTrust?

    //let policy = SecPolicyCreateBasicX509()
    //var status = SecTrustCreateWithCertificates(caCert!, policy, &trust)

    //if status == errSecSuccess {
       // caPublicKey = SecTrustCopyPublicKey(trust!)!
    //}

    let certArray:CFTypeRef? = [caCert, devCert] as CFTypeRef
    let secPolicy = SecPolicyCreateBasicX509()

    let statusTrust = SecTrustCreateWithCertificates(certArray!, secPolicy, &trust)
    if statusTrust != errSecSuccess{
        return false
    }
        
    /*var statusBool:Bool? = false
    DispatchQueue.global().async {
        SecTrustEvaluateAsyncWithError(trust!, DispatchQueue.global()) {
            trust, result, error in
            statusBool=result
            if result {
                let publicKey = SecTrustCopyPublicKey(trust)
                // Use key . . .
            } else {
                print("Trust failed: \(error!.localizedDescription)")
            }
        }
    }
    return statusBool!
    */
    
    var statusTrustEval: Bool? = false
    let resultType: UnsafeMutablePointer<CFError?>?=nil
    statusTrustEval = SecTrustEvaluateWithError(trust!, resultType)
    if statusTrustEval!{
        let publicKey = SecTrustCopyPublicKey(trust!)
        return true
    }
 
    /*statusTrustEval = try? SecTrustEvaluateWithError(statusTrust as! SecTrust, resultType)
    if statusTrustEval!{
        print("Certificate verification successful")
        return true
    }else{
        print("Certificate verification fail")
    }*/

    //verify device certificate with root ca certificate
    return false
}
