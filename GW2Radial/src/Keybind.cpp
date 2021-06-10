#include <Keybind.h>
#include <Utility.h>
#include <sstream>
#include <ConfigurationFile.h>
#include <numeric>

namespace GW2Radial
{
std::set<Keybind*> Keybind::keybinds_;

Keybind::Keybind(std::string nickname, std::string displayName, std::string category, ScanCode key, Modifier mod, bool saveToConfig) :
	nickname_(std::move(nickname)), displayName_(std::move(displayName)), category_(std::move(category)), saveToConfig_(saveToConfig)
{
	keybinds_.insert(this);

	this->key(key);
	this->modifier(mod);

	isBeingModified_ = false;
}

Keybind::Keybind(std::string nickname, std::string displayName, std::string category) :
	nickname_(std::move(nickname)), displayName_(std::move(displayName)), category_(std::move(category))
{
	keybinds_.insert(this);
	auto keys = ConfigurationFile::i()->ini().GetValue("Keybinds.2", nickname_.c_str());
	if(keys) ParseConfig(keys);
	else {
		keys = ConfigurationFile::i()->ini().GetValue("Keybinds", nickname_.c_str());
		if(keys) ParseKeys(keys);
	}
	isBeingModified_ = false;
}

Keybind::~Keybind()
{
	keybinds_.erase(this);
}

void Keybind::ParseKeys(const char* keys)
{
	if(!isBeingModified_)
		return;

	key_ = ScanCode::NONE;
	mod_ = Modifier::NONE;

	if (strnlen_s(keys, 256) > 0)
	{
		std::stringstream ss(keys);
		std::vector<std::string> result;

		while (ss.good())
		{
			std::string substr;
			std::getline(ss, substr, ',');
			auto val = std::stoi(substr);
			val = MapVirtualKeyA(val, MAPVK_VK_TO_VSC);

			ScanCode code = ScanCode(uint(val));

			if (IsModifier(code)) {
				if (key_ != ScanCode::NONE)
					mod_ = mod_ | ToModifier(code);
				else
					key_ = code;
			} else {
				if (IsModifier(key_))
					mod_ = mod_ | ToModifier(key_);
				
				key_ = code;
			}
		}
	}

	ApplyKeys();
}

void Keybind::ParseConfig(const char* keys)
{
	std::vector<std::string> k;
	SplitString(keys, ",", std::back_inserter(k));
	if (k.empty()) {
		key_ = ScanCode::NONE;
		return;
	}

	key_ = ScanCode(uint(std::stoi(k[0].c_str())));
	if (k.size() == 1)
		mod_ = Modifier::NONE;
	else
		mod_ = Modifier(ushort(std::stoi(k[1].c_str())));
}

void Keybind::ApplyKeys()
{
	UpdateDisplayString();
	
	if(saveToConfig_ && key_ != ScanCode::NONE)
	{
		std::string settingValue = std::to_string(uint(key_)) + ", " + std::to_string(uint(mod_));

		auto cfg = ConfigurationFile::i();
		cfg->ini().SetValue("Keybinds.2", nickname_.c_str(), settingValue.c_str());
		cfg->Save();
	}
}

[[nodiscard]]
bool Keybind::matches(const Keyset& ks) const {
	return key_ == ks.first && mod_ == ks.second;
}

[[nodiscard]]
bool Keybind::matchesPartial(const Keyset& ks) const {
	return key_ == ks.first;
}

bool Keybind::matchesNoLeftRight(const Keyset& ks) const
{
	return MakeUniversal(key_) == MakeUniversal(ks.first) && MakeUniversal(mod_) == MakeUniversal(ks.second);
}

void Keybind::UpdateDisplayString()
{
	if(key_ == ScanCode::NONE)
	{
		keysDisplayString_[0] = '\0';
		return;
	}

	std::wstring keybind;
	if (Is(mod_ & Modifier::CTRL))
		keybind += L"CTRL + ";
	else if (Is(mod_ & Modifier::LCTRL))
		keybind += L"LCTRL + ";
	else if (Is(mod_ & Modifier::RCTRL))
		keybind += L"RCTRL + ";

	if (Is(mod_ & Modifier::ALT))
		keybind += L"ALT + ";
	else if (Is(mod_ & Modifier::LALT))
		keybind += L"LALT + ";
	else if (Is(mod_ & Modifier::RALT))
		keybind += L"RALT + ";

	if (Is(mod_ & Modifier::SHIFT))
		keybind += L"SHIFT + ";
	else if (Is(mod_ & Modifier::LSHIFT))
		keybind += L"LSHIFT + ";
	else if (Is(mod_ & Modifier::RSHIFT))
		keybind += L"RSHIFT + ";

	keybind += GetScanCodeName(key_);

	strcpy_s(keysDisplayString_.data(), keysDisplayString_.size(), utf8_encode(keybind).c_str());
}

void Keybind::ForceRefreshDisplayStrings() {
	for (auto& kb : keybinds_)
		kb->UpdateDisplayString();
}

}
