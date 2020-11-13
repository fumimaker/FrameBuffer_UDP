# FrameBuffer_UDP

### 各ファイルの使い方

- ./fb_test.c
  - 1280 720 24でグラデーションを表示するプログラム。
- ./fb_macchan.c
  - 1280 720 24でUDPデータを受信してそれを/dev/fb0に書いて映像出力するプログラム。
- ./digitalcamerapi.c
  - /dev/video0でjpg写真を撮るプログラム
- ./displayCamera.c
  - /dev/video0と通信して設定して映像を/dev/fb0に連続で書いて動画を表示するプログラム。

UDPで受信したデータをいい感じに整列して/dev/fb0に書き込んで，HDMIに表示するプログラム．



## Raspi4で何か描画する
- `git clone https://github.com/fumimaker/Framebuffer_UDP`でクローンする。
- `sudo cp Framebuffer_UDP/config.txt /boot/config.txt`すると上書きされてしまう。これによってCUIモードの色深度が32BITに設定される。
  - これでRaspiのカーネルが起動できなくなることがあるのでできればバックアップを取ったほうがいいかもしれない。一回壊れた。
- `sudo raspi-config`からboot-->commandline modeを選択すると再起動してCUIモードになる。
- `fbset -i`を実行すると解像度や色深度などを見ることができる。
- `cd FrameBuffer_UDP`をして`gcc fb_test.c`
- `sudo ./a.out`で実行するとグラデーションが表示される。super+Cでキャンセルして、clearで画面をきれいにする。
- GUIに戻すには同様にraspi-configで戻す。

## Ubutnu環境で何か描画する
自分の場合ではinit 3でCUIモードに入ると解像度が800*600になってしまい、まともに描画することができなかった。その解決策をここに書く。
- `sudo nano /etc/default/grub`で適当なエディタでひらいて編集する。
  - + GRUB_GFXMODE=1920x1080
- `sudo update-grub`
  - これがなかった場合：
  - sudo apt install grub-efi
  - sudo apt install grub-common
  - これでもう一度実行する。
- 

## 動かす手順

- `gcc -O fb_macchan.c`で最適化ありでコンパイル。
- raspiの解像度設定を確認する。
  - `fbset -i`で現在の状況を見ることができる1280 720 24bitになっていなければ問題。
  - `fbset -g 1280 720 1280 720 24`でジオメトリ？を設定することで出力設定を変更できる。

# TODO

-  比較用のRaspiで送信できるものを作る。
  - MIPIカメラ（Raspiカメラ）は/dev/videoで開くことができる。
  - v4l2 io-cirlで制御することができる？
    - 解像度やRGBフォーマットの設定などをする。
  - /dev/videoをmemapしてキャプチャ？
  - UDPで送信する。
- 受信側は同じものが使えるはず。

  