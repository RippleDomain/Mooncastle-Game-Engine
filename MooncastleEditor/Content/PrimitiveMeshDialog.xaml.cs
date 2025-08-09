using MooncastleEditor.ContentToolsAPIStructs;
using MooncastleEditor.DllWrappers;
using MooncastleEditor.Editors;
using MooncastleEditor.Utilities.Controls;
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace MooncastleEditor.Content
{
    /// <summary>
    /// Interaction logic for PrimitiveMeshDialog.xaml
    /// </summary>
    public partial class PrimitiveMeshDialog : Window
    {
        private static readonly List<ImageBrush> _textures = new();

        private void OnPrimitiveType_ComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e) => UpdatePrimitive(); 
        private void OnSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e) => UpdatePrimitive();
        private void OnTextBox_TextChanged(object sender, TextChangedEventArgs e) => UpdatePrimitive();
        private void OnScalarBox_ValueChanged(object sender, RoutedEventArgs e) => UpdatePrimitive();

        private void OnTexture_CheckBox_Click(object sender, RoutedEventArgs e)
        {
            Brush brush = Brushes.White;

            if ((sender as CheckBox).IsChecked == true)
            {
                brush = _textures[(int)primTypeComboBox.SelectedItem];
            }

            var vm = DataContext as GeometryEditor;

            foreach (var mesh in vm.MeshRenderer.Meshes)
            {
                mesh.Diffuse = brush;
            }
        }

        private int Value(Slider slider) => (int)slider.Value;

        private float Value(TextBox textBox, float min)
        {
            float.TryParse(textBox.Text, out float result);
            return Math.Max(result, min);
        }

        private int Value(TextBox textBox, int min)
        {
            int.TryParse(textBox.Text, out int result);
            return Math.Max(result, min);
        }

        private float Value(ScalarBox scalarBox, float min)
        {
            float.TryParse(scalarBox.Value, out var result);
            return Math.Max(result, min);
        }

        private void UpdatePrimitive()
        {
            if (!IsInitialized) return;

            var primitiveType = (PrimitiveMeshType)primTypeComboBox.SelectedItem;
            var info = new PrimitiveInitInfo() { Type = primitiveType };
            var smoothingAngle = 0;

            switch (primitiveType)
            {
                case PrimitiveMeshType.Plane:
                    {
                        info.SegmentX = Value(xSliderPlane);
                        info.SegmentZ = Value(zSliderPlane);
                        info.Size.X = Value(widthScalarBoxPlane, 0.001f);
                        info.Size.Z = Value(lengthScalarBoxPlane, 0.001f);
                    }
                    break;
                case PrimitiveMeshType.Cube:
                    {
                        info.SegmentX = Value(xSliderCube);
                        info.SegmentY = Value(ySliderCube);
                        info.SegmentZ = Value(zSliderCube);
                        info.Size.X = Value(xTextBoxCube, 0.001f);
                        info.Size.Y = Value(yTextBoxCube, 0.001f);
                        info.Size.Z = Value(zTextBoxCube, 0.001f);
                        info.LOD = Value(lodTextBoxCube, 0);
                    }
                    break;
                case PrimitiveMeshType.UvSphere:
                    {
                        info.SegmentX = Value(xSliderUvSphere);
                        info.SegmentY = Value(ySliderUvSphere);
                        info.Size.X = Value(xScalarBoxUvSphere, 0.001f);
                        info.Size.Y = Value(yScalarBoxUvSphere, 0.001f);
                        info.Size.Z = Value(zScalarBoxUvSphere, 0.001f);
                        smoothingAngle = Value(angleSliderUvSphere);
                    }
                    break;
                case PrimitiveMeshType.IcoSphere:
                    return;
                case PrimitiveMeshType.Cylinder:
                    return;
                case PrimitiveMeshType.Capsule:
                    return;
                default:
                    break;
            }

            var geometry = new Geometry();
            geometry.ImportSettings.SmoothingAngle = smoothingAngle;
            ContentToolsAPI.CreatePrimitiveMesh(geometry, info);
            (DataContext as GeometryEditor).SetAsset(geometry);
            OnTexture_CheckBox_Click(textureCheckBox, null);
        }

        private static void LoadTextures()
        {
            var uris = new List<Uri>
            {
                new("pack://application:,,,/Resources/PrimitiveMeshView/vilekuna.png"),
                new("pack://application:,,,/Resources/PrimitiveMeshView/CubeCheckermap.png"),
                new("pack://application:,,,/Resources/PrimitiveMeshView/chess.png"),
            };

            _textures.Clear();

            foreach (var uri in uris)
            {
                var resource = Application.GetResourceStream(uri);
                using var reader = new BinaryReader(resource.Stream);
                var data = reader.ReadBytes((int)resource.Stream.Length);
                var imageSource = (BitmapSource)new ImageSourceConverter().ConvertFrom(data);

                imageSource.Freeze();

                var brush = new ImageBrush(imageSource);
                brush.Transform = new ScaleTransform(1, -1, 0.5, 0.5);
                brush.ViewportUnits = BrushMappingMode.Absolute;
                brush.Freeze();

                _textures.Add(brush);
            }
        }

        private void OnSave_Button_Click(object sender, RoutedEventArgs e)
        {
            var saveDialog = new SaveDialog();

            if (saveDialog.ShowDialog() == true)
            {
                Debug.Assert(!string.IsNullOrEmpty(saveDialog.SaveFilePath));

                var asset = (DataContext as IAssetEditor).Asset;
                Debug.Assert(asset != null);
                asset.FullPath = saveDialog.SaveFilePath;
                asset.SaveAsset();

                saveDialog.Close();
            }
        }

        static PrimitiveMeshDialog()
        {
            LoadTextures();
        }

        public PrimitiveMeshDialog()
        {
            InitializeComponent();
            Loaded += (s, e) => UpdatePrimitive();
        }
    }
}
