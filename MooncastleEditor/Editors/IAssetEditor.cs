using MooncastleEditor.Content;

namespace MooncastleEditor.Editors
{
    enum AssetEditorState
    {
        Done = 0,
        Importing,
        Processing,
        Loading,
        Saving,
    }

    interface IAssetEditor
    {
        AssetEditorState State { get; }
        Guid AssetGuid { get; }

        Asset Asset { get; }

        void SetAsset(AssetInfo info);
    }
}
