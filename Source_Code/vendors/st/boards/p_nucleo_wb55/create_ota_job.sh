#!/bin/bash
projectPath=${PWD}"/../../../.."

if [[ ${OSTYPE} == "linux-gnu" ]]; then
	pythonCommand="python3"
else
	pythonCommand="python"
fi

${pythonCommand} ./start_ota_update.py --codelocation ${projectPath} --profile default --name stwb55 --role ota_update_role --s3bucket stwb55otas3 --otasigningprofile stwb55ProfileOta --signingcertificateid c2f5e43b-02b9-4300-86c7-edbb46e597ca
