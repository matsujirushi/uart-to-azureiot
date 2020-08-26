# UART to AzureIoT

<br>
<div align="center">
<img src="media/5.png" height="200">
</div>
<br>

UART to AzureIoT�́AArduino�����S�ɃC���^�[�l�b�g�ɐڑ����AAzure IoT�ƘA�g����AAzure Sphere�̃v���O�����ł��B  
Azure Sphere�𒇉�邱�ƂŁA���ӂ̂���C���^�[�l�b�g����̍U���A���Ђ���ی�ł��܂��B  
Arduino��Azure Sphere�̒ʐM��UART�Ńe�L�X�g�x�[�X�Ȃ̂ŁAArduino�ȊO�̓Ǝ��f�o�C�X���g�����Ƃ��\�ł��B

```
+---------+                  +------------+             +-------------------+
| Arduino | <- UART(3.3V) -> | MT3620 RDB | <- Wi-Fi -> | Azure IoT Central |
+---------+                  +------------+             | Azure IoT Hub     |
                                                        +-------------------+
```

## �K�v�Ȃ���

* [Seeeduino V4.2](https://www.seeedstudio.com/Seeeduino-V4-2-p-2517.html)
* [Azure Sphere MT3620�J���{�[�h](https://www.seeedstudio.com/Azure-Sphere-MT3620-Development-Kit-JP-Version-p-3135.html)�iMT3620 RDB�j
* [�W�����p�[�E���X�t��Grove�P�[�u��](https://www.seeedstudio.com/Grove-4-pin-Female-Jumper-to-Grove-4-pin-Conversion-Cable-5-PCs-per-PAck.html)

## ��������

### Azure IoT Hub

TBD

* [use a direct connection to authenticate.](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub#authenticate-using-a-direct-connection)

### Seeeduino V4.2

> �y���ӎ����z  
> Seeeduino V4.2��MT3620 RDB������������Ԃł́A�X�P�b�`��Seeeduino V4.2�֏������ނ��Ƃ��ł��܂���B  
> �X�P�b�`���������ނƂ��́AGrove�P�[�u�����O���Ă��������B

1. �d���d���؂�ւ��X�C�b�`��3V3���ɂ��Ă��������B�i[�ʐ^](media/6.jpg)�j
1. PC��USB�ڑ����Ă��������B
1. Arduino IDE�ŁA[Telemetry�T���v���X�P�b�`](examples/arduino/telemetry/telemetry.ino)���R���p�C���A�������݂��Ă��������B

### MT3620 RDB

1. Visual Studio�̃t�H���_���J����**app/uart-to-azureiot-hlapp**���J���Ă��������B
1. [app_manifest.json](app/uart-to-azureiot-hlapp/app_manifest.json)�����������Ă��������B  
**CmdArgs**�� **--ConnectionType**��**Direct**�ɂ���B  
**CmdArgs**�� **--Hostname**��Azure IoT Hub�̃z�X�g���������B  
**CmdArgs**�� **--DeviceID**��MT3620 RDB�̃f�o�C�XID���������ŏ����B�i`powershell -Command ((azsphere device show-attached)[0] -split ': ')[1].ToLower()`�j  
**AllowedConnections**��Azure IoT Hub�̃z�X�g���������B  
**DeviceAuthentication**��Azure Sphere�e�i���gID�������B�i`azsphere tenant show-selected`�j
1. �r���h�A���s���Ă��������B

### Seeeduino V4.2��MT3620 RDB�̌���

1. �W�����p�[�E���X�t��Grove�P�[�u���́A  
�������AMT3620 RDB��H2-2(GND)  
�������AMT3620 RDB��H2-3(TXD0)  
�������AMT3620 RDB��H2-1(RXD0)  
�ɐڑ����Ă��������B
1. Grove�P�[�u���̃R�l�N�^���ASeeeduino V4.2��Grove-UART�R�l�N�^�ɐڑ����Ă��������B

### �����

<img src="media/1.jpg" height="200">  
<img src="media/2.png" height="200">  
<img src="media/3.png" height="200">  
<img src="media/4.png" height="200">  

## ���[�h�}�b�v

**Phase 1**
* �e�����g���@�\�̂݁B�i�V���A���v���b�^�Ɠ���v���g�R���j
* �ڑ����Azure IoT Hub�̂݁B

**Phase 2**  <--- �C�}�R�R
* Azure IoT Hub DPS�ɑΉ��B
* Azure IoT Central�ɑΉ��B

**Phase 3**
* �v���p�e�B�@�\�ƃR�}���h�@�\��ǉ��B�i�Ǝ��v���g�R���j

**Phase 4**
* IoT Plug and Play�ɑΉ��H

## Licence

[MIT](LICENSE.txt)
