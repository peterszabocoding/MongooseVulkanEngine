#include "util/filesystem.h"
#include <iostream>
#include <fstream>
#include <string>

namespace MongooseVK
{
    std::string FileSystem::ReadFileFromPath(const std::filesystem::path& filepath)
    {
        std::ifstream file(filepath);
        std::string str;
        std::string content;
        while (std::getline(file, str))
        {
            content += str;
            content.push_back('\n');
        }

        return content;
    }

    std::vector<char> FileSystem::ReadFile(const std::string& filepath)
    {
        std::ifstream file(filepath, std::ios::ate | std::ios::binary);

        if (!file.is_open()) throw std::runtime_error("Failed to open file: " + filepath);

        const size_t file_size = file.tellg();
        std::vector<char> buffer(file_size);

        file.seekg(0);
        file.read(buffer.data(), file_size);

        file.close();

        return buffer;
    }

    bool FileSystem::IsFileExist(const std::filesystem::path& path)
    {
        return exists(path);
    }

    bool FileSystem::IsFileExist(const std::string& path)
    {
        return exists(std::filesystem::path(path));
    }

    bool FileSystem::MakeDirectory(const std::filesystem::path& directory)
    {
        return create_directories(directory);
    }

    bool FileSystem::MakeDirectory(const std::string& directory)
    {
        return MakeDirectory(std::filesystem::path(directory));
    }

    bool FileSystem::IsDirectory(const std::filesystem::path& filepath)
    {
        return is_directory(filepath);
    }

    bool FileSystem::DeleteSingleFile(const std::filesystem::path& filepath)
    {
        if (!IsFileExist(filepath)) return false;
        if (is_directory(filepath)) return remove_all(filepath) > 0;

        return std::filesystem::remove(filepath);
    }

    bool FileSystem::MoveSingleFile(const std::filesystem::path& filepath, const std::filesystem::path& dest)
    {
        if (IsFileExist(dest)) return false;

        std::filesystem::rename(filepath, dest);
        return true;
    }

    bool FileSystem::CopySingleFile(const std::filesystem::path& filepath, const std::filesystem::path& dest)
    {
        if (IsFileExist(dest))
            return false;

        copy(filepath, dest);
        return true;
    }

    bool FileSystem::RenameSingleFile(const std::filesystem::path& filepath, const std::string& newName)
    {
        return MoveSingleFile(filepath, newName);
    }

    std::vector<std::filesystem::path> FileSystem::GetDirectoriesFromDirectory(const std::filesystem::path& directoryPath)
    {
        std::vector<std::filesystem::path> dirs;
        for (const auto& entry: std::filesystem::directory_iterator(directoryPath))
        {
            if (!entry.is_directory()) continue;
            dirs.push_back(entry.path());
        }
        return dirs;
    }

    std::vector<std::filesystem::path> FileSystem::GetFilesFromDirectory(const std::filesystem::path& directoryPath, bool forceUpdate)
    {
        std::vector<std::filesystem::path> files;

        const bool isCacheAvailable = s_FolderContentCache.find(directoryPath) != s_FolderContentCache.end();
        if (isCacheAvailable && !forceUpdate) return s_FolderContentCache[directoryPath];

        for (const auto& entry: std::filesystem::directory_iterator(directoryPath))
        {
            if (entry.is_directory()) continue;
            files.push_back(entry.path());
        }

        s_FolderContentCache[directoryPath] = files;
        return files;
    }

    std::vector<std::filesystem::path> FileSystem::GetFilesFromDirectoryDeep(const std::filesystem::path& directoryPath)
    {
        std::stack<std::filesystem::path> directories;
        std::vector<std::filesystem::path> files;

        directories.push(directoryPath);

        while (!directories.empty())
        {
            const std::filesystem::path dir = directories.top();
            directories.pop();
            for (const auto& entry: std::filesystem::directory_iterator(dir))
            {
                if (entry.is_directory())
                {
                    directories.push(entry);
                    GetFilesFromDirectoryDeepInner(entry, files, directories);
                } else
                {
                    files.push_back(entry.path());
                }
            }
        }
        return files;
    }

    std::vector<std::filesystem::path> FileSystem::GetFilesFromDirectoryDeep(const std::filesystem::path& directoryPath,
                                                                             const std::string& extensionFilter)
    {
        std::vector<std::filesystem::path> files = GetFilesFromDirectoryDeep(directoryPath);
        std::vector<std::filesystem::path> filteredFiles;

        std::copy_if(files.begin(), files.end(), std::back_inserter(filteredFiles), [&](const std::filesystem::path& file) {
                         return file.extension() == extensionFilter;
                     }
        );

        return filteredFiles;
    }

    std::vector<std::filesystem::path> FileSystem::GetFilesByExtension(const std::filesystem::path& directoryPath,
                                                                       const std::string& extension)
    {
        std::vector<std::filesystem::path> files;
        for (const auto& entry: std::filesystem::directory_iterator(directoryPath))
        {
            if (entry.is_directory()) continue;
            if (entry.path().extension() != extension) continue;
            files.push_back(entry.path());
        }
        return files;
    }

    FileStructureNode FileSystem::GetFileStructure(const std::filesystem::path& directoryPath, bool forceUpdate)
    {
        if (!forceUpdate && s_FileStructureCache.find(directoryPath) != s_FileStructureCache.end())
        {
            return s_FileStructureCache[directoryPath];
        }

        FileStructureNode root;
        root.path = directoryPath;
        root.isDirectory = is_directory(directoryPath);
        root.hasChildDirectory = false;
        root.isEmpty = true;

        if (!root.isDirectory) return root;

        const auto directories = GetDirectoriesFromDirectory(directoryPath);
        const auto files = GetFilesFromDirectory(directoryPath);

        for (auto& dir: directories)
        {
            FileStructureNode node = GetFileStructure(dir);
            node.path = dir;
            node.isDirectory = true;

            root.hasChildDirectory = true;
            root.isEmpty = false;
            root.children.push_back(node);
        }

        for (auto& file: files)
        {
            FileStructureNode node;
            node.isDirectory = false;
            node.path = file;

            root.isEmpty = false;
            root.children.push_back(node);
        }

        s_FileStructureCache[directoryPath] = root;

        return root;
    }

    std::string FileSystem::GetFileExtension(const std::filesystem::path& filePath)
    {
        return filePath.extension().string();
    }

    void FileSystem::GetFilesFromDirectoryDeepInner(
        const std::filesystem::path& directoryPath, std::vector<std::filesystem::path> files,
        std::stack<std::filesystem::path> directories)
    {
        while (!directories.empty())
        {
            const std::filesystem::path dir = directories.top();
            directories.pop();
            for (const auto& entry: std::filesystem::directory_iterator(dir))
            {
                if (entry.is_directory())
                    GetFilesFromDirectoryDeepInner(dir, files, directories);
                else
                    files.push_back(entry.path());
            }
        }
    }


    std::unordered_map<std::filesystem::path, FileStructureNode> FileSystem::s_FileStructureCache;
    std::unordered_map<std::filesystem::path, std::vector<std::filesystem::path>> FileSystem::s_FolderContentCache;
}
