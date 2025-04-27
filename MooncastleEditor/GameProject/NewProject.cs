using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.ComponentModel;
using System.IO;
using System.Diagnostics;
using System.Runtime.Serialization;
using MooncastleEditor.Utilities;
using System.Collections.ObjectModel;

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

        public byte[] Icon {get; set;}
        public byte[] ScreenShot {get; set;}
        public string IconFilePath {get; set;}
        public string ScreenShotFilePath {get; set;}
        public string ProjectFilePath {get; set;}

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
                    template.Icon = File.ReadAllBytes(template.IconFilePath);
                    template.ScreenShotFilePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(file), "Screenshot.png"));
                    template.ScreenShot = File.ReadAllBytes(template.ScreenShotFilePath);
                    template.ProjectFilePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(file), template.ProjectName));

                    _projectTemplates.Add(template);
                }

                IsProjectPathValid();
            }
            catch (Exception e)
            {
                //TODO: Handle the exceptions properly.
                Debug.WriteLine(e.Message);
            }
        }
    }
}
