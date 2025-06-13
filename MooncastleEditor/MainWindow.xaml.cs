using MooncastleEditor.GameProject;
using System.ComponentModel;
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
        public static string MooncastlePath { get; private set; } = @"C:\Users\mfurk\source\repos\Mooncastle";

        public MainWindow()
        {
            InitializeComponent();
            Loaded += MainWindowLoaded;
            Closing += MainWindowClosing;
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

        private void MainWindowClosing(object sender, CancelEventArgs e)
        {
            Closing -= MainWindowClosing;
            Project.Current?.Unload();
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

                DataContext = projectBrowser.DataContext;
            }
        }
    }
}