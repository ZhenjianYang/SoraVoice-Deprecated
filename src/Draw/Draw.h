#pragma once

namespace Draw {
	enum class InfoType
	{
		Hello = 0,
		InfoOnoff,
		AutoPlayMark,

		Volume,
		OriginalVoice,
		OriVolumePercent,

		AutoPlay,
		SkipVoice,
		DisableDialogSE,
		DisableDududu,

		ConfigReset,

		All,
		Dead
	};

	static constexpr unsigned ShowTimeInfinity = 0;

	bool Init();
	bool End();

	void DrawInfos();

	void AddInfo(InfoType type, unsigned time, unsigned color, const char* text);
	void RemoveInfo(InfoType type);

	const unsigned& Showing();
};
