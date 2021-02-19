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
            if (userAgent) {
                [AWSServiceConfiguration addGlobalUserAgentProductToken:userAgent];
            }
        }

        NSDictionary <NSString *, id> *defaultInfoDictionary = [_rootInfoDictionary objectForKey:AWSInfoDefault];

        NSDictionary <NSString *, id> *defaultCredentialsProviderDictionary = [[[_rootInfoDictionary objectForKey:AWSInfoCredentialsProvider] objectForKey:AWSInfoCognitoIdentity] objectForKey:AWSInfoDefault];
        NSString *cognitoIdentityPoolID = [defaultCredentialsProviderDictionary objectForKey:AWSInfoCognitoIdentityPoolId];
        AWSRegionType cognitoIdentityRegion =  [[defaultCredentialsProviderDictionary objectForKey:AWSInfoRegion] aws_regionTypeValue];
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
   // NSString *pathToAWSConfigJson1 = [[NSBundle mainBundle] pathForResource:@"awsconfiguration1" ofType:@"json"];
    NSUserDefaults *preferences = [NSUserDefaults standardUserDefaults];

    NSString *currentLevelKey = @"cIPoolId";
    NSString *pathToAWSConfigJson =@"";
    if ([preferences objectForKey:currentLevelKey] != nil)
    {
        NSString *cIPoolId = [preferences stringForKey:@"cIPoolId"];
        NSString *cIRegion = [preferences stringForKey:@"cIRegion"];
        NSString *cUPPoolId = [preferences stringForKey:@"cUPPoolId"];
        NSString *cUPAppClientId = [preferences stringForKey:@"cUPAppClientId"];
        NSString *cUPAppClientSecret = [preferences stringForKey:@"cUPAppClientSecret"];
        pathToAWSConfigJson=[NSString stringWithFormat:@"{\"UserAgent\":\"MobileHub/1.0\",\"Version\": \"1.0\",\"CredentialsProvider\": {\"CognitoIdentity\": {\"Default\": {\"PoolId\": \"%@\",\"Region\": \"%@\"}}},\"IdentityManager\": {\"Default\": {}},\"CognitoUserPool\": {\"Default\": {\"PoolId\": \"%@\",\"AppClientId\": \"%@\",\"AppClientSecret\": \"%@\",\"Region\": \"%@\"}}}",cIPoolId,cIRegion,cUPPoolId,cUPAppClientId,cUPAppClientSecret,cIRegion];
    }
    else
    {
        //  Get current level
        //return [self initWithConfiguration:config];
    }
    
    //NSString *cIPoolId = @"ap-south-1:ea502ec1-4b68-4684-9a24-d526b9745ef7";
    //NSString *cIRegion = @"ap-south-1";
    //NSString *cUPPoolId = @"ap-south-1_rEpe6QS4n";
    //NSString *cUPAppClientId = @"73v1jsebhcva9cu8r0auckf1ag";
    //NSString *cUPAppClientSecret = @"1u4alsj7phlj3um7mqvo20k71b9cfgj1eo1tc6odojso264o2fca";

    
    
    if (pathToAWSConfigJson) {
        NSData* data = [pathToAWSConfigJson dataUsingEncoding:NSUTF8StringEncoding];

        //NSData *data = [NSData dataWithContentsOfFile:pathToAWSConfigJson];
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

@end
