#!/bin/bash

# Breakpad 工具的路径
DUMP_SYMS_PATH="/home/fredyvia/breakpad/breakpad/src/tools/linux/dump_syms/dump_syms"
MINIDUMP_STACKWALK_PATH="/home/fredyvia/breakpad/breakpad/src/processor/minidump_stackwalk"

# 二进制文件路径
BINARY_FILE="build/node"

# 符号文件的输出目录
SYMBOLS_DIR="tmp/"

# 创建符号文件
SYM_FILE="$SYMBOLS_DIR/$(basename $BINARY_FILE).sym"
$DUMP_SYMS_PATH $BINARY_FILE >$SYM_FILE

# 从符号文件获取模块和 ID
MODULE_INFO=$(head -n1 $SYM_FILE)
MODULE_NAME=$(echo $MODULE_INFO | cut -d' ' -f5)
MODULE_ID=$(echo $MODULE_INFO | cut -d' ' -f4)

# 创建符号目录结构
SYMBOLS_PATH="$SYMBOLS_DIR/$MODULE_NAME/$MODULE_ID"
mkdir -p $SYMBOLS_PATH
mv $SYM_FILE $SYMBOLS_PATH

# 用于存放分析报告的目录
REPORTS_DIR="tmp/reports"
mkdir -p "$REPORTS_DIR"

# 循环遍历 core dump 目录
for DUMP_DIR in tmp/spkdfs_*; do
  NODE_ID=$(basename "$DUMP_DIR" | cut -d '_' -f 2)
  COREDUMP_DIR="$DUMP_DIR/coredumps"

  if [ -d "$COREDUMP_DIR" ]; then
    for DUMP_FILE in "$COREDUMP_DIR"/*.dmp; do
      if [ ! -f "$DUMP_FILE" ]; then
        continue
      fi

      REPORT_FILE="$REPORTS_DIR/report_node_${NODE_ID}.txt"
      $MINIDUMP_STACKWALK_PATH "$DUMP_FILE" "$SYMBOLS_DIR" >"$REPORT_FILE"

      echo "Report generated for node $NODE_ID: $REPORT_FILE"
    done
  else
    echo "No coredump directory found for node $NODE_ID"
  fi
done

echo "All reports generated."
