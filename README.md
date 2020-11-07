# FrameBuffer_UDP

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