package software.amazon.freertos.demo;

import android.Manifest;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.le.ScanResult;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.location.LocationManager;
import android.os.Bundle;
import android.provider.Settings;
import android.support.v4.app.Fragment;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.PopupMenu;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import com.amazonaws.auth.AWSCredentialsProvider;
import com.amazonaws.mobile.client.AWSMobileClient;

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.security.InvalidKeyException;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.SignatureException;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.List;

import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManagerFactory;

import software.amazon.freertos.amazonfreertossdk.AmazonFreeRTOSConstants;
import software.amazon.freertos.amazonfreertossdk.AmazonFreeRTOSDevice;
import software.amazon.freertos.amazonfreertossdk.AmazonFreeRTOSManager;
import software.amazon.freertos.amazonfreertossdk.BleConnectionStatusCallback;
import software.amazon.freertos.amazonfreertossdk.BleScanResultCallback;
import software.amazon.freertos.demo.utils.Notify;

import static com.amazonaws.mobile.auth.core.internal.util.ThreadUtils.runOnUiThread;

public class DeviceScanFragment extends Fragment {
    private static final String TAG = "DeviceScanFragment";
    private boolean mqttOn = true;
    private RecyclerView mBleDeviceRecyclerView;
    private BleDeviceAdapter mBleDeviceAdapter;
    List<BleDevice> mBleDevices = new ArrayList<>();

    private static final int REQUEST_ENABLE_BT = 1;
    private static final int PERMISSION_REQUEST_FINE_LOCATION = 1;

    private Button scanButton;

    private AmazonFreeRTOSManager mAmazonFreeRTOSManager;

    private class BleDeviceHolder extends RecyclerView.ViewHolder {
        private TextView mBleDeviceNameTextView;
        private TextView mBleDeviceMacTextView;
        private Switch mBleDeviceSwitch;
        private TextView mMenuTextView;
        private TextView tvSecret;
        private TextView tvError;

        private BleDevice mBleDevice;

        private boolean userDisconnect = true;

        public BleDeviceHolder(LayoutInflater inflater, ViewGroup parent) {
            super(inflater.inflate(R.layout.list_device, parent, false));
            mBleDeviceNameTextView = (TextView) itemView.findViewById(R.id.device_name);
            mBleDeviceMacTextView = (TextView) itemView.findViewById(R.id.device_mac);
            mBleDeviceSwitch = (Switch) itemView.findViewById(R.id.connect_switch);
            mMenuTextView = (TextView) itemView.findViewById(R.id.menu_option);
            tvSecret = (TextView) itemView.findViewById(R.id.tvSecret);
            tvError = (TextView) itemView.findViewById(R.id.tvError);
        }

        public void bind(BleDevice bleDevice) {
            mBleDevice = bleDevice;
            mBleDeviceNameTextView.setText(mBleDevice.getName());
            mBleDeviceMacTextView.setText(mBleDevice.getMacAddr());
            mBleDeviceSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                AmazonFreeRTOSDevice aDevice = null;
                boolean autoReconnect = true;

                @Override
                public void onCheckedChanged(CompoundButton v, boolean isChecked) {
                    Log.e(TAG, ":[BLE APP] : connect switch isChecked: " + (isChecked ? "ON" : "OFF"));
                    if (isChecked) {
                        if (aDevice == null) {
                            AWSCredentialsProvider credentialsProvider = AWSMobileClient.getInstance();

                            aDevice = mAmazonFreeRTOSManager.connectToDevice(mBleDevice.getBluetoothDevice(),
                                    connectionStatusCallback, credentialsProvider, autoReconnect);
                        }
                    } else {
                        if (userDisconnect || !autoReconnect) {
                            if (aDevice != null) {
                                mAmazonFreeRTOSManager.disconnectFromDevice(aDevice);
                                aDevice = null;
                            }
                        } else {
                            userDisconnect = true;
                        }
                        resetUI();
                    }
                }
            });

