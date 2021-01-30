#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause

#----------------------------------------------------------------------------------------------------------------
#  Yocto setup build script
#----------------------------------------------------------------------------------------------------------------
#  The script will create Yocto environment which is require to build the system.
#
#----------------------------------------------------------------------------------------------------------------
YOCTO_DIR="$(pwd )"

echo "$YOCTO_DIR"
build_repo()
{
	cd $YOCTO_DIR
	echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo "Yocto Repo Setup Tool"
	echo  "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo " "
	echo " "
	echo "--->Initializing Yocto Repo for Avenger96<---"
	echo " "
	echo " "
	mkdir -p ~/bin
	# check for repo dir and create one if not there
	if [[ -f ~/bin/repo ]]; then
		echo "Repo is already created. No need to do anything."
	else
		echo "Creating repo..."
		curl https://storage.googleapis.com/git-repo-downloads/repo  > ~/bin/repo
	fi
	sudo chmod a+x ~/bin/repo
	sleep 5
	echo "Creating yocto setup..."
	[ -d .repo ] > /dev/null
	if [[ $? -eq 0 ]]; then
		echo "Yocto Directory is already configured. No need to do anything."
	else
	git init
	repo init -u https://github.com/dh-electronics/manifest-av96 -b thud
	sleep 5
	echo "--->Copying Repo Manifest<---"
	echo " "
	echo " "
	cp patches/manifests/default.xml .repo/manifests/
	echo "--->Clonning Yocto Repo for Avenger96-will take 10-15 mins<---"
	echo " "
	echo " "
	repo sync
	fi
	sleep 1
	echo "Exiting Repo-Build Script..."	
}

build_avg()
{
	cd $YOCTO_DIR
	echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo "Yocto Meta-Avg Setup Tool"
	echo  "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo " "
	echo " "
	echo "--->Initializing Yocto Repo for Avenger96<---"
	echo " "
	echo " "
	git clone https://github.com/dh-electronics/meta-av96.git
	cd meta-av96/
	git checkout  master
	cd ..
	cp meta-av96/  layers/ -R
	pushd .
	cd layers
	echo "--->Applying Meta-St Patch<---"
	echo " "
	echo " "
	patch -p0 < ../meta-av96/meta-av96.thud/meta-st.patch
	if [ $? == 1 ]; then
		echo "--->Above Patch failed to apply <---"
		exit 1
	fi
	cd meta-st/meta-st-stm32mp-addons/conf/eula/
	ln -s ST_EULA_SLA stm32mp1-av96
	popd
	cd $YOCTO_DIR
	echo "--->Adding meta-av96 layer in Yocto <---"
	echo " "
	echo " "
	EULA=1 DISTRO=openstlinux-weston MACHINE=stm32mp1-av96 source layers/meta-st/scripts/envsetup.sh
	bitbake-layers add-layer ../layers/meta-av96/meta-av96.thud/meta-av96/
	sleep 2
	echo "Exiting Meta-Avg Build Script..."
}

