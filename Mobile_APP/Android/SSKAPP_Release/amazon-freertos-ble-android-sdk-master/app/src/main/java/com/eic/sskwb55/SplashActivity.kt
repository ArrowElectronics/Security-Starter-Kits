package com.eic.sskwb55

import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.view.View
import org.spongycastle.jce.provider.BouncyCastleProvider
import java.security.KeyFactory
import java.security.Security


class SplashActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_splash)
        findViewById<View>(R.id.btnClick).setOnClickListener {
            generateKeyPair()
        }
    }

    private fun generateKeyPair() {
      /*  Security.addProvider(BouncyCastleProvider())
        val sig: Signature = Signature.getInstance("NONEwithECDSA", "BC")
        KeyFactory.getInstance("ECDSA").generatePublic(new X509EncodedKeySpec (bytes));*/

    }


}