#!/bin/bash

# ディレクトリ
script_dir="$(dirname "$(readlink -f "$0")")"

# ビルドディレクトリー
BUILD_DIR="${script_dir}/build"

# イメージファイル
IMAGE_PATH="${BUILD_DIR}/neoboot.img"

# ブートローダーパス
LOADER_PATH="${BUILD_DIR}/loader.efi"

# ボリュームの名前
VOLUME_NAME="NEOBOOT"

# コンパイル対象のファイル名を読み込む
COMPILE_FILES=$(<${BUILD_DIR}/compile_files.txt)

# ローダーをビルド
function loader_build() {
    mkdir -p "${BUILD_DIR}"
    obj_files=()
    
    for file in ${COMPILE_FILES}; do
        obj_file="${BUILD_DIR}/$(basename "${file}" .c).o"
        
        # 各ファイルをコンパイル
        x86_64-elf-gcc -I"${script_dir}/gnu-efi/inc" -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c "${file}" -o "${obj_file}"
        
        obj_files+=("${obj_file}")
    done

    # オブジェクトファイルをリンク
    x86_64-elf-ld -z noexecstack -shared -Bsymbolic -L"${script_dir}/gnu-efi/x86_64/lib" -L"${script_dir}/gnu-efi/x86_64/gnuefi" -T"${script_dir}/gnu-efi/gnuefi/elf_x86_64_efi.lds" "${script_dir}/gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o" "${obj_files[@]}" -o "${BUILD_DIR}/main.so" -lgnuefi -lefi
    
    # オブジェクトファイルをEFIファイルに変換
    x86_64-elf-objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym -j .rel -j .rela -j '.rel.*' -j '.rela.*' -j .reloc --target efi-app-x86_64 --subsystem=10 "${BUILD_DIR}/main.so" "${LOADER_PATH}"
}

# イメージファイルを作成
function make_image() {
    # DMGファイルの作成
    hdiutil create -size 1g -fs "FAT32" -layout GPTSPUD -volname "${VOLUME_NAME}" "${IMAGE_PATH}.dmg"

    # DMGファイルをCDRファイルに変換
    hdiutil convert "${IMAGE_PATH}.dmg" -format UDTO -o "${IMAGE_PATH}"

    # CDRファイルをIMGファイルに変換
    mv "${IMAGE_PATH}.cdr" "${IMAGE_PATH}"

    # マウント
    hdiutil mount "${IMAGE_PATH}"

    # ファイル構成の作成
    mkdir -p "/Volumes/${VOLUME_NAME}/EFI/BOOT"

    # ファイルを追加
    cp "${LOADER_PATH}" "/Volumes/${VOLUME_NAME}/EFI/BOOT/BOOTX64.efi"

    # アンマウント
    hdiutil unmount "/Volumes/${VOLUME_NAME}" -force

    # 作業ファイルの削除
    rm "${IMAGE_PATH}.dmg"
}

# 成功率を増やす関数
function kill_proc() {
    # lsofコマンドでディスクイメージファイルを開いているプロセスを取得し、PIDを抽出
    pids=$(lsof "$IMAGE_PATH" | awk '{if(NR>1)print $2}')

    # 各PIDに対してkillコマンドを実行
    for pid in $pids; do
        kill -9 "$pid"
    done
}

# QEMUで実行する関数
function run_image() {
    qemu-system-x86_64 \
    -m 4G \
    -drive if=pflash,format=raw,file="${BUILD_DIR}/fw/OVMF.fd" \
    -drive if=ide,index=0,media=disk,format=raw,file="${IMAGE_PATH}" \
    -device nec-usb-xhci,id=xhci \
    -device usb-mouse -device usb-kbd \
    -monitor mon:stdio
}

# クリーン
function trouble() {
    rm -f "${IMAGE_PATH}" "${IMAGE_PATH}.dmg"
}

# 使い方
function usage() {
    echo ""
    echo "Neo Boot Build Tool - NeoBootを今すぐビルド。"
    echo "RUN ビルドして実行"
    echo "CLEAN 関連ファイルの削除"
    echo ""
}

# メイン
while (( $# > 0 )); do
  case $1 in
    run | RUN)
      loader_build
      make_image
      kill_proc
      run_image
      ;;
    help | HELP)
      usage
      ;;
    runonly | runonly)
      kill_proc
      run_image
      ;;
    build | BUILD)
      loader_build
      ;;
    clean | trouble | CLEAN | TROUBLE)
      trouble
      echo "削除完了"
      ;;
    *)
      usage
      ;;
  esac
  shift
done
