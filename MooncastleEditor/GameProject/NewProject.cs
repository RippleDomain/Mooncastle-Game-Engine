using MooncastleEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace MooncastleEditor.GameProject
{
    [DataContract]
    public class ProjectTemplate
    {
        [DataMember]
        public string ProjectType {get; set;}
        [DataMember]
        public string ProjectName {get; set;}
        [DataMember]
        public List<string> Folders {get; set;}

        public string IconFilePath {get; set;}
        public string ScreenShotFilePath {get; set;}
        public string ProjectFilePath {get; set;}

        private ImageSource _icon;
        public ImageSource Icon
        {
            get
            {
                if (_icon == null && File.Exists(IconFilePath))
                {
                    _icon = LoadImage(IconFilePath);
                }
                    
                return _icon;
            }
        }


        private ImageSource _screenshot;
        public ImageSource ScreenShot
        {
            get
            {
                if (_screenshot == null && File.Exists(ScreenShotFilePath))
                {
                    _screenshot = LoadImage(ScreenShotFilePath);
                }

                return _screenshot;
            }
        }

        private static BitmapImage LoadImage(string path)
        {
            var image = new BitmapImage();

            image.BeginInit();

            image.UriSource = new Uri(path);
            image.CacheOption = BitmapCacheOption.OnLoad;

            image.EndInit();
            image.Freeze();

            return image;
        }
    }

    class NewProject : ViewModelBase
    {
        //TODO: Get the path from the installation location instead.
        private readonly string _templatePath = @"..\..\MooncastleEditor\ProjectTemplates";

        private string projectName = "NewProject";

        public string ProjectName
        {
            get 
            {
                return projectName; 
            }
            set
            {
                if (projectName != value)
                {
                    projectName = value;
                    IsProjectPathValid();
                    OnPropertyChanged(nameof(ProjectName));
                }
            }
        }

        private string path = @$"{Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)}\MooncastleProjects\";
        public string ProjectPath
        {
            get
            {
                return path;
            }
            set
            {
                if (path != value)
                {
                    path = value;
                    IsProjectPathValid();
                    OnPropertyChanged(nameof(ProjectPath));
                }
            }
        }

        private bool _isValid;
        public bool IsValid
        {
            get
            {
                return _isValid;
            }
            set
            {
                if (_isValid != value)
                {
                    _isValid = value;
                    OnPropertyChanged(nameof(IsValid));
                }
            }
        }

        private string _errorMessage;
        public string ErrorMessage
        {
            get
            {
                return _errorMessage;
            }
            set
            {
                if (_errorMessage != value)
                {
                    _errorMessage = value;
                    OnPropertyChanged(nameof(ErrorMessage));
                }
            }
        }

        private ObservableCollection<ProjectTemplate> _projectTemplates = new ObservableCollection<ProjectTemplate>();
        public ReadOnlyObservableCollection<ProjectTemplate> ProjectTemplates {get;}

        private bool IsProjectPathValid()
        {
            var path = ProjectPath;

            if (!Path.EndsInDirectorySeparator(path))
            {
                path += Path.DirectorySeparatorChar;
            }

            path += $@"{ProjectName}\";

            IsValid = false;

            if (string.IsNullOrWhiteSpace(ProjectName))
            {
                ErrorMessage = "Project name cannot be empty.";
            }
            else if (ProjectName.IndexOfAny(Path.GetInvalidFileNameChars()) != -1)
            {
                ErrorMessage = "Project name should not include invalid characters.";
            }
            else if (string.IsNullOrWhiteSpace(ProjectPath))
            {
                ErrorMessage = "Select a valid path for your project.";
            }
            else if (ProjectPath.IndexOfAny(Path.GetInvalidPathChars()) != -1)
            {
                ErrorMessage = "Project path should not include invalid characters.";
            }
            else if (Directory.Exists(path) && Directory.EnumerateFileSystemEntries(path).Any())
            {
                ErrorMessage = "The path you have chosen already exists and is not empty.";
            }
            else
            {
                ErrorMessage = string.Empty;
                IsValid = true;
            }

            return IsValid;
        }

        public string CreateProject(ProjectTemplate template)
        {
            IsProjectPathValid();

            if (IsValid != true)
            {
                return string.Empty;
            }
            if (Path.EndsInDirectorySeparator(ProjectPath) != true)
            {
                ProjectPath += Path.DirectorySeparatorChar;

                return string.Empty;
            }
            
            var path = $@"{ProjectPath}{ProjectName}\";

            try
            {
                if (Directory.Exists(path) == false)
                {
                    Directory.CreateDirectory(path);
                }

                foreach (var folder in template.Folders)
                {
                    Directory.CreateDirectory(Path.GetFullPath(Path.Combine(Path.GetDirectoryName(path), folder)));
                }

                var directoryInfo = new DirectoryInfo(path + @".Mooncastle\");
                directoryInfo.Attributes |= FileAttributes.Hidden;

                File.Copy(template.IconFilePath, Path.GetFullPath(Path.Combine(directoryInfo.FullName, "Icon.png")));
                File.Copy(template.ScreenShotFilePath, Path.GetFullPath(Path.Combine(directoryInfo.FullName, "ScreenShot.png")));

                var projectXMLFile = File.ReadAllText(template.ProjectFilePath);
                projectXMLFile = string.Format(projectXMLFile, ProjectName, ProjectPath);
                var projectFilePath = Path.GetFullPath(Path.Combine(path, $"{ProjectName}{Project.Extension}"));
                File.WriteAllText(projectFilePath, projectXMLFile);

                return path;
            }
            catch (Exception e)
            {
                Debug.WriteLine(e.Message);
                Logger.Log(MessageType.Error, $"Failed to create {ProjectName} to {path}");

                throw;
            }
        }

        public NewProject()
        {
            ProjectTemplates = new ReadOnlyObservableCollection<ProjectTemplate>(_projectTemplates);

            try
            {
                var templates = Directory.GetFiles(_templatePath, "template.xml", SearchOption.AllDirectories);

                Debug.Assert(templates.Any());

                foreach (var file in templates)
                {
                    var template = Serializer.FromFile<ProjectTemplate>(file);

                    template.IconFilePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(file), "Icon.png"));
                    template.ScreenShotFilePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(file), "ScreenShot.png"));
                    template.ProjectFilePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(file), template.ProjectName));

                    _projectTemplates.Add(template);
                }

                IsProjectPathValid();
            }
            catch (Exception e)
            {
                Debug.WriteLine(e.Message);
                Logger.Log(MessageType.Error, $"Failed to read project templates.");

                throw;
            }
        }
    }
}
