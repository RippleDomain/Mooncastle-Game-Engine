using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.IO;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace MooncastleEditor
{
    /// <summary>
    /// Interaction logic for EnginePathDialog.xaml
    /// </summary>
    public partial class EnginePathDialog : Window
    {
        public string MooncastlePath { get; private set; }

        public EnginePathDialog()
        {
            InitializeComponent();
            Owner = Application.Current.MainWindow;
        }

        private void OnOk_Button_Click(object sender, RoutedEventArgs e)
        {
            var path = pathTextBox.Text.Trim();
            messageTextBlock.Text = string.Empty;

            if (string.IsNullOrEmpty(path))
            {
                messageTextBlock.Text = "Invalid path.";
            }
            else if (path.IndexOfAny(Path.GetInvalidPathChars()) != -1)
            {
                messageTextBlock.Text = "Path contains invalid character(s).";
            }
            else if (!Directory.Exists(Path.Combine(path, @"Mooncastle\EngineAPI\")))
            {
                messageTextBlock.Text = "Unable to find the engine at the specified location.";
            }

            if (string.IsNullOrEmpty(messageTextBlock.Text))
            {
                if (!Path.EndsInDirectorySeparator(path))
                {
                    path += Path.DirectorySeparatorChar;
                }

                MooncastlePath = path;
                DialogResult = true;

                Close();
            }
        }
    }
}
