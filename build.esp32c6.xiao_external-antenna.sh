#!/bin/sh
set -eu
GITHUB_REPOSITORY=${GITHUB_REPOSITORY:-filimonic/mithbridge2}
GITHUB_REF_NAME=${GITHUB_REF_NAME:-v0.0.0-unpublished}
PROJECT_NAME=$(echo "$GITHUB_REPOSITORY" | sed 's|.*/||')
IDF_TARGET=$(basename "$0" | sed 's/^[^.]*\.\([^.]*\)\..*/\1/')
FIRMWARE_FILENAME_FULL=$(basename "$0" | sed "s/^build\./${PROJECT_NAME}-full\./; s/\.sh$/\.${GITHUB_REF_NAME}\.bin/")
FIRMWARE_FILENAME_APP=$(basename "$0" | sed "s/^build\./${PROJECT_NAME}-app\./; s/\.sh$/\.${GITHUB_REF_NAME}\.bin/")

echo "IDF_TARGET=$IDF_TARGET"
echo "PROJECT_NAME=$PROJECT_NAME"
echo "FIRMWARE_FILENAME_FULL=$FIRMWARE_FILENAME_FULL"
echo "FIRMWARE_FILENAME_APP=$FIRMWARE_FILENAME_APP"
export IDF_TARGET
export PROJECT_NAME
rm -f ./sdkconfig || true
rm -f ./firmware.bin || true
rm -fr ./build || true
idf.py fullclean 
mkdir -p ./dist
idf.py build merge-bin --output ../dist/${FIRMWARE_FILENAME_FULL}
mv ./build/${PROJECT_NAME}.bin ./dist/${FIRMWARE_FILENAME_APP}