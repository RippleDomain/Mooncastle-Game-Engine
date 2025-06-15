using MooncastleEditor.Components;
using MooncastleEditor.DllWrappers;
using MooncastleEditor.GameDev;
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
using System.Windows;
using System.Windows.Input;
using static System.Formats.Asn1.AsnWriter;

namespace MooncastleEditor.GameProject
{
    enum BuildConfiguration
    {
        Debug,
        Release,
        DebugEditor,
        ReleaseEditor
    }

    [DataContract(Name = "Game")]
    class Project : ViewModelBase
    {
        public static string Extension = ".mooncastle";

        [DataMember]
        public string Name { get; private set; } = "NewProject";
        [DataMember]
        public string Path { get; private set; }
        public string FullPath => $@"{Path}{Name}{Extension}";
        public string Solution => $@"{Path}{Name}.sln";

        private static readonly string[] _buildConfigNames = new string[] { "Debug", "Release", "DebugEditor", "ReleaseEditor" };

        private int _buildConfig;
        [DataMember]
        public int BuildConfig
        {
            get => _buildConfig;
            set
            {
                if (_buildConfig != value)
                {
                    _buildConfig = value;
                    OnPropertyChanged(nameof(BuildConfig));
                }
            }
        }

        public BuildConfiguration StandaloneBuildConfig => BuildConfig == 0 ? BuildConfiguration.Debug : BuildConfiguration.Release;
        public BuildConfiguration DllBuildConfig => BuildConfig == 0 ? BuildConfiguration.DebugEditor : BuildConfiguration.ReleaseEditor;

        private string[] _availabeScripts;
        public string[] AvailableScripts
        {
            get => _availabeScripts;
            set
            {
                if (_availabeScripts != value)
                {
                    _availabeScripts = value;
                    OnPropertyChanged(nameof(AvailableScripts));
                }
            }
        }

        [DataMember(Name = "Scenes")]
        private ObservableCollection<Scene> _scenes = new ObservableCollection<Scene>();
        public ReadOnlyObservableCollection<Scene> Scenes { get; private set; }

        private Scene _sceneOnScreen;
        public Scene SceneOnScreen
        {
            get
            {
                return _sceneOnScreen;
            }
            set
            {
                if (_sceneOnScreen != value)
                {
                    _sceneOnScreen = value;
                    OnPropertyChanged(nameof(SceneOnScreen));
                }
            }
        }

        public static Project Current => Application.Current.MainWindow.DataContext as Project;

        public static UndoRedo UndoRedo { get; } = new UndoRedo();

        public ICommand UndoCommand { get; private set; }

        public ICommand RedoCommand { get; private set; }

        public ICommand AddSceneCommand { get; private set; }
        public ICommand RemoveSceneCommand { get; private set; }
        public ICommand SaveCommand { get; private set; }
        public ICommand BuildCommand { get; private set; }

        private void SetCommands()
        {
            AddSceneCommand = new RelayCommand<object>(x =>
            {
                AddScene($"NewScene {_scenes.Count}");

                var newScene = _scenes.Last();
                var sceneIndex = _scenes.Count - 1;

                UndoRedo.Add(new UndoRedoAction(
                    () => RemoveScene(newScene),
                    () => _scenes.Insert(sceneIndex, newScene),
                    $"Add {newScene.Name}"));
            });

            RemoveSceneCommand = new RelayCommand<Scene>(x =>
            {
                var sceneIndex = _scenes.IndexOf(x);
                RemoveScene(x);

                UndoRedo.Add(new UndoRedoAction(
                    () => _scenes.Insert(sceneIndex, x),
                    () => RemoveScene(x),
                    $"Remove {x.Name}"));
            }, x => !x.IsOnScreen);

            UndoCommand = new RelayCommand<object>(x => UndoRedo.Undo(), x => UndoRedo.UndoList.Any());
            RedoCommand = new RelayCommand<object>(x => UndoRedo.Redo(), x => UndoRedo.RedoList.Any());
            SaveCommand = new RelayCommand<object>(x => Save(this));
            BuildCommand = new RelayCommand<bool>(async x => await BuildGameCodeDll(x), x => !VisualStudio.IsDebugging() && VisualStudio.BuildDone);

            OnPropertyChanged(nameof(AddSceneCommand));
            OnPropertyChanged(nameof(RemoveSceneCommand));
            OnPropertyChanged(nameof(UndoCommand));
            OnPropertyChanged(nameof(RedoCommand));
            OnPropertyChanged(nameof(SaveCommand));
            OnPropertyChanged(nameof(BuildCommand));
        }

        private static string GetBuildConfigName(BuildConfiguration config) => _buildConfigNames[(int)config];

        private void AddScene(string sceneName)
        {
            Debug.Assert(!string.IsNullOrEmpty(sceneName.Trim()));

            _scenes.Add(new Scene(sceneName, this));
        }

        private void RemoveScene(Scene scene)
        {
            Debug.Assert(_scenes.Contains(scene));

            _scenes.Remove(scene);
        }

        public static Project Load(string file)
        {
            Debug.Assert(File.Exists(file));

            return Serializer.FromFile<Project>(file);
        }

        public void Unload()
        {
            UnloadGameCodeDll();
            VisualStudio.CloseVisualStudio();
            UndoRedo.Reset();
        }

        public static void Save(Project project)
        {
            Serializer.ToFile(project, project.FullPath);
            Logger.Log(MessageType.Info, $"Saved project to {project.FullPath}");
        }

        private async Task BuildGameCodeDll(bool showWindow = true)
        {
            try
            {
                UnloadGameCodeDll();

                await Task.Run(() => VisualStudio.BuildSolution(this, GetBuildConfigName(DllBuildConfig), showWindow));

                if (VisualStudio.BuildSucceeded)
                {
                    LoadGameCodeDll();
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                throw;
            }
        }

        private void LoadGameCodeDll()
        {
            var configName = GetBuildConfigName(DllBuildConfig);

            var dllPath = System.IO.Path.Combine(Path, "x64", configName, $"{Name}.dll");

            AvailableScripts = null;

            if (File.Exists(dllPath) && EngineAPI.LoadGameCodeDll(dllPath) != 0)
            {
                AvailableScripts = EngineAPI.GetScriptNames();
                SceneOnScreen.GameEntities.Where(x => x.GetComponent<Script>() != null).ToList().ForEach(x => x.IsActive = true);

                Logger.Log(MessageType.Info, $"Game code DLL loaded successfully.");
            }
            else
            {
                Logger.Log(MessageType.Warning, $"Game code DLL not found at {dllPath}");
            }
        }

        private void UnloadGameCodeDll()
        {
            SceneOnScreen.GameEntities.Where(x => x.GetComponent<Script>() != null).ToList().ForEach(x => x.IsActive = false);

            if (EngineAPI.UnloadGameCodeDll() != 0)
            {
                Logger.Log(MessageType.Info, "Successfully unloaded game code DLL.");

                AvailableScripts = null;
            }
        }

        [OnDeserialized]
        private async void OnDeserialized(StreamingContext context)
        {
            if (_scenes != null)
            {
                Scenes = new ReadOnlyObservableCollection<Scene>(_scenes);
                OnPropertyChanged(nameof(Scenes));
            }

            SceneOnScreen = Scenes.FirstOrDefault(x => x.IsOnScreen);
            Debug.Assert(SceneOnScreen != null);

            await BuildGameCodeDll(false);

            SetCommands();
        }

        public Project(string name, string path)
        {
            Name = name;
            Path = path;

            OnDeserialized(new StreamingContext());
        }
    }
}