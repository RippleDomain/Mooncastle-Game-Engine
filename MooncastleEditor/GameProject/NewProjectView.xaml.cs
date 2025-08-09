using System.Windows;
using System.Windows.Controls;

namespace MooncastleEditor.GameProject
{
    /// <summary>
    /// Interaction logic for NewProjectView.xaml
    /// </summary>
    public partial class NewProjectView : UserControl
    {
        public NewProjectView()
        {
            InitializeComponent();
        }

        private void On_Create_Button_Click(object sender, RoutedEventArgs e)
        {
            var vm = DataContext as NewProject;
            var projectPath = vm.CreateProject(templateListBox.SelectedItem as ProjectTemplate);

            bool result = false;

            var window = Window.GetWindow(this);

            if (string.IsNullOrEmpty(projectPath) != true)
            {
                result = true;

                var project = OpenProject.Open(new ProjectData() { ProjectName = vm.ProjectName, ProjectPath = projectPath });

                window.DataContext = project;
            }

            window.DialogResult = result;
            window.Close();
        }
    }
}
