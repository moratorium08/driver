# e1000

e1000e向けのデバイスドライバ

## 使用方法
- `make run`
  - プログラムの実行
- `make check`
  - デバイスの状態チェック
- `make clean`

## PCに不調が出たら
`make check`で「driver current」と「default」がどちらもe1000eであることを確認してください  
万が一「driver current」がuio_pci_genericとなっていた場合`make restore`を実行して下さい。

それでも直らない場合はPCを再起動してください。


