using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace MooncastleEditor.GameProject
{
    /// <summary>
    /// Interaction logic for OpenProjectView.xaml
    /// </summary>
    public partial class OpenProjectView : UserControl
    {
        public OpenProjectView()
        {
            InitializeComponent();

            Loaded += (s, e) =>
            {
                var item = projectsListBox.ItemContainerGenerator.ContainerFromIndex(projectsListBox.SelectedIndex) as ListBoxItem;

                if (item != null)
                {
                    item.Focus();
                }
            };
        }

        private void On_Open_Button_Click(object sender, RoutedEventArgs e)
        {
            OpenClickedProject();
        }

        private void On_Mouse_Double_Click_On_List_Box(object sender, RoutedEventArgs e)
        {
            OpenClickedProject();
        }

        private void OpenClickedProject()
        {
            var project = OpenProject.Open(projectsListBox.SelectedItem as ProjectData);

            bool result = false;

            var window = Window.GetWindow(this);

            if (project != null)
            {
                result = true;

                window.DataContext = project;
            }

            window.DialogResult = result;
            window.Close();
        }
    }
}
