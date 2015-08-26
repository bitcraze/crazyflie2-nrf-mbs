#!/usr/bin/env bash
set -e

scriptDir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
root="${scriptDir}/../.."

s110Dir=${root}/s110
s110Zip=${s110Dir}/s110_nrf51822_7.0.0.zip
if [ ! -f ${s110Zip} ]; then
    curl -L -o ${s110Zip} http://www.nordicsemi.com/eng/nordic/download_resource/48420/8/62400196
    unzip -q ${s110Dir}/s110_nrf51822_7.0.0.zip -d ${s110Dir}
fi
