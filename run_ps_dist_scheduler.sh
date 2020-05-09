root_path=`pwd`
export DMLC_NUM_WORKER=2
export DMLC_NUM_SERVER=1
# export DMLC_PS_ROOT_URI=$HOSTIP
export DMLC_PS_ROOT_PORT=9092
export DMLC_ROLE='scheduler'
bash ./scripts/start_scheduler.sh  $root_path/build/test/src/xflow_lr
