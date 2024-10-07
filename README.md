# dantto4k

## 使用方法

### dantto4k.exe
mmtsから復号化およびMPEG TSへの変換を行います。
```
dantto4k.exe <input.mmts> <output.ts>
```

### BonDriver_dantto4k.dll
リアルタイムで復号化とMPEG TSへの変換を行うBonDriverです。
BonDriver_dantto4k.iniで設定されたBonDriverをロードして、復号化とMPEG TSへの変換を行います。
dantto4kは64bitで配布しており、BonRecTestおよびBonDriver_BDAは64bitである必要があります。

#### mirakurunでの動作
PT4Kで動作する場合、チャンネル再生まで15～20秒かかるため、mirakurunのtimeout(20秒)を超える場合があります。
mirakurunのソースコードを修正してtimeoutを30秒以上に変更する必要があります。

https://github.com/Chinachu/Mirakurun/blob/master/src/Mirakurun/Tuner.ts#L175C13-L175C55

## ビルド
/thirdpartyフォルダにopenssl 3, ffmpeg, tsduckを準備します。
下記のURLからbinaryをダウンロードすることができます。

https://slproweb.com/products/Win32OpenSSL.html
https://github.com/BtbN/FFmpeg-Builds/releases
https://github.com/tsduck/tsduck/