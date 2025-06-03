using MooncastleEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
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
    public class ProjectData
    {
        [DataMember]
        public string ProjectName { get; set; }

        [DataMember]
        public string ProjectPath { get; set; }

        [DataMember]
        public DateTime Date { get; set; }

        public string FullPath { get => Path.Combine(ProjectPath, ProjectName + Project.Extension); }
        public string IconPath { get; set; }
        public string ScreenShotPath { get; set; }

        private ImageSource _icon;
        public ImageSource Icon
        {
            get
            {
                if (_icon == null && File.Exists(IconPath)) 
                {
                    _icon = LoadImage(IconPath);
                }

                return _icon;
            }
        }

        private ImageSource _screenShot;
        public ImageSource ScreenShot
        {
            get
            {
                if (_screenShot == null && File.Exists(ScreenShotPath))
                {
                    _screenShot = LoadImage(ScreenShotPath);
                }
                    
                return _screenShot;
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

    [DataContract]
    public class ProjectDataList
    {
        [DataMember]
        public List<ProjectData> Projects { get; set; } = new List<ProjectData>();
    }

    class OpenProject : ViewModelBase
    {
        private static readonly string _appDataPath = $@"{Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData)}\MooncastleEditor\";
        private static readonly string _projectDataPath;
        private static ObservableCollection<ProjectData> _projects = new ObservableCollection<ProjectData>();
        public static ReadOnlyObservableCollection<ProjectData> Projects { get; }

        private static void ReadProjectData()
        {
            if (File.Exists(_projectDataPath))
            {
                var projects = Serializer.FromFile<ProjectDataList>(_projectDataPath).Projects.OrderByDescending(x => x.Date);

                _projects.Clear();

                foreach (var project in projects)
                {
                    if (File.Exists(project.FullPath))
                    {
                        project.IconPath = Path.Combine(project.ProjectPath, ".Mooncastle", "Icon.png");
                        project.ScreenShotPath = Path.Combine(project.ProjectPath, ".Mooncastle", "ScreenShot.png");

                        _projects.Add(project);
                    }
                }
            }
        }

        private static void WriteProjectData()
        {
            var projects = _projects.OrderBy(x => x.Date).ToList();

            Serializer.ToFile(new ProjectDataList { Projects = projects }, _projectDataPath);
        }

        public static Project Open(ProjectData data)
        {
            ReadProjectData();

            var project = _projects.FirstOrDefault(x => x.FullPath == data.FullPath);

            if (project != null)
            {
                project.Date = DateTime.Now;
            }
            else
            {
                project = data;
                project.Date = DateTime.Now;
                _projects.Add(project);
            }

            WriteProjectData();

            return Project.Load(project.FullPath);
        }

        static OpenProject()
        {
            try
            {
                if (!Directory.Exists(_appDataPath))
                {
                    Directory.CreateDirectory(_appDataPath);
                }

                _projectDataPath = $@"{_appDataPath}ProjectData.xml";

                Projects = new ReadOnlyObservableCollection<ProjectData>(_projects);

                ReadProjectData();
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, $"Failed to read project data.");

                throw;
            }
        }
    }
}
