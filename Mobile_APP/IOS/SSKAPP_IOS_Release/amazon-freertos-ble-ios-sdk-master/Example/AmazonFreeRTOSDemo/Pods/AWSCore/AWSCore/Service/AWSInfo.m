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
#import "AWSInfo.h"
#import "AWSCategory.h"
#import "AWSCredentialsProvider.h"
#import "AWSCocoaLumberjack.h"
#import "AWSService.h"

NSString *const AWSInfoDefault = @"Default";

static NSString *const AWSInfoRoot = @"AWS";
static NSString *const AWSInfoCredentialsProvider = @"CredentialsProvider";
static NSString *const AWSInfoRegion = @"Region";
static NSString *const AWSInfoUserAgent = @"UserAgent";
static NSString *const AWSInfoCognitoIdentity = @"CognitoIdentity";
static NSString *const AWSInfoCognitoIdentityPoolId = @"PoolId";
static NSString *const AWSInfoCognitoUserPool = @"CognitoUserPool";

static NSString *const AWSInfoIdentityManager = @"IdentityManager";

@interface AWSInfo()

@property (nonatomic, strong) AWSCognitoCredentialsProvider *defaultCognitoCredentialsProvider;
@property (nonatomic, assign) AWSRegionType defaultRegion;
@property (nonatomic, strong) NSDictionary <NSString *, id> *rootInfoDictionary;

@end

@interface AWSServiceInfo()

@property (nonatomic, strong) NSDictionary <NSString *, id> *infoDictionary;

- (instancetype)initWithInfoDictionary:(NSDictionary <NSString *, id> *)infoDictionary
                           serviceName:(NSString *) serviceName;

@end

@implementation AWSInfo

static AWSInfo *_defaultAWSInfo = nil;
static NSDictionary<NSString *, id> * _userConfig = nil;

- (instancetype)initWithConfiguration:(NSDictionary<NSString *, id> *)config {
    if (self = [super init]) {
        _rootInfoDictionary = config;

        if (_rootInfoDictionary) {
            NSString *userAgent = [self.rootInfoDictionary objectForKey:AWSInfoUserAgent];
            //[self.rootInfoDictionary :AWSInfoUserAgent];

            if (userAgent) {
                [AWSServiceConfiguration addGlobalUserAgentProductToken:userAgent];
            }
        }

        NSDictionary <NSString *, id> *defaultInfoDictionary = [_rootInfoDictionary objectForKey:AWSInfoDefault];

        NSDictionary <NSString *, id> *defaultCredentialsProviderDictionary = [[[_rootInfoDictionary objectForKey:AWSInfoCredentialsProvider] objectForKey:AWSInfoCognitoIdentity] objectForKey:AWSInfoDefault];
        NSString *cognitoIdentityPoolID = [defaultCredentialsProviderDictionary objectForKey:AWSInfoCognitoIdentityPoolId];
        AWSRegionType cognitoIdentityRegion =  [[defaultCredentialsProviderDictionary objectForKey:AWSInfoRegion] aws_regionTypeValue];
        
        //edited by alpesh
//        NSString* strRegion = [[NSUserDefaults standardUserDefaults] stringForKey:@"cIRegion"];
//        //cognitoIdentityRegion = [self getRegionName1:strRegion];
//        cognitoIdentityRegion = AWSRegionAPSouth1;
//        cognitoIdentityPoolID = [[NSUserDefaults standardUserDefaults] stringForKey:@"cIPoolId"];
        
        if (cognitoIdentityPoolID && cognitoIdentityRegion != AWSRegionUnknown) {
            _defaultCognitoCredentialsProvider = [[AWSCognitoCredentialsProvider alloc] initWithRegionType:cognitoIdentityRegion
                                                                                            identityPoolId:cognitoIdentityPoolID];
        }

        _defaultRegion = [[defaultInfoDictionary objectForKey:AWSInfoRegion] aws_regionTypeValue];
    }
    return self;
}

- (instancetype)init {
    NSDictionary<NSString *, id> *config;
    // read json file
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];

    NSString *pathToAWSConfigJson = [documentsDirectory stringByAppendingPathComponent:@"awsconfiguration.json"];

//    NSString *pathToAWSConfigJson1 = [[NSBundle mainBundle] pathForResource:@"awsconfiguration"
//                                                                    ofType:@"json"];
    if (pathToAWSConfigJson) {
        NSData *data = [NSData dataWithContentsOfFile:pathToAWSConfigJson];
        
        if (!data) {
            AWSDDLogError(@"Couldn't read the awsconfiguration.json file. Skipping load of awsconfiguration.json.");
        } else {
            NSError *error = nil;
            NSDictionary <NSString *, id> *jsonDictionary = [NSJSONSerialization JSONObjectWithData:data
                                                                                            options:kNilOptions
                                                                                              error:&error];
            if (!jsonDictionary || [jsonDictionary count] <= 0 || error) {
                AWSDDLogError(@"Couldn't deserialize data from the JSON file or the contents are empty. Please check the awsconfiguration.json file.");
            } else {
                config = jsonDictionary;
            }
        }
        
    } else {
        AWSDDLogDebug(@"Couldn't locate the awsconfiguration.json file. Skipping load of awsconfiguration.json.");
    }
    
    if (!config) {
        config = [[[NSBundle mainBundle] infoDictionary] objectForKey:AWSInfoRoot];
    }

    return [self initWithConfiguration:config];
}

+ (instancetype)defaultAWSInfo {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        if (_userConfig) {
            _defaultAWSInfo = [[AWSInfo alloc] initWithConfiguration:_userConfig];
        } else {
            _defaultAWSInfo = [AWSInfo new];
        }
    });

    return _defaultAWSInfo;
}

