//
// Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License").
// You may not use this file except in compliance with the License.
// A copy of the License is located at
//
// http://aws.amazon.com/apache2.0
//
// or in the "license" file accompanying this file. This file is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied. See the License for the specific language governing
// permissions and limitations under the License.
//

#import "AWSCognitoUserPoolsSignInProvider.h"
#import <AWSAuthCore/AWSAuthCore.h>

NSString *const AWSCognitoUserPoolsSignInProviderKey = @"CognitoUserPools";

typedef void (^AWSSignInManagerCompletionBlock)(id result, NSError *error);

@interface AWSSignInManager()

- (void)completeLogin;

@end

@interface AWSCognitoUserPoolsSignInProvider()

@property (strong, nonatomic) UIViewController *signInViewController;
@property (atomic, copy) AWSSignInManagerCompletionBlock completionHandler;
@property (strong, nonatomic) id<AWSCognitoUserPoolsSignInHandler> interactiveAuthenticationDelegate;
@property (strong, nonatomic) AWSCognitoIdentityUserPool *currentUserPool;

@end

@implementation AWSCognitoUserPoolsSignInProvider

+ (void)setupUserPoolWithId:(NSString *)cognitoIdentityUserPoolId
cognitoIdentityUserPoolAppClientId:(NSString *)cognitoIdentityUserPoolAppClientId
cognitoIdentityUserPoolAppClientSecret:(NSString *)cognitoIdentityUserPoolAppClientSecret
                     region:(AWSRegionType)region{
    //edited by alpesh
//    NSString* strRegion = [[NSUserDefaults standardUserDefaults] stringForKey:@"cIRegion"];
//    region = AWSRegionAPSouth1;
//    cognitoIdentityUserPoolAppClientId = [[NSUserDefaults standardUserDefaults] stringForKey:@"cUPPoolId"];
//    cognitoIdentityUserPoolAppClientSecret = [[NSUserDefaults standardUserDefaults] stringForKey:@"cUPAppClientSecret"];
//    cognitoIdentityUserPoolAppClientId = [[NSUserDefaults standardUserDefaults] stringForKey:@"cUPAppClientId"];
    
    AWSServiceConfiguration *serviceConfiguration = [[AWSServiceConfiguration alloc] initWithRegion:region credentialsProvider:nil];

    AWSCognitoIdentityUserPoolConfiguration *configuration = [[AWSCognitoIdentityUserPoolConfiguration alloc]
                                                              initWithClientId:cognitoIdentityUserPoolAppClientId
                                                              clientSecret:cognitoIdentityUserPoolAppClientSecret
                                                              poolId:cognitoIdentityUserPoolId];
    [AWSCognitoIdentityUserPool registerCognitoIdentityUserPoolWithConfiguration:serviceConfiguration userPoolConfiguration:configuration forKey:AWSCognitoUserPoolsSignInProviderKey];
}

+ (instancetype)sharedInstance {
    
    if (![AWSCognitoIdentityUserPool defaultCognitoIdentityUserPool]
        && ![AWSCognitoIdentityUserPool CognitoIdentityUserPoolForKey:AWSCognitoUserPoolsSignInProviderKey]) {
        @throw [NSException exceptionWithName:NSInternalInconsistencyException
                                       reason:@"User Pool not registered. The User Pool configuration has to be set in the `awsconfiguration.json` / `info.plist` or using the method  `setupUserPoolWithId:cognitoIdentityUserPoolAppClientId:cognitoIdentityUserPoolAppClientSecret:region` before accessing the shared instance."
                                     userInfo:nil];
        return nil;
    }
    static AWSCognitoUserPoolsSignInProvider *_sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _sharedInstance = [[AWSCognitoUserPoolsSignInProvider alloc] init];
        if ([AWSCognitoIdentityUserPool defaultCognitoIdentityUserPool]) {
            _sharedInstance.currentUserPool = [AWSCognitoIdentityUserPool defaultCognitoIdentityUserPool];
        } else {
            _sharedInstance.currentUserPool = [AWSCognitoIdentityUserPool CognitoIdentityUserPoolForKey:AWSCognitoUserPoolsSignInProviderKey];
        }
    });
    
    return _sharedInstance;
}

- (AWSCognitoIdentityUserPool *)getUserPool {
    return self.currentUserPool;
}

- (void)setViewControllerForUserPoolsSignIn:(UIViewController *)signInViewController {
    self.signInViewController = signInViewController;
}


#pragma mark - AWSIdentityProvider

- (NSString *)identityProviderName {
    return self.currentUserPool.identityProviderName;
}

