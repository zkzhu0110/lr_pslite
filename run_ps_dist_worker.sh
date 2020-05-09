root_path=`pwd`
model_name=0
epochs=2
export DMLC_NUM_WORKER=2
export DMLC_NUM_SERVER=1
export DMLC_PS_ROOT_URI=$HOSTIP
export DMLC_PS_ROOT_PORT=9092
export DMLC_ROLE='worker'
bash ./scripts/start_worker.sh $root_path/build/test/src/xflow_lr $root_path/data/small_train $root_path/data/small_test $model_name $epochs
