#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>

inline constexpr std::filesystem::directory_options kDirectoryOptions = std::filesystem::directory_options::skip_permission_denied;

inline void AddIncludePath(std::string* _pIncludeCommand, const std::filesystem::path& _Path)
{
  *_pIncludeCommand += "-I\"" + std::filesystem::weakly_canonical(_Path).string() + "\"" + " ";
};

//  現在のフォルダが指定したフォルダと同じならtrue
inline bool MatchCurrentDirectory(const std::string& _DirectoryName, const std::string& _Path)
{
  const std::size_t Index = _Path.rfind("\\");

  return _DirectoryName.compare(&_Path[Index + 1]) == 0;
}

//  最新バージョンを取得
inline std::filesystem::path GetLatestVersion(const std::filesystem::path& _Directory)
{
  std::vector<std::filesystem::path> Versions;
  for (auto& Entry : std::filesystem::directory_iterator(_Directory, kDirectoryOptions))
  {
    Versions.emplace_back(Entry.path());
  }
  std::sort(Versions.begin(), Versions.end());

  return Versions.back();
}

int main(int Argc, char* Argv[])
{
  try
  {
    const std::string IWYUFilePath = std::filesystem::path(std::filesystem::path(Argv[0]).remove_filename() / "include-what-you-use.exe").string();
    if (!std::filesystem::exists(IWYUFilePath))
    {
      throw "include-what-you-use.exeが存在しません。";
    }

    std::string IncludeCommand;

    ////  VisualStudioのIncludeパスを全取得
    //{
    //  constexpr const char* kVSBasePath = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019";
    //
    //  std::filesystem::path Directory(kVSBasePath);
    //
    //  //  Editionを取得  
    //  {
    //    constexpr const char* kEditionPriority[] = { "Enterprise", "Professional", "Community" };
    //
    //    const std::filesystem::directory_iterator EditionIterator(Directory, std::filesystem::directory_options::skip_permission_denied);
    //
    //    auto SearchEdition = [&_Iterator = EditionIterator](const char* _EditionName)->bool
    //    {
    //      for (auto& Entry : _Iterator)
    //      {
    //        if (!Entry.is_directory())
    //        {
    //          continue;
    //        }
    //
    //        if (Entry.path().string().find(_EditionName) != std::string::npos)
    //        {
    //          return true;
    //        }
    //      }
    //
    //      return false;
    //    };
    //
    //    for (auto& Edition : kEditionPriority)
    //    {
    //      if (SearchEdition(Edition))
    //      {
    //        Directory /= std::string(Edition) + "\\VC";
    //        break;
    //      }
    //    }
    //  }
    //
    //  //  Versionを取得
    //  Directory /= "Tools\\MSVC";
    //  Directory /= GetLatestVersion(Directory);
    //
    //  constexpr const char* kSearchDirectoryName = "include";
    //  //  残りのIncludeパスを追加
    //  for (auto& Entry : std::filesystem::recursive_directory_iterator(Directory, kDirectoryOptions))
    //  {
    //    if (!Entry.is_directory())
    //    {
    //      continue;
    //    }
    //
    //    if (const std::string Path = Entry.path().string();
    //      MatchCurrentDirectory(kSearchDirectoryName, Path))
    //    {
    //      AddIncludePath(&IncludeCommand, Path);
    //    }
    //  }
    //}
    //
    ////  WindowsSDKのIncludeパスを全取得
    //{
    //  constexpr const char* kWinSDKBasePath = "C:\\Program Files (x86)\\Windows Kits\\10\\Include";
    //
    //  std::filesystem::path Directory(kWinSDKBasePath);
    //  Directory /= GetLatestVersion(Directory);
    //
    //
    //  for (auto& Entry : std::filesystem::directory_iterator(Directory, kDirectoryOptions))
    //  {
    //    if (!Entry.is_directory())
    //    {
    //      continue;
    //    }
    //
    //    AddIncludePath(&IncludeCommand, Entry.path().string());
    //  }
    //}

    for (int i = 1; i < Argc; ++i)
    {
      std::string Path = Argv[i];
      if (Path.find(".vcxproj") == std::string::npos)
      {
        continue;
      }

      std::vector<std::string> ProjectSourcePaths;
      std::vector<std::string> ProjectIncludePaths;
      auto AddProjectIncludePath = [&ProjectIncludePaths](std::string&& _Path)->void
      {
        if (_Path[0] == '%')
        {
          return;
        }

        for (auto& IncludePath : ProjectIncludePaths)
        {
          if (IncludePath == _Path)
          {
            return;
          }
        }

        ProjectIncludePaths.emplace_back(std::move(_Path));
      };

      std::ifstream IStream(Path);
      std::string Line;

      Path = std::filesystem::path(Path).remove_filename().string();
      while (std::getline(IStream, Line, '\n'))
      {
        //  ソースコードを取得
        if (Line.find("ClCompile Include") != std::string::npos)
        {
          const std::size_t Start = Line.find("\"") + 1;
          ProjectSourcePaths.emplace_back(Path + std::string(&Line[Start], Line.find("\"", Start) - Start));
        }
        //  Includeパスを取得
        else if (Line.find("AdditionalIncludeDirectories") != std::string::npos)
        {
          std::size_t Start = Line.find(">") + 1;

          while (true)
          {
            if (const std::size_t Index = Line.find(";", Start);
              Index != std::string::npos)
            {
              AddProjectIncludePath(Path + std::string(&Line[Start], Index - Start));
              Start = Index + 1;
              continue;
            }

            AddProjectIncludePath(Path + std::string(&Line[Start], Line.find("<", Start) - Start));
            break;
          }
        }
      }

      std::string ProjectIncludeCommand = IncludeCommand;
      for (auto& ProjectIncludePath : ProjectIncludePaths)
      {
        AddIncludePath(&ProjectIncludeCommand, ProjectIncludePath);
      }

      for (auto& ProjectSourcePath : ProjectSourcePaths)
      {
        ::system(std::string(IWYUFilePath + " " + std::filesystem::weakly_canonical(ProjectSourcePath).string() + " " + ProjectIncludeCommand + ">>Result.txt").c_str());
      }
    }
  }
  catch (const std::string& e)
  {
    std::cout << e << std::endl;
  }

  return 0;
}
