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

* [Seeeduino V4.2](https://wiki.seeedstudio.com/Seeeduino_v4.2/)
* [Azure Sphere MT3620�J���{�[�h](https://wiki.seeedstudio.com/Azure_Sphere_MT3620_Development_Kit/)�iMT3620 RDB�j
* [�W�����p�[�E���X�t��Grove�P�[�u��](https://www.seeedstudio.com/Grove-4-pin-Female-Jumper-to-Grove-4-pin-Conversion-Cable-5-PCs-per-PAck.html)

## ��������

TBD

<img src="media/1.jpg" height="200">  
<img src="media/2.png" height="200">  
<img src="media/3.png" height="200">  
<img src="media/4.png" height="200">  

## ���[�h�}�b�v

**Phase 1**�@<--- �C�}�R�R
* �e�����g���@�\�̂݁B�i�V���A���v���b�^�Ɠ���v���g�R���j
* �ڑ����Azure IoT Hub�̂݁B

**Phase 2**
* Azure IoT Hub DPS�ɑΉ��B
* Azure IoT Central�ɑΉ��B

**Phase 3**
* �v���p�e�B�@�\�ƃR�}���h�@�\��ǉ��B�i�Ǝ��v���g�R���j

**Phase 4**
* IoT Plug and Play�ɑΉ��H

## Licence

[MIT](LICENSE.txt)
