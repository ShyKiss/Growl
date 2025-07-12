#pragma once
#include "PCH.h"

class BNKManager {
	std::string Path;
	std::vector<char> Data;
	std::vector<BNK::DIDXEntry> Entries;
	std::map<std::string, BNK::Block> Blocks;
	std::vector<BNK::WemFile> Wems;
	bool IsValid;

	std::string ConvertWemToOgg(const std::string& wempath);
	std::string NormalizeOgg(const std::string& oggpath);
	std::vector<char> Read(const std::string& filepath);
	std::map<std::string, BNK::Block> ParseBnkBlocks(const std::vector<char>& data);
	std::vector<BNK::DIDXEntry> ParseDIDX(const std::vector<char>& data, const BNK::Block& didx_block);
	std::vector<char> BuildDIDX(const std::vector<BNK::DIDXEntry>& entries);
	void WriteBnk(const std::string& path, const std::vector<char>& new_didx, const std::vector<char>& new_data);
	void GenerateWemInfo();

public:
	BNKManager(const std::string& filepath);
	bool								GetIsValid() { return IsValid; }
	std::string							GetPath() { return Path; }
	std::vector<char>					GetData() { return Data; }
	std::vector<BNK::DIDXEntry>			GetEntries() { return Entries; }
	std::map<std::string, BNK::Block>	GetBlocks() { return Blocks; }
	std::vector<BNK::WemFile>			GetWems() { return Wems; }

	std::string ReplaceWems(const std::vector<std::pair<int, std::string>>& replacements, const std::string& out_path);
	std::string ExtractWem(int index, const std::string out_path);

};