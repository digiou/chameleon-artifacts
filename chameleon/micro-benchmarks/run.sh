#!/bin/bash

allowed_datasets=("arrhythmia" "daphnet" "intel_lab" "company" "sensorscope" "smartgrid" "soccer")
allowed_experiments=("nu_nyq_only" "nu")
only_do_nyq=false
default_nyq_window_size=200
nyq_window_sizes=(50 100 200 500 1000)
data_path="/data-ssd/dimitrios"


if [ "$1" == "" ]; then
  echo "Experiment name missing!"
  exit 1
fi

if [[ ! "${allowed_experiments[@]}" =~ "$1" ]]; then
    echo "$1 not valid experiment."
    exit 1
fi

if [ "$2" == "" ] || [ -z "$2" ]; then
  echo "IO dir missing!"
  exit 1
fi

# Function to call Python script
call_python_script() {
    local script_name="$1.py"
    python3 "$script_name" "${@:2}"
}

call_nu_nyq_only() {
  for nyq_window_size in "${nyq_window_sizes[@]}"; do
    call_python_script "$1" "--$1_path=$data_path --sampling --use_nyquist_only --nyquist_window_size=$nyq_window_size"
  done
}

call_nu() {
  call_nu_nyq_only "$1"
  for nyq_window_size in "${nyq_window_sizes[@]}"; do
    call_python_script "$1" "--$1_path=$data_path --sampling --nyquist_window_size=$nyq_window_size"
  done
}

case "$1" in
    "nu_nyq_only")
        call_nu_nyq_only "$2"
        ;;
    "nu")
        call_nu "$2"
        ;;
    *)
        echo "Parameter value is not in the list of strings"
        ;;
esac