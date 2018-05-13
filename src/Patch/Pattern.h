#pragma once

class Pattern {
public:
	Pattern(const char* pattern):length(0), data{} {
		while (length < MAX_LENGTH && *pattern) {
			if (*pattern == ' ') {
				++pattern;
			}
			else if (*pattern == '?') {
				++pattern;
				if (*pattern == '?') ++pattern;
				data[length] = -1;
				length++;
			}
			else {
				data[length] = 0;
				for (int i = 0; i < 2; i++) {
					if (*pattern >= '0' && *pattern <= '9') (data[length] *= 16) += *pattern - '0';
					else if (*pattern >= 'a' && *pattern <= 'f') (data[length] *= 16) += 10 + *pattern - 'a';
					else if (*pattern >= 'A' && *pattern <= 'F') (data[length] *= 16) += 10 + *pattern - 'f';
					else data[length] = -1;
				}
				if (data[length] < 0) data[length] = -1;
				length++;
			}
		}
	}
	bool Check(void* buff) const {
		unsigned char* p = (unsigned char*)buff;
		for (int i = 0; i < length; i++, p++) {
			if (this->data[i] < 0) continue;
			else if (this->data[i] != p[i]) return false;
		}
		return true;
	}
	int Legnth() const { return length; };

	static constexpr int MAX_LENGTH = 16;

private:
	int data[MAX_LENGTH];
	int length;
};

