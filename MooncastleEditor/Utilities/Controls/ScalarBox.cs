using System.Windows;

namespace MooncastleEditor.Utilities.Controls
{
    class ScalarBox : NumberBox
    {
        static ScalarBox()
        {
            DefaultStyleKeyProperty.OverrideMetadata(typeof(ScalarBox), new FrameworkPropertyMetadata(typeof(ScalarBox)));

        }
    }
}
