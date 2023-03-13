#! /usr/bin/env bash

DOMAIN=$1
INSTANCE=$2
PLAN_FILE=$3

if [ -z "${PLAN_FILE}" ]; then
    PLAN_FILE=sas_plan
fi

apptainer run satisficing.img $DOMAIN $INSTANCE $PLAN_FILE
