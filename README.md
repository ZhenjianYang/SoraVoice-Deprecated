SoraVoice
=========

Voice patches for PC games *Sora/Zero/Ao no Kiseki*.

**NOTE:** This projcet is licensed under the GPLv3. You MUST copy,
distribute and/or modify any code or binaries from this projcet under
this license. See
[LICENSE](https://github.com/ZhenjianYang/SoraVoice/blob/master/LICENSE)
for details.

## Build

You can get built files in [Release](https://github.com/ZhenjianYang/SoraVoice/releases),
or build them with VS2017 (Desktop development with c++).   

## How to use
**NOTE**: No games' files would be overwritten.   
1. Copy **dinput8.dll** to `<Game Root>/`.   
2. Copy **ed_voice.dll** to `<Game Root>/voice/`.   
3. Copy **Voice Scripts**(**.bin**(*Zaro*/*Ao*) or **\.\_SN**(*Sora*)) to `<Game Root>/voice/scena/`   
   (**NOTE**：**NOT NEED** to copy them to `<Game Root>/data/scena/`(*Zaro*/*Ao*) or pack them as `.dat/.dir`(*Sora*)).    
4. Extract voice files(.at9) from the Vita edition game, convert them to ogg (**at9** -> **wav** -> **ogg**), then copy them to `<Game Root>/voice/ogg/`.  
   (**ch0123456789.at9** should be converted to **ch0123456789.ogg**)    
5. (**For *Zaro/Ao no Kiseki***) Copy **ogg.dll**, **vorbis.dll** and **vorbisfile.dll** to `<Game Root>/voice/dll/`.   
6. (**For *Trails in the Sky* series published by Xseed**) Copy [**SoraDataEx.ini**](https://github.com/ZhenjianYang/SoraDataEx/blob/master/SoraDataEx/SoraDataEx.ini) to `<Game Root>/voice/`. 
7. Launch your game.   

## About the Voice Scripts   
[ZeroAoVoiceScripts](https://github.com/ZhenjianYang/ZeroAoVoiceScripts) is a project about **Voice Scripts** of
*Zero no Kiseki* & *Ao no Kiseki*. **Voice Scripts** for Chinese PC version games are done.       
But, **Voice Scripts** for *Sora no Kiseki* series are not finished yet. So voice patches for *Sora no Kiseki* series
are not available now.

## About dsound.dll   
For *Sora no Kiseki*/*Trails in the Sky* series, you can use **dsound.dll** instead of **dinput8.dll**, in case that
you couldn't use **dinput8.dll** for some reasons (e.g. another MOD also use dinput8.dll).   

## About the configuration file   
The voice patch's configuration file is `<Game Root>/voice/ed_voice.ini`.
It will be created after the game launched. And some configurations can be changed
by hotkeys during game playing. Check the configuration file with a text editor for
more details.

## About extra features
As a default setting, voice patch will automaticly advance dialogs.
And the dialog's SE will be diabled if the dialog has voice.   
For more details about these features, check the configuration file `ed_voice.ini`.

## By now, this project can work for   
|Game Title                    |Publisher |Version       | Language 
|------------------------------|----------|--------------|-------------------
|*Zero no Kiseki*              |Joyoland  |1.1           |Chinese Simplified
|                              |          |JOYO Platform |Chinese Simplified
|*Ao no Kiseki*                |Joyoland  |1.0           |Chinese Simplified
|                              |          |JOYO Platform |Chinese Simplified
|*Sora no Kiseki FC*           |YLT       |Final         |Chinese Simplified
|*Sora no Kiseki SC*           |YLT       |Final         |Chinese Simplified
|*Sora no Kiseki the 3RD*      |YLT       |Final         |Chinese Simplified

- **NOTE**: *Trails in the Sky* series published by Xseed are supported by project
[SoraDataEx](https://github.com/ZhenjianYang/SoraDataEx). Check this project for its supported games.   
             **Usage**: Copy **SoraDataEx.ini** to `<Game Root>/voice/`.   
- **NOTE**: Voice patches for *Sora no Kiseki* series are not available because their voice scripts are not done.

## External libraries used in this project   
-   [libVorbis & libOgg](https://www.xiph.org/), licensed under the
    [BSD-like license](https://www.xiph.org/licenses/bsd/).   
-   DirectX8 SDK. From Microsoft.    
-   DirectX9 SDK. Only header files are needed, I got them from [minidx9](https://github.com/hrydgard/minidx9).

## Special Thanks   

[Ouroboros](https://github.com/Ouroboros), developed decompile/compile tools for Kiseki series games, and provided many awesome hook ideas. 

------------------------------------------------------------------------

------------------------------------------------------------------------

SoraVoice
=========

PC游戏《空·零·碧之轨迹》系列的语音补丁

**注意：** 本项目基于GPLv3开源协议，对本项目的任何代码或二进制文件的复制、修改、分发需遵循此协议。
具体细节请参见[LICENSE](https://github.com/ZhenjianYang/SoraVoice/blob/master/LICENSE)文件。

## 编译

可以从[Release](https://github.com/ZhenjianYang/SoraVoice/releases)获取已编译好的文件，或者用VS2017(使用c++的桌面开发)来进行编译。

## 使用方法   
**注意**：语音补丁不会覆盖任何原有的游戏文件。   
1. 复制**dinput8.dll**到`<游戏根目录>/`。   
2. 复制**ed_voice.dll**到`<游戏根目录>/voice/`   
3. 复制**语音脚本**(**.bin**(零/碧) 或 **\.\_SN**(空))到`<游戏根目录>/voice/scena/`。   
   (**注意**：**没有必要**将语音脚本放到`<游戏根目录>/data/scena/`(零/碧)下面，或者打包成`.dat/.dir`(空)。)   
4. 导出Vita版的语音文件(.at9), 并转换为ogg格式(**at9** -> **wav** -> **ogg**), 然后复制到`<游戏根目录>/voice/ogg/`。   
   (**ch0123456789.at9**需转换为**ch0123456789.ogg**)  
5. (**对于《零/碧之轨迹》**)复制**ogg.dll**、**vorbis.dll**、**vorbisfile.dll**到`<游戏根目录>/voice/dll/`。   
6. (**对于Xseed发行的《空之轨迹》系列**) 复制[**SoraDataEx.ini**](https://github.com/ZhenjianYang/SoraDataEx/blob/master/SoraDataEx/SoraDataEx.ini)到`<游戏根目录>/voice/`。
7. 运行游戏。   

## 关于语音脚本   
[ZeroAoVoiceScripts](https://github.com/ZhenjianYang/ZeroAoVoiceScripts)是一个关于《零之轨迹》和《碧之轨迹》的**语音脚本**的项目。
用于简体中文PC版的语音脚本已经完成。   
但是，《空之轨迹》系列的语音脚本尚未完成，所以目前《空之轨迹》系列的语音补丁尚不可用。

## 关于dsound.dll   
对于《空之轨迹》系列, 可以用**dsound.dll**替换掉**dinput8.dll**, 以应对**dinput8.dll**无法使用的情况(比如另一个MOD也使用了dinput8.dll)。 

## 关于配置文件   
语音补丁的配置文件为`<游戏根目录>/voice/ed_voice.ini`。
配置文件会在游戏启动的时候自动创建。并且，部分配置项可以在游戏进行的过程中通过快捷键来进行修改。
具体请参考配置文件内的说明(用文本编辑器打开即可)。

## 关于补丁的额外功能    
在默认配置下，补丁会让对话框自动前进；同时，会禁用掉有语音对话框的文字音效。   
关于这些功能的更多说明，请参考配置文件`ed_voice.ini`。

## 目前，本工程可用于   
|游戏标题          |发行商    |版本       | 语言 
|------------------|----------|---------- |---------
|零之轨迹          |欢乐百世  |1.1        |简体中文
|                  |          |JOYO平台版 |简体中文
|碧之轨迹          |欢乐百世  |1.0        |简体中文
|                  |          |JOYO平台版 |简体中文
|空之轨迹 FC       |娱乐通    |最终版     |简体中文
|空之轨迹 SC       |娱乐通    |最终版     |简体中文
|空之轨迹 the 3RD  |娱乐通    |最终版     |简体中文

- **注意**: 由Xseed发行的《空之轨迹》系列通过项目[SoraDataEx](https://github.com/ZhenjianYang/SoraDataEx)来获得支持。具体支持的游戏请查看项目的说明。   
            **用法**: 将**SoraDataEx.ini**复制到`<游戏根目录>/voice/`即可。   
- **注意**: 由于《空之轨迹》系列的语音脚本尚未完成，其语音补丁尚不可用。

## 本项目使用的外部库   
-   [libVorbis & libOgg](https://www.xiph.org/), 基于[BSD-like license](https://www.xiph.org/licenses/bsd/)。   
-   DirectX8 SDK。来自微软。    
-   DirectX9 SDK。只用到头文件, 这个工程里的文件是从[minidx9](https://github.com/hrydgard/minidx9)复制过来的。

## 特别感谢   
[Ouroboros](https://github.com/Ouroboros)，开发了轨迹系列游戏脚本的编译/反编译工具，并提供了各种神奇的Hook思路。

