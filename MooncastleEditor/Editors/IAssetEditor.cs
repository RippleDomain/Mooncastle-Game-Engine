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

        Asset Asset { get; }

        Task SetAsset(AssetInfo info);
        bool CheckAssetGUID(Guid guid);
    }
}
