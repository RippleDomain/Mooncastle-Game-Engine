using MooncastleEditor.Content;

namespace MooncastleEditor.Editors
{
    interface IAssetEditor
    {
        Asset Asset { get; }

        void SetAsset(AssetInfo asset);
    }
}
