#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Source function library.
Greengrass_CERT_PATH=/greengrass/certs
echo "Generate Verification Key"

gen_verify_cert()
{
cd $Greengrass_CERT_PATH
echo "Please Collect your Registration Code Provide in Security Starter Kit Cloud Connect --> from settings"
echo "Enter Registration Code"
read reg_code
echo "Generate Verification Key"
openssl genrsa -out verificationCert.key 2048
echo "Generate Verification Cetficate Signing Request"
openssl req -new -key verificationCert.key -out verificationCert.csr -subj /C="IN"/ST="GUJ"/L="AHMEDABAD"/O="Arrow"/OU="eic"/CN=$reg_code
echo "Generate Verification Cetficate signed by RootCA"
openssl x509 -req -in verificationCert.csr -CA rootCA.pem -CAkey rootCA.key -CAcreateserial -out verificationCert.crt -days 7000 -sha256
echo "Copy rootCA.pem and verificationCert.crt into your HOST PC in order to uplode to Security Starter Kit Cloud Connect....."
}

gen_verify_cert
