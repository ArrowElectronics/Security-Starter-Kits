import Alertift
import AmazonFreeRTOS
import AWSIoT
import AWSMobileClient
import CoreBluetooth
import UIKit

/// This is the main controller used to list the nearby FreeRTOS devices that has the BLE capability.
class DevicesViewController: UITableViewController {

    var uuid: UUID?
    var appDelegate: AppDelegate?

    override func viewDidLoad() {
        super.viewDidLoad()

        appDelegate = (UIApplication.shared.delegate as? AppDelegate)!

        navigationController?.delegate = self

        let preferences = UserDefaults.standard
        let cIReg: String? = (preferences.object(forKey: "cIRegion") as? String)!
        let region: AWSRegionType? = (getRegionName(cIReg: cIReg) as? AWSRegionType)!

        // Setup the user sign-in with cognito: https://aws-amplify.github.io/docs/ios/authentication#manual-setup
        AWSServiceManager.default().defaultServiceConfiguration = AWSServiceConfiguration(region: region!, credentialsProvider: AWSMobileClient.default())

        extendedLayoutIncludesOpaqueBars = true

        // Add observe for AmazonFreeRTOSManager NSNotifications
        NotificationCenter.default.addObserver(self, selector: #selector(centralManagerDidUpdateState), name: .afrCentralManagerDidUpdateState, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(centralManagerDidDisconnectDevice), name: .afrCentralManagerDidDisconnectDevice, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(reloadDataWithoutAnimation), name: .afrCentralManagerDidDiscoverDevice, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(reloadDataWithoutAnimation), name: .afrCentralManagerDidConnectDevice, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(reloadDataWithoutAnimation), name: .afrCentralManagerDidFailToConnectDevice, object: nil)

        centralManagerDidUpdateState()

        #warning("remove showLogin() if you do not plan to use the MQTT demo")
        showLogin()
    }

    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        // let appDelegate = (UIApplication.shared.delegate as? AppDelegate)!
        // if appDelegate.isCognitoSet!
        // {
        // let appDelegate = (UIApplication.shared.delegate as? AppDelegate)!
        // appDelegate.isCognitoSet = false
        if let indexPath = tableView.indexPathForSelectedRow {
            tableView.deselectRow(at: indexPath, animated: true)
        }
        // }
    }

    // Segue
    override func prepare(for segue: UIStoryboardSegue, sender _: Any?) {
        if segue.identifier == "toMqttProxyViewController", let viewController = segue.destination as? MqttProxyViewController {
            viewController.uuid = uuid
        } else if segue.identifier == "toNetworkConfigViewController", let viewController = segue.destination as? NetworkConfigViewController {
            viewController.uuid = uuid
        } else if segue.identifier == "toCustomGattMqttViewController", let viewController = segue.destination as? CustomGattMqttViewController {
            viewController.uuid = uuid
        }
    }

    func getRegionName(cIReg: String?) -> AWSRegionType {
        var reginSel: AWSRegionType?
        if cIReg == "ca-central-1" {
            reginSel = AWSRegionType.CACentral1
        } else if cIReg == "cn-north-1" {
            reginSel = AWSRegionType.CNNorth1
        } else if cIReg == "sa-east-1" {
            reginSel = AWSRegionType.SAEast1
        } else if cIReg == "ap-southeast-2" {
            reginSel = AWSRegionType.APSoutheast2
        } else if cIReg == "ap-northeast-2" {
            reginSel = AWSRegionType.APNortheast2
        } else if cIReg == "ap-northeast-1" {
            reginSel = AWSRegionType.APNortheast1
        } else if cIReg == "ap-southeast-1" {
            reginSel = AWSRegionType.APSoutheast1
        } else if cIReg == "eu-central-1" {
            reginSel = AWSRegionType.EUCentral1
        } else if cIReg == "eu-west-2" {
            reginSel = AWSRegionType.EUWest2
        } else if cIReg == "eu-west-1" {
            reginSel = AWSRegionType.EUWest1
        } else if cIReg == "us-west-2" {
            reginSel = AWSRegionType.USWest2
        } else if cIReg == "us-west-1" {
            reginSel = AWSRegionType.USWest1
        } else if cIReg == "us-east-2" {
            reginSel = AWSRegionType.USEast2
        } else if cIReg == "us-east-1" {
            reginSel = AWSRegionType.USEast1
        } else if cIReg == "unknown" {
            reginSel = AWSRegionType.Unknown
        } else if cIReg == "us-govwest-1" {
            reginSel = AWSRegionType.USGovWest1
        } else if cIReg == "eu-west-3" {
            reginSel = AWSRegionType.EUWest3
        } else if cIReg == "cn-northwest-1" {
            reginSel = AWSRegionType.CNNorthWest1
        } else if cIReg == "me-south-1" {
            reginSel = AWSRegionType.MESouth1
        } else if cIReg == "ap-east-1" {
            reginSel = AWSRegionType.APEast1

        } else if cIReg == "eu-north-1" {
            reginSel = AWSRegionType.EUNorth1

        } else if cIReg == "us-goveast-1" {
            reginSel = AWSRegionType.USGovEast1
        } else if cIReg == "ap-south-1" {
            reginSel = AWSRegionType.APSouth1
        } else if cIReg == "ap-east-1" {
            reginSel = AWSRegionType.APEast1
        }
        return reginSel!
    }
}

