#!/bin/bash

help()
{
  echo -e "\nUsage:  $0 \${accessKeyId} \${secret} \${security}\n"
  echo -e "accessKeyId: access key id of your aliyun account"
  echo -e "secret     : secret of your aliyun account"
  echo -e "security   : \"none\" for no authentication, \"1\" for enable authentication"
  echo -e ""
}

if [ $UID -ne 0 ];then
  echo -e "\nSuperuser privileges are required to run this script."
  echo -e "e.g. \"sudo $0\"\n"
  exit 1
fi

if [ "$1" = "" -o "$2" = "" -o "$3" == "" ];then
  help
  exit 1
fi

ACCESSKEY_ID=$1
SECRET=$2
SECURITY=$3
LINKKIT_NAME=linkkit
LINKKIT_TAG=${LINKKIT_NAME}:v0.1

yum install epel-release -y
yum clean all
yum list

yum install docker-io -y
systemctl start docker

docker info

docker pull registry.cn-shanghai.aliyuncs.com/linkkitteam/linkkit:v0.1
docker tag registry.cn-shanghai.aliyuncs.com/linkkitteam/linkkit:v0.1 ${LINKKIT_TAG}

CONTAINERS=`docker ps | sed -n '1d;p'`
echo -e "${CONTAINERS}" | while read LINE
do
  CONTAINER_ID=`echo ${LINE} | awk -F' ' '{print $1}'`
  CONTAINER_TAG=`echo ${LINE} | awk -F' ' '{print $2}'`
  CONTAINER_TAG_NAME=`echo ${CONTAINER_TAG} | awk -F':' '{print $1}'`

  if [ "${CONTAINER_TAG_NAME}" = "${LINKKIT_NAME}" ];then
    echo "linkkit docker[${CONTAINER_ID}] already running,stop...."
    docker stop ${CONTAINER_ID}
  fi
done

docker run -d -p 80:8080 -e accessKeyId="${ACCESSKEY_ID}" -e secret="${SECRET}" -e security=${SECURITY} -v "/var/linkkit:/var/linkkit" linkkit:v0.1
docker ps

if [ -f "/var/linkkit/application-custom.properties" ];then
  echo "config file already exist"
else
  echo "creating config file..."
  mkdir -p /var/linkkit/
  echo -e "server.port=7001\n"\
"# server.ssl.key-store=\n"\
"# server.ssl.key-alias=\n"\
"# server.ssl.key-store-password=\n" > /var/linkkit/application-custom.properties
fi