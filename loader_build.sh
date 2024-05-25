# ディレクトリ
script_dir="$(dirname "$(readlink -f "$0")")"

# ビルドディレクトリー
BUILD_DIR=${script_dir}/build/

# イメージファイル
IMAGE_PATH=${BUILD_DIR}/neoboot.img

# ブートローダーパス
LOADER_PATH=${BUILD_DIR}/loader.efi

# ボリュームの名前
VOLUME_NAME=NEOBOOT

# ローダーをビルド
function loader_build() {
    x86_64-elf-gcc -I${script_dir}/gnu-efi/inc -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/new.c -o ${BUILD_DIR}main.o
    x86_64-elf-ld -z noexecstack -shared -Bsymbolic -L${script_dir}/gnu-efi/x86_64/lib -L${script_dir}/gnu-efi/x86_64/gnuefi -T${script_dir}/gnu-efi/gnuefi/elf_x86_64_efi.lds ${script_dir}/gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o ${BUILD_DIR}main.o -o ${BUILD_DIR}main.so -lgnuefi -lefi
    x86_64-elf-objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 ${BUILD_DIR}main.so ${LOADER_PATH}
}

# イメージファイルを作成
function make_image() {
    # DMGファイルの作成 - ${IMAGE_PATH}.dmg
    hdiutil create -size 1g -fs "FAT32" -layout GPTSPUD -volname ${VOLUME_NAME} ${IMAGE_PATH}

    # DMGファイルをCDRファイルに変換 - ${IMAGE_PATH}.cdr
    hdiutil convert ${IMAGE_PATH}.dmg -format UDTO -o ${IMAGE_PATH}

    # CDRファイルをIMGファイルに変換 - ${IMAGE_PATH}
    mv ${IMAGE_PATH}.cdr ${IMAGE_PATH}

    # マウント
    hdiutil mount ${IMAGE_PATH}

    # それに移動
    cd /Volumes/${VOLUME_NAME}

    # ファイル構成の作成
    mkdir -p /Volumes/${VOLUME_NAME}/EFI/BOOT

    # EFI/BOOTに移動
    cd /Volumes/${VOLUME_NAME}/EFI/BOOT

    # 移動したら、ファイルを追加
    cp ${LOADER_PATH} /Volumes/${VOLUME_NAME}/EFI/BOOT/BOOTX64.efi

    # 作業フォルダーに戻る
    cd ${script_dir}

    #作業ファイルの削除
    rm ${BUILD_DIR}/*.img.dmg

    # アンマウント
    hdiutil unmount /Volumes/${VOLUME_NAME} -force
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
    -drive if=pflash,format=raw,file=${BUILD_DIR}/fw/OVMF.fd \
    -drive if=ide,index=0,media=disk,format=raw,file=${IMAGE_PATH} \
    -device nec-usb-xhci,id=xhci \
    -device usb-mouse -device usb-kbd \
    -monitor mon:stdio
}

#クリーン
function trouble() {
    rm -f ${IMAGE_PATH} ${IMAGE_PATH}.dmg
}

#使い方
function usage() {
    echo ""
    echo "Neo Boot Build Tool - NeoBootを今すぐビルド。"
    echo "RUN ビルドして実行"
    echo "CLEAN 関連ファイルの削除"
    echo ""
}


#メイン
while (( $# > 0 ))
do
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