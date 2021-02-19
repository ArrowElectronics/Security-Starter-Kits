package software.amazon.freertos.demo.utils;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;

public class Preferences {
    public static final String PREF_KEY_AWS_REGION = "pref_key_aws_region";
    public static final String PREF_KEY_AWS_ITPOLITY = "pref_key_aws_it_policy";
    public static final String PREF_KEY_AWS_COGNITO_IDENTITY_POOLID = "pref_key_aws_cognito_identify_poolid";
    public static final String PREF_KEY_AWS_COGNITO_USER_POOLID = "pref_key_aws_cognito_user_poolid";
    public static final String PREF_KEY_AWS_COGNITO_USER_CLIENTID = "pref_key_aws_cognito_user_clientid";
    public static final String PREF_KEY_AWS_COGNITO_USER_SECRETID = "pref_key_aws_cognito_user_secretid";

    SharedPreferences sharedPref;
    Context mContext;

    public Preferences(Context context) {
        this.mContext = context;
        sharedPref = PreferenceManager.getDefaultSharedPreferences(mContext);
    }

    public void setAWSRegion(String name) {
        SharedPreferences.Editor edit = sharedPref.edit();
        edit.putString(Preferences.PREF_KEY_AWS_REGION, name);
        edit.commit();
    }
    public String getAWSRegion() {
        return sharedPref.getString(Preferences.PREF_KEY_AWS_REGION, "");
    }

    public void setAWSIOTName(String name) {
        SharedPreferences.Editor edit = sharedPref.edit();
        edit.putString(Preferences.PREF_KEY_AWS_ITPOLITY, name);
        edit.commit();
    }
    public String getAWSIOTName() {
        return sharedPref.getString(Preferences.PREF_KEY_AWS_ITPOLITY, "");
    }

    public void setAWSCognitoIdentityPoolID(String name) {
        SharedPreferences.Editor edit = sharedPref.edit();
        edit.putString(Preferences.PREF_KEY_AWS_COGNITO_IDENTITY_POOLID, name);
        edit.commit();
    }
    public String getAWSCognitoIdentityPoolID() {
        return sharedPref.getString(Preferences.PREF_KEY_AWS_COGNITO_IDENTITY_POOLID, "");
    }

    public void setAWSCognitoUserPoolId(String name) {
        SharedPreferences.Editor edit = sharedPref.edit();
        edit.putString(Preferences.PREF_KEY_AWS_COGNITO_USER_POOLID, name);
        edit.commit();
    }
    public String getAWSCognitoUserPoolId() {
        return sharedPref.getString(Preferences.PREF_KEY_AWS_COGNITO_USER_POOLID, "");
    }

    public void setAWSCognitoUserClientId(String name) {
        SharedPreferences.Editor edit = sharedPref.edit();
        edit.putString(Preferences.PREF_KEY_AWS_COGNITO_USER_CLIENTID, name);
        edit.commit();
    }
    public String getAWSCognitoUserClientId() {
        return sharedPref.getString(Preferences.PREF_KEY_AWS_COGNITO_USER_CLIENTID, "");
    }

    public void setAWSCognitoUserClientSecret(String name) {
        SharedPreferences.Editor edit = sharedPref.edit();
        edit.putString(Preferences.PREF_KEY_AWS_COGNITO_USER_SECRETID, name);
        edit.commit();
    }
    public String getAWSCognitoUserClientSecret() {
        return sharedPref.getString(Preferences.PREF_KEY_AWS_COGNITO_USER_SECRETID, "");
    }

}
