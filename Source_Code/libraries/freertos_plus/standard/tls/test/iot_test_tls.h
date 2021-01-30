/*
 * FreeRTOS TLS V1.1.7
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

#ifndef _AWS_TLS_TEST_H_
#define _AWS_TLS_TEST_H_


#define tlstestMQTT_BROKER_ENDPOINT_EC			"a3vwfgdm06d0xb-ats.iot.ap-south-1.amazonaws.com"
/*
 * PEM-encoded client certificate.
 *
 * Certificate for P-256 elliptic curve key.
 */
static const char tlstestCLIENT_CERTIFICATE_PEM_EC[] =
"-----BEGIN CERTIFICATE-----\n"\
"MIICzjCCAbagAwIBAgITaWdJCbDCOTJa4JKRvbZSZvGjgTANBgkqhkiG9w0BAQsF\n"\
"ADBNMUswSQYDVQQLDEJBbWF6b24gV2ViIFNlcnZpY2VzIE89QW1hem9uLmNvbSBJ\n"\
"bmMuIEw9U2VhdHRsZSBTVD1XYXNoaW5ndG9uIEM9VVMwHhcNMjAwNTE4MTQxODE2\n"\
"WhcNNDkxMjMxMjM1OTU5WjBfMQswCQYDVQQGEwJJTjEQMA4GA1UECAwHR3VqYXJh\n"\
"dDEMMAoGA1UEBwwDQWhtMRMwEQYDVQQKDApFaW5mb2NoaXBzMQwwCgYDVQQLDANQ\n"\
"RVMxDTALBgNVBAMMBFNFRUQwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAATcOa3V\n"\
"quLp8l1Q9kIug0dnNdLjqUgsTiDqjUDA5ckI/36HwC5I2HaN7ALsuLyc3yKzqY+I\n"\
"Qsa5Db8W1dTxUDkTo2AwXjAfBgNVHSMEGDAWgBQAf7ohwKeAfb/skGE1U2RSiOvG\n"\
"qjAdBgNVHQ4EFgQUHXWuNkTWWpUDn1RbV6yB5Gs9y0gwDAYDVR0TAQH/BAIwADAO\n"\
"BgNVHQ8BAf8EBAMCB4AwDQYJKoZIhvcNAQELBQADggEBADLU5UqvR1FVemgrTMAL\n"\
"dSxpX/7N6XVgUQ+R8e5WNlehssZYKnf9U0tnOL20sKB6uERlqM1F+k8j722g97JF\n"\
"Go4nkaHKqtOfk/USe0szVOkULgBbpwbn+tfvOuOzgeLU3o38hfLh6EkN1kDf7r3x\n"\
"zDmLnJPyag+aJxP6zaU9Dlrxfkl4lEZuH1xOULk2ZhQ9qsQ72jnVdCxT6hch/qJP\n"\
"cW8oZp3qr9IGLpMXtjPGLj+qDN6Ss0di8e9ngcuuqCSe6ePgpMT/Ppq9PMkI4+d8\n"\
"HNJoskkK1sZiUdYxlSibq/eSxnpY42QK2mLT14Eoht81t7BtoJSLGJo5uBl1l0Ds\n"\
"G0A=\n"\
"-----END CERTIFICATE-----";

/*
 * PEM-encoded client private key.
 *
 * This is a P-256 elliptic curve key.
 */
static const char tlstestCLIENT_PRIVATE_KEY_PEM_EC[] =
"-----BEGIN EC PRIVATE KEY-----\n"\
"MHcCAQEEIM34mlmtRLGhc7OSsACeLor1op7m2O1l4Rb86ajzkBOGoAoGCCqGSM49\n"\
"AwEHoUQDQgAE3Dmt1ari6fJdUPZCLoNHZzXS46lILE4g6o1AwOXJCP9+h8AuSNh2\n"\
"jewC7Li8nN8is6mPiELGuQ2/FtXU8VA5Ew==\n"\
"-----END EC PRIVATE KEY-----";

/* One character of this certificate has been changed in the issuer
 * name from Amazon Web Services to Amazon Web Cervices. */
static const char tlstestCLIENT_CERTIFICATE_PEM_MALFORMED[] =
"-----BEGIN CERTIFICATE-----\n"\
"MIICzjCCAbagAwIBAgITaWdJCbDCOTJa4JKRvbZSZvGjgTANBgkqhkiG9w0BAQsF\n"\
"ADBNMUswSQYDVQQLDEJBbWF6b24gV2ViIENlcnZpY2VzIE89QW1hem9uLmNvbSBJ\n"\
"bmMuIEw9U2VhdHRsZSBTVD1XYXNoaW5ndG9uIEM9VVMwHhcNMjAwNTE4MTQxODE2\n"\
"WhcNNDkxMjMxMjM1OTU5WjBfMQswCQYDVQQGEwJJTjEQMA4GA1UECAwHR3VqYXJh\n"\
"dDEMMAoGA1UEBwwDQWhtMRMwEQYDVQQKDApFaW5mb2NoaXBzMQwwCgYDVQQLDANQ\n"\
"RVMxDTALBgNVBAMMBFNFRUQwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAATcOa3V\n"\
"quLp8l1Q9kIug0dnNdLjqUgsTiDqjUDA5ckI/36HwC5I2HaN7ALsuLyc3yKzqY+I\n"\
"Qsa5Db8W1dTxUDkTo2AwXjAfBgNVHSMEGDAWgBQAf7ohwKeAfb/skGE1U2RSiOvG\n"\
"qjAdBgNVHQ4EFgQUHXWuNkTWWpUDn1RbV6yB5Gs9y0gwDAYDVR0TAQH/BAIwADAO\n"\
"BgNVHQ8BAf8EBAMCB4AwDQYJKoZIhvcNAQELBQADggEBADLU5UqvR1FVemgrTMAL\n"\
"dSxpX/7N6XVgUQ+R8e5WNlehssZYKnf9U0tnOL20sKB6uERlqM1F+k8j722g97JF\n"\
"Go4nkaHKqtOfk/USe0szVOkULgBbpwbn+tfvOuOzgeLU3o38hfLh6EkN1kDf7r3x\n"\
"zDmLnJPyag+aJxP6zaU9Dlrxfkl4lEZuH1xOULk2ZhQ9qsQ72jnVdCxT6hch/qJP\n"\
"cW8oZp3qr9IGLpMXtjPGLj+qDN6Ss0di8e9ngcuuqCSe6ePgpMT/Ppq9PMkI4+d8\n"\
"HNJoskkK1sZiUdYxlSibq/eSxnpY42QK2mLT14Eoht81t7BtoJSLGJo5uBl1l0Ds\n"\
"G0A=\n"\
"-----END CERTIFICATE-----";

/* Certificate which is not trusted by the broker. */
static const char tlstestCLIENT_UNTRUSTED_CERTIFICATE_PEM[] = "Paste client certificate here.";

/* Private key corresponding to the untrusted certificate. */
static const char tlstestCLIENT_UNTRUSTED_PRIVATE_KEY_PEM[] = "Paste client private key here.";

/* Device certificate created using BYOC instructions. */
static const char tlstestCLIENT_BYOC_CERTIFICATE_PEM[] = "Paste client certificate here.";

/* Device private key created using BYOC instructions. */
static const char tlstestCLIENT_BYOC_PRIVATE_KEY_PEM[] = "Paste client private key here.";

#endif /* ifndef _AWS_TLS_TEST_H_ */
