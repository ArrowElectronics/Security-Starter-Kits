import AmazonFreeRTOS
import AWSMobileClient
import UIKit

@UIApplicationMain
class AppDelegate: UIResponder, UIApplicationDelegate {

    var window: UIWindow?
    // var navCon: UINavigationController?
    // var navCon1: UINavigationController?

    func application(_: UIApplication, didFinishLaunchingWithOptions _: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {

        // FreeRTOS SDK Logging, will switch to AWSDDLog in future releases
        _ = AmazonContext.shared
        AmazonFreeRTOSManager.shared.isDebug = true

        let story = UIStoryboard(name: "Main", bundle: nil)
        let objVC = story.instantiateViewController(withIdentifier: "CognitoInfoViewController")
        let navCon = UINavigationController(rootViewController: objVC)
        window?.rootViewController = navCon
        // Override advertising Service UUIDs if needed.
        // AmazonFreeRTOSManager.shared.advertisingServiceUUIDs = []

        // AWS SDK Logging
        // AWSDDLog.sharedInstance.logLevel = .all
        // AWSDDLog.add(AWSDDTTYLogger.sharedInstance)

        // Setup the user sign-in with cognito: https://aws-amplify.github.io/docs/ios/authentication#manual-setup
        // AWSServiceManager.default().defaultServiceConfiguration = AWSServiceConfiguration(region: AmazonConstants.AWS.region, credentialsProvider: AWSMobileClient.default())
        return true
    }
}
