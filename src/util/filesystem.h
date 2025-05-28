#pragma once

#include <fstream>
#include <stdexcept>
#include <vector>

#include <filesystem>
#include <stack>
#include <unordered_map>

namespace Raytracing {

	struct FileStructureNode
	{
		std::vector<FileStructureNode> children;
		std::filesystem::path path;

		bool isDirectory = false;
		bool isEmpty = false;
		bool hasChildDirectory = false;

		bool operator==(const FileStructureNode& fsn) const
		{
			return fsn.path == path;
		}

		std::string Filename() const
		{
			return path.filename().string();
		}
		std::string Directory() const
		{
			std::string dirPath = path.string();

			const std::string parentPath = path.parent_path().string();
			const auto substrPos = dirPath.find(parentPath);

			if (dirPath != parentPath) dirPath = dirPath.erase(substrPos, parentPath.length());

			dirPath.erase(std::remove(dirPath.begin(), dirPath.end(), '\\'), dirPath.end());
			return dirPath;
		}
	};

	class FileSystem
	{
	public:
		static std::string ReadFileFromPath(const std::filesystem::path& filepath);
		static std::vector<char> ReadFile(const std::string& filepath);


		static bool IsFileExist(const std::filesystem::path& path);
		static bool IsFileExist(const std::string& path);

		static bool MakeDirectory(const std::filesystem::path& directory);
		static bool MakeDirectory(const std::string& directory);
		static bool IsDirectory(const std::filesystem::path& filepath);

		static bool DeleteSingleFile(const std::filesystem::path& filepath);
		static bool MoveSingleFile(const std::filesystem::path& filepath, const std::filesystem::path& dest);
		static bool CopySingleFile(const std::filesystem::path& filepath, const std::filesystem::path& dest);
		static bool RenameSingleFile(const std::filesystem::path& filepath, const std::string& newName);

		static std::vector<std::filesystem::path> GetDirectoriesFromDirectory(const std::filesystem::path& directoryPath);
		static std::vector<std::filesystem::path> GetFilesFromDirectory(const std::filesystem::path& directoryPath, bool forceUpdate = false);
		static std::vector<std::filesystem::path> GetFilesFromDirectoryDeep(const std::filesystem::path& directoryPath);
		static std::vector<std::filesystem::path> GetFilesFromDirectoryDeep(const std::filesystem::path& directoryPath, const std::string& extensionFilter);
		static std::vector<std::filesystem::path> GetFilesByExtension(const std::filesystem::path& directoryPath, const std::string& extension);
		static FileStructureNode GetFileStructure(const std::filesystem::path& directoryPath, bool forceUpdate = false);

		static std::string GetFileExtension(const std::filesystem::path& filePath);
	private:
		static void GetFilesFromDirectoryDeepInner(const std::filesystem::path& directoryPath, std::vector<std::filesystem::path> files, std::stack<std::filesystem::path> directories);

	private:
		static std::unordered_map<std::filesystem::path, FileStructureNode> s_FileStructureCache;
		static std::unordered_map<std::filesystem::path, std::vector<std::filesystem::path>> s_FolderContentCache;
	};
}
