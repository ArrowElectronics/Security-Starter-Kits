#!/bin/bash


#----------------------------------------------------------------------------------------------------------------
#  Yocto setup build script
#----------------------------------------------------------------------------------------------------------------
#  The script will create Yocto environment which is require to build the system.
#
#----------------------------------------------------------------------------------------------------------------

#YOCTO_DIR="$(pwd )"
blue=`tput setaf 4`
reset=`tput sgr0`

echo "$YOCTO_DIR"

# Get BUILD SETUP parameter from User
get_build_setup_parameters()
{
	# TODO : ALL EXPORT
	export PATH=${PATH}:~/bin

	echo -e "\nPlease choose TARGET BOARD from following : (enter numeric value 1-2 only)"
	echo "1. AI_ML"

	while :
	do
		read -p "TARGET BOARD : " target_board

		case "$target_board" in

			1)  echo "SELECT 1. AI_ML"
		   		MACHINE="imx8qxpaiml"
		   		DISTRO="fsl-imx-xwayland"
				BUILD_DIR="bld-xwayland-aiml"
				break
				;;
			*) echo "INVALID OPTION. Please Select 1 or exit"
				;;
		esac
	done

	echo -e "\n Yocto build variables: "
	echo "MACHINE          := $MACHINE"
	echo "DISTRO           := $DISTRO"
	echo "BUILD DIR Name   := $BUILD_DIR"
}


# Required packages to build
# Install all the required build HOST packages
prerequisite()
{
	echo "Checking for required host packages and if not installed then install it..."
	sudo apt-get install repo gcc g++ gawk wget git-core diffstat unzip texinfo gcc-multilib build-essential chrpath socat libsdl1.2-dev libsdl1.2-dev xterm sed cvs subversion coreutils texi2html docbook-utils python3-pip python-pip python-pysqlite2 help2man make desktop-file-utils libgl1-mesa-dev libglu1-mesa-dev mercurial autoconf automake groff curl lzop asciidoc u-boot-tools -y

	if [ $? -ne 0 ]
	then
		echo "[ERROR] : Failed to get required HOST packages. Please correct error and try again."
		exit -1
	fi

	echo "Required HOST packages successfully installed."
}



# To get the BSP you need to have `repo` installed.
# Install the `repo` utility. (only need to do this once)
create_repo()
{
	# create ~/bin if not there
	mkdir -p ~/bin

	# check for repo dir and create one if not there
	if [[ -f ~/bin/repo ]]; then
		echo "Repo is already created. No need to do anything."
	else
		echo "Creating repo..."
		curl https://storage.googleapis.com/git-repo-downloads/repo  > ~/bin/repo
	fi

	sudo chmod a+x ~/bin/repo
}




# Download the Yocto Project Environment into your directory
download_yocto()
{
	echo "Creating yocto setup..."
	[ -d .repo ] > /dev/null
	if [[ $? -eq 0 ]]; then
		echo "Yocto Directory is already configured. No need to do anything."
	else
		echo "Download the yocto repo"
		repo init -u https://source.codeaurora.org/external/imx/imx-manifest -b imx-linux-sumo -m imx-4.14.78-1.0.0_machinelearning.xml
		echo "Sync downloaded repo. Please wait..."
		repo sync
		if [[ $? -eq 0 ]]; then
			mv meta-einfochips-SSK-V-1_0.tar.gz sources/
			cd sources
			tar -xvf meta-einfochips-SSK-V-1_0.tar.gz
			cd meta-fsl-bsp-release
			# Add required BBLAYER in local bblayer.conf.
			git apply ../meta-einfochips/conf/0001-SEED-Added-required-BBLAYERs-in-local-bblayer.conf.patch
			cd ../poky
			git apply ../../patches/poky/0001-SEED-Adding-P11-Kit-Support-for-TPM.patch
		else
			echo "Error in repo sync. Please correct the error manually and try again."
		fi
	fi
}



# Build an image
build_image()
{
	echo "Starting $IMAGE image build now. If the image is build first time then It will take a long time (approx 3 to 5 hrs) to finish."
	bitbake -v fsl-image-qt5 > BUILD_Log.txt 2>&1
	if [ $? -ne 0 ];then
		echo "[ERROR] : Error in building the image. Please see the BUILD_Log.txt for resolution."
		exit -1
	else
		echo "$IMAGE image build successfully"
		cd bld-xwayland-aiml/tmp/deploy/images/imx8qxpaiml/
		cp fsl-image-qt5-imx8qxpaiml-*.rootfs.sdcard.bz2 ../../../../../../Imx8x_Yocto_Build/../Firmware_image/
	fi
}

#--------------------------
# Main execution starts here
#---------------------------

if [ ! -f $(pwd)/meta-einfochips-SSK-V-1_0.tar.gz ]; then
	echo "meta-einfochips-SSK-V-1_0.tar.gz not present. Please download and copy here:: $(pwd)" 
	echo "${blue}https://github.com/ArrowElectronics/Security-Starter-Kits/releases/download/v1.0-ssk/meta-einfochips-SSK-V-1_0.tar.gz${reset}"
	exit 1
fi
echo "########## Build script started successfully ##########"

# Get BUILD setup parameters and initialize
get_build_setup_parameters


# Check prerequisite
prerequisite


# create repo
create_repo

sync

# Setup Yocto environment
download_yocto
sync
cd $YOCTO_DIR

sleep 2
# Run Yocto setup
# [MACHINE=<machine>] [DISTRO=fsl-imx-<backend>] source ./fsl-setup-release.sh -b bld-<backend>
EULA=1 MACHINE=imx8qxpaiml DISTRO=fsl-imx-xwayland source fsl-setup-release.sh -b bld-xwayland-aiml


# Build an image
build_image

echo "########## Build script ended successfully ##########"
