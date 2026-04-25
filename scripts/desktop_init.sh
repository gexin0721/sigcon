#!/usr/bin/env bash

# ============================================================================
# 脚本名称: desktop_init.sh
# 脚本目标:
# 1) 为 desktop 工程准备 项目依赖。
# 2) 将第三方库统一下载到 desktop/lib，避免污染其它开发环境。
# 3) 具备幂等性：如果库已存在则直接跳过下载，不重复拉取。
# ============================================================================

# 遇到错误立即退出(-e)，未定义变量报错(-u)，管道任一命令失败即失败(-o pipefail)
set -euo pipefail

# 通过脚本所在目录反推出仓库根目录，避免依赖当前执行目录。
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# 按项目约定，第三方依赖统一放在 desktop/lib。
LIB_DIR="${REPO_ROOT}/desktop/lib"
# =====  QT环境检查 =====

# =====  IXWebSocket =====
# IXWebSocket 的目标目录与远程仓库地址。
IXWS_DIR="${LIB_DIR}/IXWebSocket"
IXWS_REPO_URL="https://github.com/machinezone/IXWebSocket.git"

# 输出当前动作，便于排查脚本执行过程。
echo "[desktop_init] 准备初始化 IXWebSocket 开发环境..."
echo "[desktop_init] 仓库根目录: ${REPO_ROOT}"
echo "[desktop_init] 依赖目录: ${LIB_DIR}"

# 确保 desktop/lib 目录存在；若不存在则创建。
mkdir -p "${LIB_DIR}"

# 若 IXWebSocket 已存在，则直接跳过下载，保证脚本可重复执行。
if [ -d "${IXWS_DIR}" ]; then
    echo "[desktop_init] 检测到 ${IXWS_DIR} 已存在，跳过下载。"
    exit 0
fi

# 如果目录不存在，则执行浅克隆以减少下载体积和时间。
echo "[desktop_init] 未检测到 IXWebSocket，开始下载..."
git clone --depth 1 "${IXWS_REPO_URL}" "${IXWS_DIR}"

# 下载完成后给出结果提示。
echo "[desktop_init] IXWebSocket 下载完成: ${IXWS_DIR}"


# 