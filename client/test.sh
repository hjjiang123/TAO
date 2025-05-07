#!/bin/bash

# 检查参数数量
if [ $# -le 1 ]; then
    echo "Please provide compiling options"
    exit 1
fi

# 根据第一个参数执行不同的命令
echo "Start compiling $1.c, option is $2"
# 在这里添加启动服务的命令
gcc -g -O0 -Wall -D $2 \
    $1.c \
    /home/hjjiang/hjjiang_capture_v3/public/object.c \
    /home/hjjiang/hjjiang_capture_v3/public/send_command.c\
    -o $1 \
    -I /home/hjjiang/hjjiang_capture_v3/public \
    -lm
