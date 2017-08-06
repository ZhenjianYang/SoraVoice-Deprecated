#pragma once

struct CMessage {
	static void LoadMessage();

	const char * Title;
	const char * Version;

	const char * CurrentTitle;

	const char * Switch[2];

	const char * Volume;
	const char * Mute;

	const char * OriginalVoiceSwitch[3];
	const char * OriginalVoice;

	const char * OriVolumePercent;

	const char * Reset;

	const char * AutoPlaySwitch[3];
	const char * AutoPlay;

	const char * SkipVoice;

	const char * DisableDialogSE;

	const char * DisableDududu;

	const char * ShowInfoSwitch[3];

	const char * ShowInfo;

	const char * AutoPlayMark;

	struct CCMT {
		const char * Volume;

		const char * OriginalVoice;

		const char * OriVolumePercent;

		const char * AutoPlay;

		const char * WaitTimePerChar;

		const char * WaitTimeDialog;

		const char * WaitTimeDialogVoice;

		const char * SkipVoice;

		const char * DisableDialogSE;

		const char * DisableDududu;

		const char * ShowInfo;

		const char * FontName;

		const char * FontColor;

		const char * EnableKeys;

		const char * SaveChange;

		friend struct CMessage;
	private:
		const char * EnableKeys_ZA;
		const char * EnableKeys_Sora;

		CCMT() { };
	} CMT;

private:
	const char * Title_Sora;
	const char * Title_ZA;

	const char * On;
	const char * Off;

	const char * OriEvoVoiceBoth;
	const char * EvoVoicOnly;
	const char * OriVoiceOnly;

	const char * AutoPlayVoice;
	const char * AutoPlayAll;

	const char * ShowInfoAuto;

	CMessage() { }
	static bool LoadMessage(const char* resName);

public:
	static CMessage _Message;
};
extern const CMessage& Message;


