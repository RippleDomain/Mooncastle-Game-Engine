using System.Windows;
using System.Windows.Controls;
using System.Windows.Media.Animation;

namespace MooncastleEditor.GameProject
{
    /// <summary>
    /// Interaction logic for ProjectBrowser.xaml
    /// </summary>
    public partial class ProjectBrowser : Window
    {
        private readonly CubicEase _easing = new() { EasingMode = EasingMode.EaseInOut };

        public static bool GotoNewProjectTab { get; set; }

        public ProjectBrowser()
        {
            InitializeComponent();
            Loaded += OnProjectBrowserLoaded;
        }

        private void OnProjectBrowserLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnProjectBrowserLoaded;

            if (!OpenProject.Projects.Any() || GotoNewProjectTab)
            {
                if (!GotoNewProjectTab)
                {
                    openProjectButton.IsEnabled = false;
                    openProjectView.Visibility = Visibility.Hidden;
                }

                onToggleButton_Clicked(createProjectButton, new RoutedEventArgs());
            }

            GotoNewProjectTab = false;
        }

        private void onToggleButton_Clicked(object sender, RoutedEventArgs e)
        {
            if (sender == openProjectButton)
            {
                if (createProjectButton.IsChecked == true)
                {
                    createProjectButton.IsChecked = false;
                    AnimateToOpenProject();
                    openProjectView.IsEnabled = true;
                    newProjectView.IsEnabled = false;
                }

                openProjectButton.IsChecked = true;
            }
            else
            {
                if (openProjectButton.IsChecked == true)
                {
                    openProjectButton.IsChecked = false;
                    AnimateToCreateProject();
                    newProjectView.IsEnabled = true;
                    openProjectView.IsEnabled = false;
                }

                createProjectButton.IsChecked = true;
            }  
        }

        private void AnimateToOpenProject()
        {
            var highlightAnimation = new DoubleAnimation(400, 200, new Duration(TimeSpan.FromSeconds(0.2)));
            highlightAnimation.EasingFunction = _easing;

            highlightAnimation.Completed += (s, e) =>
            {
                var animation = new ThicknessAnimation(new Thickness(-1600, 0, 0, 0), new Thickness(0), new Duration(TimeSpan.FromSeconds(0.3)));
                animation.EasingFunction = _easing;
                browserContent.BeginAnimation(MarginProperty, animation);
            };

            highlightRect.BeginAnimation(Canvas.LeftProperty, highlightAnimation);
        }

        private void AnimateToCreateProject()
        {
            var highlightAnimation = new DoubleAnimation(200, 400, new Duration(TimeSpan.FromSeconds(0.2)));
            highlightAnimation.EasingFunction = _easing;

            highlightAnimation.Completed += (s, e) =>
            {
                var animation = new ThicknessAnimation(new Thickness(0), new Thickness(-1600, 0, 0, 0), new Duration(TimeSpan.FromSeconds(0.3)));
                animation.EasingFunction = _easing;
                browserContent.BeginAnimation(MarginProperty, animation);
            };

            highlightRect.BeginAnimation(Canvas.LeftProperty, highlightAnimation);
        }
    }
}
