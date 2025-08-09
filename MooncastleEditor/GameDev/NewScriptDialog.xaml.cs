using MooncastleEditor.GameProject;
using MooncastleEditor.Utilities;
using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Animation;

namespace MooncastleEditor.GameDev
{
    /// <summary>
    /// Interaction logic for NewScriptDialog.xaml
    /// </summary>
    public partial class NewScriptDialog : Window
    {
        private static readonly string _cppCode = 
@"#include ""{0}.h""

namespace {1}
{{
	REGISTER_SCRIPT({0});

	void {0}::beginPlay()
	{{
		
	}}

	void {0}::update(float deltaTime)
	{{
		
	}}
}} //namespace {1}";
        private static readonly string _hCode = 
@"#pragma once

namespace {1}
{{
    class {0} : public mooncastle::script::entity_script
    {{
        public:
        constexpr explicit {0}(mooncastle::gameEntity::entity entity) : mooncastle::script::entity_script(entity) {{}}

        void beginPlay() override;
        void update(float deltaTime) override;
        }};
}} //namespace {1}";

        private static readonly string _namespace = GetNameSpaceFromProjectName();

        private static string GetNameSpaceFromProjectName()
        {
            var projectName = Project.Current.Name.Trim();
            if (string.IsNullOrEmpty(projectName)) return string.Empty;

            return projectName;
        }

        private bool Validate()
        {
            bool isValid = false;

            var name = scriptNameTextBox.Text.Trim();
            var path = scriptPathTextBox.Text.Trim();
            string errorMsg = string.Empty;
            var nameRegex = new Regex(@"^[A-Za-z_][A-Za-z0-9_]*$");

            if (string.IsNullOrEmpty(name))
            {
                errorMsg = "Type in a script name.";
            }
            else if (!nameRegex.IsMatch(name))
            {
                errorMsg = "Invalid character(s) used in script name.";
            }
            else if (string.IsNullOrEmpty(path))
            {
                errorMsg = "Select a valid script path.";
            }
            else if (path.IndexOfAny(Path.GetInvalidPathChars()) != -1)
            {
                errorMsg = "Invalid character(s) used in script path.";
            }
            else if (!Path.GetFullPath(Path.Combine(Project.Current.Path, path)).Contains(Path.Combine(Project.Current.Path, @"GameCode\")))
            {
                errorMsg = "Script must be added to (a sub-folder of) GameCode.";
            }
            else if (File.Exists(Path.GetFullPath(Path.Combine(Path.Combine(Project.Current.Path, path), $"{name}.cpp"))) ||
                     File.Exists(Path.GetFullPath(Path.Combine(Path.Combine(Project.Current.Path, path), $"{name}.h"))))
            {
                errorMsg = $"Script {name} already exists in this folder.";
            }
            else
            {
                isValid = true;
            }

            messageTextBlock.Foreground = FindResource(isValid ? "Editor.FontBrush" : "Editor.RedBrush") as Brush;
            messageTextBlock.Text = errorMsg;

            return isValid;
        }

        private void OnScriptName_TextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (Validate() == false) return;

            var name = scriptNameTextBox.Text.Trim();
            var project = Project.Current;

            messageTextBlock.Text = $"{name}.h and {name}.cpp will be created in {Project.Current.Name}";
        }

        private void OnScriptPath_TextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            Validate();
        }

        private async void OnCreate_Button_Click(object sender, RoutedEventArgs e)
        {
            if (Validate() == false) return;

            IsEnabled = false;

            busyAnimation.Opacity = 0;
            busyAnimation.Visibility = Visibility.Visible;
            DoubleAnimation fadeIn = new(0, 1, new Duration(TimeSpan.FromMilliseconds(500)));
            busyAnimation.BeginAnimation(OpacityProperty, fadeIn);

            try 
            {
                var name = scriptNameTextBox.Text.Trim();
                var path = Path.GetFullPath(Path.Combine(Project.Current.Path, scriptPathTextBox.Text.Trim()));
                var solution = Project.Current.Solution;
                var projectName = Project.Current.Name;

                await Task.Run(() => CreateScript(name, path, solution, projectName));
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, $"Failed to create script {scriptNameTextBox.Text}");
            }
            finally
            {
                DoubleAnimation fadeOut = new(1, 0, new Duration(TimeSpan.FromMilliseconds(200)));
                fadeOut.Completed += (s, args) => 
                {
                    busyAnimation.Opacity = 0;
                    busyAnimation.Visibility = Visibility.Hidden;
                    Close();
                };
                busyAnimation.BeginAnimation(OpacityProperty, fadeOut);
            }
        }

        private void CreateScript(string name, string path, string solution, string projectName)
        {
            if (Directory.Exists(path) == false)
            {
                Directory.CreateDirectory(path);
            }

            var cpp = Path.GetFullPath(Path.Combine(path, $"{name}.cpp"));
            var h = Path.GetFullPath(Path.Combine(path, $"{name}.h"));

            File.WriteAllText(cpp, string.Format(_cppCode, name, _namespace));
            File.WriteAllText(h, string.Format(_hCode, name, _namespace));

            string[] files = new string[] { cpp, h };

            VisualStudio.AddFilesToSolution(solution, projectName, files);
        }

        public NewScriptDialog()
        {
            InitializeComponent();
            Owner = Application.Current.MainWindow;
            scriptPathTextBox.Text = @"GameCode\";
        }
    }
}
