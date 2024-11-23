#!/usr/bin/env bash

install_hostdeps_lsb() {
    if [[ "$(lsb_release -i -s)" = "Ubuntu" ]]; then
        # These applications are needed for the base ubuntu 16 / 17 / 18 to build Carbonite.
        # FIXME: Once the docker build process is rolled out, these deps shouldn't be needed anymore.
        echo "Warning: about to run potentially destructive apt-get commands."
        echo "         waiting 5 seconds..."
        sleep 5
        sudo apt-get update
        sudo apt-get install libxcursor-dev libxrandr-dev libxinerama-dev libxi-dev mesa-common-dev zip unzip make g++ -y
    fi
}

install_docker_lsb() {
    ver=`lsb_release -r -s`
    if [[ "$ver" = "16.04" ]]; then
        curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
        sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"
        sudo apt-get update
        sudo apt-get install -y docker-ce

        sudo usermod -aG docker ${USER}
        echo "You need to log out and back in to ensure you're part of the docker user group"
    fi

    if [ 1 -eq "$(echo "$ver >= 18.04"|bc)" ]; then
        sudo apt update
        sudo apt install apt-transport-https ca-certificates curl software-properties-common
        curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
        sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu bionic stable"
        sudo apt update
        sudo apt install docker-ce

        sudo usermod -aG docker ${USER}
        echo "You need to log out and back in to ensure you're part of the docker user group"
    fi
}

install_docker_centos() {
    sudo yum install -y yum-utils device-mapper-persistent-data lvm2
    sudo yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
    sudo yum install docker-ce
    sudo systemctl start docker

    sudo usermod -aG docker $(whoami)
    echo "You need to log out and back in to ensure you're part of the docker user group"
}

DOCKER=$(which docker)

lsb_release >& /dev/null
if [[ $? == 0 ]]; then
    install_hostdeps_lsb
    [[ "x$DOCKER" = "x" ]] && install_docker_lsb
elif [[ $(cut -c-22 /etc/system-release) = "CentOS Linux release 7" ]]; then
    [[ "x$DOCKER" = "x" ]] && install_docker_centos
else
    echo "Unable to determine distribution." > /dev/stderr
    exit 1
fi


