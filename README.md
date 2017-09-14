SoraVoice
=========

This project's objective is to bring Full Voice acting to the PC versions of *Sora/Zero/Ao no Kiseki*. Be aware that the *Evolution* version of these games feature new lines which do NOT appear in the originals, meaning those WON'T BE in game.

**NOTE:** This project is licensed under the GPLv3. You MUST copy, distribute and/or modify any code or binaries from this project under
this license. See [LICENSE](https://github.com/ZhenjianYang/SoraVoice/blob/master/LICENSE) for details.

## Build

You can get built files in [Release](https://github.com/ZhenjianYang/SoraVoice/releases),
or build them with VS2017 (Desktop development with C++).   

## Supported versions

|Game Title                    |Publisher |Version       | Language 
|------------------------------|----------|--------------|-------------------
|*Zero no Kiseki*              |Joyoland  |1.1           |Chinese Simplified
|                              |          |Cubejoy       |Chinese Simplified
|                              |          |JOYO Platform |Chinese Simplified
|*Ao no Kiseki*                |Joyoland  |1.0           |Chinese Simplified
|                              |          |Cubejoy       |Chinese Simplified
|                              |          |JOYO Platform |Chinese Simplified
|*Sora no Kiseki FC*           |YLT       |Final         |Chinese Simplified
|*Sora no Kiseki SC*           |YLT       |Final         |Chinese Simplified
|*Sora no Kiseki the 3RD*      |YLT       |Final         |Chinese Simplified

- **NOTE**: *Sora no Kiseki* series published by Xseed are supported by project
[SoraDataEx](https://github.com/ZhenjianYang/SoraDataEx). 
- **NOTE**: Not all voice scripts for *Sora no Kiseki* series are finished.

## About the Voice Scripts   

**Voice Scripts** are at the very core of the patch, as they call the needed **Voice Files** line by line. They contain all the dialogues, and because of that, obviously, each set of **Voice Scripts** is tied to a specific version of the games.

[ZeroAoVoiceScripts](https://github.com/ZhenjianYang/ZeroAoVoiceScripts) is a project about **Voice Scripts** for
*Zero no Kiseki* & *Ao no Kiseki*. **Voice Scripts** for the Chinese PC versions are done. **Voice Scripts** for the English Patches will be done once the official team will release them.    

[SoraVoiceScripts](https://github.com/ZhenjianYang/SoraVoiceScripts) is a project about **Voice Scripts** for
*Sora no Kiseki*/*Trails in the Sky* series. **Voice Scripts** for *Sora no Kiseki FC* (English, Steam/GOG/Humble)
are done, but *SC* & *3rd*'s scripts are not finished yet.       

- **NOTE**: This patch works in a strict way, based on **Timestamps**. EXE Files which are NOT listed in the [SoraDataEx](https://github.com/ZhenjianYang/SoraDataEx#about-the-timestamp) page are NOT supported. Use the mentioned software to check your EXE's timestamp.

## About dsound.dll   

For *Sora no Kiseki*/*Trails in the Sky* series, you can use **dsound.dll** instead of **dinput8.dll**, in case that
you couldn't use the latter for some reasons (e.g. another MOD also uses dinput8.dll).   

## About Audio Files

You may be wondering *Where am I supposed to find voice files?* Let me tell you how to extract them from the game itself.

- **NOTE**: We're going to assume you own the original *Evolution* versions, and DO know **how to dump your game from your PSVita**. We do NOT have, nor plan to upload those files here on GitHub.

So first, here's what you're gonna need:
1. Dumped *Sora*/*Zero*/*Ao no Kiseki Evolution*
2. [PSArcTool](https://github.com/periander/PSArcTool)
3. [at9Tool](http://s15.zetaboards.com/Amicitia/topic/8532372/1/) <-- See if you wanna download and upload it on Google Drive, to be on the safe side.
4. Any GUI with [oggenc2](http://www.rarewares.org/ogg-oggenc.php), or the encoder itself from CLI. I used [Foobar2000 Portable](http://www.foobar2000.org/download) and its [Encoder Pack](http://www.foobar2000.org/encoderpack) for batch encoding. Pick your poison.

Now, for the real deal:
1. Go to your `<Evolution Game Folder>\gamedata\` and you should find **data.psarc**.
- **NOTE**: You may also find **data0.psarc**. All **dataX.psarc** are needed.
2. Drag & Drop those on **PSArcTool.exe**, and it will extract (in the same place) the folder `data`.
3. Inside `data`, you'll find the `bgm` and `talk` folders, both saved as **at9**.
4. Create a `bat` file with Notepad. `Save as > All Files (format wise)> WhateverName.bat`.

For **BGMs**, copy this code inside the `bat`:
~~~~
@echo off

mkdir wav_out

for %%f in (*.at9) do (
   at9tool -d -wholeloop "%%f" "wav_out\%%~nf.wav"
)
~~~~

For **Voice Files**, copy this code inside the `bat`:

~~~~
@echo off

mkdir wav_out

for %%f in (*.at9) do (
   at9tool -d -repeat 1 "%%f" "wav_out\%%~nf.wav"
)
~~~~

Place the `bat` file and `at9Tool.exe` inside the `bgm` and/or `talk` folder, then launch the former. The converted files will be in the newly created `wav_out` folder.

- **NOTE**: In regard to *Sora no Kiseki FC* Xseed version, you'll also need `<Evolution Game Folder>\gamedata\data\bgm\arrange\ed6501.at9`, extracted as a **Voice File**.

5. Encode them to **ogg** and do NOT rename them.

Now you have your **BGMs** and **Voice Files**. The former are pretty straightforward to deal with, so we're gonna see how to setup the patch next.

## How to apply the patch

- **NOTE**: No game files will be overwritten.

1. Extract the files from the [Release](https://github.com/ZhenjianYang/SoraVoice/releases), and copy the `voice` folder and **dinput8.dll** into `<Game Folder>` (where the game EXE is).   
2. Copy **ed_voice.dll** into `<Game Folder>\voice\`. This one applies auto-scrolling and other features we're gonna check after this part.

- **NOTE**: A Beta version for *Sora no Kiseki FC* (and probably *SC* and *3rd* when **Voice Scripts** are done) Xseed version has been worked on as, at times, English subtitles are longer than their Voice Files, making those lines unreadable 'cause they scroll too fast.

3. Download the **Voice Scripts** linked above. Inside the archive, you'll find a folder named `scena`. Extract it AS IS inside `<Game Folder>\voice\`.   
4. Again, inside `<Game Folder>\voice\` create a folder named `ogg`, then copy **Voice Files** made in the previous step here.  

- **NOTE**: Remember that you also need **ed6501.ogg** inside this folder when it comes to *Sora no Kiseki FC* Xseed version.

5. (**For *Zero/Ao no Kiseki***) Create the `dll` folder inside `<Game Folder>\voice\`, then copy **ogg.dll**, **vorbis.dll** and **vorbisfile.dll** into it.   

- **NOTE**: *Sora no Kiseki* Xseed versions do NOT need the `dll` folder, as they come with it already. 

5. (**For *Sora no Kiseki* Xseed version**) Copy [**SoraDataEx.ini**](https://github.com/ZhenjianYang/SoraDataEx/blob/master/SoraDataEx/SoraDataEx.ini) into `<Game Folder>\voice\`. 
6. Launch your game.   

- **NOTE**: To disable the patch, simply rename **dinput8.dll** to something else.

## About the configuration file   

The voice patch's configuration file will be created after launching the game: `<Game Folder>\voice\ed_voice.ini`. This is what it looks like:

~~~~

Volume = 90
#Volume. 0~100, default is 100(Maximum).


AutoPlay = 1
#AutoPlay, list modes below:
;    0    Off
;    1    Enable for voiced dialogs only
;    2    Enable for all dialogs
;Waiting time for AutoPlay can be set by next 3 configurations.
#This configuration's default value is 2.
###NOTE: Setting this configuration on will force enable configuration SkipVoice.###

WaitTimePerChar = 60
#Waiting time for per character in a non-voiced dialog. Default is 60 (Time unit: millisecond).

WaitTimeDialog = 800
#Extra waiting time for a non-voiced dialog. Default is 800.
#Then with default settings, the total waiting time for a non-voiced dialog with 20 characters is 800+20x60=2000 milliseconds.

WaitTimeDialogVoice = 500
#Extra waiting time for a voiced dialog. Default is 500.
#Then with default settings, the total waiting time for a voiced dialog is 500+'length of voice'.


SkipVoice = 1
#Stop voice after dialog closed. Default is 1(Enable).
###NOTE: Setting this configuration to 0 will force disable AutoPlay.###

DisableDialogSE = 1
#Disable dialogs' closing/switching SE for voiced dialogs. Default is 1(Enable).

DisableDududu = 1
#Disable texts' SE for voiced dialogs. Default is 1(Enable).

ShowInfo = 0
#Show infomation, list modes below:
;    0    Off
;    1    Show infomation when configurations changed.
;    2    Show infomation when configurations changed, and show AutoPlay mark when AutoPlay a dialog.
#Default is 1


FontName = Microsoft Yahei
#Infomation's Font name(Not path). The font must be installed in your OS. Default is Microsoft Yahei.

FontColor = 0xFFFFFFFF
#Infomation's color. It's format is 0xAARRGGBB. Default is 0xFFFFFFFF(White).


EnableKeys = 1
#Enable hotkeys for configurations, list them below:
;     =     Volume +1(+5 when SHIFT is holding)
;     -     Volume -1(-5 when SHIFT is holding)
;     -=    Set mute (Mute status will be canceled after volume changed).
;     0     Switch AutoPlay
;     9     Switch SkipVoice
;     8     Switch DisableDialogSE
;     7     Switch DisableDududu
;     6     Switch ShowInfo
;\(Holding) Show all configurations' infomation (Even if ShowInfo is disabled).
;     []    Cancel mute, and:
;           1.When SaveChange=1, set all configurations to default, except for EnableKeys and SaveChange.
;           2.When SaveChange=0, reload configurations file.
#Default is 1(Enable).

SaveChange = 1
#When configurations changed, whether save them to disk or not.
#This configuration is meaningless if EnableKeys is disabled.
#Default is 1(Save).
###NOTE: Status of mute will not be saved.###

~~~~

I don't think I need to explain how it works, as it's self explanatory enough, right?

## External libraries used in this project   
-   [libVorbis & libOgg](https://www.xiph.org/), licensed under the [BSD-like license](https://www.xiph.org/licenses/bsd/).   
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
   (**对于《空之轨迹FC》**,还需一个额外的Vita版文件：`data/bgm/arrange/ed6501.at9`，并且需使用这个命令转为wav:   
   `at9tool.exe -d -repeat 1 ed6501.at9 ed6501.wav`)
5. (**对于《零/碧之轨迹》**)复制**ogg.dll**、**vorbis.dll**、**vorbisfile.dll**到`<游戏根目录>/voice/dll/`。   
6. (**对于Xseed发行的《空之轨迹》系列**) 复制[**SoraDataEx.ini**](https://github.com/ZhenjianYang/SoraDataEx/blob/master/SoraDataEx/SoraDataEx.ini)到`<游戏根目录>/voice/`。
7. 运行游戏。   

## 关于语音脚本   
[ZeroAoVoiceScripts](https://github.com/ZhenjianYang/ZeroAoVoiceScripts)是一个关于《零之轨迹》和《碧之轨迹》的**语音脚本**的项目。
用于简体中文PC版的语音脚本已经完成。   

[SoraVoiceScripts](https://github.com/ZhenjianYang/SoraVoiceScripts)是一个关于《空之轨迹》系列的**语音脚本**的项目。
用于《空之轨迹FC》(Steam/GOG/Humble, 英文版)的语音脚本已经完成。但SC和3rd的语音脚本尚未完成。   

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
|                  |          |方块平台版 |简体中文
|                  |          |JOYO平台版 |简体中文
|碧之轨迹          |欢乐百世  |1.0        |简体中文
|                  |          |方块平台版 |简体中文
|                  |          |JOYO平台版 |简体中文
|空之轨迹 FC       |娱乐通    |最终版     |简体中文
|空之轨迹 SC       |娱乐通    |最终版     |简体中文
|空之轨迹 the 3RD  |娱乐通    |最终版     |简体中文

- **注意**: 由Xseed发行的《空之轨迹》系列通过项目[SoraDataEx](https://github.com/ZhenjianYang/SoraDataEx)来获得支持。具体支持的游戏请查看项目的说明。   
            **用法**: 将**SoraDataEx.ini**复制到`<游戏根目录>/voice/`即可。   
- **注意**: 部分《空之轨迹》系列的游戏的语音脚本尚未完成，其语音补丁尚不可用。

## 本项目使用的外部库   
-   [libVorbis & libOgg](https://www.xiph.org/), 基于[BSD-like license](https://www.xiph.org/licenses/bsd/)。   
-   DirectX8 SDK。来自微软。    
-   DirectX9 SDK。只用到头文件, 这个工程里的文件是从[minidx9](https://github.com/hrydgard/minidx9)复制过来的。

## 特别感谢   
[Ouroboros](https://github.com/Ouroboros)，开发了轨迹系列游戏脚本的编译/反编译工具，并提供了各种神奇的Hook思路。

