# FrameBuffer_UDP

UDPで受信したデータをいい感じに整列して/dev/fb0に書き込んで，HDMIに表示するプログラム．

## Raspi4で何か描画する
- `git clone https://github.com/fumimaker/Framebuffer_UDP`でクローンする。
- `sudo cp Framebuffer_UDP/config.txt /boot/config.txt`すると上書きされてしまう。これによってCUIモードの色深度が32BITに設定される。
- `sudo raspi-config`からboot-->commandline modeを選択すると再起動してCUIモードになる。
- `fbset -i`を実行すると解像度や色深度などを見ることができる。
- `cd FrameBuffer_UDP`をして`gcc fb_test.c`
- `sudo ./a.out`で実行するとグラデーションが表示される。super+Cでキャンセルして、clearで画面をきれいにする。
- GUIに戻すには同様にraspi-configで戻す。