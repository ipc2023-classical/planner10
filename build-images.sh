#! /usr/bin/env bash

echo "Building image for satisficing track"
apptainer build satisficing.img Apptainer.opcount4sat_sat

echo "Building image for agile track"
apptainer build agile.img Apptainer.opcount4sat_agl
