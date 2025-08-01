using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
namespace MooncastleEditor.Editors
{
    public static class TextureViewCommands
    {
        public static readonly RoutedCommand CenterCommand = new(nameof(CenterCommand), typeof(TextureEditorView), new() { new KeyGesture(Key.Home) });
        public static readonly RoutedCommand ZoomInCommand = new(nameof(ZoomInCommand), typeof(TextureEditorView), new() { new KeyGesture(Key.OemPlus, ModifierKeys.Control) });
        public static readonly RoutedCommand ZoomOutCommand = new(nameof(ZoomOutCommand), typeof(TextureEditorView), new() { new KeyGesture(Key.OemMinus, ModifierKeys.Control) });
        public static readonly RoutedCommand ZoomFitCommand = new(nameof(ZoomFitCommand), typeof(TextureEditorView), new() { new KeyGesture(Key.D0, ModifierKeys.Alt) });
        public static readonly RoutedCommand ActualSizeCommand = new(nameof(ActualSizeCommand), typeof(TextureEditorView), new() { new KeyGesture(Key.D0, ModifierKeys.Control) });
    }

    /// <summary>
    /// Interaction logic for TextureEditorView.xaml
    /// </summary>
    public partial class TextureEditorView : UserControl
    {
        private void OnCenterTexture(object sender, ExecutedRoutedEventArgs e) => textureView.Center();

        private void OnZoomInTexture(object sender, ExecutedRoutedEventArgs e) => textureView.ZoomIn();

        private void OnZoomOutTexture(object sender, ExecutedRoutedEventArgs e) => textureView.ZoomOut();

        private void OnZoomFitTexture(object sender, ExecutedRoutedEventArgs e) => textureView.ZoomFit();

        private void OnActualSizeTexture(object sender, ExecutedRoutedEventArgs e) => textureView.ActualSize();

        public TextureEditorView()
        {
            InitializeComponent();
            Focus();
        }
    }
}
