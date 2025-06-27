using MooncastleEditor.ContentToolsAPIStructs;
using MooncastleEditor.DllWrappers;
using MooncastleEditor.Editors;
using MooncastleEditor.Utilities.Controls;
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
using System.Windows.Shapes;

namespace MooncastleEditor.Content
{
    /// <summary>
    /// Interaction logic for PrimitiveMeshDialog.xaml
    /// </summary>
    public partial class PrimitiveMeshDialog : Window
    {
        private void OnPrimitiveType_ComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e) => UpdatePrimitive(); 
        private void OnSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e) => UpdatePrimitive();
        private void OnScalarBox_ValueChanged(object sender, RoutedEventArgs e) => UpdatePrimitive();

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

            switch (primitiveType)
            {
                case PrimitiveMeshType.Plane:
                    {
                        info.SegmentX = Math.Max(1, (int)xSliderPlane.Value);
                        info.SegmentZ = Math.Max(1, (int)zSliderPlane.Value);
                        info.Size.X = Math.Max(Value(widthScalarBoxPlane, 0.001f), 0.001f);
                        info.Size.Z = Math.Max(Value(lengthScalarBoxPlane, 0.001f), 0.001f);
                        break;
                    }
                case PrimitiveMeshType.Cube:
                    break;
                case PrimitiveMeshType.UvSphere:
                    break;
                case PrimitiveMeshType.IcoSphere:
                    break;
                case PrimitiveMeshType.Cylinder:
                    break;
                case PrimitiveMeshType.Capsule:
                    break;
                default:
                    break;
            }

            var geometry = new Geometry();
            ContentToolsAPI.CreatePrimitiveMesh(geometry, info);
            (DataContext as GeometryEditor).SetAsset(geometry);
        }

        public PrimitiveMeshDialog()
        {
            InitializeComponent();
            Loaded += (s, e) => UpdatePrimitive();
        }
    }
}