build_meta()
{
	cd $YOCTO_DIR
	echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo "Yocto Meta-Security Build Tool"
	echo  "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo " "
	cd layers
	git clone git://git.yoctoproject.org/meta-security
	sleep 1
	cd meta-security/
	git checkout 51a4c6b5179d087f647cf0c458edb8ab107826ef
	sleep 2
	wget https://github.com/autoconf-archive/autoconf-archive/archive/v2019.01.06.tar.gz
	sleep 2
	tar -xvf v2019.01.06.tar.gz
	sleep 2
	cp -r autoconf-archive-2019.01.06/ meta-tpm/recipes-tpm2/tpm2-abrmd/files/
	sleep 2
	cp -r autoconf-archive-2019.01.06/ meta-tpm/recipes-tpm2/tpm2-pkcs11/files/
	sleep 2
	mkdir meta-tpm/recipes-tpm2/tpm2-tools/files
	cp -r autoconf-archive-2019.01.06/ meta-tpm/recipes-tpm2/tpm2-tools/files/
	mkdir meta-tpm/recipes-tpm2/tpm2-tss/files
	cp -r autoconf-archive-2019.01.06/ meta-tpm/recipes-tpm2/tpm2-tss/files/
	sleep 2
	echo "--->Applying "0001-SEED-Adding-autoconf-archive-support-in-tpm2-abrmd.patch" Patch<---"
	echo " "
	echo " "
	git am ../../patches/meta-security/0001-SEED-Adding-autoconf-archive-support-in-tpm2-abrmd.patch
	if [ $? == 1 ]; then
		echo "--->Above Patch failed to apply <---"
		exit 1
	fi
	sleep 2
	echo "--->Applying "0002-SEED-Enable-install-the-adaptation-of-PKCS-11-for-TP.patch" Patch<---"
	echo " "
	echo " "
	git am ../../patches/meta-security/0002-SEED-Enable-install-the-adaptation-of-PKCS-11-for-TP.patch
	if [ $? == 1 ]; then
		echo "--->Above Patch failed to apply <---"
		exit 1
	fi
	sleep 2
	echo "--->Applying "0003-SEED-Adding-tpm2-tools-support-for-AWS-TPM2.0.patch" Patch<---"
	echo " "
	echo " "
	git am ../../patches/meta-security/0003-SEED-Adding-tpm2-tools-support-for-AWS-TPM2.0.patch
	if [ $? == 1 ]; then
		echo "--->Above Patch failed to apply <---"
		exit 1
	fi
	sleep 2
	echo "--->Applying "0004-SEED-Adding-tpm2-tss-support-for-AWS-TPM-comp.patch" Patch<---"
	echo " "
	echo " "
	git am ../../patches/meta-security/0004-SEED-Adding-tpm2-tss-support-for-AWS-TPM-comp.patch
	if [ $? == 1 ]; then
		echo "--->Above Patch failed to apply <---"
		exit 1
	fi
	echo "Exiting meta-security Build Script..."
	
}

build_st()
{
	cd $YOCTO_DIR
	echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo "Yocto Meta-st-stm32mp Build Tool"
	echo  "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo " "
	cd layers/meta-st/meta-st-stm32mp
	sleep 2
	echo "--->Applying "0001-SEED-Increased-ROOTFS-Size_2GB.patch" Patch<---"
	echo " "
	echo " "
	git am ../../../patches/meta-st/0001-SEED-Increased-ROOTFS-Size_2GB.patch
	if [ $? == 1 ]; then
		echo "--->Above Patch failed to apply <---"
		exit 1
	fi
	sleep 2
	echo "--->Applying "0002-SEED-Adding-TPM2.0-Module.patch" Patch<---"
	echo " "
	echo " "
	git am ../../../patches/meta-st/0002-SEED-Adding-TPM2.0-Module.patch
	if [ $? == 1 ]; then
		echo "--->Above Patch failed to apply <---"
		exit 1
	fi
	sleep 2
	echo "--->Applying "0003-SEED-SD-card-Size-modify-to-4GB-in-script.patch" Patch<---"
	echo " "
	echo " "
	git am ../../../patches/meta-st/0003-SEED-SD-card-Size-modify-to-4GB-in-script.patch
	if [ $? == 1 ]; then
		echo "--->Above Patch failed to apply <---"
		exit 1
	fi
	echo "Exiting meta-stm Build Script..."
}

build_core()
{
	cd $YOCTO_DIR
	echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo "Yocto Openembedded-Core Build Tool"
	echo  "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo " "
	echo " "
	cd layers/openembedded-core/
	sleep 2
	echo "--->Applying "0001-SEED-Adding-P11-kit-Module-in-Build.patch" Patch<---"
	echo " "
	echo " "
	git am ../../patches/openembedded-core/0001-SEED-Adding-P11-kit-Module-in-Build.patch
	if [ $? == 1 ]; then
		echo "--->Above Patch failed to apply <---"
		exit 1
	fi
	echo "--->Applying "0002-SEED-Removed-sanity-check-for-root-user-to-build-pok.patch" Patch<---"
	echo " "
	echo " "
	git am ../../patches/openembedded-core/0002-SEED-Removed-sanity-check-for-root-user-to-build-pok.patch
	if [ $? == 1 ]; then
		echo "--->Above Patch failed to apply <---"
		exit 1
	fi
	sleep 2
	echo "Exiting Openembedded-Core Build Script..."
}

