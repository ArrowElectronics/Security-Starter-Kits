package com.eic.sskwb55;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;

import com.amazonaws.auth.AWSCredentialsProvider;
import com.amazonaws.mobile.client.AWSMobileClient;
import com.amazonaws.mobile.client.Callback;
import com.amazonaws.mobile.client.SignInUIOptions;
import com.amazonaws.mobile.client.UserStateDetails;
import com.amazonaws.mobile.config.AWSConfiguration;
import com.amazonaws.regions.Region;
import com.amazonaws.services.iot.AWSIotClient;
import com.amazonaws.services.iot.model.AttachPolicyRequest;


import org.json.JSONException;
import org.json.JSONObject;

import java.util.concurrent.CountDownLatch;

import com.eic.sskwb55.utils.Preferences;



public class AuthenticatorActivity extends AppCompatActivity {
    private final static String TAG = "AuthActivity";
    private HandlerThread handlerThread;
    private Handler handler;
    Preferences preference;

    public static Intent newIntent(Context packageContext) {
        Intent intent = new Intent(packageContext, AuthenticatorActivity.class);
        return intent;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_fragment);

        preference =  new Preferences(getApplicationContext());

        if (handlerThread == null) {
            handlerThread = new HandlerThread("SignInThread");
            handlerThread.start();
            handler = new Handler(handlerThread.getLooper());
        }
        final CountDownLatch latch = new CountDownLatch(1);

        try{
            JSONObject jsonObj = GetCognitoDetails(preference.getAWSCognitoIdentityPoolID(),preference.getAWSRegion(),preference.getAWSCognitoUserPoolId(),preference.getAWSCognitoUserClientId(),preference.getAWSCognitoUserClientSecret());
            AWSConfiguration config = new AWSConfiguration(jsonObj);
            AWSMobileClient.getInstance().initialize(getApplicationContext(),config, new Callback<UserStateDetails>() {

                        @Override
                        public void onResult(UserStateDetails userStateDetails) {
                            Log.i(TAG, "AWSMobileClient initialization onResult: " + userStateDetails.getUserState());
                            latch.countDown();
                        }

                        @Override
                        public void onError(Exception e) {
                            Log.e(TAG, "Initialization error.", e);
                            //AWSMobileClient.getInstance().signOut();
                            //finish();
                        }
                    }
            );
            Log.d(TAG, "waiting for AWSMobileClient Initialization");
            try {
                latch.await();
            } catch (Exception e) {
                e.printStackTrace();
            }

            handler.post(new Runnable() {
                @Override
                public void run() {
                    if (AWSMobileClient.getInstance().isSignedIn()) {
                        signinsuccessful();
                    } else {
                        AWSMobileClient.getInstance().showSignIn(
                                AuthenticatorActivity.this,
                                SignInUIOptions.builder()
                                        .nextActivity(null)
                                        .build(),
                                new Callback<UserStateDetails>() {
                                    @Override
                                    public void onResult(UserStateDetails result) {
                                        Log.d(TAG, "onResult: " + result.getUserState());
                                        switch (result.getUserState()) {
                                            case SIGNED_IN:
                                                Log.i(TAG, "logged in!");
                                                signinsuccessful();
                                                finish();
                                                break;
                                            case SIGNED_OUT:
                                                Log.i(TAG, "onResult: User did not choose to sign-in");
                                                break;
                                            default:
                                                AWSMobileClient.getInstance().signOut();
                                                break;
                                        }
                                    }

                                    @Override
                                    public void onError(Exception e) {
                                        Log.e(TAG, "onError: ", e);
                                        //AWSMobileClient.getInstance().signOut();
                                        //finish();
                                    }
                                }
                        );
                    }
                }
            });

        }catch (Exception ex){
            //AWSMobileClient.getInstance().signOut();
            //RedirectToCognitoPage();
        }


    }

    private void signinsuccessful() {
        Log.i(TAG, "signinsuccessful");
            try{
                AWSCredentialsProvider credentialsProvider = AWSMobileClient.getInstance();
                AWSIotClient awsIotClient = new AWSIotClient(credentialsProvider);
                awsIotClient.setRegion(Region.getRegion(preference.getAWSRegion()));

                AttachPolicyRequest attachPolicyRequest = new AttachPolicyRequest()
                        .withPolicyName(preference.getAWSIOTName())
                        .withTarget(AWSMobileClient.getInstance().getIdentityId());
                try{
                    awsIotClient.attachPolicy(attachPolicyRequest);
                }catch (Exception ex){
                    Log.i(TAG, "IOT Policy "+ex.getMessage());
                }
                Intent i = new Intent(this, DeviceScanActivity.class);
                startActivity(i);
                finish();
                Log.i(TAG, "Iot policy attached successfully.");
            }catch (Exception ex){
                //RedirectToCognitoPage();
                Log.i(TAG, "IOT Policy "+ex.getMessage());

            }
    }

    public JSONObject GetCognitoDetails(String cIPoolId, String cIRegion, String cUPPoolId, String cUPAppClientId, String cUPAppClientSecret){
        String jsonStr="{\n" +
                "  \"UserAgent\": \"MobileHub/1.0\",\n" +
                "  \"Version\": \"1.0\",\n" +
                "  \"CredentialsProvider\": {\n" +
                "    \"CognitoIdentity\": {\n" +
                "      \"Default\": {\n" +
                "        \"PoolId\": \"" + cIPoolId + "\",\n" +
                "        \"Region\": \"" + cIRegion + "\"\n" +
                "      }\n" +
                "    }\n" +
                "  },\n" +
                "  \"IdentityManager\": {\n" +
                "    \"Default\": {}\n" +
                "  },\n" +
                "  \"CognitoUserPool\": {\n" +
                "    \"Default\": {\n" +
                "      \"PoolId\": \"" + cUPPoolId + "\",\n" +
                "      \"AppClientId\": \"" + cUPAppClientId + "\",\n" +
                "      \"AppClientSecret\": \"" + cUPAppClientSecret + "\",\n" +
                "      \"Region\": \"" + cIRegion + "\"\n" +
                "    }\n" +
                "  }\n" +
                "}";
        JSONObject jsonObj = null;

        Log.e("JSON Str ",jsonStr);
        try {
            jsonObj = new JSONObject(jsonStr);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return jsonObj;
    }

    private void RedirectToCognitoPage(){
        Toast.makeText(getApplicationContext(),"Please verify cognito settings",Toast.LENGTH_LONG).show();
        startActivity(new Intent(AuthenticatorActivity.this,CognitoInfoActivity.class));
        finish();
    }


}
