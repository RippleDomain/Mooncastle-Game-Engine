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
                    OnPropertyChanged(nameof(ProjectPath));
                }
            }
        }

        private ObservableCollection<ProjectTemplate> _projectTemplates = new ObservableCollection<ProjectTemplate>();
        public ReadOnlyObservableCollection<ProjectTemplate> ProjectTemplates {get;}

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
            }
            catch (Exception e)
            {
                //TODO: Handle the exceptions properly.
                Debug.WriteLine(e.Message);
            }
        }
    }
}