extension DevicesViewController {

    #warning("Scan forever when BLE is on, stop when off. However in production app there should be a timer to stop scan after some time.")
    @objc
    func centralManagerDidUpdateState() {
        if AmazonFreeRTOSManager.shared.central?.state == .poweredOn {
            AmazonFreeRTOSManager.shared.startScanForDevices()
            return
        }
        AmazonFreeRTOSManager.shared.stopScanForDevices()
    }

    @objc
    func centralManagerDidDisconnectDevice(_ notification: NSNotification) {
        reloadDataWithoutAnimation()
        if notification.userInfo?["identifier"] as? UUID == uuid {
            _ = navigationController?.popToRootViewController(animated: true)
        }
    }

    @objc
    func reloadDataWithoutAnimation() {
        UIView.performWithoutAnimation {
            self.tableView.reloadData()
        }
    }

    func showLogin() {

        AWSMobileClient.default().initialize { userState, error in

            // initialize error

            if let error = error {
                DispatchQueue.main.async {
                    Alertift.alert(title: NSLocalizedString("Error", comment: String()), message: error.localizedDescription)
                        .action(.default(NSLocalizedString("OK", comment: String())))
                        .show(on: self)
                }
                return
            }
            // let appDelegate = (UIApplication.shared.delegate as? AppDelegate)!
            // let navigationContr = appDelegate.navCon
            // process userState
            guard let navigationController = self.navigationController, let userState = userState else {
                return
            }

            switch userState {

            case .signedIn:
                self.attachPrincipalPolicy()

            case .signedOut:

                let signInUIOptions = SignInUIOptions(canCancel: false, logoImage: #imageLiteral(resourceName: "common_logo"), backgroundColor: UIColor(named: "teal_color"))
                AWSMobileClient.default().showSignIn(navigationController: navigationController, signInUIOptions: signInUIOptions) { _, error in

                    // signin error

                    if let error = error {
                        DispatchQueue.main.async {
                            Alertift.alert(title: NSLocalizedString("Error", comment: String()), message: error.localizedDescription)
                                .action(.default(NSLocalizedString("OK", comment: String())))
                                .show(on: self)
                        }
                        return
                    }

                    self.attachPrincipalPolicy()
                }

            default:
                AWSMobileClient.default().signOut()
            }
        }
    }

    #warning("attachPrincipalPolicy should NOT be done in the app, this is just for demo purposes. See getting started guide.")

    /// Attach the coginto identity to the AWS IoT policy.
    ///
    /// - Precondition: The AWS IoT policy must already have been created. Follow the get started guide if not.
    func attachPrincipalPolicy() {

        // get the AWS Cognito Identity

        AWSMobileClient.default().getIdentityId().continueWith { task -> Any? in

            if let error = task.error {
                DispatchQueue.main.async {
                    Alertift.alert(title: NSLocalizedString("Error", comment: String()), message: error.localizedDescription)
                        .action(.default(NSLocalizedString("OK", comment: String())))
                        .show(on: self)
                }
                return task
            }

            guard let attachPrincipalPolicyRequest = AWSIoTAttachPrincipalPolicyRequest(), let principal = task.result else {
                return task
            }

            let preferences = UserDefaults.standard
            let policyName = preferences.object(forKey: "cIPolicyName")
            // The AWS IoT Policy
            attachPrincipalPolicyRequest.policyName = (policyName! as? String)!
            // The AWS Cognito Identity
            attachPrincipalPolicyRequest.principal = String(principal)

            AWSIoT.default().attachPrincipalPolicy(attachPrincipalPolicyRequest, completionHandler: { error in
                if let error = error {
                    DispatchQueue.main.async {
                        Alertift.alert(title: NSLocalizedString("Error", comment: String()), message: error.localizedDescription)
                            .action(.default(NSLocalizedString("OK", comment: String())))
                            .show(on: self)
                        return
                    }
                }
            })

            return task
        }
    }
}

// UITableView

extension DevicesViewController {

