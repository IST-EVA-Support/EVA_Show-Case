#!/bin/bash

if [ "$#" -ne 2 ]; then
  echo "Usage $0 SourceFileName TargetFileName" >&2
  exit 1
fi

if ! [ -e "$1" ]; then
  echo "$1 not found" >&2
  exit 1
fi

SourceFileName=$1;
TargetFileName=$2;
echo "Source File Name: $SourceFileName";
echo "Target File Name: $TargetFileName";

/usr/src/tensorrt/bin/trtexec --uff=$SourceFileName --output=NMS --uffInput=Input,3,300,300 --batch=1 --maxBatch=1 --saveEngine=$TargetFileName