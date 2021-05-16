/*
 * Amazon FreeRTOS V201912.00
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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

#ifndef __AWS_CODESIGN_KEYS__H__
#define __AWS_CODESIGN_KEYS__H__

/*
 * PEM-encoded code signer certificate
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"
 * "...base64 data...\n"
 * "-----END CERTIFICATE-----\n";
 */

char ota_cert[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIBVzCB/qADAgECAhRCduvStZY8Rv1fnFAZzhQ989rSnzAKBggqhkjOPQQDAjAZ\n"
"MRcwFQYDVQQDDA5zZWVkQGFycm93LmNvbTAeFw0yMDA5MDYwNjQ5NDBaFw0zOTEx\n"
"MDYwNjQ5NDBaMBkxFzAVBgNVBAMMDnNlZWRAYXJyb3cuY29tMFkwEwYHKoZIzj0C\n"
"AQYIKoZIzj0DAQcDQgAEJNhvWb/fa9zedCEwvL5fRNCJT8go3FxxpMYfNw0dSBaC\n"
"zKsjthjTf2Npch/DIKKjk+CJf0un3332dHbHt0D8baMkMCIwCwYDVR0PBAQDAgeA\n"
"MBMGA1UdJQQMMAoGCCsGAQUFBwMDMAoGCCqGSM49BAMCA0gAMEUCIEO5KKBo4f0F\n"
"hymVwTvvU+E1EfxlBQCGyrRBZ3iI2bKMAiEAgfiKwfTNQpZlRJnzwsa71yCy1KCZ\n"
"xHTC6/Cd+XaQVnU=\n"
"-----END CERTIFICATE-----\n";

#define OTA_CERT_STORE 0
#define OTA_CERT_SIZE 700
extern char signingcredentialSIGNING_CERTIFICATE_PEM[OTA_CERT_SIZE];

#endif
