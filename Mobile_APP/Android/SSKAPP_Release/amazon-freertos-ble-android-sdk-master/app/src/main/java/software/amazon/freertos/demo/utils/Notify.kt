package software.amazon.freertos.demo.utils

import android.app.Activity
import android.app.AlertDialog
import android.content.Context
import android.content.DialogInterface
import android.text.TextUtils
import android.util.Log
import android.widget.Toast
import software.amazon.freertos.demo.R


object Notify {
    private val TAG = Notify::class.java.name
    private var alertdialog: AlertDialog? = null

    fun toast(text: String, activity: Context) {
        try {
            Toast.makeText(activity, text, Toast.LENGTH_SHORT).show()

        } catch (e: Exception) {
            e.printStackTrace()
        }

    }




    fun dialogOK(message: String, activity: Activity, finish: Boolean) {
        if (TextUtils.isEmpty(message)) {
            return
        }

        try {
            val builder = AlertDialog.Builder(activity)
            builder.setTitle(activity.getString(R.string.app_name))
            builder.setMessage(message)
            builder.setPositiveButton("OK") { dialog, which ->
                dialog.dismiss()
                if (finish) {
                    activity.finish()
                }
            }
            builder.setCancelable(false)
            builder.show()

        } catch (e: Exception) {
            Log.d(TAG, e.toString())
        }

    }

    fun dialogOK(title: String, message: String, activity: Activity, finish: Boolean) {
        if (TextUtils.isEmpty(message)) {
            return
        }

        try {
            val builder = AlertDialog.Builder(activity)
            builder.setTitle(title)
            builder.setMessage(message)
            builder.setPositiveButton("OK") { dialog, which ->
                dialog.dismiss()
                if (finish) {
                    activity.finish()
                }
            }
            builder.show()

        } catch (e: Exception) {
            Log.e(TAG, e.toString())
        }

    }


    fun dialogOK(title: String, message: String, activity: Activity, finish: Boolean, cancallable: Boolean) {
        if (TextUtils.isEmpty(message)) {
            return
        }

        try {
            val builder = AlertDialog.Builder(activity)
            builder.setTitle(title)
            builder.setMessage(message)
            builder.setPositiveButton("OK") { dialog, which ->
                dialog.dismiss()
                if (finish) {
                    activity.finish()
                }
            }
            builder.setCancelable(cancallable)
            builder.show()

        } catch (e: Exception) {
            Log.e(TAG, e.toString())
        }

    }






    fun showAlertDialogOkClick(context: Context,
                               message: String,
                               positive: DialogInterface.OnClickListener) {
        try {
            val builder = AlertDialog.Builder(context)
            builder.setCancelable(false)
            builder.setTitle(R.string.app_name)
            builder.setMessage(message)
            builder.setPositiveButton("Ok", positive)

            if (alertdialog != null && alertdialog!!.isShowing) {
                alertdialog!!.dismiss()
                alertdialog = null
            }
            alertdialog = builder.create()
            alertdialog!!.show()
        } catch (e: Exception) {
            Log.e("Utils", "Display Alert Dialog ErrorBean" + e.message)
        }

    }

    fun showAlertDialogCustom(context: Context, message: String, positiveTitle: String, negativeTitle: String, positive: DialogInterface.OnClickListener, negative: DialogInterface.OnClickListener?) {
        try {
            val builder = AlertDialog.Builder(context)
            builder.setCancelable(false)
            builder.setTitle(R.string.app_name)
            builder.setMessage(message)
            builder.setPositiveButton(positiveTitle, positive)
            if (negative != null) {
                builder.setNegativeButton(negativeTitle, negative)
            }

            if (alertdialog != null && alertdialog!!.isShowing) {
                alertdialog!!.dismiss()
                alertdialog = null
            }
            alertdialog = builder.create()
            alertdialog!!.show()
        } catch (e: Exception) {
            Log.e("Utils", "Display Alert Dialog ErrorBean" + e.message)
        }

    }


}
