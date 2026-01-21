# dantto4k

## 使用方法

### dantto4k.exe
MMTSから復号化およびMPEG-2 TSへの変換を行います。
```
Usage:
  dantto4k [OPTION...] input output ('-' for stdin/stdout)

      --listSmartCardReader     List available smart card readers
      --casProxyServer arg      Specify the address of a CasProxyServer
      --smartCardReaderName arg
                                Specify the smart card reader to use
      --customWinscardDLL arg   Specify the path to a winscard.dll
      --disableADTSConversion   Disable ADTS conversion
      --no-progress             Disable progress
      --no-stats                Disable packet statistics
      --help                    Show help
```

### BonDriver_dantto4k.dll
リアルタイムで復号化とMPEG-2 TSへの変換を行うBonDriverです。
BonDriver_dantto4k.iniで設定されたBonDriverをロードして、復号化とMPEG-2 TSへの変換を行います。
dantto4kは64bitで配布しており、BonRecTestおよびBonDriver_BDAは64bitである必要があります。

#### Mirakurunでの動作
PT4Kで動作する場合、チャンネル再生まで15～20秒かかるため、Mirakurunのtimeout(20秒)を超える場合があります。
mirakurunのソースコードを修正してtimeoutを30秒以上に変更する必要があります。

https://github.com/Chinachu/Mirakurun/blob/master/src/Mirakurun/Tuner.ts

### CasProxyServer
スマートカードのプロキシサーバーが必要な場合、以下のリポジトリから構築できます。
https://github.com/nekohkr/casproxyserver

## ビルド
### Windows
/thirdpartyフォルダにtsduckとasio(v1.32.0)を準備します。
下記のURLからbinaryをダウンロードすることができます。

- https://github.com/tsduck/tsduck/
### Ubuntu

```bash
sudo apt install make g++ libssl-dev libpcsclite-dev pcscd pkgconf

git clone https://github.com/nekohkr/dantto4k.git
cd dantto4k
git submodule update --init --recursive

cd thirdparty/tsduck
scripts/install-prerequisites.sh
make -j10
make install

cd ../..
make
make install
```

## References
- ARIB STD-B32
- ARIB STD-B60
- [superfashi/FFmpeg](https://github.com/superfashi/FFmpeg)
- b61decoder
