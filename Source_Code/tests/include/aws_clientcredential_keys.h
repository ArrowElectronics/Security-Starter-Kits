/*
 * FreeRTOS V202002.00
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

#ifndef AWS_CLIENT_CREDENTIAL_KEYS_H
#define AWS_CLIENT_CREDENTIAL_KEYS_H

/*
 * @brief PEM-encoded client certificate.
 *
 * @todo If you are running one of the FreeRTOS demo projects, set this
 * to the certificate that will be used for TLS client authentication.
 *
 * @note Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */
#define keyCLIENT_CERTIFICATE_PEM  ""
#if 0
\
"-----BEGIN CERTIFICATE-----\n"\
"MIICRjCCAS4CFFHmK2oMcM0lW+VALQ9G3Wgw9pPMMA0GCSqGSIb3DQEBCwUAMEUx\n"\
"CzAJBgNVBAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYDVQQKDBhJbnRl\n"\
"cm5ldCBXaWRnaXRzIFB0eSBMdGQwHhcNMjAwODA1MTYyMDMyWhcNMjExMjE4MTYy\n"\
"MDMyWjBFMQswCQYDVQQGEwJJTjETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UE\n"\
"CgwYSW50ZXJuZXQgV2lkZ2l0cyBQdHkgTHRkMFkwEwYHKoZIzj0CAQYIKoZIzj0D\n"\
"AQcDQgAEDl+TDDKLEpiIOUnw8LHzwBo9zQY4/tP5+xlpM91MNwbnQZbe7sx+7Rgg\n"\
"JfbU7uSyb5ZWAzH037dy28zuSkeuQDANBgkqhkiG9w0BAQsFAAOCAQEAO4YdZyXx\n"\
"Crkkz9oMt4dsKdSvcmtj9I3Y9286Er/dhe9u/hKD8t0vmZgL/lHXshUCokKtbFVI\n"\
"i5dSK6jKUVg84xVuKuSbJZhpIAYZQdyBHb+aZ31zT29Mk4OysF2Q8eBBr6Q3rWBr\n"\
"4EzfQXZJjFAUUSu9OiReMFENz15kbhvb6CEcPMjfDCWzPa7DLWn7+D77oHUlv60s\n"\
"QCmMEy3otyVE3Ru443C2M6CkJC8VbHbdJ6UaKkYAxkpc4DLf6DvfzTB4q5UlxplX\n"\
"Zk9VURBkSxhd3JGV12cbWTrI6NKQkv9HbR9qORB7tJCLQaPU/DRRGSjcW964JZqD\n"\
"q7W7UFaY9uCKBQ==\n"\
"-----END CERTIFICATE-----"
#endif
#if 0
\
"-----BEGIN CERTIFICATE-----\n"\
"MIICFDCB/QIJAJ2PaPIS7BqXMA0GCSqGSIb3DQEBCwUAMDIxCzAJBgNVBAYTAlVT\n"\
"MRMwEQYDVQQIDApTb21lLVN0YXRlMQ4wDAYDVQQKDAVBcnJvdzAeFw0yMDA3MzAx\n"\
"MzQ4MTVaFw0yMTEyMTIxMzQ4MTVaMDIxCzAJBgNVBAYTAkFVMRMwEQYDVQQIDApT\n"\
"b21lLVN0YXRlMQ4wDAYDVQQKDAVBcnJvdzBZMBMGByqGSM49AgEGCCqGSM49AwEH\n"\
"A0IABN5Ab0nkj+i0mWmChimlpJIO/yjojul0JZCfzRKnPbaFhzAxSA+uQE60i8wp\n"\
"P55RFvJBhWUGjccdUlEmyJo9EikwDQYJKoZIhvcNAQELBQADggEBAApqM9bXUQ2V\n"\
"5CPjSXqy5tvXDn0DOIp40U2spK9FLc6mdx7U9Q3sGhHILgjbk/UffE/zPnm3dnHw\n"\
"BsFe20I2r/kv7gW/G67pmvUOHLaICG1/YiLA4MlhT23s0aPLMVvxG/sCwPJtGlYM\n"\
"2moY8AkwD3D/Q0oJXxuT7Ydcr8/9xfyZZk1e002Yg32T0KKps+l1TkEYmlKDLUey\n"\
"yrjs1gL1OVhYopW6DfciLMxNk6JQZeG+JOxNZ7oPq5uBq7rAY0nML12z6tcwYSHM\n"\
"XPqr43HpVTzP5JsNRcT5el8jxkX3QPRWM5IeKkHRkJznhUw73JjRDPnkqYOQ9Cs9\n"\
"fugdZkXj2Z8=\n"\
"-----END CERTIFICATE-----\n"
#endif
/*
 * @brief PEM-encoded issuer certificate for AWS IoT Just In Time Registration (JITR).
 *
 * @todo If you are using AWS IoT Just in Time Registration (JITR), set this to
 * the issuer (Certificate Authority) certificate of the client certificate above.
 *
 * @note This setting is required by JITR because the issuer is used by the AWS
 * IoT gateway for routing the device's initial request. (The device client
 * certificate must always be sent as well.) For more information about JITR, see:
 *  https://docs.aws.amazon.com/iot/latest/developerguide/jit-provisioning.html,
 *  https://aws.amazon.com/blogs/iot/just-in-time-registration-of-device-certificates-on-aws-iot/.
 *
 * If you're not using JITR, set below to NULL.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */
