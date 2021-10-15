

root_dir=$(cd "$(dirname "$0")/../"; pwd)
cd ${root_dir}/build

cmake -G "Unix Makefiles" ../tests/compact_udt/ -DCMAKE_BUILD_TYPE=Debug

