# UART to AzureIoT

<br>
<div align="center">
<img src="media/5.png" height="200">
</div>
<br>

UART to AzureIoTは、Arduinoを安全にインターネットに接続し、Azure IoTと連携する、Azure Sphereのプログラムです。  
Azure Sphereを仲介することで、悪意のあるインターネットからの攻撃、脅威から保護できます。  
ArduinoとAzure Sphereの通信はUARTでテキストベースなので、Arduino以外の独自デバイスを使うことも可能です。

```
+---------+                  +------------+             +-------------------+
| Arduino | <- UART(3.3V) -> | MT3620 RDB | <- Wi-Fi -> | Azure IoT Central |
+---------+                  +------------+             | Azure IoT Hub     |
                                                        +-------------------+
```

## 必要なもの

* [Seeeduino V4.2](https://www.seeedstudio.com/Seeeduino-V4-2-p-2517.html)
* [Azure Sphere MT3620開発ボード](https://www.seeedstudio.com/Azure-Sphere-MT3620-Development-Kit-JP-Version-p-3135.html)（MT3620 RDB）
* [ジャンパー・メス付きGroveケーブル](https://www.seeedstudio.com/Grove-4-pin-Female-Jumper-to-Grove-4-pin-Conversion-Cable-5-PCs-per-PAck.html)

## 動かし方

### Azure IoT Hub

TBD

* [use a direct connection to authenticate.](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub#authenticate-using-a-direct-connection)

### Seeeduino V4.2

> 【注意事項】  
> Seeeduino V4.2とMT3620 RDBを結線した状態では、スケッチをSeeeduino V4.2へ書き込むことができません。  
> スケッチを書き込むときは、Groveケーブルを外してください。

1. 電源電圧切り替えスイッチを3V3側にしてください。（[写真](media/6.jpg)）
1. PCとUSB接続してください。
1. Arduino IDEで、[Telemetryサンプルスケッチ](examples/arduino/telemetry/telemetry.ino)をコンパイル、書き込みしてください。

### MT3620 RDB

1. Visual Studioのフォルダを開くで**app/uart-to-azureiot-hlapp**を開いてください。
1. [app_manifest.json](app/uart-to-azureiot-hlapp/app_manifest.json)を書き換えてください。  
**CmdArgs**の **--ConnectionType**を**Direct**にする。  
**CmdArgs**の **--Hostname**にAzure IoT Hubのホスト名を書く。  
**CmdArgs**の **--DeviceID**にMT3620 RDBのデバイスIDを小文字で書く。（`powershell -Command ((azsphere device show-attached)[0] -split ': ')[1].ToLower()`）  
**AllowedConnections**にAzure IoT Hubのホスト名を書く。  
**DeviceAuthentication**にAzure SphereテナントIDを書く。（`azsphere tenant show-selected`）
1. ビルド、実行してください。

### Seeeduino V4.2とMT3620 RDBの結線

1. ジャンパー・メス付きGroveケーブルの、  
黒線を、MT3620 RDBのH2-2(GND)  
黄線を、MT3620 RDBのH2-3(TXD0)  
白線を、MT3620 RDBのH2-1(RXD0)  
に接続してください。
1. Groveケーブルのコネクタを、Seeeduino V4.2のGrove-UARTコネクタに接続してください。

### 動作状況

<img src="media/1.jpg" height="200">  
<img src="media/2.png" height="200">  
<img src="media/3.png" height="200">  
<img src="media/4.png" height="200">  

## ロードマップ

**Phase 1**
* テレメトリ機能のみ。（シリアルプロッタと同一プロトコル）
* 接続先はAzure IoT Hubのみ。

**Phase 2**  <--- イマココ
* Azure IoT Hub DPSに対応。
* Azure IoT Centralに対応。

**Phase 3**
* プロパティ機能とコマンド機能を追加。（独自プロトコル）

**Phase 4**
* IoT Plug and Playに対応？

## Licence

[MIT](LICENSE.txt)