    override func tableView(_: UITableView, numberOfRowsInSection _: Int) -> Int {
        return AmazonFreeRTOSManager.shared.devices.count
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "DeviceCell", for: indexPath)
        guard let deviceCell = cell as? DeviceCell else {
            return cell
        }
        let device = Array(AmazonFreeRTOSManager.shared.devices.values)[indexPath.row]

        #warning("the GAP name (peripheral.name) is cached on iOS and refreshes on connect so we use advertisementData name to get the latest.")
        deviceCell.labDeviceName.text = device.advertisementData?["kCBAdvDataLocalName"] as? String ?? device.peripheral.name
        // iOS use generated identifier, it will be different on other devices.
        deviceCell.labDeviceIdentifier.text = device.peripheral.identifier.uuidString
        deviceCell.labDeviceRSSI.text = device.RSSI?.stringValue ?? NSLocalizedString("N/A", comment: String())
        if device.peripheral.state == .connected {
            deviceCell.viewDeviceStateIndicator.backgroundColor = UIColor(named: "seafoam_green_color")
        } else {
            deviceCell.viewDeviceStateIndicator.backgroundColor = UIColor.clear
        }
        return deviceCell
    }

    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        guard let cell = tableView.cellForRow(at: indexPath) else {
            return
        }
        tableView.deselectRow(at: indexPath, animated: true)
        uuid = Array(AmazonFreeRTOSManager.shared.devices.keys)[indexPath.row]
        let device = Array(AmazonFreeRTOSManager.shared.devices.values)[indexPath.row]
        if device.peripheral.state == .connected {
            if device.mtu != nil {
                Alertift.actionSheet()
                    .popover(anchorView: cell)

                    // Example 1: MQTT Proxy

                    .action(.default(NSLocalizedString("MQTT Proxy", comment: String()))) { _, _ in
                        self.performSegue(withIdentifier: "toMqttProxyViewController", sender: self)
                        return
                    }

                    // Example 2: Network Config

                    .action(.default(NSLocalizedString("Network Config", comment: String()))) { _, _ in
                        self.performSegue(withIdentifier: "toNetworkConfigViewController", sender: self)
                        return
                    }

                    // Example 3: Custom GATT MQTT

                    .action(.default(NSLocalizedString("Custom GATT MQTT", comment: String()))) { _, _ in
                        self.performSegue(withIdentifier: "toCustomGattMqttViewController", sender: self)
                        return
                    }
                    .action(.cancel(NSLocalizedString("Cancel", comment: String())))
                    .show(on: self)
            } else {
                if let deviceCell = cell as? DeviceCell {
                    deviceCell.viewDeviceStateIndicator.backgroundColor = UIColor(named: "orange_color")
                    Alertift.alert(title: NSLocalizedString("Error", comment: String()), message: "Unable to read secure values. You need to forget this device pair it again")
                        .action(.default(NSLocalizedString("OK", comment: String())))
                        .show(on: self)
                }
            }
        } else {
            device.connect(reconnect: true, credentialsProvider: AWSMobileClient.default())
        }
    }

    override func tableView(_: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
        let device = Array(AmazonFreeRTOSManager.shared.devices.values)[indexPath.row]
        if device.peripheral.state == .connected {
            return true
        }
        return false
    }

    override func tableView(_: UITableView, titleForDeleteConfirmationButtonForRowAt _: IndexPath) -> String? {
        return NSLocalizedString("Disconnect", comment: String())
    }

    override func tableView(_: UITableView, commit _: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
        let device = Array(AmazonFreeRTOSManager.shared.devices.values)[indexPath.row]
        device.disconnect()
    }
}

extension DevicesViewController {

    @IBAction private func btnRescanPush(_: UIBarButtonItem) {
        AmazonFreeRTOSManager.shared.rescanForDevices()
        reloadDataWithoutAnimation()
    }

    @IBAction private func btnLogoutPush(_: UIBarButtonItem) {
        AWSMobileClient.default().signOut()
        showLogin()
    }

    public func logoutMethod() {
        AWSMobileClient.default().signOut()
        showLogin()
    }
}