#define keyJITR_DEVICE_CERTIFICATE_AUTHORITY_PEM  ""
#if 0
\
"-----BEGIN CERTIFICATE-----\n"\
"MIIDazCCAlOgAwIBAgIUZbM8bR7ETPr3+VSleoUDsKXPoX8wDQYJKoZIhvcNAQEL\n"\
"BQAwRTELMAkGA1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoM\n"\
"GEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZDAeFw0yMDA4MDUxNjEyNTFaFw0yMzA1\n"\
"MjYxNjEyNTFaMEUxCzAJBgNVBAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEw\n"\
"HwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQwggEiMA0GCSqGSIb3DQEB\n"\
"AQUAA4IBDwAwggEKAoIBAQDHnOS1H8s7ea6KDsBYQMoXF1uapx+6PIXjMJHxWFjv\n"\
"L3OVcvBGOtTDDnj+YfW6SIOLE6W29OdJ3ZkrY8oqnMAxnK0VY9K6YALQEAxTgQSH\n"\
"N78FTAY0NazQaOzISLVCh8390LRacDpUNFuBjupnHDtVi8daOAt+XeJeIpOBK6D1\n"\
"2QFLbqjhdMYNjLpSGazYNnNjjwzUZwu5UNAta4kT9pruv5bbDQ8WPu/sXL54Tr2B\n"\
"qw8DEazmEgDwjta6zgDn5ohGOyi46CvfgNg873zzrw/EpEv1mW7QsNeMcn5+Y2eo\n"\
"wXaQTanBq2bmW3oyACel0R1FNIkwkBfbId88ThttcVV1AgMBAAGjUzBRMB0GA1Ud\n"\
"DgQWBBQC3xRgQyRJleiR+9bPTt9HNTPnIDAfBgNVHSMEGDAWgBQC3xRgQyRJleiR\n"\
"+9bPTt9HNTPnIDAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQBi\n"\
"w00g/pCCpsTUfl9IvNDyr7VXMjJn6aEn3OVyod/LHPJvF39ryDGBejWPUxaENnbs\n"\
"krZJ42ZTNfZtIQAesv6aXq4CM8pMSx1LwOlRBdaqVJRdQiS19NqeU14Pu4WOTOJS\n"\
"m6cG1wUtTyRddTykM91Q/r8lshYJGFR4kK44q13yJq6WlwGs2f2Un14Gho7hZewo\n"\
"IfqLsXDuqDqV54fyAjSOOucSkWtNg2LGzCU+NzjaqJZOqMvnapc6Qq+hrh5wDH6R\n"\
"51x0skEp/WncQRs0n9fxh8Sn9DBL5afDBajm/h3xLWIfiLIe88rAEOHj/DITfJzC\n"\
"IWdU/SeLyeu612wGo/O8\n"\
"-----END CERTIFICATE-----" 
#endif
#if 0
\
"-----BEGIN CERTIFICATE-----\n"\
"MIIDNzCCAh+gAwIBAgIJAM7vBSYUR1x5MA0GCSqGSIb3DQEBCwUAMDIxCzAJBgNV\n"\
"BAYTAlVTMRMwEQYDVQQIDApTb21lLVN0YXRlMQ4wDAYDVQQKDAVBcnJvdzAeFw0y\n"\
"MDA3MzAxMzIyMzBaFw0yMzA1MjAxMzIyMzBaMDIxCzAJBgNVBAYTAlVTMRMwEQYD\n"\
"VQQIDApTb21lLVN0YXRlMQ4wDAYDVQQKDAVBcnJvdzCCASIwDQYJKoZIhvcNAQEB\n"\
"BQADggEPADCCAQoCggEBAPhX2/PFs9NRKED9gSEpSdF3+dzyzwCIan+9zCv/1T9z\n"\
"30sg+V2uDYM9R+hIur27XvXEvJVlW6n7R0YawWPJo5yGeTu5xNpRO4et/XKSIiFZ\n"\
"WDujuF/tD1CxrKO0djmS7NKbvBKOCa4qAboPzRU5zbrP3CxQDJrVpkIUcORp+nGD\n"\
"UBPKg0DAnZO4px4C1Gx+asSebS4rQSxk8m1IjelMYVB5uLzOlluKuVF8fFMhbRH0\n"\
"E/Ya2yI+Bc9oDr28N45ugsmsozAGYcNanBnT3U3ktDmQ6J2lJQoUr6zivw3tntMf\n"\
"3HtD5jacGX1mdnqig10DLLKBW9bAqPmbz/yyaYH9N/8CAwEAAaNQME4wHQYDVR0O\n"\
"BBYEFN3qDElNsZ/I+hdod4xl/pzTyuFxMB8GA1UdIwQYMBaAFN3qDElNsZ/I+hdo\n"\
"d4xl/pzTyuFxMAwGA1UdEwQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEBANJM9VvL\n"\
"v9U+kbNtwuWU1om/4x3PFpBTkuLgKAUxVYsSHsTAPKEwQ6fOHsd5h8PWQm2yefPY\n"\
"kWlMTW3evoUE0kGZEBsLIE9pkVWG3s6wTPNhUEOpAQ6Tdf4Li7Ho1UBgK6Ufs4qV\n"\
"oHVLBk/dehsqbNS/6uuxeskrHVWKDy9t94AxHmCtk4gjabVaAy6Y3ZnNHJ40m0eM\n"\
"wZtoo7gakPI5oHTIl+Qk6yy7rxU1MXg7tSLjvKu0zgVQYj/Nwb5kD0Ubird+Erj6\n"\
"IKOugLKdMWig6tb0HaN4MyyNGEiSg1pT7OpamyX0EyMXyZvLUw5b9gxw+7QypFwx\n"\
"DwPOkJ3WAaV5mJo=\n"\
"-----END CERTIFICATE-----\n"
#endif
/*
 * @brief PEM-encoded client private key.
 *
 * @todo If you are running one of the FreeRTOS demo projects, set this
 * to the private key that will be used for TLS client authentication.
 *
 * @note Must include the PEM header and footer:
 * "-----BEGIN RSA PRIVATE KEY-----\n"\
 * "...base64 data...\n"\
 * "-----END RSA PRIVATE KEY-----\n"
 */
#define keyCLIENT_PRIVATE_KEY_PEM                   ""

#endif /* AWS_CLIENT_CREDENTIAL_KEYS_H */



