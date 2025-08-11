using MooncastleEditor.Content;
using MooncastleEditor.DllWrappers;
using MooncastleEditor.GameProject;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Windows;

namespace MooncastleEditor
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public static string MooncastlePath { get; private set; }

        private void MainWindowLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= MainWindowLoaded;
            DefaultAssets.GenerateDefaultAssets();
            GetEnginePath();

            var initResult = EngineAPI.InitializeEngine();

            if (initResult == EngineAPIStructs.EngineInitError.Succeeded)
            {
                openProjectBrowser();
            }
            else
            {
                MessageBox.Show($"{initResult.GetDescription()}", "Engine initialization failed", MessageBoxButton.OK, MessageBoxImage.Error);
                Application.Current.Shutdown();
            }
        }

        private void GetEnginePath()
        {
            var mooncastlePath = Environment.GetEnvironmentVariable("MOONCASTLE_PATH", EnvironmentVariableTarget.User);

            if (mooncastlePath == null || !Directory.Exists(Path.Combine(mooncastlePath, @"Mooncastle\EngineAPI")))
            {
                var dlg = new EnginePathDialog();

                if (dlg.ShowDialog() == true)
                {
                    MooncastlePath = dlg.MooncastlePath;
                    Environment.SetEnvironmentVariable("MOONCASTLE_PATH", MooncastlePath.ToUpper(), EnvironmentVariableTarget.User);
                }
                else
                {
                    Application.Current.Shutdown();
                }
            }
            else
            {
                MooncastlePath = mooncastlePath;
            }
        }

        private void Shutdown()
        {
            Closing -= OnMainWindowClosing;
            Project.Current?.Unload();
            DataContext = null;
            ContentToolsAPI.ShutDownContentTools();
            EngineAPI.ShutdownEngine();
        }


        private void OnMainWindowClosing(object sender, CancelEventArgs e)
        {
            if (DataContext == null)
            {
                e.Cancel = true;
                Application.Current.MainWindow.Hide();
                openProjectBrowser();

                if (DataContext != null)
                {
                    Application.Current.MainWindow.Show();
                }
            }
            else
            {
                Shutdown();
            }
        }

        private void openProjectBrowser()
        {
            var projectBrowser = new ProjectBrowser();

            if (projectBrowser.ShowDialog() == false || projectBrowser.DataContext == null)
            {
                Shutdown();
                Application.Current.Shutdown();
            }
            else
            {
                Project.Current?.Unload();

                var project = projectBrowser.DataContext as Project;
                Debug.Assert(project != null);
                ContentWatcher.Reset(project.ContentPath, project.Path);
                DataContext = project;
            }
        }

        public MainWindow()
        {
            InitializeComponent();
            Loaded += MainWindowLoaded;
            Closing += OnMainWindowClosing;
        }
    }
}