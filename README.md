SoraVoice
=========

Voice patches for the PC games *Sora/Zero/Ao no Kiseki*.

**NOTE:** This projcet is licensed under the GPLv3. You MUST copy,
distribute and/or modify any code or binaries from this projcet under
this license. See
[LICENSE](https://github.com/ZhenjianYang/SoraVoice/blob/master/LICENSE)
for details.

## Build

You can get built files in [Release](https://github.com/ZhenjianYang/SoraVoice/releases),
or build them with VS2017 (Desktop development with c++, with Windows XP support).   

## How to use

### For ***Zero/Ao no Kiseki***:   
1. Copy **za_voice.dll**, **dinput.dll** to the game's root folder.   
2. Copy **Voice Scripts**(.bin) to `data/scena/`.   
3. Extract voice files(.at9) from the Vita edition game, convert them to ogg, then copy them to `voice/ogg/`.   
   (**v1234567.at9** should be converted to **v1234567.ogg**)   
4. Copy **ogg.dll**, **vorbis.dll** and **vorbisfile.dll** (or libogg.dll, libvorbis.dll and libvorbisfile.dll)
to `voice/dll/`.   
5. Launch your game.   

[ZeroAoVoiceScripts](https://github.com/ZhenjianYang/ZeroAoVoiceScripts) is a project about **Voice Scripts** of
*Zero no Kiseki* & *Ao no Kiseki*.

### For ***Sora no Kiseki*** series:   
1. Copy **ed_voice.dll**(or **ed_voice_dx9.dll** for DX9 mode), **dinput.dll** to the game's root folder.   
2. Copy **ED6_DT01.dat/.dir** (FC) or **ED6_DT21.dat/.dir** (SC & 3RD) with **Voice Scripts** to the game's root folder.   
3. Extract voice files(.at9) from the Vita edition game, convert them to ogg, then copy them to `voice/ogg/`.   
   (**ch0123456789.at9** should be converted to **ch0123456789.ogg**)     
4. Launch your game.   

#### About Voice Scripts for *Sora no Kiseki* series:   
Sorry, they are not done. So voice patches for *Sora no Kiseki* series are not available now.

## About the configuration file   
The voice patch's configuration file is **ed_voice.ini**(Sora) or **za_voice.ini**(Zero/Ao),
it will be created after the game launched. And some configurations can be changed
by hotkeys during game playing. Check the configuration file with a text editor for
more details.

## By now, this project can work for:   
|Game Title                    |Publisher |Version       | Language 
|------------------------------|----------|--------------|-------------------
|*Zero no Kiseki*              |Joyoland  |1.1           |Chinese Simplified
|                              |          |JOYO Platform |Chinese Simplified
|*Ao no Kiseki*                |Joyoland  |1.0           |Chinese Simplified
|                              |          |JOYO Platform |Chinese Simplified
|*Sora no Kiseki FC*           |YLT       |Final         |Chinese Simplified
|*Sora no Kiseki SC*           |YLT       |Final         |Chinese Simplified
|*Sora no Kiseki the 3RD*      |YLT       |Final         |Chinese Simplified

- **NOTE**: *Trails in the Sky* series published by Xseed could be supported by [SoraDataEx](https://github.com/ZhenjianYang/SoraDataEx).   
             Copy **SoraDataEx.ini** to `<Game Root Folder>/voice/`.   
- **NOTE**: Voice patches for *Sora no Kiseki* series are not available because their voice scripts are not done.

## External libraries used in this project   
-   [libVorbis & libOgg](https://www.xiph.org/), licensed under the
    [BSD-like license](https://www.xiph.org/licenses/bsd/).   
-   DirectX8 SDK. From Microsoft.    
-   DirectX9 SDK. Only header files are needed, I got them from [minidx9](https://github.com/hrydgard/minidx9).

------------------------------------------------------------------------

------------------------------------------------------------------------

SoraVoice
=========

PC游戏《空·零·碧之轨迹》系列的语音补丁

**注意：** 本项目基于GPLv3开源协议，对本项目的任何代码或二进制文件的复制、修改、分发需遵循此协议。
具体细节请参见[LICENSE](https://github.com/ZhenjianYang/SoraVoice/blob/master/LICENSE)文件。

## 编译

可以从[Release](https://github.com/ZhenjianYang/SoraVoice/releases)获取已编译好的文件，或者用VS2017(使用c++的桌面开发, 含对C++的Windows XP支持)来进行编译。

## 使用方法

### 《零/碧之轨迹》:   
1. 复制**za_voice.dll**、**dinput.dll**到游戏根目录。  
2. 复制**语音脚本**(.bin)到`data/scena/`。   
3. 导出Vita版的语音文件(.at9), 转换为ogg格式后, 复制到`voice/ogg/`。   
   (**v1234567.at9**需转换为**v1234567.ogg**)   
4. 复制**ogg.dll**、**vorbis.dll**、**vorbisfile.dll** (或者libogg.dll、libvorbis.dll、libvorbisfile.dll)到`voice/dll/`。   
5. 运行游戏。   

[ZeroAoVoiceScripts](https://github.com/ZhenjianYang/ZeroAoVoiceScripts)是一个关于《零之轨迹》和《碧之轨迹》的语音脚本的项目。

### 《空之轨迹》系列:   
1. 复制**ed_voice.dll**(DX9模式为**ed_voice_dx9.dll**), **dinput.dll**到游戏根目录。  
2. 复制带**语音脚本**的**ED6_DT01.dat/.dir**(FC)或**ED6_DT21.dat/.dir**(SC及3RD)到游戏根目录。   
3. 导出Vita版的语音文件(.at9), 转换为ogg格式后, 复制到`voice/ogg/`。  
   (**ch0123456789.at9**需转换为**ch0123456789.ogg**)     
5. 运行游戏。     

#### 关于《空之轨迹》系列的语音脚本：   
很抱歉，《空之轨迹》系列的语音脚本尚未完成，所以目前《空之轨迹》系列的语音补丁尚不可用。

## 关于配置文件   
语音补丁的配置文件为**ed_voice.ini**(空)或**za_voice.ini**(零/碧)。
配置文件会在游戏启动的时候自动创建。并且，部分配置项可以在游戏进行的过程中通过快捷键来进行修改。
具体请参考配置文件内的说明(用文本编辑器打开即可)。

## 目前，本工程可用于:   
|游戏标题          |发行商    |版本       | 语言 
|------------------|----------|---------- |---------
|零之轨迹          |欢乐百世  |1.1        |简体中文
|                  |          |JOYO平台版 |简体中文
|碧之轨迹          |欢乐百世  |1.0        |简体中文
|                  |          |JOYO平台版 |简体中文
|空之轨迹 FC       |娱乐通    |最终版      |简体中文
|空之轨迹 SC       |娱乐通    |最终版      |简体中文
|空之轨迹 the 3RD  |娱乐通    |最终版      |简体中文

- **注意**: 由Xseed发行的《空之轨迹》系列可通过[SoraDataEx](https://github.com/ZhenjianYang/SoraDataEx)来获得支持。   
            将**SoraDataEx.ini**复制到`<Game Root Folder>/voice/`即可。   
- **注意**: 由于《空之轨迹》系列的语音脚本尚未完成，其语音补丁尚不可用。

## 本项目使用的外部库   
-   [libVorbis & libOgg](https://www.xiph.org/), 基于[BSD-like license](https://www.xiph.org/licenses/bsd/)。  
-   DirectX8 SDK。来自微软。    
-   DirectX9 SDK。只用到头文件, 这个工程里的文件是从[minidx9](https://github.com/hrydgard/minidx9)复制过来的。
