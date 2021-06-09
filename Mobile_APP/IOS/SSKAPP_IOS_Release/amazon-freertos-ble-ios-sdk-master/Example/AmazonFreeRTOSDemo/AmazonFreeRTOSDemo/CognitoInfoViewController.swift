import Alertift
import AmazonFreeRTOS
import CoreBluetooth
import MobileCoreServices
import UIKit

class CognitoInfoViewController: UIViewController, UIDocumentPickerDelegate {

    @IBOutlet private var tfRegion: AmazonTextField!
    @IBOutlet private var tfIOTPolicyName: AmazonTextField!
    @IBOutlet private var tfCIPoolId1: AmazonTextField!
    @IBOutlet private var tfCUPoolId: AmazonTextField!
    @IBOutlet private var tfCUACId: AmazonTextField!
    @IBOutlet private var tfCUACS: AmazonTextField!
    @IBOutlet private var save: UIButton!
    @IBOutlet private var skip: UIButton!
    @IBOutlet private var browse: UIButton!
    @IBOutlet private var view1: UIView!
    @IBOutlet private var view2: UIView!
    @IBOutlet private var view3: UIView!
    @IBOutlet private var view4: UIView!
    @IBOutlet private var view5: UIView!
    @IBOutlet private var view6: UIView!

    // @IBOutlet private var viewPassword: UIView!
    // @IBOutlet private var segSecurity: UISegmentedControl!
    @IBOutlet private var lcScrollViewBottom: NSLayoutConstraint!

    // var uuid: UUID?
    var appDelegate: AppDelegate?

    override func viewDidLoad() {
        super.viewDidLoad()

        appDelegate = (UIApplication.shared.delegate as? AppDelegate)!

        extendedLayoutIncludesOpaqueBars = true

        NotificationCenter.default.addObserver(self, selector: #selector(keyboardWillShow(notification:)), name: UIResponder.keyboardWillShowNotification, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(keyboardWillHide(notification:)), name: UIResponder.keyboardWillHideNotification, object: nil)

        let preferences = UserDefaults.standard

        if preferences.object(forKey: "cIPoolId") != nil {
            // save.setTitle("Modify", for: UIControl.State.normal)
        } else {
            skip.isHidden = true
        }

        tfRegion.isUserInteractionEnabled = false
        tfIOTPolicyName.isUserInteractionEnabled = false
        tfCIPoolId1.isUserInteractionEnabled = false
        tfCUPoolId.isUserInteractionEnabled = false
        tfCUACId.isUserInteractionEnabled = false
        tfCUACS.isUserInteractionEnabled = false

        hideCognitoView(isHide: true)
    }
}

// Keyboard

extension CognitoInfoViewController {

    @objc
    func keyboardWillShow(notification: NSNotification) {
        guard let keyboardSize = (notification.userInfo?[UIResponder.keyboardFrameEndUserInfoKey] as? NSValue)?.cgRectValue else {
            return
        }
        lcScrollViewBottom.constant = keyboardSize.height - view.safeAreaInsets.bottom
        UIView.performWithoutAnimation {
            view.layoutIfNeeded()
        }
    }

    @objc
    func keyboardWillHide(notification _: NSNotification) {
        lcScrollViewBottom.constant = 170.0
        UIView.performWithoutAnimation {
            view.layoutIfNeeded()
        }
    }
}

extension CognitoInfoViewController {
    func readFileFromStorage() {

        let types: [String] = [kUTTypeText as String]
        let documentPicker = UIDocumentPickerViewController(documentTypes: types, in: .import)
        documentPicker.delegate = self
        documentPicker.modalPresentationStyle = .formSheet
        present(documentPicker, animated: true, completion: nil)
    }

    @available(iOS 8.0, *)
    public func documentPicker(_: UIDocumentPickerViewController, didPickDocumentAt url: URL) {
        let aws = url as URL
        do {
            let awsData = try NSData(contentsOf: aws, options: NSData.ReadingOptions())
            let awsStr = String(decoding: awsData, as: UTF8.self)
            print(awsStr)
            let awsStrArr = awsStr.components(separatedBy: .newlines)
            if !awsStrArr.isEmpty {
                hideCognitoView(isHide: false)
                for awsStr in awsStrArr {
                    print("\(awsStr)")
                    let awsItem = awsStr.components(separatedBy: "#")
                    if awsItem[0].trimmingCharacters(in: .whitespacesAndNewlines) == "AWS_REGION" {
                        tfRegion.text = awsItem[1]
                    } else if awsItem[0].trimmingCharacters(in: .whitespacesAndNewlines) == "AWS_IOT_POLICY" {
                        tfIOTPolicyName.text = awsItem[1]
                    } else if awsItem[0].trimmingCharacters(in: .whitespacesAndNewlines) == "AWS_COGNITO_IDENTITY_POOLID" {
                        tfCIPoolId1.text = awsItem[1]
                    } else if awsItem[0].trimmingCharacters(in: .whitespacesAndNewlines) == "AWS_COGNITO_USER_POOLID" {
                        tfCUPoolId.text = awsItem[1]
                    } else if awsItem[0].trimmingCharacters(in: .whitespacesAndNewlines) == "AWS_COGNITO_USER_APPCLIENTID" {
                        tfCUACId.text = awsItem[1]
                    } else if awsItem[0].trimmingCharacters(in: .whitespacesAndNewlines) == "AWS_COGNITO_USER_APPCLIENTSECRET" {
                        tfCUACS.text = awsItem[1]
                    }
                }
            }
        } catch {
            print(error)
        }
    }