- (AWSTask<NSString *> *)token {
    AWSCognitoIdentityUserPool *pool = [self getUserPool];
    return [[[pool currentUser] getSession] continueWithSuccessBlock:^id _Nullable(AWSTask<AWSCognitoIdentityUserSession *> * _Nonnull task) {
        return [AWSTask taskWithResult:task.result.idToken.tokenString];
    }];
}

- (BOOL)isLoggedIn {
    AWSCognitoIdentityUserPool *pool = [self getUserPool];
    return [pool.currentUser isSignedIn];
}

- (void)reloadSession {
    if ([self isLoggedIn]) {
        [self completeLogin];
    }
}

- (void)completeLogin {
    [[AWSSignInManager sharedInstance] completeLogin];
}

- (void)setInteractiveAuthDelegate:(id)interactiveAuthDelegate {
    self.interactiveAuthenticationDelegate = interactiveAuthDelegate;
    [self getUserPool].delegate = interactiveAuthDelegate;
}

- (void)login:(AWSSignInManagerCompletionBlock) completionHandler {
    self.completionHandler = completionHandler;
    AWSCognitoIdentityUserPool *pool = [self getUserPool];
    [[pool.getUser getSession] continueWithSuccessBlock:^id _Nullable(AWSTask<AWSCognitoIdentityUserSession *> * _Nonnull task) {
        [self completeLogin];
        return nil;
    }];
    [self.interactiveAuthenticationDelegate handleUserPoolSignInFlowStart];
}

- (void)logout {
    AWSCognitoIdentityUserPool *pool = [self getUserPool];
    [pool.currentUser signOut];
}

- (BOOL)interceptApplication:(UIApplication *)application
didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    return YES;
}

- (BOOL)interceptApplication:(UIApplication *)application
                     openURL:(NSURL *)url
           sourceApplication:(NSString *)sourceApplication
                  annotation:(id)annotation {
    
    return YES;
}

//+ (AWSRegionType) getRegionName:(NSString *)cIReg{
//    AWSRegionType reginSel;
//    if(cIReg == @"ca-central-1") {
//        reginSel = AWSRegionCACentral1;
//    } else if (cIReg == @"cn-north-1") {
//        reginSel = AWSRegionCNNorth1;
//    } else if (cIReg == @"sa-east-1") {
//        reginSel = AWSRegionSAEast1;
//    } else if (cIReg == @"ap-southeast-2") {
//        reginSel = AWSRegionAPSoutheast2;
//    } else if (cIReg == @"ap-northeast-2") {
//        reginSel = AWSRegionAPNortheast2;
//    } else if (cIReg == @"ap-northeast-1") {
//        reginSel = AWSRegionAPNortheast1;
//    } else if (cIReg == @"ap-southeast-1") {
//        reginSel = AWSRegionAPSoutheast1;
//    } else if (cIReg == @"eu-central-1") {
//        reginSel = AWSRegionEUCentral1;
//    } else if (cIReg == @"eu-west-2") {
//        reginSel = AWSRegionEUWest2;
//    } else if (cIReg == @"eu-west-1") {
//        reginSel = AWSRegionEUWest1;
//    } else if (cIReg == @"us-west-2") {
//        reginSel = AWSRegionUSWest2;
//    } else if (cIReg == @"us-west-1") {
//        reginSel = AWSRegionUSWest1;
//    } else if (cIReg == @"us-east-2") {
//        reginSel = AWSRegionUSEast2;
//    } else if (cIReg == @"us-east-1") {
//        reginSel = AWSRegionUSEast1;
//    } else if (cIReg == @"unknown") {
//        reginSel = AWSRegionUnknown;
//    } else if (cIReg == @"us-govwest-1") {
//        reginSel = AWSRegionUSGovWest1;
//    } else if (cIReg == @"eu-west-3") {
//        reginSel = AWSRegionEUWest3;
//    } else if (cIReg == @"cn-northwest-1") {
//        reginSel = AWSRegionCNNorthWest1;
//    } else if (cIReg == @"me-south-1") {
//        reginSel = AWSRegionMESouth1;
//    } else if (cIReg == @"ap-east-1") {
//        reginSel = AWSRegionAPEast1;
//
//    } else if (cIReg == @"eu-north-1") {
//        reginSel = AWSRegionEUNorth1;
//
//    } else if (cIReg == @"us-goveast-1") {
//        reginSel = AWSRegionUSGovEast1;
//    } else if (cIReg == @"ap-south-1") {
//        reginSel = AWSRegionAPSouth1;
//    } else if (cIReg == @"ap-east-1") {
//        reginSel = AWSRegionAPEast1;
//    }
//    return reginSel;
//}
@end