+ (void)configureDefaultAWSInfo:(NSDictionary<NSString *, id> *)config {
    if (_defaultAWSInfo) {
        AWSDDLogWarn(@"Configuration already set, you cannot call configure after AWSInfo is created.");
    } else {
        _userConfig = config;
    }
}

+ (void)overrideCredentialsProvider:(AWSCognitoCredentialsProvider *)cognitoCredentialsProvider {
    AWSInfo.defaultAWSInfo.defaultCognitoCredentialsProvider = cognitoCredentialsProvider;
}

- (AWSServiceInfo *)serviceInfo:(NSString *)serviceName
                         forKey:(NSString *)key {
    NSDictionary <NSString *, id> *infoDictionary = [[self.rootInfoDictionary objectForKey:serviceName] objectForKey:key];
    return [[AWSServiceInfo alloc] initWithInfoDictionary:infoDictionary
                                              serviceName:serviceName];
}

- (AWSServiceInfo *)defaultServiceInfo:(NSString *)serviceName {
    return [self serviceInfo:serviceName
                      forKey:AWSInfoDefault];
}

@end

@implementation AWSServiceInfo

- (instancetype)initWithInfoDictionary:(NSDictionary <NSString *, id> *)infoDictionary
                           serviceName:(NSString *) serviceName {
    if (self = [super init]) {
        BOOL checkRegion = ![serviceName isEqualToString:AWSInfoIdentityManager];
        _infoDictionary = infoDictionary;
        if (!_infoDictionary) {
            _infoDictionary = @{};
        }
        
        _cognitoCredentialsProvider = [AWSInfo defaultAWSInfo].defaultCognitoCredentialsProvider;
        
        _region = [[_infoDictionary objectForKey:AWSInfoRegion] aws_regionTypeValue];
        if (_region == AWSRegionUnknown) {
            _region = [AWSInfo defaultAWSInfo].defaultRegion;
        }
        
        //If there is no credentials provider configured and this isn't Cognito User Pools (which
        //doesn't need one)
        if (!_cognitoCredentialsProvider && ![serviceName isEqualToString:AWSInfoCognitoUserPool]) {
            if (![AWSServiceManager defaultServiceManager].defaultServiceConfiguration) {
                AWSDDLogDebug(@"Couldn't read credentials provider configuration from `awsconfiguration.json` / `Info.plist`. Please check your configuration file if you are loading the configuration through it.");
            }
            return nil;
        }
        
        if (checkRegion
            && _region == AWSRegionUnknown) {
            if (![AWSServiceManager defaultServiceManager].defaultServiceConfiguration) {
                AWSDDLogDebug(@"Couldn't read the region configuration from `awsconfiguration.json` / `Info.plist`. Please check your configuration file if you are loading the configuration through it.");
            }
            return nil;
        }
    }
    
    return self;
}
- (AWSRegionType) getRegionName1:(NSString *)cIReg{
    AWSRegionType reginSel;
    if(cIReg == @"ca-central-1") {
        reginSel = AWSRegionCACentral1;
    } else if (cIReg == @"cn-north-1") {
        reginSel = AWSRegionCNNorth1;
    } else if (cIReg == @"sa-east-1") {
        reginSel = AWSRegionSAEast1;
    } else if (cIReg == @"ap-southeast-2") {
        reginSel = AWSRegionAPSoutheast2;
    } else if (cIReg == @"ap-northeast-2") {
        reginSel = AWSRegionAPNortheast2;
    } else if (cIReg == @"ap-northeast-1") {
        reginSel = AWSRegionAPNortheast1;
    } else if (cIReg == @"ap-southeast-1") {
        reginSel = AWSRegionAPSoutheast1;
    } else if (cIReg == @"eu-central-1") {
        reginSel = AWSRegionEUCentral1;
    } else if (cIReg == @"eu-west-2") {
        reginSel = AWSRegionEUWest2;
    } else if (cIReg == @"eu-west-1") {
        reginSel = AWSRegionEUWest1;
    } else if (cIReg == @"us-west-2") {
        reginSel = AWSRegionUSWest2;
    } else if (cIReg == @"us-west-1") {
        reginSel = AWSRegionUSWest1;
    } else if (cIReg == @"us-east-2") {
        reginSel = AWSRegionUSEast2;
    } else if (cIReg == @"us-east-1") {
        reginSel = AWSRegionUSEast1;
    } else if (cIReg == @"unknown") {
        reginSel = AWSRegionUnknown;
    } else if (cIReg == @"us-govwest-1") {
        reginSel = AWSRegionUSGovWest1;
    } else if (cIReg == @"eu-west-3") {
        reginSel = AWSRegionEUWest3;
    } else if (cIReg == @"cn-northwest-1") {
        reginSel = AWSRegionCNNorthWest1;
    } else if (cIReg == @"me-south-1") {
        reginSel = AWSRegionMESouth1;
    } else if (cIReg == @"ap-east-1") {
        reginSel = AWSRegionAPEast1;

    } else if (cIReg == @"eu-north-1") {
        reginSel = AWSRegionEUNorth1;

    } else if (cIReg == @"us-goveast-1") {
        reginSel = AWSRegionUSGovEast1;
    } else if (cIReg == @"ap-south-1") {
        reginSel = AWSRegionAPSouth1;
    } else if (cIReg == @"ap-east-1") {
        reginSel = AWSRegionAPEast1;
    }
    return reginSel;
}
@end
