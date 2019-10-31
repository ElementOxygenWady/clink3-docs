#!/bin/bash

help()
{
  echo -e "\nUsage:  $0 \${accessKeyId} \${secret} \${security}\n"
  echo -e "accessKeyId: access key id of your aliyun account"
  echo -e "secret     : secret of your aliyun account"
  echo -e "security   : \"none\" for no authentication, \"1\" for enable authentication"
  echo -e ""
}

if [ "$1" = "" -o "$2" = "" -o "$3" == "" ];then
  help
  exit 1
fi

if [ $UID -ne 0 ];then
  echo -e "\nSuperuser privileges are required to run this script."
  echo -e "e.g. \"sudo $0\"\n"
  exit 1
fi

ACCESSKEY_ID=$1
SECRET=$2
SECURITY=$3
LINKKIT_NAME=linkkit
LINKKIT_DOCKER=registry.cn-shanghai.aliyuncs.com/linkkitteam/linkkit:v0.2
LINKKIT_TAG=${LINKKIT_NAME}:v0.2
PORT=80

yum install epel-release -y
yum clean all
yum list

yum install docker-io -y
systemctl start docker

docker info

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

docker pull ${LINKKIT_DOCKER}
docker tag ${LINKKIT_DOCKER} ${LINKKIT_TAG}

echo -n "Please confirm if using https(y/n):"
read
if [ "${REPLY}" = "y" ];then
  PORT=443
  rm -rf /var/linkkit/application-custom.properties
  mkdir -p /var/linkkit/
  touch /var/linkkit/application-custom.properties

  echo -n "Please enter your cert type(default:PKCS12):"
  read
  if [ "${REPLY}" = "" ];then
    echo "server.ssl.keyStoreType=PKCS12" >> /var/linkkit/application-custom.properties
  else
    echo "server.ssl.keyStoreType=${REPLY}" >> /var/linkkit/application-custom.properties
  fi

  echo -n "Please enter your cert absolute path:"
  read
  if [ -f "${REPLY}" ];then
    cp -rf ${REPLY} /var/linkkit/server.cert
    echo "server.ssl.key-store=file:/var/linkkit/server.cert" >> /var/linkkit/application-custom.properties
  else
    echo "invalid path."
    exit 1
  fi

  echo -n "Please enter your cert passwrod:"
  read
  if [ "${REPLY}" != "" ];then
    echo "server.ssl.key-store-password=${REPLY}" >> /var/linkkit/application-custom.properties
  else
    echo "wrong password format"
  fi
else
  rm -rf /var/linkkit/application-custom.properties
  PORT=80
fi

docker run -d -p ${PORT}:8080 -e accessKeyId="${ACCESSKEY_ID}" -e secret="${SECRET}" -e security=${SECURITY} -v "/var/linkkit:/var/linkkit" linkkit:v0.1
docker ps