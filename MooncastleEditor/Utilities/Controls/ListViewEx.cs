using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace MooncastleEditor.Utilities.Controls;

class ListViewItemEx : ListViewItem
{
    private bool _deferSelection = false;

    protected override void OnMouseLeftButtonDown(MouseButtonEventArgs e)
    {
        if (e.ClickCount == 1 && IsSelected)
        {
            //The user may start drag by clicking into selected items.
            _deferSelection = true;
        }
        else
        {
            base.OnMouseLeftButtonDown(e);
        }
    }

    protected override void OnMouseLeftButtonUp(MouseButtonEventArgs e)
    {
        if (_deferSelection)
        {
            try
            {
                base.OnMouseLeftButtonDown(e);
            }
            finally
            {
                _deferSelection = false;
            }
        }

        base.OnMouseLeftButtonUp(e);
    }

    protected override void OnMouseLeave(MouseEventArgs e)
    {
        _deferSelection = false;
        base.OnMouseLeave(e);
    }
}

class ListViewEx : ListView
{
    protected override DependencyObject GetContainerForItemOverride() => new ListViewItemEx();
}