build_meta_openembedded()
{
	cd $YOCTO_DIR
	echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo "Yocto Meta-Openembedded Build Tool"
	echo  "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo " "
	cd layers/meta-openembedded/
	echo "--->Applying "0001-SEED-Upgrade-revision-of-python-setuptools.patch" Patch<---"
	echo " "
	echo " "
	git am ../../patches/meta-openembedded/0001-SEED-Upgrade-revision-of-python-setuptools.patch
	if [ $? == 1 ]; then
		echo "--->Above Patch failed to apply <---"
		exit 1
	fi
	sleep 2
	echo "Exiting Meta-Openembedded Build Script..."
}


build_meta-predmnt()
{
	cd $YOCTO_DIR
	echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo "Yocto meta-predmnt Build Tool"
	echo  "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo " "
	cd layers/
	echo "--->Clonning Meta-predmnt layer<---"
	echo " "
	echo " "
	git clone https://github.com/STMicroelectronics/meta-predmnt
	sleep 2
	cd meta-predmnt
	git checkout 63b351d825b409e664d4ce2454e1ed77434d3fe6
	sleep 2
	echo "--->Applying "0001-SEED-meta-predmnt-Image-append.patch" Patch<---"
	echo " "
	echo " "
	git am ../../patches/meta-predmnt/0001-SEED-meta-predmnt-Image-append.patch
	if [ $? == 1 ]; then
		echo "--->Above Patch failed to apply <---"
		exit 1
	fi
	echo "--->Applying "0002-SEED-Modification-in-Greengrass-Support.patch" Patch<---"
	echo " "
	echo " "
	git am ../../patches/meta-predmnt/0002-SEED-Modification-in-Greengrass-Support.patch
	if [ $? == 1 ]; then
		echo "--->Above Patch failed to apply <---"
		exit 1
	fi
	cd $YOCTO_DIR
	EULA=1 DISTRO=openstlinux-weston MACHINE=stm32mp1-av96 source layers/meta-st/scripts/envsetup.sh
	bitbake-layers add-layer ../layers/meta-predmnt/
	sleep 2
	echo "Exiting meta-predmnt Build Script..."
}	

build_avg_yocto()
{

	cd $YOCTO_DIR
	echo "---> All Setup Ready Building the Avenger Image <---"
	echo " "
	echo " "
	EULA=1 DISTRO=openstlinux-weston MACHINE=stm32mp1-av96 source layers/meta-st/scripts/envsetup.sh
	
	bitbake -v av96-weston
	if [ $? -ne 0 ];then
		echo "[ERROR] : Error in building the image. Please see the BUILD_Log.txt for resolution."
		exit -1
	else
		echo "$IMAGE image build successfully"
		cd tmp-glibc/deploy/images/stm32mp1-av96/ 
		./scripts/create_sdcard_from_flashlayout.sh flashlayout_av96-weston/FlashLayout_sdcard_stm32mp157a-av96-trusted.tsv
		cp flashlayout_av96-weston_FlashLayout_sdcard_stm32mp157a-av96-trusted.raw ../../../../../../Stm32mp1_Yocto_Build/../Firmware_Image/
	fi	
	cd $YOCTO_DIR
}

prerequisite()
{
	echo "Checking for required host packages and if not installed then install it..."
	sudo apt-get update
	sudo apt-get install repo gcc g++ gawk wget git-core diffstat unzip texinfo gcc-multilib build-essential chrpath socat libsdl1.2-dev libsdl1.2-dev xterm sed cvs subversion coreutils texi2html docbook-utils python3-pip python-pip python-pysqlite2 help2man make desktop-file-utils libgl1-mesa-dev libglu1-mesa-dev mercurial autoconf automake groff curl lzop asciidoc u-boot-tools -y

	if [ $? -ne 0 ]
	then
		echo "[ERROR] : Failed to get required HOST packages. Please correct error and try again."
		exit -1
	fi

	echo "Required HOST packages successfully installed."
}

#Script will start from here

prerequisite
build_repo
sync
build_avg
build_meta
build_st
build_core
build_meta_openembedded
build_meta-predmnt
sync
build_avg_yocto
echo "Exiting Build Script..."
