using MooncastleEditor.Content;
using MooncastleEditor.GameProject;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;

namespace MooncastleEditor
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public static string MooncastlePath { get; private set; }

        public MainWindow()
        {
            InitializeComponent();
            Loaded += MainWindowLoaded;
            Closing += OnMainWindowClosing;
        }
        private void MainWindowLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= MainWindowLoaded;
            GetEnginePath();
            openProjectBrowser();
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
                Closing -= OnMainWindowClosing;
                Project.Current?.Unload();
                DataContext = null;
            }
        }

        private void openProjectBrowser()
        {
            var projectBrowser = new ProjectBrowser();

            if (projectBrowser.ShowDialog() == false || projectBrowser.DataContext == null)
            {
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
    }
}