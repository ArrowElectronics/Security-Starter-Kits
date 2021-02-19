package software.amazon.freertos.demo;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Switch;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import software.amazon.freertos.demo.utils.Preferences;


public class CognitoInfoActivity extends AppCompatActivity {
    EditText edtRegion;
    EditText edtAWSIOTName;
    EditText edtAWSCogIdePool;
    EditText edtAWSCogUserPoolId;
    EditText edtAWSCogUserAppClientId;
    EditText edtAWSCogUserAppClientSecret;
    Button btnSave;
    Button btnSkip;
    Preferences preference;
    private String filePath = null;
    private File sourceFile;

    private static final int FILE_SELECT_CODE = 0;
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.cognitoinfo);
        HideCognitoView(false);
        preference =  new Preferences(getApplicationContext());
        edtRegion = (EditText)findViewById(R.id.awsRegionEDT);
        edtAWSIOTName = (EditText)findViewById(R.id.awsIOTPolicyEDT);
        edtAWSCogIdePool = (EditText)findViewById(R.id.awsCogIdePoolIdEDT);
        edtAWSCogUserPoolId = (EditText)findViewById(R.id.awsCogUserPoolIdEDT);
        edtAWSCogUserAppClientId = (EditText)findViewById(R.id.awsCogUserAppClientIdEDT);
        edtAWSCogUserAppClientSecret = (EditText)findViewById(R.id.awsCogUserAppClientSecretEDT);
        btnSave = (Button)findViewById(R.id.btnCognitoSave);
        btnSkip = (Button)findViewById(R.id.btnCogniSkip);
        if(preference.getAWSRegion().isEmpty() || preference.getAWSIOTName().isEmpty()){
            btnSkip.setVisibility(View.INVISIBLE);
        }else{
            //btnSave.setText("Modify");
        }
    }

    public void SaveCognitoDetailsClick(View view){
        if(edtRegion.getText().toString().trim().isEmpty() || edtAWSIOTName.getText().toString().trim().isEmpty() || edtAWSCogIdePool.getText().toString().trim().isEmpty() || edtAWSCogUserPoolId.getText().toString().trim().isEmpty()|| edtAWSCogUserAppClientId.getText().toString().trim().isEmpty() || edtAWSCogUserAppClientSecret.getText().toString().trim().isEmpty()){
            Toast.makeText(getApplicationContext(),"Required all cognito settings",Toast.LENGTH_LONG).show();
        }else{
            preference.setAWSRegion(edtRegion.getText().toString().trim());
            preference.setAWSIOTName(edtAWSIOTName.getText().toString().trim());
            preference.setAWSCognitoIdentityPoolID(edtAWSCogIdePool.getText().toString().trim());
            preference.setAWSCognitoUserPoolId(edtAWSCogUserPoolId.getText().toString().trim());
            preference.setAWSCognitoUserClientId(edtAWSCogUserAppClientId.getText().toString().trim());
            preference.setAWSCognitoUserClientSecret(edtAWSCogUserAppClientSecret.getText().toString().trim());

            Toast.makeText(getApplicationContext(),"Saved Cognito details successfully",Toast.LENGTH_LONG).show();
            Intent intent = new Intent(CognitoInfoActivity.this, AuthenticatorActivity.class);
            startActivity(intent);
            finish();
        }
    }
    public void SkipCognitoDetailsClick(View view) {
        Intent intent = new Intent(CognitoInfoActivity.this, AuthenticatorActivity.class);
        startActivity(intent);
        finish();
    }

    public void BrowseForCognitoDetailsClick(View view){
        showFileChooser();

    }
    private void showFileChooser() {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.setType("*/*");
        intent.addCategory(Intent.CATEGORY_OPENABLE);

        try {
            startActivityForResult(
                    Intent.createChooser(intent, "Select a File"),
                    FILE_SELECT_CODE);
        } catch (android.content.ActivityNotFoundException ex) {
            Toast.makeText(this, "Please install a File Manager.",
                    Toast.LENGTH_SHORT).show();
        }
    }
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case FILE_SELECT_CODE:
                if (resultCode == RESULT_OK) {
                    HideCognitoView(true);

                    Uri content_describer = data.getData();
                    BufferedReader reader = null;
                    try {
                        // open the user-picked file for reading:
                        InputStream in = getContentResolver().openInputStream(content_describer);
                        // now read the content:
                        reader = new BufferedReader(new InputStreamReader(in));
                        String line;
                        StringBuilder builder = new StringBuilder();
                        while ((line = reader.readLine()) != null){
                            builder.append(line);
                            String[] strArr=line.split("#");
                            if(strArr.length == 2){
                                SetCognitoDetailsOnEditText(strArr);
                            }
                        }
                        // Do something with the content in
                        //some_view.setText(builder.toString());
                        //Toast.makeText(this, "File content : "+ builder.toString(),
                          //      Toast.LENGTH_SHORT).show();

                    } catch (FileNotFoundException e) {
                        e.printStackTrace();
                    } catch (IOException e) {
                        e.printStackTrace();
                    } finally {
                        if (reader != null) {
                            try {
                                reader.close();
                            } catch (IOException e) {
                                e.printStackTrace();
                            }
                        }
                    }



                }

                break;
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    private void SetCognitoDetailsOnEditText(String[] strArr){
        String key=strArr[0].trim();
        String value=strArr[1].trim();
        if(!key.isEmpty() && !value.isEmpty()){
            //Log.e("Cognito_Details","key : "+key +" value : "+value);

            switch (key){
                case "AWS_REGION":
                    edtRegion.setText(value);
                    break;
                case "AWS_IOT_POLICY":
                    edtAWSIOTName.setText(value);
                    break;
                case "AWS_COGNITO_IDENTITY_POOLID":
                    edtAWSCogIdePool.setText(value);
                    break;
                case "AWS_COGNITO_USER_POOLID":
                    edtAWSCogUserPoolId.setText(value);
                    break;
                case "AWS_COGNITO_USER_APPCLIENTID":
                    edtAWSCogUserAppClientId.setText(value);
                    break;
                case "AWS_COGNITO_USER_APPCLIENTSECRET":
                    edtAWSCogUserAppClientSecret.setText(value);
                    break;
                default:
                    break;

            }
        }
    }

    private void HideCognitoView(boolean isVisible){
        findViewById(R.id.linearlay1).setVisibility(isVisible?View.VISIBLE:View.INVISIBLE);
        findViewById(R.id.linearlay2).setVisibility(isVisible?View.VISIBLE:View.INVISIBLE);
        findViewById(R.id.linearlay3).setVisibility(isVisible?View.VISIBLE:View.INVISIBLE);
        findViewById(R.id.linearlay4).setVisibility(isVisible?View.VISIBLE:View.INVISIBLE);
        findViewById(R.id.linearlay5).setVisibility(isVisible?View.VISIBLE:View.INVISIBLE);
        findViewById(R.id.linearlay6).setVisibility(isVisible?View.VISIBLE:View.INVISIBLE);
        findViewById(R.id.linearlay7).setVisibility(isVisible?View.VISIBLE:View.INVISIBLE);
    }
}
