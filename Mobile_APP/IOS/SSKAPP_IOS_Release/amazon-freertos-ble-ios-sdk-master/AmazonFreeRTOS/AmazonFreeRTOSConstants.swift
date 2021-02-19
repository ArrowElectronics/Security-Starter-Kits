import CoreBluetooth

/// FreeRTOS Constants
public struct AmazonFreeRTOS {
    /// FreeRTOS SDK Version.
    static let SDKVersion = "1.1.0"
}

/// BLE services used by the SDK.
public struct AmazonFreeRTOSGattService {
    //Manual Authentication service
    static let MutualAuthService = CBUUID(string: "c6f2d9e3-49e7-4125-9014-bfc6d669ff00")
    
    /// Device Info Service. This is a required service for FreeRTOS.
    static let DeviceInfo = CBUUID(string: "8a7f1168-48af-4efb-83b5-e679f932ff00")
    /// Mqtt Proxy Service.
    static let MqttProxy = CBUUID(string: "a9d7166a-d72e-40a9-a002-48044cc30000")
    /// Network Config Service.
    static let NetworkConfig = CBUUID(string: "a9d7166a-d72e-40a9-a002-48044cc30100")
    
}

/// BLE characteristics used by the SDK.
public struct AmazonFreeRTOSGattCharacteristic {
    
    //Mutual Authentication service characterstics
    static let MutualAuthChar_WriteDataIndication = CBUUID(string: "c6f2d9e3-49e7-4125-9014-bfc6d669ff01")
    static let MutualAuthChar_WriteDataParamValue = CBUUID(string: "c6f2d9e3-49e7-4125-9014-bfc6d669ff02")
    static let MutualAuthChar_CharGatewayStatus = CBUUID(string: "c6f2d9e3-49e7-4125-9014-bfc6d669ff03")
    static let MutualAuthChar_EdgeParamValue = CBUUID(string: "c6f2d9e3-49e7-4125-9014-bfc6d669ff04")
    static let MutualAuthChar_EdgeStatus = CBUUID(string: "c6f2d9e3-49e7-4125-9014-bfc6d669ff05")

    /// The version of the FreeRTOS.
    static let AfrVersion = CBUUID(string: "8a7f1168-48af-4efb-83b5-e679f932ff01")
    /// The broker endpoint of the mqtt.
    static let BrokerEndpoint = CBUUID(string: "8a7f1168-48af-4efb-83b5-e679f932ff02")
    /// The mtu of the device.
    static let Mtu = CBUUID(string: "8a7f1168-48af-4efb-83b5-e679f932ff03")
    /// The platform of the FreeRTOS.
    static let AfrPlatform = CBUUID(string: "8a7f1168-48af-4efb-83b5-e679f932ff04")
    /// The device id of the FreeRTOS.
    static let AfrDevId = CBUUID(string: "8a7f1168-48af-4efb-83b5-e679f932ff05")

    /// Used for mqtt control state.
    static let MqttProxyControl = CBUUID(string: "a9d7166a-d72e-40a9-a002-48044cc30001")
    /// Used for transfer mqtt messages.
    static let TXMqttMessage = CBUUID(string: "a9d7166a-d72e-40a9-a002-48044cc30002")
    /// Used for transfer mqtt messages.
    static let RXMqttMessage = CBUUID(string: "a9d7166a-d72e-40a9-a002-48044cc30003")
    /// Used for mqtt large object transfer.
    static let TXLargeMqttMessage = CBUUID(string: "a9d7166a-d72e-40a9-a002-48044cc30004")
    /// Used for mqtt large object transfer.
    static let RXLargeMqttMessage = CBUUID(string: "a9d7166a-d72e-40a9-a002-48044cc30005")

    /// Used for network control state.
    static let NetworkConfigControl = CBUUID(string: "a9d7166a-d72e-40a9-a002-48044cc30101")
    /// Used for transfer network messages.
    static let TXNetworkMessage = CBUUID(string: "a9d7166a-d72e-40a9-a002-48044cc30102")
    /// Used for transfer network messages.
    static let RXNetworkMessage = CBUUID(string: "a9d7166a-d72e-40a9-a002-48044cc30103")
    /// Used for network large object transfer.
    static let TXLargeNetworkMessage = CBUUID(string: "a9d7166a-d72e-40a9-a002-48044cc30104")
    /// Used for network large object transfer.
    static let RXLargeNetworkMessage = CBUUID(string: "a9d7166a-d72e-40a9-a002-48044cc30105")
}