    @available(iOS 8.0, *)
    public func documentMenu(_: UIDocumentMenuViewController, didPickDocumentPicker documentPicker: UIDocumentPickerViewController) {

        documentPicker.delegate = self
        present(documentPicker, animated: true, completion: nil)
    }

    func documentPickerWasCancelled(_: UIDocumentPickerViewController) {
        print(" cancelled by user")
        dismiss(animated: true, completion: nil)
    }

    // @IBAction private func segSecurityValueChanged(_: UISegmentedControl) {}

    @IBAction private func btnSaveInfoPush(_: UIButton) {
        guard let cIRegion = tfRegion.text?.trimmingCharacters(in: .whitespaces), let cIPolicyName = tfIOTPolicyName.text?.trimmingCharacters(in: .whitespaces), let cIPoolID = tfCIPoolId1.text?.trimmingCharacters(in: .whitespaces), let cUPoolId = tfCUPoolId.text?.trimmingCharacters(in: .whitespaces), let cUAppCId = tfCUACId.text?.trimmingCharacters(in: .whitespaces), let cUAppSec = tfCUACS.text?.trimmingCharacters(in: .whitespaces), !cIRegion.isEmpty || !cIPolicyName.isEmpty || !cIPoolID.isEmpty || !cUPoolId.isEmpty || !cUAppCId.isEmpty || !cUAppSec.isEmpty else {
            Alertift.alert(title: NSLocalizedString("SSK APP", comment: String()), message: NSLocalizedString("Please enter all fields.", comment: String()))
                .action(.default(NSLocalizedString("OK", comment: String())))
                .show(on: self)
            return
        }

        let preferences = UserDefaults.standard
        preferences.set(cIPolicyName, forKey: "cIPolicyName")
        preferences.set(cIPoolID, forKey: "cIPoolId")
        preferences.set(cIRegion, forKey: "cIRegion")
        preferences.set(cUPoolId, forKey: "cUPPoolId")
        preferences.set(cUAppCId, forKey: "cUPAppClientId")
        preferences.set(cUAppSec, forKey: "cUPAppClientSecret")
        preferences.synchronize()

        let story = UIStoryboard(name: "Main", bundle: nil)
        let objVC = story.instantiateViewController(withIdentifier: "DevicesViewController")
        let navCon = UINavigationController(rootViewController: objVC)

        // edited by alpesh
        let jsonString = "{\"UserAgent\": \"MobileHub/1.0\",\"Version\": \"1.0\",\"CredentialsProvider\": {\"CognitoIdentity\":{\"Default\": {\"PoolId\": \"" + cIPoolID + "\",\"Region\":\"" + cIRegion + "\"}}},\"IdentityManager\": {\"Default\":{}},\"CognitoUserPool\": {\"Default\": {\"PoolId\": \"" + cUPoolId + "\",\"AppClientId\":\"" + cUAppCId + "\",\"AppClientSecret\":\"" + cUAppSec + "\",\"Region\": \"" + cIRegion + "\"}}}"

        // write to test file
        do {
            let fm = FileManager.default
            let urls = fm.urls(for: .documentDirectory, in: .userDomainMask)
            if let url = urls.first {
                var fileURL = url.appendingPathComponent("awsconfiguration")
                fileURL = fileURL.appendingPathExtension("json")
                try jsonString.write(to: fileURL,
                                     atomically: true,
                                     encoding: .utf8)
            }
        } catch {
            // Handle error
        }

        // read json file
//        do {
//            let fm = FileManager.default
//            let urls = fm.urls(for: .documentDirectory, in: .userDomainMask)
//            if let url = urls.first {
//                var fileURL = url.appendingPathComponent("awsconfiguration")
//                fileURL = fileURL.appendingPathExtension("json")
//                let data = try Data(contentsOf: fileURL)
//                let jsonObject = try JSONSerialization.jsonObject(with: data, options: [.mutableContainers, .mutableLeaves])
//                print(jsonObject)
//            }
//        } catch {
//            // Handle error
//        }

        appDelegate!.window?.rootViewController = navCon
    }

    @IBAction private func btnSkipPush(_: UIButton) {
        let story = UIStoryboard(name: "Main", bundle: nil)
        let objVC = story.instantiateViewController(withIdentifier: "DevicesViewController")
        let navCon = UINavigationController(rootViewController: objVC)
        appDelegate!.window?.rootViewController = navCon
    }

    @IBAction private func btnBrowsePush(_: UIButton) {
        readFileFromStorage()
    }

    func hideCognitoView(isHide: Bool) {
        save.isHidden = isHide
        view1.isHidden = isHide
        view2.isHidden = isHide
        view3.isHidden = isHide
        view4.isHidden = isHide
        view5.isHidden = isHide
        view6.isHidden = isHide
    }
}