            mMenuTextView.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    Log.e(TAG, "Click menu.");
                    PopupMenu popup = new PopupMenu(getContext(), mMenuTextView);
                    popup.inflate(R.menu.options_menu);
                    popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
                        @Override
                        public boolean onMenuItemClick(MenuItem item) {
                            switch (item.getItemId()) {
                                case R.id.wifi_provisioning_menu_id:
                                    Intent intentToStartWifiProvision
                                            = WifiProvisionActivity.newIntent(getActivity(), mBleDevice.getMacAddr());
                                    startActivity(intentToStartWifiProvision);
                                    return true;
                                case R.id.mqtt_proxy_menu_id:
                                    Toast.makeText(getContext(), "Already signed in", Toast.LENGTH_SHORT).show();
                                    return true;
                            }
                            return false;
                        }
                    });
                    popup.show();
                }
            });

            resetUI();
        }

        final BleConnectionStatusCallback connectionStatusCallback = new BleConnectionStatusCallback() {
            @Override
            public void onFail(final String errorText, final int errorCode) {
                super.onFail(errorText, errorCode);
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        //tvError.setVisibility(View.VISIBLE);
                        mBleDeviceSwitch.setChecked(false);
                        openDialogOnError(errorText, errorCode);
                    }
                });
            }

            @Override
            public void onSecreteGenerated(final String secret) {
                super.onSecreteGenerated(secret);
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Log.e(TAG, "[Mutual Auth] : Completed successfully");
                        //tvSecret.setText(getString(R.string.authentication_successess));
                        Toast.makeText(getContext(),getString(R.string.authentication_successess),Toast.LENGTH_LONG).show();
                    }
                });
            }

            @Override
            public void onBleConnectionStatusChanged(AmazonFreeRTOSConstants.BleConnectionState connectionStatus) {
                Log.e(TAG, ":[BLE APP] : BLE connection status changed to: " + connectionStatus);
                if (connectionStatus == AmazonFreeRTOSConstants.BleConnectionState.BLE_CONNECTED) {
                    getActivity().runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            mMenuTextView.setEnabled(true);
                            mMenuTextView.setTextColor(getResources().getColor(R.color.colorAccent, null));
                            mBleDeviceSwitch.setChecked(true);
                        }
                    });
                } else if (connectionStatus == AmazonFreeRTOSConstants.BleConnectionState.BLE_DISCONNECTED) {
                    userDisconnect = false;
                    resetUI();
                }
            }
        };

        private void resetUI() {
            getActivity().runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mMenuTextView.setEnabled(false);
                    mMenuTextView.setTextColor(Color.GRAY);
                    mBleDeviceSwitch.setChecked(false);
                }
            });
        }

    }

    private void openDialogOnError(String errorText, final int errorCode) {
        Notify.INSTANCE.showAlertDialogOkClick(getActivity(),
                getString(R.string.authentication_fail, errorCode), new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i) {
                        //Logout
                        //Toast.makeText(getActivity(), getString(R.string.authentication_fail, errorCode), Toast.LENGTH_LONG).show();
                        Log.e(TAG, "[Mutual Auth] : Mutual authentication fail");
                        Toast.makeText(getContext(),getString(R.string.authentication_fail, errorCode),Toast.LENGTH_LONG).show();
                        AWSMobileClient.getInstance().signOut();
                        Intent intentToStartAuthenticator
                                = AuthenticatorActivity.newIntent(getActivity());
                        startActivity(intentToStartAuthenticator);
                        getActivity().finish();
                    }
                }
        );
    }

    private class BleDeviceAdapter extends RecyclerView.Adapter<BleDeviceHolder> {
        private List<BleDevice> mDeviceList;

        public BleDeviceAdapter(List<BleDevice> devices) {
            mDeviceList = devices;
        }

        @Override
        public BleDeviceHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            LayoutInflater layoutInflater = LayoutInflater.from(getActivity());
            return new BleDeviceHolder(layoutInflater, parent);
        }

        @Override
        public void onBindViewHolder(BleDeviceHolder holder, int position) {
            BleDevice device = mDeviceList.get(position);
            holder.bind(device);
        }

        @Override
        public int getItemCount() {
            return mDeviceList.size();
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View view = inflater.inflate(R.layout.fragment_device_scan, container, false);

        //Enabling Bluetooth
        Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
        startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);

        // requesting user to grant permission.
        requestPermissions(new String[]{Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.WRITE_EXTERNAL_STORAGE}, PERMISSION_REQUEST_FINE_LOCATION);

        //Getting AmazonFreeRTOSManager
        mAmazonFreeRTOSManager = AmazonFreeRTOSAgent.getAmazonFreeRTOSManager(getActivity());

        scanButton = (Button) view.findViewById(R.id.scanbutton);
        scanButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                LocationManager lm = (LocationManager) getActivity().getSystemService(Context.LOCATION_SERVICE);
                boolean gps_enabled = false;
                boolean network_enabled = false;

                try {
                    gps_enabled = lm.isProviderEnabled(LocationManager.GPS_PROVIDER);
                } catch (Exception ex) {
                }

                try {
                    network_enabled = lm.isProviderEnabled(LocationManager.NETWORK_PROVIDER);
                } catch (Exception ex) {
                }

                if (!gps_enabled && !network_enabled) {
                    // notify user
                    new AlertDialog.Builder(getActivity())
                            .setMessage(R.string.enable_location_warning)
                            .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface paramDialogInterface, int paramInt) {
                                    Toast.makeText(getActivity(), R.string.location_not_enabled, Toast.LENGTH_LONG).show();
                                    getActivity().finish();
                                }
                            })
                            .setPositiveButton(R.string.open_location, new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface paramDialogInterface, int paramInt) {
                                    startActivity(new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS));
                                }
                            }).setCancelable(false).show();
                } else {
                    Log.e(TAG, ":[BLE APP] : scan button clicked.");
                    mAmazonFreeRTOSManager.startScanDevices(new BleScanResultCallback() {
                        @Override
                        public void onBleScanResult(ScanResult result) {
                            BleDevice thisDevice = new BleDevice(result.getDevice().getName(),
                                    result.getDevice().getAddress(), result.getDevice());
                            if (!mBleDevices.contains(thisDevice)) {
                                Log.e(TAG, ":[BLE APP] : new ble device found. Mac: " + thisDevice.getMacAddr());
                                mBleDevices.add(thisDevice);
                                mBleDeviceAdapter.notifyDataSetChanged();
                            }
                        }
                    }, 10000);
                }


            }
        });

        //RecyclerView for displaying list of BLE devices.
        mBleDeviceRecyclerView = (RecyclerView) view.findViewById(R.id.device_recycler_view);
        mBleDeviceRecyclerView.setLayoutManager(new LinearLayoutManager(getActivity()));

        mBleDeviceAdapter = new BleDeviceAdapter(mBleDevices);
        mBleDeviceRecyclerView.setAdapter(mBleDeviceAdapter);

        /*if (mqttOn) {
            Intent intentToStartAuthenticator
                    = AuthenticatorActivity.newIntent(getActivity());
            startActivity(intentToStartAuthenticator);
        }*/

        return view;
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onCreateOptionsMenu(menu, inflater);
        inflater.inflate(R.menu.fragment_device_scan, menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.logout:
                AWSMobileClient.getInstance().signOut();
                Intent intentToStartAuthenticator
                        = AuthenticatorActivity.newIntent(getActivity());
                intentToStartAuthenticator.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                startActivity(intentToStartAuthenticator);
                getActivity().finish();
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == REQUEST_ENABLE_BT) {
            if (resultCode == getActivity().RESULT_OK) {
                Log.e(TAG, ":[BLE APP] : successfully enabled bluetooth");
            } else {
                Log.e(TAG, ":[BLE APP] : Failed to enable bluetooth");
            }
        }

    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        switch (requestCode) {
            case PERMISSION_REQUEST_FINE_LOCATION: {
                if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Log.e(TAG, ":[BLE APP] : ACCESS_FINE_LOCATION granted.");
                } else {
                    Log.e(TAG, ":[BLE APP] : ACCESS_FINE_LOCATION denied");
                    scanButton.setEnabled(false);
                }
            }
        }
    }

}



